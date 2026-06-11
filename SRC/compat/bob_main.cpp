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
extern "C" int bob_video_smoketest(void);
extern "C" int bob_render_smoketest(void);
extern "C" int bob_input_smoketest(void);

int main(int argc, char** argv)
{
	(void)argc; (void)argv;
	fprintf(stderr,
		"Rowan's Battle of Britain - Linux native port\n"
		"  All %d source modules link.\n", 16);

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
