/* BoB Linux port - process entry point.
 *
 * On Windows the entry was MFC's WinMain -> AfxWinMain -> theApp.InitInstance().
 * The CWinApp subclass (CMIGApp) and its global instance live in the MFC module;
 * full runtime bring-up (SDL2 window + OpenGL present + OpenAL) is the next phase.
 * For now this provides the ELF entry so a `bob` binary links and starts. */
#ifdef FF_LINUX

#include <iostream>
#include <cstdio>
#include <unistd.h>   /* _exit */
#include <execinfo.h>
#include <signal.h>

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

static void ma_crash_handler(int sig) {
	void* bt[48]; int n = backtrace(bt, 48);
	fprintf(stderr, "\n=== CRASH: signal %d (tid %ld) ===\n", sig, (long)gettid());
	backtrace_symbols_fd(bt, n, 2);
	signal(sig, SIG_DFL); raise(sig);
}
int main(int argc, char** argv)
{
	if (!getenv("MA_NO_CRASH_BT")) { signal(SIGSEGV, ma_crash_handler); signal(SIGABRT, ma_crash_handler); signal(SIGBUS, ma_crash_handler); }
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
