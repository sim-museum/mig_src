/* BoB Linux port - process entry point.
 *
 * On Windows the entry was MFC's WinMain -> AfxWinMain -> theApp.InitInstance().
 * The CWinApp subclass (CMIGApp) and its global instance live in the MFC module;
 * full runtime bring-up (SDL2 window + OpenGL present + OpenAL) is the next phase.
 * For now this provides the ELF entry so a `bob` binary links and starts. */
#ifdef FF_LINUX

#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* expose REG_* register indices in <ucontext.h> */
#endif
#include <iostream>
#include <cstdio>
#include <unistd.h>   /* _exit */
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <cstring>   /* memset */

/* Static-init-order fix: some game globals (e.g. the Lib3D object created via
   Inst3d::commonkeymaps' TU init) construct a std:: stream in their ctor, which
   copies the global std::locale. That locale is set up by std::ios_base::Init
   (the <iostream> static). Force that Init to run FIRST (highest init_priority)
   so it precedes every default-priority game global ctor — otherwise the stream
   ctor reads an uninitialised locale and segfaults. */
static std::ios_base::Init __bob_iostream_init __attribute__((init_priority(101)));

/* Runtime bring-up: drive the real MFC boot path (theApp.InitInstance) via the
   C-linkage hook defined in MIG.CPP. Set BOB_RUN_INIT=0 to skip (link-only run). */
extern "C" int bob_init_instance(void);
extern "C" int bob_run(void);
extern "C" void* bob_LoadLibrary(const char* path);
extern "C" void  bob_set_main_module(void* h);
extern "C" int bob_video_smoketest(void);
extern "C" int bob_render_smoketest(void);
extern "C" int bob_input_smoketest(void);

static void ma_crash_handler(int sig, siginfo_t* si, void* ucv) {
	void* bt[48]; int n = backtrace(bt, 48);
	fprintf(stderr, "\n=== CRASH: signal %d (tid %ld) fault_addr=%p ===\n",
		sig, (long)gettid(), si ? si->si_addr : (void*)0);
#if defined(__i386__)
	if (ucv) {  /* dump the i386 register file -- for the XASM span-filler OOB, compare
	               fault_addr to edi (dest write) vs esi+ebx (texture read) to localise it */
		greg_t* r = ((ucontext_t*)ucv)->uc_mcontext.gregs;
		fprintf(stderr, "  eip=%08x eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x esp=%08x\n",
			(unsigned)r[REG_EIP],(unsigned)r[REG_EAX],(unsigned)r[REG_EBX],(unsigned)r[REG_ECX],
			(unsigned)r[REG_EDX],(unsigned)r[REG_ESI],(unsigned)r[REG_EDI],(unsigned)r[REG_EBP],(unsigned)r[REG_ESP]);
	}
#endif
	backtrace_symbols_fd(bt, n, 2);
	signal(sig, SIG_DFL); raise(sig);
}
static void ma_install_crash_handler(int sig) {
	struct sigaction sa; memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = ma_crash_handler;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, 0);
}
int main(int argc, char** argv)
{
	if (!getenv("MA_NO_CRASH_BT")) { ma_install_crash_handler(SIGSEGV); ma_install_crash_handler(SIGABRT); ma_install_crash_handler(SIGBUS); }
	(void)argc; (void)argv;
	fprintf(stderr,
		"Mig Alley - Linux native port (Rowan engine)\n"
		"  wmig ELF links (15 game unities + MFC UI + standalones + runtime).\n");

	/* The engine assumes cwd == the install dir (FileMan's HERE/".\" root, e.g.
	   makerootdirlist() probing ".\ROOTS.DIR"). Original runtime ran from the
	   install dir; under Wine it's drive_c\rowan\mig. chdir there so relative
	   "." paths resolve. BOB_GAME_DIR overrides; else derive from BOB_DRIVE_C. */
	{
		const char* gdir = getenv("BOB_GAME_DIR");
		char derived[2048];
		if ((!gdir || !gdir[0])) {
			const char* dc = getenv("BOB_DRIVE_C");
			if (dc && dc[0]) {
				snprintf(derived, sizeof(derived), "%s/rowan/mig", dc);
				gdir = derived;
			}
		}
		if (gdir && gdir[0]) {
			if (chdir(gdir) == 0)
				fprintf(stderr, "  cwd -> %s\n", gdir);
			else
				fprintf(stderr, "  WARN: chdir(%s) failed (relative game paths may not resolve)\n", gdir);
		}
	}

	/* Load Mig.exe as the main resource module: on Windows the dialog templates / UI
	   bitmaps live in the .exe (AfxGetInstanceHandle); the localized miglang.dll has only
	   a few. Resource lookups (ma_dlgtmpl etc.) fall back to this for everything else. */
	{
		void* mm = bob_LoadLibrary("Mig.exe");
		if (mm) { bob_set_main_module(mm); fprintf(stderr, "  main module: Mig.exe resources loaded\n"); }
		else fprintf(stderr, "  WARN: Mig.exe resources not loaded (dialog templates unavailable)\n");
	}

	/* Runtime bring-up is in progress: InitInstance() drives the real MFC boot
	   (registry, OLE, doc templates, command-line parse, ProcessShellCommand) and
	   currently stops at the first main-window use -- no CMainFrame is created yet
	   (the doc/view framework + window backend is the next subsystem). Opt in with
	   BOB_RUN_INIT=1 to drive it; the default run stays clean. */
	if (getenv("BOB_VID_SMOKETEST")) {
		fprintf(stderr, "  Video smoke test (SDL2 window + GL present)...\n");
		bob_video_smoketest();
		_exit(0);
	}
	if (getenv("BOB_RENDER_SMOKETEST")) {
		fprintf(stderr, "  Render smoke test (D3D7 device -> GL textured quad)...\n");
		bob_render_smoketest();
		_exit(0);
	}
	if (getenv("BOB_INPUT_SMOKETEST")) {
		fprintf(stderr, "  Input smoke test (DirectInput keyboard -> SDL)...\n");
		bob_input_smoketest();
		_exit(0);
	}

	if (getenv("BOB_RUN_INIT") && getenv("BOB_RUN_INIT")[0] == '1') {
		fprintf(stderr, "  Driving CMIGApp::InitInstance()...\n");
		int ok = bob_init_instance();
		fprintf(stderr, "  InitInstance() returned %d\n", ok);
		if (ok) {
			fprintf(stderr, "  Entering CMIGApp::Run()...\n");
			bob_run();
		}
	} else {
		fprintf(stderr,
			"  Runtime bring-up in progress (set BOB_RUN_INIT=1 to drive"
			" CMIGApp::InitInstance).\n");
	}

	/* Global dtors (e.g. Mast3d::~Mast3d -> Sound::ShutDownSound) assume their
	   subsystems were brought up by InitInstance(), which hasn't run yet, so
	   running them at exit derefs uninitialised DirectSound/3D state. Until the
	   runtime loop initialises those subsystems, skip C++ static teardown and
	   let the OS reclaim the process. */
	fflush(NULL);
	_exit(0);
}

#endif /* FF_LINUX */
