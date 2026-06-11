/* BoB Linux port - threads + multimedia timer backend (Phase 2c).
 *
 * The compat layer already has a real pthread-backed handle/event system
 * (compat_winbase.h: CreateThread/CreateEvent/SetEvent/WaitForSingleObject).
 * Two MFC/MM entry points were still stubbed and gate the render loop:
 *   - AfxBeginThread  -> the per-view draw thread (View3d::drawloop) never ran.
 *   - timeSetEvent    -> the periodic "move cycle" (Mast3d::StaticTimeProc ->
 *                        TimeProc) never fired.
 * Both are implemented here over pthreads. The Win32 event handshake the draw
 * thread/move-cycle use (doneframe etc.) is already real in compat_winbase.h.
 */
#ifdef FF_LINUX

#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

/* ---- AfxBeginThread: run an MFC AFX_THREADPROC on a detached pthread ----- */
typedef unsigned int (*bob_threadproc)(void*);

struct ThreadArg { bob_threadproc proc; void* arg; };
static void* thread_trampoline(void* p) {
	ThreadArg a = *(ThreadArg*)p; free(p);
	if (a.proc) a.proc(a.arg);
	return NULL;
}
extern "C" void bob_begin_thread(bob_threadproc proc, void* arg)
{
	if (!proc) return;
	ThreadArg* a = (ThreadArg*)malloc(sizeof(ThreadArg));
	a->proc = proc; a->arg = arg;
	pthread_t t;
	if (pthread_create(&t, NULL, thread_trampoline, a) == 0)
		pthread_detach(t);          /* draw loops run until their view closes */
	else
		free(a);
}

/* ---- timeSetEvent: a periodic (or one-shot) multimedia timer ------------- */
typedef void (*bob_timecb)(unsigned, unsigned, unsigned long, unsigned long, unsigned long);

struct Timer {
	bob_timecb    cb;
	unsigned long user;
	unsigned      delayMs;
	int           periodic;
	volatile int  kill;
	pthread_t     th;
	int           inUse;
};
#define BOB_MAX_TIMERS 32
static Timer g_timers[BOB_MAX_TIMERS];
static pthread_mutex_t g_timerLock = PTHREAD_MUTEX_INITIALIZER;

static void* timer_thread(void* p)
{
	Timer* t = (Timer*)p;
	do {
		/* sleep in small slices so timeKillEvent is responsive */
		unsigned remaining = t->delayMs ? t->delayMs : 1;
		while (remaining && !t->kill) {
			unsigned slice = remaining > 5 ? 5 : remaining;
			usleep(slice * 1000);
			remaining -= slice;
		}
		if (t->kill) break;
		if (t->cb) t->cb((unsigned)(t - g_timers) + 1, 0, t->user, 0, 0);
	} while (t->periodic && !t->kill);
	pthread_mutex_lock(&g_timerLock);
	t->inUse = 0;
	pthread_mutex_unlock(&g_timerLock);
	return NULL;
}

/* fdwTimer: TIME_ONESHOT=0, TIME_PERIODIC=1 (bit 0) */
extern "C" unsigned int bob_time_set_event(unsigned delayMs, unsigned /*res*/,
		bob_timecb cb, unsigned long user, unsigned fdwTimer)
{
	if (getenv("BOB_NO_TIMER")) return 1;   /* diagnostic: no real timer thread */
	pthread_mutex_lock(&g_timerLock);
	int slot = -1;
	for (int i = 0; i < BOB_MAX_TIMERS; i++) if (!g_timers[i].inUse) { slot = i; break; }
	if (slot < 0) { pthread_mutex_unlock(&g_timerLock); return 0; }
	Timer* t = &g_timers[slot];
	memset(t, 0, sizeof(*t));
	t->cb = cb; t->user = user; t->delayMs = delayMs;
	t->periodic = (fdwTimer & 1) ? 1 : 0;
	t->inUse = 1; t->kill = 0;
	if (pthread_create(&t->th, NULL, timer_thread, t) != 0) { t->inUse = 0; pthread_mutex_unlock(&g_timerLock); return 0; }
	pthread_detach(t->th);
	pthread_mutex_unlock(&g_timerLock);
	return (unsigned)slot + 1;          /* timer id (non-zero) */
}

extern "C" unsigned int bob_time_kill_event(unsigned id)
{
	if (id == 0 || id > BOB_MAX_TIMERS) return 0;
	Timer* t = &g_timers[id - 1];
	t->kill = 1;                        /* the timer thread frees its slot */
	return 0;                           /* MMSYSERR_NOERROR */
}

#endif /* FF_LINUX */
