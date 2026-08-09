/* BoB Linux port - DirectDraw7 / Direct3D7 -> SDL2 + OpenGL backend.
 *
 * PHASE 1: window + GL context + the device/surface enumeration and creation
 * skeleton, so Lib3D::Initialise and Lib3D::SetDriverAndMode complete and a
 * real window appears. The COM interfaces declared in compat/ddraw.h +
 * compat/d3d.h are C-style { lpVtbl, ...state... }; here we make concrete
 * GL-backed objects by pointing lpVtbl at our own function tables. The game
 * (SRC/LIB3D/LIB3D.CPP) calls them via p->Method() unchanged -- NO game edits.
 *
 * Rendering methods (Clear/DrawPrimitiveVB/SetRenderState/...) are safe no-ops
 * in this phase; Phase 2+ fills them with real GL. See PORT.md.
 */
#ifdef FF_LINUX

/* SDL/GL system headers use the native ABI -- keep -fpack-struct=1 away from
 * them (see the struct-stat hazard in bob_stubs.cpp). */
#pragma pack(push,8)
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <fcntl.h>
#include <unistd.h>
#pragma pack(pop)

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "compat_types.h"
#include "ddraw.h"
#include "d3d.h"
#include "dinput.h"

extern const GUID IID_IDirect3D7;
extern const GUID IID_IDirect3DTnLHalDevice;
extern const GUID IID_IDirect3DHALDevice;

/* ============================ SDL window/context ========================== */
static SDL_Window*  g_win = NULL;
static SDL_GLContext g_ctx = NULL;

/* --- GL context thread handoff ------------------------------------------------
   The game renders on a dedicated draw thread (View3d::drawloop), but the GL context
   is created on the main thread (during SetDriverAndMode, where the loader screen is
   also drawn). A GL context can be current on only one thread at a time, so we hand it
   off: the main thread keeps it through setup, then releases it the first time it parks
   in bob_msg_wait (it does no further rendering); the draw thread waits for that release
   and then makes the context current on itself for the rest of the run. gl_bind_thread()
   is a no-op once this thread already owns the context. */
static volatile unsigned long g_glOwner = 0;   /* SDL_threadID owning g_ctx, 0 = none */

/* Set when the 3D device renders into the GL framebuffer this frame (BeginScene/Clear/
   DrawPrimitive). The game presents by flipping the DDraw back buffer, whose system-
   memory bits the 3D path never touched -- so when this is set, present by swapping the
   GL framebuffer instead of uploading the (stale) back-buffer bits over the 3D render.
   Pure 2D frames (DDraw Lock/Blt, e.g. the loader) leave it clear and present via bits. */
static int g_devRendered = 0;
static void gl_bind_thread(void)
{
	if (!g_win || !g_ctx) return;
	unsigned long me = (unsigned long)SDL_ThreadID();
	if (me == g_glOwner) return;
	for (int spins=0; g_glOwner != 0 && spins < 3000; spins++) SDL_Delay(1);
	SDL_GL_MakeCurrent(g_win, g_ctx);
	g_glOwner = me;
}
static int g_scrW = 1024, g_scrH = 768;     /* current display-mode size */
static int g_traceVid = 0;
/* mouse state captured by pump_events (window pixels) */
static int g_mouseWinX = 0, g_mouseWinY = 0, g_mouseLDown = 0;
static int g_clickWinX = 0, g_clickWinY = 0, g_clickPending = 0;
/* front-end operational-map navigation input (2D campaign map; see MIG.CPP idle loop).
   Captured in pump_events while the keyboard isn't owned by the 3D flight (g_diKbAcquired==0). */
static int g_navHeld = 0;          /* held pan dirs: bit0 L, bit1 R, bit2 U, bit3 D */
static int g_navActQ[16];          /* one-shot action ring (zoom/exit/fly) */
static int g_navActHead = 0, g_navActTail = 0;
static int g_wheelAccum = 0;       /* mouse-wheel ticks since last read */
static void nav_push_act(int a){ int n=(g_navActHead+1)&15; if(n!=g_navActTail){ g_navActQ[g_navActHead]=a; g_navActHead=n; } }

#define VLOG(...) do{ if(g_traceVid) fprintf(stderr,"[vid] " __VA_ARGS__); }while(0)

static void ensure_window(int w, int h)
{
	if (w > 0 && h > 0) { g_scrW = w; g_scrH = h; }
	if (g_win) {
		/* Skip redundant resizes: ensure_window is hit per-frame, so SDL_SetWindowSize was
		   firing thousands of times for an unchanged size (wasteful, flicker risk at high res). */
		static int lastW=0, lastH=0;
		if (g_scrW==lastW && g_scrH==lastH) return;
		lastW=g_scrW; lastH=g_scrH;
		if (getenv("MA_TRACE_RES")) fprintf(stderr,"[res] ensure_window -> resize to %dx%d\n", g_scrW, g_scrH);
		SDL_SetWindowSize(g_win, g_scrW, g_scrH);
		/* Re-center the window for the new size (a resize keeps the old top-left, so a larger
		   mode spills off-screen). If the chosen size fills the desktop, drop the border and pin
		   to (0,0) so it sits flush with no title bar pushing it off the bottom. */
		SDL_DisplayMode dm;
		if (SDL_GetDesktopDisplayMode(0, &dm) == 0 && g_scrW >= dm.w && g_scrH >= dm.h) {
			SDL_SetWindowBordered(g_win, SDL_FALSE);
			SDL_SetWindowPosition(g_win, 0, 0);
		} else {
			SDL_SetWindowBordered(g_win, SDL_TRUE);
			SDL_SetWindowPosition(g_win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}
		return;
	}
	g_traceVid = getenv("BOB_TRACE_VID") ? 1 : 0;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
		fprintf(stderr, "[vid] SDL_Init failed: %s\n", SDL_GetError());
		return;
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	/* S58: under SDL_VIDEODRIVER=dummy (the GL-free MA_SHOT capture path) the OPENGL
	   window can never be created; ensure_window is hit per-frame, so don't retry (and
	   spam stderr) after the first failure -- the canvas/capture path runs windowless. */
	static int createFailed = 0;
	if (createFailed) return;
	g_win = SDL_CreateWindow("Mig Alley (Linux native port)",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		g_scrW, g_scrH, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!g_win) { createFailed = 1; fprintf(stderr, "[vid] SDL_CreateWindow failed (won't retry): %s\n", SDL_GetError()); return; }
	g_ctx = SDL_GL_CreateContext(g_win);
	if (!g_ctx) { fprintf(stderr, "[vid] SDL_GL_CreateContext failed: %s\n", SDL_GetError()); return; }
	SDL_GL_MakeCurrent(g_win, g_ctx);
	g_glOwner = (unsigned long)SDL_ThreadID();   /* main thread owns it through setup */
	fprintf(stderr, "[vid] SDL2 window %dx%d + GL context: %s | %s\n",
		g_scrW, g_scrH, (const char*)glGetString(GL_RENDERER), (const char*)glGetString(GL_VERSION));
	/* clear once so the window isn't garbage while the rest of init runs */
	glViewport(0, 0, g_scrW, g_scrH);
	glClearColor(0.05f, 0.05f, 0.10f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(g_win);
}

/* ====================================================================== *
 * Phase 2: DirectInput keyboard -> SDL.                                   *
 * The game opens GUID_SysKeyboard in BUFFERED mode and reads events via   *
 * GetDeviceData: each event {dwOfs = DIK scancode, dwData = 0x80 down/0   *
 * up}. The key map is indexed by dwOfs, so the DIK codes must be the      *
 * PS/2-set-1 values DirectInput uses. We translate SDL_Scancode -> DIK,   *
 * queue events here, and signal readiness through MsgWaitForMultipleObjects*
 * (bob_msg_wait returns the keyboard-notification handle's index).        *
 * ====================================================================== */
static int sdl_to_dik(int sc) {
	switch (sc) {
	case SDL_SCANCODE_ESCAPE: return 0x01;
	case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03;
	case SDL_SCANCODE_3: return 0x04; case SDL_SCANCODE_4: return 0x05;
	case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
	case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09;
	case SDL_SCANCODE_9: return 0x0A; case SDL_SCANCODE_0: return 0x0B;
	case SDL_SCANCODE_MINUS: return 0x0C; case SDL_SCANCODE_EQUALS: return 0x0D;
	case SDL_SCANCODE_BACKSPACE: return 0x0E; case SDL_SCANCODE_TAB: return 0x0F;
	case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11;
	case SDL_SCANCODE_E: return 0x12; case SDL_SCANCODE_R: return 0x13;
	case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
	case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17;
	case SDL_SCANCODE_O: return 0x18; case SDL_SCANCODE_P: return 0x19;
	case SDL_SCANCODE_LEFTBRACKET: return 0x1A; case SDL_SCANCODE_RIGHTBRACKET: return 0x1B;
	case SDL_SCANCODE_RETURN: return 0x1C; case SDL_SCANCODE_LCTRL: return 0x1D;
	case SDL_SCANCODE_A: return 0x1E; case SDL_SCANCODE_S: return 0x1F;
	case SDL_SCANCODE_D: return 0x20; case SDL_SCANCODE_F: return 0x21;
	case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
	case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25;
	case SDL_SCANCODE_L: return 0x26; case SDL_SCANCODE_SEMICOLON: return 0x27;
	case SDL_SCANCODE_APOSTROPHE: return 0x28; case SDL_SCANCODE_GRAVE: return 0x29;
	case SDL_SCANCODE_LSHIFT: return 0x2A; case SDL_SCANCODE_BACKSLASH: return 0x2B;
	case SDL_SCANCODE_Z: return 0x2C; case SDL_SCANCODE_X: return 0x2D;
	case SDL_SCANCODE_C: return 0x2E; case SDL_SCANCODE_V: return 0x2F;
	case SDL_SCANCODE_B: return 0x30; case SDL_SCANCODE_N: return 0x31;
	case SDL_SCANCODE_M: return 0x32; case SDL_SCANCODE_COMMA: return 0x33;
	case SDL_SCANCODE_PERIOD: return 0x34; case SDL_SCANCODE_SLASH: return 0x35;
	case SDL_SCANCODE_RSHIFT: return 0x36; case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
	case SDL_SCANCODE_LALT: return 0x38; case SDL_SCANCODE_SPACE: return 0x39;
	case SDL_SCANCODE_CAPSLOCK: return 0x3A;
	case SDL_SCANCODE_F1: return 0x3B; case SDL_SCANCODE_F2: return 0x3C;
	case SDL_SCANCODE_F3: return 0x3D; case SDL_SCANCODE_F4: return 0x3E;
	case SDL_SCANCODE_F5: return 0x3F; case SDL_SCANCODE_F6: return 0x40;
	case SDL_SCANCODE_F7: return 0x41; case SDL_SCANCODE_F8: return 0x42;
	case SDL_SCANCODE_F9: return 0x43; case SDL_SCANCODE_F10: return 0x44;
	case SDL_SCANCODE_F11: return 0x57; case SDL_SCANCODE_F12: return 0x58;
	case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45; case SDL_SCANCODE_SCROLLLOCK: return 0x46;
	/* numeric keypad (C1 Sprint 3: were missing — the flight sim's primary view-pan and
	   trim/control keys live here). DIK PS/2 set-1 keypad codes. */
	case SDL_SCANCODE_KP_7: return 0x47; case SDL_SCANCODE_KP_8: return 0x48;
	case SDL_SCANCODE_KP_9: return 0x49; case SDL_SCANCODE_KP_MINUS: return 0x4A;
	case SDL_SCANCODE_KP_4: return 0x4B; case SDL_SCANCODE_KP_5: return 0x4C;
	case SDL_SCANCODE_KP_6: return 0x4D; case SDL_SCANCODE_KP_PLUS: return 0x4E;
	case SDL_SCANCODE_KP_1: return 0x4F; case SDL_SCANCODE_KP_2: return 0x50;
	case SDL_SCANCODE_KP_3: return 0x51; case SDL_SCANCODE_KP_0: return 0x52;
	case SDL_SCANCODE_KP_PERIOD: return 0x53;
	/* extended (0xE0-prefixed -> DIK uses 0x80|base) */
	case SDL_SCANCODE_RCTRL: return 0x9D; case SDL_SCANCODE_RALT: return 0xB8;
	case SDL_SCANCODE_KP_ENTER: return 0x9C; case SDL_SCANCODE_KP_DIVIDE: return 0xB5;
	case SDL_SCANCODE_UP: return 0xC8; case SDL_SCANCODE_LEFT: return 0xCB;
	case SDL_SCANCODE_RIGHT: return 0xCD; case SDL_SCANCODE_DOWN: return 0xD0;
	case SDL_SCANCODE_HOME: return 0xC7; case SDL_SCANCODE_END: return 0xCF;
	case SDL_SCANCODE_PAGEUP: return 0xC9; case SDL_SCANCODE_PAGEDOWN: return 0xD1;
	case SDL_SCANCODE_INSERT: return 0xD2; case SDL_SCANCODE_DELETE: return 0xD3;
	default: return 0;
	}
}
struct KbEvent { unsigned ofs; unsigned data; };
#define BOB_KBQ 256
static KbEvent g_kbq[BOB_KBQ];
static int g_kbHead=0, g_kbTail=0;     /* ring buffer */
static unsigned g_kbSeq=0;
static void* g_diKbNotify=0;           /* htable[EVENT_KEYS] from SetEventNotification */
static int g_diKbAcquired=0;

/* ---- Joystick (Area C): SDL_Joystick -> DirectInput DIJOYSTATE bridge ----
   Opened lazily the first time the game enumerates/creates the DI joystick. */
static SDL_Joystick* g_sdlJoy=NULL;
static int g_joyOpened=0, g_joyAxes=0, g_joyButtons=0, g_joyHats=0;
/* The game passes a STACK-LOCAL DIDATAFORMAT to SetDataFormat (ANALOGUE.CPP), so we
   must COPY it (real DInput copies too) — storing the pointer would dangle. */
static DIDATAFORMAT g_joyFmtCopy;
static DIOBJECTDATAFORMAT g_joyObjs[64];
static const DIDATAFORMAT* g_joyFmt=NULL;
/* in-flight mouse (DInput, Sprint 18): SDL relative-motion accumulator + buttons,
   drained once per poll by the mouse device GetDeviceData (mirrors the joystick). */
static int g_mouseRelX=0, g_mouseRelY=0;
static int g_mouseBtns=0;                 /* bit0 L, bit1 R, bit2 M */
static int g_mouseCaptured=0;
static DIDATAFORMAT g_mouseFmtCopy;
static DIOBJECTDATAFORMAT g_mouseObjs[16];
static const DIDATAFORMAT* g_mouseFmt=NULL;
static int g_mouseDrained=0;
static void joy_open_once(void) {
	if (g_joyOpened) return; g_joyOpened=1;
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	if (SDL_NumJoysticks() > 0) {
		g_sdlJoy = SDL_JoystickOpen(0);
		if (g_sdlJoy) {
			g_joyAxes=SDL_JoystickNumAxes(g_sdlJoy);
			g_joyButtons=SDL_JoystickNumButtons(g_sdlJoy);
			g_joyHats=SDL_JoystickNumHats(g_sdlJoy);
			fprintf(stderr,"[joy] opened '%s' axes=%d buttons=%d hats=%d\n",
				SDL_JoystickName(g_sdlJoy),g_joyAxes,g_joyButtons,g_joyHats);
		}
	}
	if (!g_sdlJoy) fprintf(stderr,"[joy] no joystick found\n");
}

static void kb_push(unsigned dik, int down) {
	if (!dik) return;
	int nt=(g_kbTail+1)%BOB_KBQ;
	if (nt==g_kbHead) return;          /* full: drop oldest-style */
	g_kbq[g_kbTail].ofs=dik; g_kbq[g_kbTail].data=down?0x80:0x00;
	g_kbTail=nt;
}

/* A2 (Sprint 1): persist preferences on the SDL shutdown path. Defined in FULLPANE.CPP
   (where Save_Data is in scope); writes settings.mig the same way the in-game Exit menu does. */
extern "C" void ma_save_preferences(void);

/* Pump the SDL event queue: window close + keyboard -> DIK queue. */
/* C4b: padlock overlay toggles, flipped in the SDL layer (the engine keymap binds BOXTARGET to
   d+no-modifier, so SHIFT+D never reaches it). Read by OVERLAY.CPP. */
int g_adi_telem = 0;   /* ALT+D: target telemetry */
int g_adi_box   = 0;   /* 'd' / SHIFT+D: padlock box */
/* S92 (C4d): both toggles are modifier-key driven (ALT+D vs plain D), and a synthesised DIK tap
   carries no SDL modifier state -- so neither can be reached from BOB_KEYSEQ. MA_PADLOCK_TELEM=1 /
   MA_PADLOCK_BOX=1 set the initial state instead, which is what makes the padlock readout testable
   headlessly. Default off, so nothing changes for a player. */
static struct MaPadlockEnvInit { MaPadlockEnvInit() {
    if (getenv("MA_PADLOCK_TELEM")) g_adi_telem = atoi(getenv("MA_PADLOCK_TELEM"));
    if (getenv("MA_PADLOCK_BOX"))   g_adi_box   = atoi(getenv("MA_PADLOCK_BOX"));
} } g_maPadlockEnvInit;

static void pump_events(void)
{
	/* S93: the synthetic-input hooks below MUST run before the `!g_win` bail-out. Under
	   SDL_VIDEODRIVER=dummy, SDL_CreateWindow fails (no GL in the dummy driver), so g_win stays
	   NULL and this function used to return immediately -- silently disabling BOB_KEYSEQ and
	   BOB_AUTOFLY in exactly the headless mode they exist to serve. Nothing errored; the taps just
	   never happened, which read as "the key had no effect" and cost S91 a wrong conclusion (a
	   60-tap dive that never occurred) and S92 a failed verification. They only push to the DIK
	   queue and never touch the window, so they are safe here. */
	/* Synthetic input for headless testing (no physical keyboard).
	   BOB_AUTOFLY=sweep : press every DIK in turn (verify key->command dispatch).
	   BOB_AUTOFLY=throttle (or 1): tap '0' (DIK 0x0B = RPM_00 = 100% throttle) a few
	   times so the parked aircraft should spool up and accelerate down the runway. */
	if (getenv("BOB_AUTOFLY") && g_diKbAcquired) {
		const char* mode=getenv("BOB_AUTOFLY");
		static int cnt=0; cnt++;
		if (mode && mode[0]=='s') { static int sweep=1;
			if ((cnt%4)==0) { kb_push(sweep,1); kb_push(sweep,0); if(++sweep>0xD8) sweep=1; } }
		else if (mode && mode[0]=='l') { /* "look": hold ROTLEFT (numpad 4, DIK 0x4B) so the
			cockpit view pans left continuously — a visible keyboard->sim response. */
			static int sent=0; if (cnt==60 && !sent) { kb_push(0x4B,1); sent=1; } }
		else { if ((cnt%30)==0 && cnt<600) { kb_push(0x0B,1); kb_push(0x0B,0); } }  /* full throttle */
	}
	/* B2 A/B harness (port/ab.sh): BOB_KEYSEQ="pump,dik;pump,dik;..." taps a DIK once
	   at the given pump count, used to switch the flight view before MA_DUMP_BACK grabs
	   the frame. View keys (KEYMAPS.H): F6 outside=0x40, F7 inside=0x41, F9 chase=0x43,
	   F10 satellite=0x44, ESC reset=0x01. Same DI keyboard path as the C1 controls. */
	if (getenv("BOB_KEYSEQ") && g_diKbAcquired) {
		static int kidle=0, kidx=0; kidle++;
		const char* p = getenv("BOB_KEYSEQ");
		for (int i=0;i<kidx && p;i++){ p=strchr(p,';'); if(p)p++; }
		if (p && *p) { int f=0,dik=0; if (sscanf(p,"%d,%i",&f,&dik)==2 && kidle>=f){ kidx++; kb_push(dik,1); kb_push(dik,0);
			if (getenv("MA_TRACE_KEY")) fprintf(stderr,"[keyseq] tap dik=0x%02x at kidle=%d\n",dik,kidle); } }
	} else if (getenv("BOB_KEYSEQ") && getenv("MA_TRACE_KEY")) {
		static int warned=0; if(!(warned++ % 200)) fprintf(stderr,"[keyseq] waiting: keyboard not acquired yet\n");
	}
	/* S93 moved the synthetic hooks above this guard because they never touch the window. The
	   guard itself stayed -- and so the EVENT QUEUE was still never drained without a window.
	   S96's drag hook pushes real SDL events on purpose (a hook that bypasses the path it tests
	   proves nothing), and they sat in the queue unread: the drag "worked" and moved nothing,
	   which a round-trip test happily reported as lossless. Draining the queue needs no window --
	   only the window-close/resize cases below care, and they simply never arrive when there is
	   no window. So poll unconditionally. */
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		/* Terminal exits: save settings, then _exit(0) IMMEDIATELY. We deliberately skip
		   SDL_Quit() — with the OpenAL mixer thread + GL context live it can block on audio/
		   video teardown (observed hang on the window-close path), and _exit terminates the
		   process without running that cleanup, which the OS reclaims anyway. */
		if (e.type == SDL_QUIT) { fprintf(stderr,"[vid] window closed -> exit\n"); ma_save_preferences(); _exit(0); }
		else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
			int dik = sdl_to_dik(e.key.keysym.scancode);
			if (getenv("MA_TRACE_DKEY"))
				fprintf(stderr,"[dkey] type=%s sym=%d scan=%d dik=0x%x mod=0x%x acq=%d rpt=%d\n",
					e.type==SDL_KEYDOWN?"DN":"UP", e.key.keysym.sym, e.key.keysym.scancode,
					dik, (unsigned)e.key.keysym.mod, g_diKbAcquired, e.key.repeat);
			/* C4b: handle the padlock 'd' keys here, not via the engine keymap (it binds
			   BOXTARGET to d+no-modifier, so SHIFT+D never reaches it). ALT+D -> telemetry;
			   plain 'd' and SHIFT+D -> box. 'd' is swallowed (not pushed to the engine).
			   In-flight only (g_diKbAcquired). */
			if (dik==0x20 /*DIK_D*/ && g_diKbAcquired) {
				if (e.type==SDL_KEYDOWN && !e.key.repeat) {
					if (e.key.keysym.mod & KMOD_ALT) g_adi_telem = !g_adi_telem;
					else                             g_adi_box   = !g_adi_box;
				}
			}
			else if (dik && g_diKbAcquired && !e.key.repeat) kb_push(dik, e.type==SDL_KEYDOWN);
			/* a hard exit hatch while the UI loop isn't wired: Ctrl+ESC quits */
			if (e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_ESCAPE && (e.key.keysym.mod & KMOD_CTRL)) {
				ma_save_preferences(); _exit(0);
			}
			/* 2D operational-map navigation: when the 3D flight doesn't own the keyboard,
			   feed arrow/WASD (pan), +/-/PageUp/Dn (zoom), Esc (exit map), F/Enter (fly). */
			if (!g_diKbAcquired) {
				int sym = e.key.keysym.sym, down = (e.type==SDL_KEYDOWN), bit = 0;
				switch (sym) {
					case SDLK_LEFT:  case SDLK_a: bit=1; break;
					case SDLK_RIGHT: case SDLK_d: bit=2; break;
					case SDLK_UP:    case SDLK_w: bit=4; break;
					case SDLK_DOWN:  case SDLK_s: bit=8; break;
				}
				if (bit) { if (down) g_navHeld|=bit; else g_navHeld&=~bit; }
				if (down && !e.key.repeat) {
					if (sym==SDLK_EQUALS||sym==SDLK_PLUS||sym==SDLK_KP_PLUS||sym==SDLK_PAGEUP) nav_push_act(1);
					else if (sym==SDLK_MINUS||sym==SDLK_KP_MINUS||sym==SDLK_PAGEDOWN) nav_push_act(2);
					else if (sym==SDLK_ESCAPE && !(e.key.keysym.mod & KMOD_CTRL)) nav_push_act(3);
					else if (sym==SDLK_f || sym==SDLK_RETURN || sym==SDLK_KP_ENTER) nav_push_act(4);
				}
			}
		}
		else if (e.type == SDL_MOUSEWHEEL) { g_wheelAccum += e.wheel.y; }
		else if (e.type == SDL_MOUSEMOTION) { g_mouseWinX = e.motion.x; g_mouseWinY = e.motion.y; g_mouseRelX += e.motion.xrel; g_mouseRelY += e.motion.yrel; }
		else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
			int down = (e.type == SDL_MOUSEBUTTONDOWN);
			int bit = e.button.button==SDL_BUTTON_LEFT?0 : e.button.button==SDL_BUTTON_RIGHT?1 : e.button.button==SDL_BUTTON_MIDDLE?2 : -1;
			if (bit>=0) { if (down) g_mouseBtns |= (1<<bit); else g_mouseBtns &= ~(1<<bit); }
			g_mouseWinX = e.button.x; g_mouseWinY = e.button.y;
			if (e.button.button == SDL_BUTTON_LEFT) {
				g_mouseLDown = down;
				/* S96: a DRAG is not a click. The map pans on left-drag, and the release at the end
				   of a pan used to raise the same one-click edge as a tap -- which since S95 (map
				   clicks reach CMapDlg) meant every pan finished by opening a dossier. Windows only
				   fires a control when press and release land on the same spot; require that here.
				   Synthetic BOB_CLICKSEQ/BOB_CLICK injection does not come through this path. */
				static int s_pressX = 0, s_pressY = 0;
				if (down) { s_pressX = e.button.x; s_pressY = e.button.y; }
				else {
					int dx = e.button.x - s_pressX, dy = e.button.y - s_pressY;
					if (dx < 0) dx = -dx;
					if (dy < 0) dy = -dy;
					if (dx + dy <= 4) { g_clickWinX = e.button.x; g_clickWinY = e.button.y; g_clickPending = 1; }
					else if (getenv("MA_TRACE_CLICK"))
						fprintf(stderr,"[click] release %d,%d suppressed: dragged %d px from press\n",
						        e.button.x, e.button.y, dx+dy);
				}
			}
		}
	}
}

/* ---- mouse state -> canvas coords (front-end hit-testing) --------------- */
extern "C" const void* ma_gdi_canvas(int*, int*);
static void win_to_canvas(int mx, int my, int* cx, int* cy) {
	int cw = 0, ch = 0; ma_gdi_canvas(&cw, &ch);
	if (g_scrW > 0 && g_scrH > 0 && cw > 0 && ch > 0) {
		*cx = (int)((long long)mx * cw / g_scrW);
		*cy = (int)((long long)my * ch / g_scrH);
	} else { *cx = mx; *cy = my; }
}
static void canvas_to_win(int cx, int cy, int* mx, int* my) {
	int cw = 0, ch = 0; ma_gdi_canvas(&cw, &ch);
	if (g_scrW > 0 && g_scrH > 0 && cw > 0 && ch > 0) {
		*mx = (int)((long long)cx * g_scrW / cw);
		*my = (int)((long long)cy * g_scrH / ch);
	} else { *mx = cx; *my = cy; }
}

extern "C" void ma_mouse_pos(int* x, int* y, int* lbtn) {
	/* S96 test hook: BOB_DRAG="startFrame,x1,y1,x2,y2,frames[;...]" replays a left-button drag in
	   CANVAS coords -- press at (x1,y1), move linearly to (x2,y2) over `frames` calls, release.
	   PO-2 ("click-drag on the map messes up the display") is a DRAG defect, and until now the
	   harness could only synthesise taps: BOB_CLICKSEQ injects a one-shot click edge and never
	   holds the button, so the pan path it is meant to test was unreachable headlessly (the same
	   shape of gap as S93's dead BOB_KEYSEQ). Segments are consumed in order. */
	{
		static int inited = 0, frame = 0;
		static const char* seq = 0;
		static int cur_start=-1, cx1=0, cy1=0, cx2=0, cy2=0, cn=0;
		static const char* pos = 0;
		if (!inited) { inited = 1; seq = getenv("BOB_DRAG"); pos = seq; }
		if (seq) {
			frame++;
			if (cur_start < 0 && pos && *pos) {
				int f,a1,b1,a2,b2,n;
				if (sscanf(pos, "%d,%d,%d,%d,%d,%d", &f,&a1,&b1,&a2,&b2,&n) == 6) {
					cur_start=f; cx1=a1; cy1=b1; cx2=a2; cy2=b2; cn=(n>0?n:1);
				} else { pos = 0; }
			}
			if (cur_start >= 0 && frame >= cur_start) {
				int step = frame - cur_start;
				int px = cx1 + (cx2-cx1)*(step<cn?step:cn)/cn;
				int py = cy1 + (cy2-cy1)*(step<cn?step:cn)/cn;
				int wx = px, wy = py;
				canvas_to_win(px, py, &wx, &wy);
				/* Push REAL SDL events rather than overriding the returned position. A hook that
				   bypasses the path it is meant to test proves nothing about that path -- S93
				   (�8-MA93) cost two sprints' conclusions to exactly that. Going through the event
				   queue exercises win_to_canvas, the button-state tracking and the press/release
				   click edge, which is what PO-2 is actually about. */
				SDL_Event ev;
				if (step == 0) {
					memset(&ev,0,sizeof ev); ev.type=SDL_MOUSEMOTION; ev.motion.x=wx; ev.motion.y=wy; SDL_PushEvent(&ev);
					memset(&ev,0,sizeof ev); ev.type=SDL_MOUSEBUTTONDOWN; ev.button.button=SDL_BUTTON_LEFT;
					ev.button.x=wx; ev.button.y=wy; SDL_PushEvent(&ev);
				} else if (step <= cn) {
					memset(&ev,0,sizeof ev); ev.type=SDL_MOUSEMOTION; ev.motion.x=wx; ev.motion.y=wy; SDL_PushEvent(&ev);
				}
				if (step == cn + 1) {
					memset(&ev,0,sizeof ev); ev.type=SDL_MOUSEBUTTONUP; ev.button.button=SDL_BUTTON_LEFT;
					ev.button.x=wx; ev.button.y=wy; SDL_PushEvent(&ev);
					if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[drag] release at (%d,%d)\n", px, py);
					cur_start = -1;
					if (pos) { const char* semi = strchr(pos, ';'); pos = semi ? semi+1 : 0; }
				} else if (getenv("MA_TRACE_CLICK") && (step==0 || step==cn))
					fprintf(stderr,"[drag] frame=%d step=%d/%d at canvas(%d,%d) win(%d,%d)\n",
					        frame, step, cn, px, py, wx, wy);
			}
		}
	}
	/* test hook: BOB_MOUSE="x,y" forces a hover position in canvas coords */
	const char* m = getenv("BOB_MOUSE");
	if (m) { int mx=0,my=0; if (sscanf(m,"%d,%d",&mx,&my)==2) { if(x)*x=mx; if(y)*y=my; if(lbtn)*lbtn=0; return; } }
	win_to_canvas(g_mouseWinX, g_mouseWinY, x, y);
	if (lbtn) *lbtn = g_mouseLDown;
}
extern "C" int ma_ole_menu_row_point(int row, int* outx, int* outy);   /* S63: font-independent recipes */
extern "C" int ma_ole_control_point(int id, int col, int* outx, int* outy);  /* S63: click a control by dialog id (col<0 = centre) */
extern "C" int ma_ole_control_point_p(int id, int col, const char* parentClass, int* outx, int* outy);  /* S85: ...and by hosting class, since ids are not unique */
/* edge-triggered: returns 1 (and the click canvas coords) once per left release */
extern "C" int ma_mouse_take_click(int* x, int* y) {
	/* test hook: BOB_CLICK="x,y" injects one synthetic click after a few frames */
	const char* cs = getenv("BOB_CLICK");
	if (cs) { static int fc=0, fired=0; if (!fired && ++fc>15) { int cx=0,cy=0; if (sscanf(cs,"%d,%d",&cx,&cy)==2){ fired=1; if(x)*x=cx; if(y)*y=cy; return 1; } } }
	/* test hook: BOB_CLICKSEQ="f1,x1,y1;f2,x2,y2;..." injects clicks at the given idle counts.
	   S63: an entry may instead be "f,rN" — click the CENTRE OF MENU ROW N, resolved at fire
	   time via ma_ole_menu_row_point() (the listbox's own GetRowFromY mapping). Fixed pixel
	   rows silently broke every recipe when S62's persisted FontNum changed the menu pitch
	   ~16px -> ~28px; the row form cannot break that way. Absolute "f,x,y" entries still
	   work unchanged, so existing recipes keep running while they are migrated. */
	const char* sq = getenv("BOB_CLICKSEQ");
	if (sq) {
		static int idle = 0, idx = 0; idle++;
		const char* p = sq;
		for (int i = 0; i < idx && p; i++) { p = strchr(p, ';'); if (p) p++; }
		if (p && *p) {
			int f=0, row=0, cid=0;
			if (sscanf(p,"%d,r%d",&f,&row)==2 && idle>=f) {
				int rx=0, ry=0;
				if (ma_ole_menu_row_point(row, &rx, &ry)) { idx++; if(x)*x=rx; if(y)*y=ry; return 1; }
				/* not resolvable yet (menu not built): hold this entry and retry next idle */
				return 0;
			}
			/* S85: an entry may name the HOSTING CLASS — "f,#ID@Class" or "f,#ID@Class:COL".
			   Numeric ids are not unique (MA's RESOURCE.H has five symbols for 2074), so an
			   unqualified #ID resolves to whichever hosted control matched first and can fire at a
			   class with no handler — a silent no-op that looks like a broken feature. The
			   qualifier picks the intended host; ma_ole_control_point_p warns if an unqualified
			   id is ambiguous. */
			int ccol = -1; char cclass[64];
			/* NB the class scanset must exclude ';' as well as ':' — `%63s` reads to whitespace,
			   so with a FOLLOWING step in the sequence it swallowed "CMainToolbar;330,#2018@..."
			   as the class name and the step silently never matched. Only reproducible with a
			   5th entry, which is why the 4-entry sweep recipes never showed it. */
			if ((sscanf(p,"%d,#%d@%63[^:;]:%d",&f,&cid,cclass,&ccol)==4 ||
			     sscanf(p,"%d,#%d@%63[^;]",&f,&cid,cclass)==3) && idle>=f) {
				int rx=0, ry=0;
				if (ma_ole_control_point_p(cid, ccol, cclass, &rx, &ry)) { idx++; if(x)*x=rx; if(y)*y=ry; return 1; }
				return 0;   /* control not up yet: hold and retry */
			}
			if ((sscanf(p,"%d,#%d:%d",&f,&cid,&ccol)==3 || sscanf(p,"%d,#%d",&f,&cid)==2) && idle>=f) {
				int rx=0, ry=0;
				if (ma_ole_control_point(cid, ccol, &rx, &ry)) { idx++; if(x)*x=rx; if(y)*y=ry; return 1; }
				return 0;   /* control not up yet: hold and retry */
			}
			int cx=0,cy=0;
			if (sscanf(p,"%d,%d,%d",&f,&cx,&cy)==3 && idle>=f) { idx++; if(x)*x=cx; if(y)*y=cy; return 1; }
		}
	}
	if (!g_clickPending) return 0;
	g_clickPending = 0;
	win_to_canvas(g_clickWinX, g_clickWinY, x, y);
	return 1;
}
/* operational-map nav accessors (consumed by the MIG.CPP idle-loop map branch) */
extern "C" int ma_map_nav_held(void) {
	/* test hook: BOB_NAVPAN=<bits> forces a held pan direction (1=L 2=R 4=U 8=D). */
	const char* h = getenv("BOB_NAVPAN"); if (h) return atoi(h);
	return g_navHeld;
}
extern "C" int ma_map_nav_take(void) {
	/* test hook: BOB_NAVSEQ="idle,act;idle,act;..." injects map nav actions
	   (act: 1=zoomin 2=zoomout 3=exit 4=fly) at the given idle counts. */
	const char* ns = getenv("BOB_NAVSEQ");
	if (ns) {
		static int idle=0, idx=0; idle++;
		const char* p = ns;
		for (int i=0; i<idx && p; i++) { p = strchr(p, ';'); if (p) p++; }
		if (p && *p) { int f=0,a=0; if (sscanf(p,"%d,%d",&f,&a)==2 && idle>=f) { idx++; return a; } }
	}
	if (g_navActTail == g_navActHead) return 0;
	int a = g_navActQ[g_navActTail]; g_navActTail = (g_navActTail+1)&15; return a;
}
extern "C" int ma_mouse_wheel(void) { int w = g_wheelAccum; g_wheelAccum = 0; return w; }

/* Message-loop wait, called from MsgWaitForMultipleObjects (compat_winuser.h).
   Pumps SDL events and yields the CPU briefly so CMIGApp::Run() doesn't busy-spin.
   Returns WAIT_TIMEOUT (0x102) -- a real window-message queue wired to SDL events
   is the next step. */
extern "C" unsigned long bob_msg_wait(unsigned long nCount, void* const* handles, unsigned long dwMilliseconds)
{
	pump_events();
	/* Hand the GL context off to the draw thread: the first time the owning (main)
	   thread parks here it has finished all its rendering, so release the context and
	   let the draw thread (waiting in gl_bind_thread) take it. */
	if (g_glOwner == (unsigned long)SDL_ThreadID()) {
		SDL_GL_MakeCurrent(g_win, NULL);
		g_glOwner = 0;
	}
	/* if keyboard input is queued, wake the loop on the keyboard-notification
	   handle (htable[EVENT_KEYS]) so CMIGApp::Run dispatches Inst3d::OnKeyInput. */
	if (g_kbHead != g_kbTail && g_diKbNotify && handles) {
		for (unsigned long i=0;i<nCount;i++)
			if (handles[i] == g_diKbNotify) return i;   /* WAIT_OBJECT_0 + i */
	}
	/* yield: ~3ms when the caller intended to wait, 0 when it polled (timeout 0) */
	if (dwMilliseconds != 0) SDL_Delay(3);
	return 0x00000102; /* WAIT_TIMEOUT */
}

/* ============================ object structs ============================== */
struct GLSurface7 {
	IDirectDrawSurface7Vtbl* lpVtbl;
	int   ref;                      /* COM refcount (real; free only at 0) — see cross-port note */
	DDSURFACEDESC2 desc;            /* width/height/caps/pixelformat */
	int   w, h, bpp;
	void* bits;                     /* system-memory backing for Lock/Unlock */
	size_t bytes;
	GLSurface7* back;               /* attached back buffer (complex primary) */
	GLSurface7* zbuf;               /* attached z-buffer */
	int   isPrimary;
	GLuint glTex;                   /* GL texture (lazily created for texture surfaces) */
	int    texDirty;                /* bits changed since last upload (set on Unlock) */
};
struct GLClipper  { IDirectDrawClipperVtbl* lpVtbl; HWND hwnd; };
struct GLPalette  { IDirectDrawPaletteVtbl* lpVtbl; PALETTEENTRY ent[256]; };
struct GLVB7      { IDirect3DVertexBuffer7Vtbl* lpVtbl; D3DVERTEXBUFFERDESC d; void* data; };
struct GLDevice7  { IDirect3DDevice7Vtbl* lpVtbl; };
struct GLD3D7     { IDirect3D7Vtbl* lpVtbl; };
struct GLDD7      { IDirectDraw7Vtbl* lpVtbl; int ref; HWND hwnd; DWORD coopFlags; };

/* one shared instance for the singleton sub-objects */
static IDirectDrawSurface7Vtbl  g_surfVtbl;
static IDirectDrawClipperVtbl   g_clipVtbl;
static IDirectDrawPaletteVtbl   g_palVtbl;
static IDirect3DVertexBuffer7Vtbl g_vbVtbl;
static IDirect3DDevice7Vtbl     g_devVtbl;
static IDirect3D7Vtbl           g_d3dVtbl;
static IDirectDraw7Vtbl         g_ddVtbl;
static GLDevice7 g_theDevice;
static GLD3D7    g_theD3D;

static ULONG generic_release(void*) { return 0; }   /* leak-on-release for skeleton objs */
static ULONG generic_addref(void*)  { return 1; }

/* ============================ surface methods ============================= */
static size_t surf_bytes(int w, int h, int bpp) { return (size_t)w * h * ((bpp+7)/8 ? (bpp+7)/8 : 4); }

static HRESULT SURF_GetSurfaceDesc(IDirectDrawSurface7* This, LPDDSURFACEDESC2 d) {
	GLSurface7* s = (GLSurface7*)This;
	if (d) { *d = s->desc; d->dwWidth = s->w; d->dwHeight = s->h; d->lPitch = s->w * ((s->bpp+7)/8); }
	return DD_OK;
}
static HRESULT SURF_Lock(IDirectDrawSurface7* This, LPRECT, LPDDSURFACEDESC2 d, DWORD, HANDLE) {
	GLSurface7* s = (GLSurface7*)This;
	if (!s->bits) { s->bytes = surf_bytes(s->w, s->h, s->bpp); s->bits = calloc(1, s->bytes ? s->bytes : 1); }
	if (d) { *d = s->desc; d->dwWidth=s->w; d->dwHeight=s->h; d->lPitch = s->w*((s->bpp+7)/8); d->lpSurface = s->bits; }
	return DD_OK;
}
static HRESULT SURF_Unlock(IDirectDrawSurface7* This, LPRECT) { ((GLSurface7*)This)->texDirty=1; return DD_OK; }
static HRESULT SURF_GetAttachedSurface(IDirectDrawSurface7* This, LPDDSCAPS2 caps, IDirectDrawSurface7** out) {
	GLSurface7* s = (GLSurface7*)This;
	if (!out) return DDERR_INVALIDPARAMS;
	if (caps && (caps->dwCaps & DDSCAPS_ZBUFFER)) { *out = (IDirectDrawSurface7*)s->zbuf; return s->zbuf?DD_OK:DDERR_NOTFOUND; }
	*out = (IDirectDrawSurface7*)s->back;          /* default: back buffer */
	return s->back ? DD_OK : DDERR_NOTFOUND;
}
static HRESULT SURF_AddAttachedSurface(IDirectDrawSurface7* This, IDirectDrawSurface7* a) {
	GLSurface7* s = (GLSurface7*)This; GLSurface7* as = (GLSurface7*)a;
	if (as && (as->desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)) s->zbuf = as; else s->back = as;
	return DD_OK;
}
static HRESULT SURF_GetPixelFormat(IDirectDrawSurface7* This, LPDDPIXELFORMAT pf) {
	GLSurface7* s = (GLSurface7*)This; if (pf) *pf = s->desc.ddpfPixelFormat; return DD_OK;
}
/* Present a surface's pixels to the GL window: upload as a texture and draw a
   fullscreen quad (compatibility-profile immediate mode), then swap. This is the
   path 2D (DDraw blits/locks) and, later, 3D both reach the screen through. */
static GLuint g_presentTex = 0;
static void present_dbg(const char* path)
{
	/* MA_TRACE_FPS: frame-rate over the present path (every present, 2D + 3D). Reports the
	   instantaneous fps each ~1s window plus the running average. B3 acceptance gate. */
	if (getenv("MA_TRACE_FPS")) {
		static unsigned t0=0, last=0; static long total=0, window=0;
		unsigned now=(unsigned)SDL_GetTicks(); total++; window++;
		if (!t0) { t0=last=now; }
		else if (now-last >= 1000) {
			fprintf(stderr,"[fps] %.1f inst | %.1f avg | %ld frames / %.1fs\n",
				window*1000.0/(now-last), total*1000.0/(now-t0), total, (now-t0)/1000.0);
			last=now; window=0;
		}
	}
	if (!getenv("BOB_TRACE_PRESENT") && !getenv("BOB_DUMP_FRAME")) return;
	static int frames=0; frames++;
	if (getenv("BOB_TRACE_PRESENT") && (frames<=3 || (frames%60)==0)) {
		unsigned char px[3]={0,0,0};
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(g_scrW/2,g_scrH/2,1,1,GL_RGB,GL_UNSIGNED_BYTE,px);
		fprintf(stderr,"[present] frame %d via %s centre rgb=(%d,%d,%d) glErr=%d\n",
			frames,path,px[0],px[1],px[2],(int)glGetError());
	}
	const char* df = getenv("BOB_DUMP_FRAME");
	if (df && frames == atoi(df)) {
		int w=g_scrW,h=g_scrH; unsigned char* buf=(unsigned char*)malloc(w*h*3);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);  /* rows are w*3 bytes; default pack-align 4 misaligns non-4-divisible widths (e.g. the 1021-wide campaign map) -> RGB-shift 'speckle' */
		glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,buf);
		/* raw POSIX open() to bypass the game's redirected fopen */
		int fd=::open("/tmp/bobframe.ppm",O_WRONLY|O_CREAT|O_TRUNC,0644);
		if (fd>=0){ char hdr[64]; int n=snprintf(hdr,sizeof(hdr),"P6\n%d %d\n255\n",w,h);
			if (write(fd,hdr,n)<0){} for (int y=h-1;y>=0;y--) if(write(fd,buf+y*w*3,w*3)<0){}
			close(fd); fprintf(stderr,"[present] dumped frame %d to /tmp/bobframe.ppm (%dx%d) glErr=%d\n",frames,w,h,(int)glGetError()); }
		else fprintf(stderr,"[present] dump open failed errno path\n");
		free(buf);
		if (getenv("BOB_EXIT_AFTER_DUMP")) { ma_save_preferences(); fflush(stderr); _exit(0); }
	}
}
static void present_surface(GLSurface7* s)
{
	gl_bind_thread();
	/* 3D frame: the scene is already in the GL framebuffer; just swap it (don't upload
	   the back buffer's untouched system-memory bits over the top). */
	if (g_devRendered) { g_devRendered = 0; if (g_win) { present_dbg("3d-fb"); SDL_GL_SwapWindow(g_win); } return; }
	if (!g_win || !s || !s->bits || s->w<=0 || s->h<=0) { if (g_win) { present_dbg("3d-fb"); SDL_GL_SwapWindow(g_win); } return; }
	if (!g_presentTex) { glGenTextures(1, &g_presentTex); }
	glBindTexture(GL_TEXTURE_2D, g_presentTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (s->bpp == 16)
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,s->w,s->h,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,s->bits);
	else
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,s->w,s->h,0,GL_BGRA,GL_UNSIGNED_BYTE,s->bits);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glViewport(0,0,g_scrW,g_scrH);
	glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,1,0,1,-1,1);
	glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);                 /* V flipped: DDraw top-left -> GL bottom-left */
		glTexCoord2f(0,1); glVertex2f(0,0);
		glTexCoord2f(1,1); glVertex2f(1,0);
		glTexCoord2f(1,0); glVertex2f(1,1);
		glTexCoord2f(0,0); glVertex2f(0,1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	present_dbg("2d-blit");
	SDL_GL_SwapWindow(g_win);
}

/* ===== Mig Alley legacy DirectDraw/D3D -> SDL bridge ====================== *
 * The legacy DX1/DX2 path (compat/ddraw_legacy.h) and the Rowan D3D wrapper drive
 * the screen directly. These C hooks let that path create the window, set the
 * 8-bit palette, and present a software framebuffer through the same GL machinery
 * the DX7 path uses. */
static unsigned int g_maPal[256];   /* 0x00RRGGBB per index, from the D3D SetPalette path */

extern "C" void ma_ddraw_ensure_window(int w, int h) {
	if (w > 0 && h > 0) { g_scrW = w; g_scrH = h; }
	ensure_window(g_scrW, g_scrH);
}

extern "C" void ma_ddraw_setpalette(const unsigned char* rgb, int n) {
	if (!rgb) return; if (n > 256) n = 256;
	for (int i = 0; i < n; i++)
		g_maPal[i] = ((unsigned)rgb[i*3] << 16) | ((unsigned)rgb[i*3+1] << 8) | (unsigned)rgb[i*3+2];
}

/* Upload a top-down BGRA buffer as a full-window textured quad and swap. The
   GDI software-canvas layer (ma_gdi.cpp) composes the whole front-end into one
   BGRA canvas and presents it through here once per idle frame. */
extern "C" void ma_gl_blit_bgra(const void* px, int w, int h) {
	if (!px || w <= 0 || h <= 0) return;
	ma_ddraw_ensure_window(w, h);
	if (!g_win) return;
	gl_bind_thread();
	if (!g_presentTex) glGenTextures(1, &g_presentTex);
	glBindTexture(GL_TEXTURE_2D, g_presentTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, px);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glViewport(0, 0, g_scrW, g_scrH);
	glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,1,0,1,-1,1);
	glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
	glDisable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);                 /* texcoord V flipped: row 0 (top) -> top of screen */
		glTexCoord2f(0,0); glVertex2f(0,1);
		glTexCoord2f(1,0); glVertex2f(1,1);
		glTexCoord2f(1,1); glVertex2f(1,0);
		glTexCoord2f(0,1); glVertex2f(0,0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	present_dbg("gdi-canvas");
	SDL_GL_SwapWindow(g_win);
}

/* Present a locked surface's bits: 8-bit indexed (via g_maPal) or 16-bit 5_6_5. */
extern "C" void ma_ddraw_present(const void* bits, int w, int h, int bpp) {
	if (!bits || w <= 0 || h <= 0) return;
	ma_ddraw_ensure_window(w, h);
	if (!g_win) return;
	gl_bind_thread();
	const void* upload = bits;
	GLenum fmt = GL_BGRA, type = GL_UNSIGNED_BYTE; GLint internal = GL_RGBA;
	static unsigned int* conv = 0; static int convCap = 0;
	if (bpp == 8) {
		if (convCap < w*h) { free(conv); conv = (unsigned int*)malloc((size_t)w*h*4); convCap = w*h; }
		const unsigned char* src = (const unsigned char*)bits;
		for (int i = 0; i < w*h; i++) conv[i] = 0xFF000000u | g_maPal[src[i]];
		upload = conv;
	} else if (bpp == 16) { fmt = GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; internal = GL_RGB; }
	if (!g_presentTex) glGenTextures(1, &g_presentTex);
	glBindTexture(GL_TEXTURE_2D, g_presentTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, fmt, type, upload);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glViewport(0, 0, g_scrW, g_scrH);
	glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,1,0,1,-1,1);
	glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
	glDisable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);                 /* V flipped: DDraw top-left -> GL bottom-left */
		glTexCoord2f(0,1); glVertex2f(0,0);
		glTexCoord2f(1,1); glVertex2f(1,0);
		glTexCoord2f(1,0); glVertex2f(1,1);
		glTexCoord2f(0,0); glVertex2f(0,1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	present_dbg("legacy-2d");
	SDL_GL_SwapWindow(g_win);
}

/* GDI SetDIBitsToDevice -> present. The RDialog front-end blits its full-screen background
 * art as a Windows DIB (BMP) via SetDIBitsToDevice (RDIALOG.CPP:OnPaint). Decode the DIB
 * (8-bit palettized / 24-bit BGR / 32-bit BGRA, bottom-up unless biHeight<0) to RGBA and
 * present it through the same GL path. */
extern "C" void ma_gdi_present_dib(int /*dx*/, int /*dy*/, int w, int h,
                                   const void* bits, const void* bmiv)
{
	const BITMAPINFO* bmi = (const BITMAPINFO*)bmiv;
	if (!bits || !bmi || w <= 0 || h <= 0) return;
	const BITMAPINFOHEADER* bh = &bmi->bmiHeader;
	int bpp = bh->biBitCount;
	if (getenv("MA_TRACE_DIB")) { static int n=0; if(n++<40)
		fprintf(stderr,"[dib] present w=%d h=%d bmiW=%d bmiH=%d bpp=%d\n",
			w,h,(int)bh->biWidth,(int)bh->biHeight,bpp); }
	int H = bh->biHeight < 0 ? -bh->biHeight : bh->biHeight;
	int W = bh->biWidth;
	if (W <= 0 || H <= 0) return;
	int topdown = bh->biHeight < 0;
	int srcpitch = ((W * bpp + 31) / 32) * 4;          /* DIB rows are DWORD-aligned */
	const RGBQUAD* pal = bmi->bmiColors;                /* 8-bit palette */
	const unsigned char* src8 = (const unsigned char*)bits;
	static unsigned int* rgba = 0; static int cap = 0;
	if (cap < W*H) { free(rgba); rgba = (unsigned int*)malloc((size_t)W*H*4); cap = W*H; }
	if (!rgba) return;
	for (int y = 0; y < H; y++) {
		int srcrow = topdown ? y : (H - 1 - y);        /* output top-down */
		const unsigned char* s = src8 + (size_t)srcrow * srcpitch;
		unsigned int* d = rgba + (size_t)y * W;
		if (bpp == 8) {
			for (int x = 0; x < W; x++) { const RGBQUAD& c = pal[s[x]];
				d[x] = 0xFF000000u | (c.rgbRed<<16) | (c.rgbGreen<<8) | c.rgbBlue; }
		} else if (bpp == 24) {
			for (int x = 0; x < W; x++) { const unsigned char* p = s + x*3;
				d[x] = 0xFF000000u | (p[2]<<16) | (p[1]<<8) | p[0]; }   /* BGR */
		} else { /* 32 */
			for (int x = 0; x < W; x++) { const unsigned char* p = s + x*4;
				d[x] = 0xFF000000u | (p[2]<<16) | (p[1]<<8) | p[0]; }
		}
	}
	ma_ddraw_ensure_window(W, H);
	if (!g_win) return;
	gl_bind_thread();
	if (!g_presentTex) glGenTextures(1, &g_presentTex);
	glBindTexture(GL_TEXTURE_2D, g_presentTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_BGRA, GL_UNSIGNED_BYTE, rgba);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glViewport(0, 0, g_scrW, g_scrH);
	glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,1,0,1,-1,1);
	glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
	glDisable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);                 /* texcoord V flipped: row 0 (top) -> top of screen */
		glTexCoord2f(0,0); glVertex2f(0,1);
		glTexCoord2f(1,0); glVertex2f(1,1);
		glTexCoord2f(1,1); glVertex2f(1,0);
		glTexCoord2f(0,1); glVertex2f(0,0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	present_dbg("gdi-dib");
	SDL_GL_SwapWindow(g_win);
}

static void surf_alloc_bits(GLSurface7* s) {
	if (s && !s->bits && s->w>0 && s->h>0) { s->bytes = surf_bytes(s->w, s->h, s->bpp); s->bits = calloc(1, s->bytes?s->bytes:1); }
}
/* Real surface->surface copy: the 3D texture pipeline (Lib3D::_CreateTextureMap) Locks a
   temp surface, writes the palette-converted pixels, then BltFast's it into the texture
   surface that actually gets bound. Without a real copy here, every uploaded texture is
   blank (white). Same-format copies are a memcpy; size mismatch (mip levels) nearest-scales. */
static HRESULT SURF_BltFast(IDirectDrawSurface7* This, DWORD dx, DWORD dy, IDirectDrawSurface7* src, LPRECT srcRect, DWORD) {
	GLSurface7* d=(GLSurface7*)This; GLSurface7* s=(GLSurface7*)src;
	if (!d || !s) return DD_OK;
	surf_alloc_bits(s); surf_alloc_bits(d);
	if (!s->bits || !d->bits || d->bpp!=s->bpp) { if(d) d->texDirty=1; return DD_OK; }
	int bpp=(s->bpp+7)/8;
	int sx0=0, sy0=0, sw=s->w, sh=s->h;
	if (srcRect) { sx0=srcRect->left; sy0=srcRect->top; sw=srcRect->right-srcRect->left; sh=srcRect->bottom-srcRect->top; }
	for (int y=0;y<sh;y++) {
		int dyy=(int)dy+y, syy=sy0+y;
		if (dyy<0||dyy>=d->h||syy<0||syy>=s->h) continue;
		int cw=sw, cdx=(int)dx;
		if (cdx+cw>d->w) cw=d->w-cdx;
		if (cdx<0 || cw<=0 || sx0<0 || sx0+cw>s->w) continue;
		memcpy((char*)d->bits+((size_t)dyy*d->w+cdx)*bpp, (char*)s->bits+((size_t)syy*s->w+sx0)*bpp, (size_t)cw*bpp);
	}
	d->texDirty=1;
	return DD_OK;
}
static HRESULT SURF_Blt(IDirectDrawSurface7* This, LPRECT, IDirectDrawSurface7* src, LPRECT, DWORD flags, LPDDBLTFX fx) {
	GLSurface7* d=(GLSurface7*)This;
	if (!d) { pump_events(); return DD_OK; }
	surf_alloc_bits(d);
	if ((flags & DDBLT_COLORFILL) && d->bits) {
		int bpp=(d->bpp+7)/8; size_t n=(size_t)d->w*d->h;
		if (bpp==2) { unsigned short v=(unsigned short)(fx?fx->dwFillColor:0), *p=(unsigned short*)d->bits; for(size_t i=0;i<n;i++)p[i]=v; }
		else if (bpp==4) { unsigned int v=(fx?fx->dwFillColor:0), *p=(unsigned int*)d->bits; for(size_t i=0;i<n;i++)p[i]=v; }
		else memset(d->bits,0,n*bpp);
		d->texDirty=1; pump_events(); return DD_OK;
	}
	if (src) {
		GLSurface7* s=(GLSurface7*)src; surf_alloc_bits(s);
		if (s->bits && d->bits && d->bpp==s->bpp) {
			int bpp=(s->bpp+7)/8;
			if (d->w==s->w && d->h==s->h) memcpy(d->bits,s->bits,(size_t)d->w*d->h*bpp);
			else for (int y=0;y<d->h;y++){ int sy=s->h?y*s->h/d->h:0; for(int x=0;x<d->w;x++){ int sx=s->w?x*s->w/d->w:0;
				memcpy((char*)d->bits+((size_t)y*d->w+x)*bpp,(char*)s->bits+((size_t)sy*s->w+sx)*bpp,bpp);} }
		}
		d->texDirty=1;
	}
	pump_events();
	return DD_OK;
}
static HRESULT SURF_Flip(IDirectDrawSurface7* This, IDirectDrawSurface7*, DWORD) {
	GLSurface7* s = (GLSurface7*)This;
	present_surface(s->back ? s->back : s);   /* present the back buffer */
	pump_events();
	return DD_OK;
}

/* Smoke test (BOB_VID_SMOKETEST=1): open the window, fill a back surface with a
   gradient and present it, to verify the SDL2/GL window + present pipeline works
   end-to-end independent of the (not-yet-driven) MFC UI flow. */
extern "C" int bob_video_smoketest(void)
{
	ensure_window(800, 600);
	if (!g_win) { fprintf(stderr, "[vid] smoketest: no window\n"); return 0; }
	int w=800,h=600;
	static unsigned short px[800*600];
	for (int y=0;y<h;y++) for (int x=0;x<w;x++) {
		int r=(x*31/w), g=(y*63/h), b=((x+y)*31/(w+h));
		px[y*w+x] = (unsigned short)((r<<11)|(g<<5)|b);
	}
	GLSurface7 s; memset(&s,0,sizeof(s)); s.w=w; s.h=h; s.bpp=16; s.bits=px;
	for (int f=0; f<120; f++) { present_surface(&s); pump_events(); SDL_Delay(16); }
	unsigned char mid[3]={0,0,0};
	glReadPixels(w/2,h/2,1,1,GL_RGB,GL_UNSIGNED_BYTE,mid);
	fprintf(stderr,"[vid] smoketest presented; centre pixel rgb=(%d,%d,%d) glErr=%d\n",
		mid[0],mid[1],mid[2],(int)glGetError());
	return 1;
}
static HRESULT SURF_SetPalette(IDirectDrawSurface7*, LPDIRECTDRAWPALETTE) { return DD_OK; }
static HRESULT SURF_SetClipper(IDirectDrawSurface7*, LPDIRECTDRAWCLIPPER) { return DD_OK; }
static HRESULT SURF_IsLost(IDirectDrawSurface7*) { return DD_OK; }
static HRESULT SURF_Restore(IDirectDrawSurface7*) { return DD_OK; }
static HRESULT SURF_GetDC(IDirectDrawSurface7*, HDC* p) { if (p) *p = (HDC)0; return DD_OK; }
static HRESULT SURF_ReleaseDC(IDirectDrawSurface7*, HDC) { return DD_OK; }
static HRESULT SURF_PageLock(IDirectDrawSurface7*, DWORD) { return DD_OK; }
static HRESULT SURF_PageUnlock(IDirectDrawSurface7*, DWORD) { return DD_OK; }
/* {69C11C3E-B46B-11D1-AD7A-00C04FC29B4E} (defined in bob_dx_extra.h, not pulled in here) */
static const GUID BOB_IID_GammaControl =
  {0x69C11C3E,0xB46B,0x11D1,{0xAD,0x7A,0,0xC0,0x4F,0xC2,0x9B,0x4E}};
static HRESULT SURF_QueryInterface(IDirectDrawSurface7* This, REFIID riid, void** ppv) {
	/* Reject IDirectDrawGammaControl: we don't expose a hardware gamma ramp. Returning
	   the surface itself (default below) would hand back a vtbl whose SetGammaRamp slot
	   is some unrelated surface method -> NULL/garbage call. Failing the QI makes
	   Lib3D::SetGamma return early (no gamma adjust), which is purely cosmetic. */
	if (riid == BOB_IID_GammaControl) { if (ppv) *ppv = NULL; return E_NOINTERFACE; }
	if (ppv) *ppv = This; return DD_OK;
}
/* Real COM refcounting (cross-port note, BoB inc 4.3b): free-on-first-Release UAFs the moment the
   engine does a balanced AddRef/Release (or releases a surface it still holds — e.g. the
   menu->fly->exit->menu teardown). Free only when the count reaches 0. */
static ULONG   SURF_AddRef(IDirectDrawSurface7* This) { GLSurface7* s=(GLSurface7*)This; return (ULONG)++s->ref; }
static ULONG   SURF_Release(IDirectDrawSurface7* This) {
	GLSurface7* s=(GLSurface7*)This;
	if (--s->ref > 0) return (ULONG)s->ref;
	if(s->bits) free(s->bits); free(s); return 0;
}
static HRESULT SURF_GetCaps(IDirectDrawSurface7* This, LPDDSCAPS2 c) { GLSurface7* s=(GLSurface7*)This; if(c)*c=s->desc.ddsCaps; return DD_OK; }

static GLSurface7* make_surface(const DDSURFACEDESC2* in, int defW, int defH)
{
	GLSurface7* s = (GLSurface7*)calloc(1, sizeof(GLSurface7));
	s->lpVtbl = &g_surfVtbl;
	s->ref = 1;
	if (in) s->desc = *in;
	s->desc.dwSize = sizeof(DDSURFACEDESC2);
	s->w = (in && (in->dwFlags & DDSD_WIDTH)  && in->dwWidth)  ? (int)in->dwWidth  : defW;
	s->h = (in && (in->dwFlags & DDSD_HEIGHT) && in->dwHeight) ? (int)in->dwHeight : defH;
	s->bpp = (in && (in->dwFlags & DDSD_PIXELFORMAT) && in->ddpfPixelFormat.dwRGBBitCount)
	          ? (int)in->ddpfPixelFormat.dwRGBBitCount : 16;
	VLOG("CreateSurface %dx%d bpp%d caps=%08x\n", s->w, s->h, s->bpp,
	     in?(unsigned)in->ddsCaps.dwCaps:0);
	return s;
}

/* ============================ IDirectDraw7 methods ======================= */
static HRESULT DD_CreateSurface(IDirectDraw7*, LPDDSURFACEDESC2 d, IDirectDrawSurface7** out, IUnknown*) {
	if (!out) return DDERR_INVALIDPARAMS;
	/* Render-to-texture (TEXTURE+3DDEVICE) — used by Lib3D's water/mirror reflection
	   probe (CheckIfTextureCanBeRenderTarget). Our GL backend has no FBO RTT yet, so
	   report the surface uncreatable: the probe then takes its designed fallback
	   (render straight to the back buffer, no mirror), exactly as on HW that lacks RTT. */
	if (d && (d->ddsCaps.dwCaps & DDSCAPS_TEXTURE) && (d->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)) {
		*out = NULL;
		return DDERR_OUTOFVIDEOMEMORY;
	}
	GLSurface7* s = make_surface(d, g_scrW, g_scrH);
	/* complex flip chain -> also make the back buffer and attach it */
	if (d && (d->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
		s->isPrimary = 1;
		if (d->dwFlags & DDSD_BACKBUFFERCOUNT && d->dwBackBufferCount > 0) {
			DDSURFACEDESC2 bd; memset(&bd,0,sizeof(bd)); bd.dwSize=sizeof(bd);
			bd.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
			bd.dwWidth=g_scrW; bd.dwHeight=g_scrH; bd.ddsCaps.dwCaps=DDSCAPS_BACKBUFFER;
			bd.ddpfPixelFormat=d->ddpfPixelFormat;
			s->back = make_surface(&bd, g_scrW, g_scrH);
		}
	}
	*out = (IDirectDrawSurface7*)s;
	return DD_OK;
}
static HRESULT DD_SetCooperativeLevel(IDirectDraw7* This, HWND h, DWORD f) {
	GLDD7* dd=(GLDD7*)This; dd->hwnd=h; dd->coopFlags=f; ensure_window(g_scrW, g_scrH); return DD_OK;
}
static HRESULT DD_SetDisplayMode(IDirectDraw7*, DWORD w, DWORD h, DWORD, DWORD, DWORD) { if (getenv("MA_TRACE_RES")) fprintf(stderr,"[res] DD_SetDisplayMode(%lu,%lu)\n",(unsigned long)w,(unsigned long)h); ensure_window((int)w,(int)h); return DD_OK; }
static HRESULT DD_RestoreDisplayMode(IDirectDraw7*) { return DD_OK; }
static HRESULT DD_GetCaps(IDirectDraw7*, LPDDCAPS a, LPDDCAPS b) {
	if (a) { memset(a,0,sizeof(DDCAPS)); a->dwSize=sizeof(DDCAPS); a->dwVidMemTotal=256u*1024*1024; a->dwVidMemFree=256u*1024*1024; }
	if (b) { memset(b,0,sizeof(DDCAPS)); b->dwSize=sizeof(DDCAPS); }
	return DD_OK;
}
static HRESULT DD_GetAvailableVidMem(IDirectDraw7*, LPDDSCAPS2, LPDWORD tot, LPDWORD freeM) {
	if (tot) *tot = 256u*1024*1024; if (freeM) *freeM = 256u*1024*1024; return DD_OK;
}
static HRESULT DD_GetDeviceIdentifier(IDirectDraw7*, LPDDDEVICEIDENTIFIER2 id, DWORD) {
	if (id) { memset(id,0,sizeof(*id)); strncpy(id->szDescription,"BoB Linux OpenGL backend",sizeof(id->szDescription)-1); }
	return DD_OK;
}
static HRESULT DD_GetDisplayMode(IDirectDraw7*, LPDDSURFACEDESC2 d) {
	if (d) { memset(d,0,sizeof(*d)); d->dwSize=sizeof(*d); d->dwFlags=DDSD_WIDTH|DDSD_HEIGHT; d->dwWidth=g_scrW; d->dwHeight=g_scrH; }
	return DD_OK;
}
static HRESULT DD_CreateClipper(IDirectDraw7*, DWORD, LPDIRECTDRAWCLIPPER* out, IUnknown*) {
	GLClipper* c=(GLClipper*)calloc(1,sizeof(GLClipper)); c->lpVtbl=&g_clipVtbl; if(out)*out=(IDirectDrawClipper*)c; return DD_OK;
}
static HRESULT DD_CreatePalette(IDirectDraw7*, DWORD, LPPALETTEENTRY src, LPDIRECTDRAWPALETTE* out, IUnknown*) {
	GLPalette* p=(GLPalette*)calloc(1,sizeof(GLPalette)); p->lpVtbl=&g_palVtbl;
	if (src) memcpy(p->ent, src, sizeof(p->ent));
	if (out) *out=(IDirectDrawPalette*)p; return DD_OK;
}
static HRESULT DD_QueryInterface(IDirectDraw7*, REFIID riid, void** ppv) {
	if (!ppv) return DDERR_INVALIDPARAMS;
	if (riid == IID_IDirect3D7) { *ppv = (void*)&g_theD3D; return DD_OK; }
	*ppv = NULL; return E_NOINTERFACE;
}
static ULONG DD_AddRef(IDirectDraw7* This) { GLDD7* dd=(GLDD7*)This; return (ULONG)++dd->ref; }
static ULONG DD_Release(IDirectDraw7* This) {
	GLDD7* dd=(GLDD7*)This;
	if (--dd->ref > 0) return (ULONG)dd->ref;        /* real refcount — see SURF_Release note */
	free((void*)dd); return 0;
}
/* Report a couple of display modes so EnumerateDriverModes builds a list. */
static HRESULT DD_EnumDisplayModes(IDirectDraw7*, DWORD, LPDDSURFACEDESC2, LPVOID ctx, LPDDENUMMODESCALLBACK2 cb) {
	if (!cb) return DD_OK;
	/* Must include every mode the resolution combo offers (Win3d.cpp ma_populate_software_modes),
	   else a selected mode has no matching DD.DDModes entry and the windowed flight can't apply it. */
	static const int modes[][2] = {{640,480},{800,600},{1024,768},{1280,960},{1280,1024},{1280,720},{1600,1200},{1920,1080}};
	static const int bpps[] = {16, 32};
	for (unsigned m=0; m<sizeof(modes)/sizeof(modes[0]); ++m)
	for (unsigned b=0; b<2; ++b) {
		DDSURFACEDESC2 d; memset(&d,0,sizeof(d)); d.dwSize=sizeof(d);
		d.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_PIXELFORMAT;
		d.dwWidth=modes[m][0]; d.dwHeight=modes[m][1];
		d.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
		d.ddpfPixelFormat.dwFlags=DDPF_RGB;
		d.ddpfPixelFormat.dwRGBBitCount=bpps[b];
		if (bpps[b]==16){ d.ddpfPixelFormat.dwRBitMask=0xF800; d.ddpfPixelFormat.dwGBitMask=0x07E0; d.ddpfPixelFormat.dwBBitMask=0x001F; }
		else            { d.ddpfPixelFormat.dwRBitMask=0xFF0000; d.ddpfPixelFormat.dwGBitMask=0x00FF00; d.ddpfPixelFormat.dwBBitMask=0x0000FF; }
		if (cb(&d, ctx) == 0 /*DDENUMRET_CANCEL*/) return DD_OK;
	}
	return DD_OK;
}

/* ============================ IDirect3D7 methods ========================= */
typedef HRESULT (*D3DEnumDevCB)(LPSTR, LPSTR, LPD3DDEVICEDESC7, LPVOID);
typedef HRESULT (*D3DEnumZCB)(LPDDPIXELFORMAT, LPVOID);

static void fill_devdesc(D3DDEVICEDESC7* dd, const GUID* guid) {
	memset(dd, 0, sizeof(*dd));
	dd->deviceGUID = *guid;
	dd->dwDevCaps = 0xFFFFFFFF;
	dd->dwDeviceRenderBitDepth = DDBD_16 | DDBD_32;
	dd->dwDeviceZBufferBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
	dd->dwMinTextureWidth = 1; dd->dwMinTextureHeight = 1;
	dd->dwMaxTextureWidth = 4096; dd->dwMaxTextureHeight = 4096;
	dd->dwMaxTextureRepeat = 4096; dd->dwMaxTextureAspectRatio = 4096;
	dd->dwMaxAnisotropy = 16; dd->dwMaxActiveLights = 8;
	dd->dwTextureOpCaps = 0xFFFFFFFF; dd->dwFVFCaps = 0xFFFFFFFF;
	dd->dwVertexProcessingCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwSize = sizeof(dd->dpcLineCaps);
	dd->dpcLineCaps.dwTextureCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwTextureFilterCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwTextureBlendCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwTextureAddressCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwShadeCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwRasterCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwZCmpCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwSrcBlendCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwDestBlendCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwAlphaCmpCaps = 0xFFFFFFFF;
	dd->dpcTriCaps = dd->dpcLineCaps;
}
static HRESULT D3D_EnumDevices(IDirect3D7*, void* cbv, LPVOID arg) {
	D3DEnumDevCB cb = (D3DEnumDevCB)cbv; if (!cb) return DD_OK;
	D3DDEVICEDESC7 dd;
	/* HAL first, then TnL-HAL (preferred -> the game cancels on it) */
	fill_devdesc(&dd, &IID_IDirect3DHALDevice);
	if (cb((LPSTR)"HAL", (LPSTR)"OpenGL HAL", &dd, arg) == 0) return DD_OK;
	fill_devdesc(&dd, &IID_IDirect3DTnLHalDevice);
	cb((LPSTR)"TnLHAL", (LPSTR)"OpenGL TnL HAL", &dd, arg);
	return DD_OK;
}
static HRESULT D3D_EnumZBufferFormats(IDirect3D7*, REFCLSID, void* cbv, LPVOID ctx) {
	D3DEnumZCB cb = (D3DEnumZCB)cbv; if (!cb) return DD_OK;
	DDPIXELFORMAT pf;
	memset(&pf,0,sizeof(pf)); pf.dwSize=sizeof(pf); pf.dwFlags=DDPF_ZBUFFER;
	pf.dwZBufferBitDepth=16; pf.dwZBitMask=0x0000FFFF; if (cb(&pf,ctx)==0) return DD_OK;
	memset(&pf,0,sizeof(pf)); pf.dwSize=sizeof(pf); pf.dwFlags=DDPF_ZBUFFER;
	pf.dwZBufferBitDepth=32; pf.dwZBitMask=0xFFFFFFFF; cb(&pf,ctx);
	return DD_OK;
}
static HRESULT D3D_CreateDevice(IDirect3D7*, REFCLSID, LPDIRECTDRAWSURFACE7, LPDIRECT3DDEVICE7* out) {
	if (out) *out = (IDirect3DDevice7*)&g_theDevice; return DD_OK;
}
static HRESULT D3D_CreateVertexBuffer(IDirect3D7*, LPD3DVERTEXBUFFERDESC d, LPDIRECT3DVERTEXBUFFER7* out, DWORD) {
	GLVB7* vb=(GLVB7*)calloc(1,sizeof(GLVB7)); vb->lpVtbl=&g_vbVtbl;
	if (d) { vb->d=*d; size_t n = (size_t)(d->dwNumVertices?d->dwNumVertices:1) * 64; vb->data=calloc(1,n); }
	if (out) *out=(IDirect3DVertexBuffer7*)vb; return DD_OK;
}
static HRESULT D3D_EvictManagedTextures(IDirect3D7*) { return DD_OK; }
static HRESULT D3D_QueryInterface(IDirect3D7* This, REFIID, void** ppv) { if (ppv) *ppv=This; return DD_OK; }

/* ============================ vertex-buffer methods ====================== */
static HRESULT VB_Lock(IDirect3DVertexBuffer7* This, DWORD, LPVOID* pp, LPDWORD ps) {
	GLVB7* vb=(GLVB7*)This; if(pp)*pp=vb->data; if(ps)*ps = vb->d.dwNumVertices*64; return DD_OK;
}
static HRESULT VB_Unlock(IDirect3DVertexBuffer7*) { return DD_OK; }
static HRESULT VB_ProcessVertices(IDirect3DVertexBuffer7*, DWORD, DWORD, DWORD, IDirect3DVertexBuffer7*, DWORD, IDirect3DDevice7*, DWORD) { return DD_OK; }
static ULONG   VB_Release(IDirect3DVertexBuffer7* This) { GLVB7* vb=(GLVB7*)This; if(vb->data)free(vb->data); free(vb); return 0; }

/* ====================================================================== *
 * Phase 1a: D3D7 device -> OpenGL, the 2D textured-quad path.             *
 * The game's Lib3D fills a vertex buffer with R3DTLVERTEX (FVF XYZRHW |   *
 * DIFFUSE | 1 texcoord set = pre-transformed screen-space quads) and      *
 * submits DrawPrimitiveVB(TRIANGLEFAN,...). We bind the SetTexture        *
 * surface as a GL texture, set up screen-space ortho + client arrays from *
 * the FVF, and glDrawArrays. (3D/XYZ+lit path = Phase 1b.)                *
 * ====================================================================== */

/* ---- FVF (flexible vertex format) layout ---- */
#define BFVF_XYZ      0x002
#define BFVF_XYZRHW   0x004
#define BFVF_NORMAL   0x010
#define BFVF_DIFFUSE  0x040
#define BFVF_SPECULAR 0x080
#define BFVF_TEXMASK  0xf00
#define BFVF_TEXSHIFT 8
struct FvfLayout { int stride, posOff, posComps, colOff, hasCol, texOff, hasTex; };
static FvfLayout fvf_layout(DWORD fvf) {
	FvfLayout L; memset(&L,0,sizeof(L)); int o=0;
	L.posOff=o;
	if (fvf & BFVF_XYZRHW) { L.posComps=4; o+=16; }
	else if (fvf & BFVF_XYZ) { L.posComps=3; o+=12; }
	if (fvf & BFVF_NORMAL) o+=12;
	if (fvf & BFVF_DIFFUSE) { L.colOff=o; L.hasCol=1; o+=4; }
	if (fvf & BFVF_SPECULAR) o+=4;
	int ntex=(fvf & BFVF_TEXMASK)>>BFVF_TEXSHIFT;
	if (ntex>0) { L.texOff=o; L.hasTex=1; o+=8; }  /* first 2-float set */
	o += (ntex>1)?(ntex-1)*8:0;
	L.stride=o;
	return L;
}

/* ---- device GL state ---- */
static GLSurface7* g_devTex[8] = {0};   /* SetTexture per stage */
static int g_devAlphaBlend = 0;
static GLenum gl_blend(DWORD d) {        /* D3DBLEND -> GL */
	switch(d){ case 1:return GL_ZERO; case 2:return GL_ONE; case 3:return GL_SRC_COLOR;
		case 4:return GL_SRC_ALPHA; case 5:return GL_ONE_MINUS_SRC_ALPHA;
		case 6:return GL_DST_ALPHA; case 7:return GL_ONE_MINUS_DST_ALPHA;
		case 9:return GL_DST_COLOR; case 10:return GL_ONE_MINUS_DST_COLOR; default:return GL_ONE; }
}
static GLenum g_srcBlend=GL_SRC_ALPHA, g_dstBlend=GL_ONE_MINUS_SRC_ALPHA;

/* Upload a (16-bit / 32-bit) DDraw texture surface to its GL texture. */
static void upload_texture(GLSurface7* s) {
	if (!s || !s->bits || s->w<=0 || s->h<=0) return;
	if (getenv("BOB_TRACE_TEXPIX")) { static int n=0; if(n++<24) {
		/* sample centre + corner pixels to see if the texture has real content */
		unsigned px0=0,pxc=0; if(s->bpp==16){ unsigned short* p=(unsigned short*)s->bits; px0=p[0]; pxc=p[(s->h/2)*s->w + s->w/2]; }
		else { unsigned* p=(unsigned*)s->bits; px0=p[0]; pxc=p[(s->h/2)*s->w + s->w/2]; }
		fprintf(stderr,"[texpix] %dx%d bpp%d px[0]=0x%x centre=0x%x\n",s->w,s->h,s->bpp,px0,pxc); } }
	if (getenv("BOB_DUMP_TEX") && s->w>=64 && s->h>=64) {
		static int td=0; int cap=atoi(getenv("BOB_DUMP_TEX")); if(cap<=0)cap=6;
		if (td<cap) {
			char path[64]; snprintf(path,sizeof(path),"/tmp/bobtex_%d.ppm",td);
			int fd=::open(path,O_WRONLY|O_CREAT|O_TRUNC,0644);
			if(fd>=0){ char hdr[64]; int n=snprintf(hdr,sizeof(hdr),"P6\n%d %d\n255\n",s->w,s->h); if(write(fd,hdr,n)<0){}
				for(int i=0;i<s->w*s->h;i++){ unsigned char rgb[3];
					if(s->bpp==16){ unsigned short p=((unsigned short*)s->bits)[i];
						rgb[0]=((p>>11)&0x1f)<<3; rgb[1]=((p>>5)&0x3f)<<2; rgb[2]=(p&0x1f)<<3; }
					else { unsigned p=((unsigned*)s->bits)[i]; rgb[0]=(p>>16)&0xff; rgb[1]=(p>>8)&0xff; rgb[2]=p&0xff; }
					if(write(fd,rgb,3)<0){} }
				close(fd); fprintf(stderr,"[texdump] wrote %s %dx%d bpp%d\n",path,s->w,s->h,s->bpp); td++; }
		}
	}
	if (!s->glTex) glGenTextures(1, &s->glTex);
	glBindTexture(GL_TEXTURE_2D, s->glTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	const DDPIXELFORMAT& pf = s->desc.ddpfPixelFormat;
	if (s->bpp==16) {
		GLenum type=GL_UNSIGNED_SHORT_5_6_5, fmt=GL_RGB;
		if (pf.dwRGBAlphaBitMask==0x8000) { type=GL_UNSIGNED_SHORT_1_5_5_5_REV; fmt=GL_BGRA; }
		else if (pf.dwRGBAlphaBitMask==0xF000) { type=GL_UNSIGNED_SHORT_4_4_4_4_REV; fmt=GL_BGRA; }
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,s->w,s->h,0,fmt,type,s->bits);
	} else {
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,s->w,s->h,0,GL_BGRA,GL_UNSIGNED_BYTE,s->bits);
	}
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	s->texDirty=0;
}

static HRESULT DEV_ok(IDirect3DDevice7*) { return D3D_OK; }
static HRESULT DEV_BeginScene(IDirect3DDevice7*) { gl_bind_thread(); g_devRendered=1; pump_events();
	/* BOB_DEPTH3D experiment: start each scene with a cleared depth buffer (clear=0.0,
	   GEQUAL so nearer/larger-screen-z wins) -- tests whether the flat 3D world is the
	   terrain being overdrawn in painter's order because depth is disabled. */
	if (getenv("BOB_DEPTH3D")) { glClearDepth(0.0); glClear(GL_DEPTH_BUFFER_BIT); }
	return D3D_OK; }
static HRESULT DEV_EndScene(IDirect3DDevice7*) { return D3D_OK; }
static HRESULT DEV_Clear(IDirect3DDevice7*, DWORD, LPD3DRECT, DWORD flags, D3DCOLOR col, D3DVALUE z, DWORD) {
	if (!g_win) return D3D_OK;
	gl_bind_thread();
	g_devRendered=1;
	GLbitfield m=0;
	if (flags & 0x1 /*D3DCLEAR_TARGET*/) {
		if (getenv("BOB_TRACE_FVF")) { static int c=0; if(c++<4) fprintf(stderr,"[clear] col=0x%08lx flags=%lx\n",(unsigned long)col,(unsigned long)flags); }
		glClearColor(((col>>16)&0xff)/255.f,((col>>8)&0xff)/255.f,(col&0xff)/255.f,((col>>24)&0xff)/255.f);
		m|=GL_COLOR_BUFFER_BIT;
	}
	if (flags & 0x2 /*D3DCLEAR_ZBUFFER*/) { glClearDepth(z); m|=GL_DEPTH_BUFFER_BIT; }
	if (m) glClear(m);
	return D3D_OK;
}
static HRESULT DEV_SetViewport(IDirect3DDevice7*, LPD3DVIEWPORT7 vp) {
	if (g_win && vp) glViewport(vp->dwX, vp->dwY, vp->dwWidth, vp->dwHeight);
	return D3D_OK;
}
static HRESULT DEV_GetViewport(IDirect3DDevice7*, LPD3DVIEWPORT7 v) {
	if (v) { v->dwX=0; v->dwY=0; v->dwWidth=g_scrW; v->dwHeight=g_scrH; v->dvMinZ=0; v->dvMaxZ=1; } return D3D_OK;
}
static HRESULT DEV_SetRenderTarget(IDirect3DDevice7*, LPDIRECTDRAWSURFACE7, DWORD) { return D3D_OK; }
static HRESULT DEV_SetRenderState(IDirect3DDevice7*, D3DRENDERSTATETYPE st, DWORD v) {
	/* D3DRENDERSTATE_*: SRCBLEND=19, DESTBLEND=20, ALPHABLENDENABLE=27, ZENABLE=7, ZWRITEENABLE=14 */
	switch ((int)st) {
		case 27: g_devAlphaBlend=(int)v; if(g_win){ if(v) glEnable(GL_BLEND); else glDisable(GL_BLEND);} break;
		case 19: g_srcBlend=gl_blend(v); break;
		case 20: g_dstBlend=gl_blend(v); break;
		default: break;
	}
	return D3D_OK;
}
static HRESULT DEV_GetRenderState(IDirect3DDevice7*, D3DRENDERSTATETYPE, LPDWORD v) { if(v)*v=0; return D3D_OK; }
static HRESULT DEV_SetTextureStageState(IDirect3DDevice7*, DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) { return D3D_OK; }
static HRESULT DEV_GetTextureStageState(IDirect3DDevice7*, DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD v) { if(v)*v=0; return D3D_OK; }
static HRESULT DEV_SetTransform(IDirect3DDevice7*, D3DTRANSFORMSTATETYPE, LPD3DMATRIX) { return D3D_OK; }
static HRESULT DEV_SetTexture(IDirect3DDevice7*, DWORD stage, LPDIRECTDRAWSURFACE7 tex) {
	if (stage<8) g_devTex[stage]=(GLSurface7*)tex;
	return D3D_OK;
}
static HRESULT DEV_SetMaterial(IDirect3DDevice7*, LPD3DMATERIAL7) { return D3D_OK; }
static HRESULT DEV_SetLight(IDirect3DDevice7*, DWORD, LPD3DLIGHT7) { return D3D_OK; }
static HRESULT DEV_LightEnable(IDirect3DDevice7*, DWORD, BOOL) { return D3D_OK; }

/* Render `count` verts of the FVF buffer `base` (already at start offset) as `prim`. */
static void draw_fvf(D3DPRIMITIVETYPE prim, const unsigned char* base, DWORD count, DWORD fvf) {
	if (!g_win || !base || !count) return;
	FvfLayout L = fvf_layout(fvf);
	if (!L.stride) return;
	int is2D = (fvf & BFVF_XYZRHW) != 0;     /* pre-transformed screen-space */
	/* BOB_SKIP_BACKDROP diagnostic: skip a near-full-screen near-z quad (the sky/haze
	   backdrop) to see whether terrain is rendering behind it (a draw-order problem). */
	if (is2D && getenv("BOB_SKIP_BACKDROP") && count>=3 && (fvf&BFVF_XYZRHW)) {
		float mnx=1e9f,mxx=-1e9f,mny=1e9f,mxy=-1e9f;
		for(DWORD i=0;i<count;i++){const float* p=(const float*)(base+(size_t)i*L.stride+L.posOff);
			if(p[0]<mnx)mnx=p[0]; if(p[0]>mxx)mxx=p[0]; if(p[1]<mny)mny=p[1]; if(p[1]>mxy)mxy=p[1];}
		const float* p0=(const float*)(base+L.posOff);
		if ((mxx-mnx) > g_scrW*0.7f && (mxy-mny) > g_scrH*0.7f && p0[2] < 0.01f) return;
	}
	if (getenv("BOB_TRACE_FVF")) {
		static int n2d=0, n3d=0;
		/* global screen-space bounds of ALL 2D geometry + how many quads have a texture */
		static float gminx=1e9f,gminy=1e9f,gmaxx=-1e9f,gmaxy=-1e9f; static int textured=0, untex=0;
		if (is2D) {
			for (DWORD i=0;i<count;i++){ const float* p=(const float*)(base+(size_t)i*L.stride+L.posOff);
				if(p[0]<gminx)gminx=p[0]; if(p[0]>gmaxx)gmaxx=p[0]; if(p[1]<gminy)gminy=p[1]; if(p[1]>gmaxy)gmaxy=p[1]; }
			if (g_devTex[0]) textured++; else untex++;
		}
		if (!is2D) n3d++; else n2d++;
		/* dump texcoords (both int16-IMap and float-UV views) + screen span of the
		   first N textured 2D primitives, so we can tell landscape tiles (varying
		   coords) from the sky backdrop (uniform 0,0). BOB_TRACE_FVF only. */
		/* per-distinct-texture tracker over ALL 2D textured quads: which textures the
		   landscape binds, how many quads use each, and the screen-Y band covered -- so
		   ground tiles (low on screen, looking down) separate from the cloud/horizon
		   layer (mid) and HUD. Each newly-seen texture is dumped once to /tmp. */
		static struct { GLuint id; int w,h; long n; float ymin,ymax; } seen[24]; static int nseen=0;
		if (is2D && g_devTex[0] && count>=3 && L.hasTex && g_devTex[0]->glTex) {
			float maxx=-1e9f,minx=1e9f,maxy=-1e9f,miny=1e9f;
			for(DWORD i=0;i<count;i++){const float* p=(const float*)(base+(size_t)i*L.stride+L.posOff);
				if(p[0]<minx)minx=p[0]; if(p[0]>maxx)maxx=p[0]; if(p[1]<miny)miny=p[1]; if(p[1]>maxy)maxy=p[1];}
			GLSurface7* tt=g_devTex[0];
			int idx=-1; for(int k=0;k<nseen;k++) if(seen[k].id==tt->glTex){idx=k;break;}
			if(idx<0 && nseen<24){ idx=nseen++;
				seen[idx].id=tt->glTex; seen[idx].w=tt->w; seen[idx].h=tt->h; seen[idx].n=0; seen[idx].ymin=1e9f; seen[idx].ymax=-1e9f;
				if(tt->bits){ char path[64]; snprintf(path,sizeof(path),"/tmp/bobtex_seen%d.ppm",idx);
					int fd=::open(path,O_WRONLY|O_CREAT|O_TRUNC,0644);
					if(fd>=0){ char hdr[64]; int hn=snprintf(hdr,sizeof(hdr),"P6\n%d %d\n255\n",tt->w,tt->h); if(write(fd,hdr,hn)<0){}
						for(int i=0;i<tt->w*tt->h;i++){ unsigned char rgb[3];
							if(tt->bpp==16){ unsigned short p=((unsigned short*)tt->bits)[i]; rgb[0]=((p>>11)&0x1f)<<3; rgb[1]=((p>>5)&0x3f)<<2; rgb[2]=(p&0x1f)<<3; }
							else { unsigned p=((unsigned*)tt->bits)[i]; rgb[0]=(p>>16)&0xff; rgb[1]=(p>>8)&0xff; rgb[2]=p&0xff; }
							if(write(fd,rgb,3)<0){} }
						close(fd); fprintf(stderr,"[texseen] #%d %dx%d bpp%d -> %s\n",idx,tt->w,tt->h,tt->bpp,path); }
				}
			}
			if(idx>=0){ seen[idx].n++; if(miny<seen[idx].ymin)seen[idx].ymin=miny; if(maxy>seen[idx].ymax)seen[idx].ymax=maxy; }
			/* identify the largest-area textured 2D quad (the dominant/occluding surface) */
			static float bestArea=-1; static int bw=0,bh=0; static float bz=0; static unsigned bcol=0; static float bu=0,bv=0; static float bminy=0,bmaxy=0;
			float area=(maxx-minx)*(maxy-miny);
			if(area>bestArea){ bestArea=area; bw=tt->w; bh=tt->h;
				const float* p0=(const float*)(base+L.posOff); bz=p0[2];
				if(L.hasCol){ bcol=*(const unsigned*)(base+L.colOff); }
				float uu0=1e9f,uu1=-1e9f,vv0=1e9f,vv1=-1e9f;
				for(DWORD i=0;i<count;i++){const float* fi=(const float*)(base+(size_t)i*L.stride+L.texOff);
					if(fi[0]<uu0)uu0=fi[0]; if(fi[0]>uu1)uu1=fi[0]; if(fi[1]<vv0)vv0=fi[1]; if(fi[1]>vv1)vv1=fi[1];}
				bu=uu1-uu0; bv=vv1-vv0; bminy=miny; bmaxy=maxy; }
			if (getenv("BOB_DUMP_FRAME") && (n2d+n3d)==atoi(getenv("BOB_DUMP_FRAME"))*38) {
				fprintf(stderr,"[bigquad] area=%.0f tex=%dx%d z=%.4f diffuse=0x%08x uv-SPAN=(%.3f,%.3f) scrY[%.0f..%.0f]\n",bestArea,bw,bh,bz,bcol,bu,bv,bminy,bmaxy);
			}
			static int gd=0;
			if (tt->w==32 && tt->h==32 && gd<12) { gd++;
				int ntex=(fvf & 0xf00)>>8;
				fprintf(stderr,"[gtile] ntex=%d scr[%.0f..%.0f,%.0f..%.0f]\n",ntex,minx,maxx,miny,maxy);
				for (int set=0; set<ntex && set<3; set++){
					fprintf(stderr,"        set%d:",set);
					for (DWORD i=0;i<count && i<4;i++){ const float* f=(const float*)(base+(size_t)i*L.stride+L.texOff+set*8);
						fprintf(stderr," (%.3f,%.3f)",f[0],f[1]); }
					fprintf(stderr,"\n"); }
			}
		}
		static bool texreported=false;
		if (getenv("BOB_DUMP_FRAME") && (n2d+n3d)>=atoi(getenv("BOB_DUMP_FRAME"))*38 && !texreported) {
			texreported=true;
			for(int k=0;k<nseen;k++) fprintf(stderr,"[texuse] #%d %dx%d quads=%ld screenY[%.0f..%.0f]\n",k,seen[k].w,seen[k].h,seen[k].n,seen[k].ymin,seen[k].ymax);
		}
		static int rep=0; if ((n2d+n3d)%1000==0 && rep++<8) {
			fprintf(stderr,"[fvf] 2D=%d 3D=%d  ALL-quad bounds x[%.0f..%.0f] y[%.0f..%.0f] (scr %dx%d) textured=%d untex=%d\n",
				n2d,n3d,gminx,gmaxx,gminy,gmaxy,g_scrW,g_scrH,textured,untex);
		}
	}

	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	if (is2D) glOrtho(0, g_scrW, g_scrH, 0, -1, 1);   /* DDraw screen coords: y down */
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
	static int depth3d = -1; if (depth3d<0) depth3d = getenv("BOB_DEPTH3D") ? 1 : 0;
	if (depth3d) { glEnable(GL_DEPTH_TEST); glDepthFunc(GL_GEQUAL); glDepthMask(GL_TRUE); }
	else glDisable(GL_DEPTH_TEST);
	if (g_devAlphaBlend) { glEnable(GL_BLEND); glBlendFunc(g_srcBlend,g_dstBlend); }

	GLSurface7* t=g_devTex[0];
	/* BOB_TEX_REPLACE: show texture only (ignore the software-lit/fogged vertex colour)
	   to tell whether flat-grey terrain is a texture problem or a lighting/fog wash. */
	static int texMode = -2;
	if (texMode==-2) texMode = getenv("BOB_TEX_REPLACE") ? GL_REPLACE : GL_MODULATE;
	if (t) { if (getenv("BOB_TEX_REUP") || t->texDirty || !t->glTex) upload_texture(t);
		glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,t->glTex);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode); }
	else glDisable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	/* XYZRHW stored as 4 floats; pass x,y(,z). With depth3d we pass z too (3 comps)
	   so the screen-space z drives the depth test. */
	glVertexPointer((is2D&&!depth3d)?2:3, GL_FLOAT, L.stride, base + L.posOff);
	if (L.hasCol) { glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, L.stride, base + L.colOff); }  /* D3DCOLOR=ARGB */
	if (L.hasTex && t) { glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, L.stride, base + L.texOff); }

	GLenum mode = (prim==1)?GL_POINTS : (prim==2)?GL_LINES : (prim==6)?GL_TRIANGLE_FAN :
	              (prim==5)?GL_TRIANGLE_STRIP : GL_TRIANGLES;
	glDrawArrays(mode, 0, count);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

static HRESULT DEV_DrawPrimitiveVB(IDirect3DDevice7*, D3DPRIMITIVETYPE prim, LPDIRECT3DVERTEXBUFFER7 vb, DWORD start, DWORD count, DWORD) {
	GLVB7* v=(GLVB7*)vb; if(!v||!v->data) return D3D_OK;
	if (getenv("BOB_TRACE_DPVB")) { static int n=0,zero=0,nz=0; if(count==0)zero++; else nz++;
		if (n++<30) fprintf(stderr,"[dpvb] prim=%d start=%lu count=%lu fvf=%03lx\n",(int)prim,(unsigned long)start,(unsigned long)count,(unsigned long)v->d.dwFVF);
		if ((n%500)==0) fprintf(stderr,"[dpvb] totals: nonzero=%d zerocount=%d\n",nz,zero); }
	FvfLayout L=fvf_layout(v->d.dwFVF);
	draw_fvf(prim, (const unsigned char*)v->data + (size_t)start*L.stride, count, v->d.dwFVF);
	return D3D_OK;
}
static HRESULT DEV_DrawIndexedPrimitiveVB(IDirect3DDevice7*, D3DPRIMITIVETYPE prim, LPDIRECT3DVERTEXBUFFER7 vb, DWORD start, DWORD numv, LPWORD idx, DWORD idxcount, DWORD) {
	GLVB7* v=(GLVB7*)vb; if(!v||!v->data||!idx) return D3D_OK;
	FvfLayout L=fvf_layout(v->d.dwFVF);
	/* draw via indices: build is overkill for now; draw the indexed verts directly */
	const unsigned char* base=(const unsigned char*)v->data + (size_t)start*L.stride;
	int is2D=(v->d.dwFVF & BFVF_XYZRHW)!=0;
	if(!g_win||!L.stride) return D3D_OK;
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	if(is2D) glOrtho(0,g_scrW,g_scrH,0,-1,1);
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST);
	if(g_devAlphaBlend){glEnable(GL_BLEND);glBlendFunc(g_srcBlend,g_dstBlend);}
	GLSurface7* t=g_devTex[0];
	if(t){ if(t->texDirty||!t->glTex) upload_texture(t); glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,t->glTex); glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);} else glDisable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(is2D?2:3,GL_FLOAT,L.stride,base+L.posOff);
	if(L.hasCol){glEnableClientState(GL_COLOR_ARRAY); glColorPointer(GL_BGRA,GL_UNSIGNED_BYTE,L.stride,base+L.colOff);}
	if(L.hasTex&&t){glEnableClientState(GL_TEXTURE_COORD_ARRAY); glTexCoordPointer(2,GL_FLOAT,L.stride,base+L.texOff);}
	GLenum mode=(prim==1)?GL_POINTS:(prim==2)?GL_LINES:(prim==6)?GL_TRIANGLE_FAN:(prim==5)?GL_TRIANGLE_STRIP:GL_TRIANGLES;
	glDrawElements(mode, idxcount, GL_UNSIGNED_SHORT, idx);
	glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix();
	return D3D_OK;
}
static HRESULT DEV_DrawPrimitive(IDirect3DDevice7*, D3DPRIMITIVETYPE prim, DWORD fvf, LPVOID verts, DWORD count, DWORD) {
	draw_fvf(prim, (const unsigned char*)verts, count, fvf);
	return D3D_OK;
}
static HRESULT DEV_CreateStateBlock(IDirect3DDevice7*, DWORD, LPDWORD h) { if(h)*h=1; return D3D_OK; }
static HRESULT DEV_ApplyStateBlock(IDirect3DDevice7*, DWORD) { return D3D_OK; }
static HRESULT DEV_EnumTextureFormats(IDirect3DDevice7*, void* cbv, LPVOID arg) {
	/* report a handful of formats the game understands (RGB565/1555/4444 + 8bpp) */
	typedef HRESULT (*TFCB)(LPDDPIXELFORMAT, LPVOID); TFCB cb=(TFCB)cbv; if(!cb) return DD_OK;
	DDPIXELFORMAT pf;
	struct { DWORD bits, r,g,b,a; } fmts[] = {
		{16,0xF800,0x07E0,0x001F,0}, {16,0x7C00,0x03E0,0x001F,0x8000}, {16,0x0F00,0x00F0,0x000F,0xF000},
	};
	for (unsigned i=0;i<sizeof(fmts)/sizeof(fmts[0]);++i){
		memset(&pf,0,sizeof(pf)); pf.dwSize=sizeof(pf); pf.dwFlags=DDPF_RGB|(fmts[i].a?DDPF_ALPHAPIXELS:0);
		pf.dwRGBBitCount=fmts[i].bits; pf.dwRBitMask=fmts[i].r; pf.dwGBitMask=fmts[i].g; pf.dwBBitMask=fmts[i].b; pf.dwRGBAlphaBitMask=fmts[i].a;
		if (cb(&pf,arg)==0) return DD_OK;
	}
	return DD_OK;
}
static HRESULT DEV_GetCaps(IDirect3DDevice7*, LPD3DDEVICEDESC7 d) { if(d) fill_devdesc(d,&IID_IDirect3DTnLHalDevice); return D3D_OK; }
static HRESULT DEV_GetDirect3D(IDirect3DDevice7*, LPDIRECT3D7* p) { if(p)*p=(IDirect3D7*)&g_theD3D; return D3D_OK; }
/* ValidateDevice: on real D3D7 this reports whether the current texture-stage/render
   state combo is renderable in one pass. Our GL backend renders any state the game
   sets, so always validate OK in a single pass (Lib3D::CkValidDevice). */
static HRESULT DEV_ValidateDevice(IDirect3DDevice7*, LPDWORD passes) { if(passes)*passes=1; return D3D_OK; }

/* ============================ clipper / palette ========================== */
static HRESULT CLIP_SetHWnd(IDirectDrawClipper* This, DWORD, HWND h) { ((GLClipper*)This)->hwnd=h; return DD_OK; }
static HRESULT PAL_SetEntries(IDirectDrawPalette* This, DWORD, DWORD start, DWORD n, LPPALETTEENTRY e) {
	GLPalette* p=(GLPalette*)This; if(e && start+n<=256) memcpy(&p->ent[start], e, n*sizeof(PALETTEENTRY)); return DD_OK;
}
static HRESULT PAL_GetEntries(IDirectDrawPalette* This, DWORD, DWORD start, DWORD n, LPPALETTEENTRY e) {
	GLPalette* p=(GLPalette*)This; if(e && start+n<=256) memcpy(e, &p->ent[start], n*sizeof(PALETTEENTRY)); return DD_OK;
}

/* ============================ vtbl wiring ================================ */
/* Generic no-op for any device vtbl slot we don't implement. STDMETHODCALLTYPE is
   cdecl here, so this single (void*)->HRESULT signature is ABI-compatible with every
   slot regardless of its real argument list (the caller cleans the stack). */
static HRESULT DEV_generic_stub(void*) { return D3D_OK; }
/* QueryInterface that hands back the device itself (the game only ever QIs the device
   for a directly-usable interface, never a distinct COM object). */
static HRESULT DEV_QueryInterface(IDirect3DDevice7* This, REFIID, void** ppv) { if (ppv) *ppv = This; return D3D_OK; }

static void init_vtbls_once(void)
{
	static int done = 0; if (done) return; done = 1;

	g_surfVtbl.AddRef=SURF_AddRef;
	g_surfVtbl.Release=SURF_Release;
	g_surfVtbl.QueryInterface=SURF_QueryInterface;
	g_surfVtbl.GetSurfaceDesc=SURF_GetSurfaceDesc;
	g_surfVtbl.Lock=SURF_Lock; g_surfVtbl.Unlock=SURF_Unlock;
	g_surfVtbl.GetAttachedSurface=SURF_GetAttachedSurface;
	g_surfVtbl.AddAttachedSurface=SURF_AddAttachedSurface;
	g_surfVtbl.GetPixelFormat=SURF_GetPixelFormat;
	g_surfVtbl.Blt=SURF_Blt; g_surfVtbl.BltFast=SURF_BltFast; g_surfVtbl.Flip=SURF_Flip;
	g_surfVtbl.SetPalette=SURF_SetPalette; g_surfVtbl.SetClipper=SURF_SetClipper;
	g_surfVtbl.IsLost=SURF_IsLost; g_surfVtbl.Restore=SURF_Restore;
	g_surfVtbl.GetDC=SURF_GetDC; g_surfVtbl.ReleaseDC=SURF_ReleaseDC;
	g_surfVtbl.PageLock=SURF_PageLock; g_surfVtbl.PageUnlock=SURF_PageUnlock;
	g_surfVtbl.GetCaps=SURF_GetCaps;

	g_clipVtbl.AddRef=(ULONG(*)(IDirectDrawClipper*))generic_addref;
	g_clipVtbl.Release=(ULONG(*)(IDirectDrawClipper*))generic_release;
	g_clipVtbl.SetHWnd=CLIP_SetHWnd;

	g_palVtbl.AddRef=(ULONG(*)(IDirectDrawPalette*))generic_addref;
	g_palVtbl.Release=(ULONG(*)(IDirectDrawPalette*))generic_release;
	g_palVtbl.SetEntries=PAL_SetEntries; g_palVtbl.GetEntries=PAL_GetEntries;

	g_vbVtbl.AddRef=(ULONG(*)(IDirect3DVertexBuffer7*))generic_addref;
	g_vbVtbl.Release=VB_Release;
	g_vbVtbl.Lock=VB_Lock; g_vbVtbl.Unlock=VB_Unlock; g_vbVtbl.ProcessVertices=VB_ProcessVertices;

	g_devVtbl.AddRef=(ULONG(*)(IDirect3DDevice7*))generic_addref;
	g_devVtbl.Release=(ULONG(*)(IDirect3DDevice7*))generic_release;
	g_devVtbl.BeginScene=DEV_BeginScene; g_devVtbl.EndScene=DEV_EndScene; g_devVtbl.Clear=DEV_Clear;
	g_devVtbl.SetViewport=DEV_SetViewport; g_devVtbl.GetViewport=DEV_GetViewport;
	g_devVtbl.SetRenderTarget=DEV_SetRenderTarget;
	g_devVtbl.SetRenderState=DEV_SetRenderState; g_devVtbl.GetRenderState=DEV_GetRenderState;
	g_devVtbl.SetTextureStageState=DEV_SetTextureStageState; g_devVtbl.GetTextureStageState=DEV_GetTextureStageState;
	g_devVtbl.SetTransform=DEV_SetTransform; g_devVtbl.SetTexture=DEV_SetTexture;
	g_devVtbl.SetMaterial=DEV_SetMaterial; g_devVtbl.SetLight=DEV_SetLight; g_devVtbl.LightEnable=DEV_LightEnable;
	g_devVtbl.DrawPrimitiveVB=DEV_DrawPrimitiveVB; g_devVtbl.DrawIndexedPrimitiveVB=DEV_DrawIndexedPrimitiveVB;
	g_devVtbl.DrawPrimitive=DEV_DrawPrimitive;
	g_devVtbl.CreateStateBlock=DEV_CreateStateBlock; g_devVtbl.ApplyStateBlock=DEV_ApplyStateBlock;
	g_devVtbl.EnumTextureFormats=DEV_EnumTextureFormats; g_devVtbl.GetCaps=DEV_GetCaps;
	g_devVtbl.ValidateDevice=DEV_ValidateDevice;
	g_devVtbl.GetDirect3D=DEV_GetDirect3D;
	g_devVtbl.QueryInterface=DEV_QueryInterface;
	/* Backstop: any device method left unwired above gets a no-op returning D3D_OK, so an
	   unimplemented vtbl slot can never be a NULL function-pointer call. The device vtbl is
	   a flat array of cdecl function pointers, so this is safe to fill positionally. */
	{
		typedef HRESULT (*devfn)(void*);
		devfn* slots = (devfn*)&g_devVtbl;
		unsigned n = sizeof(g_devVtbl)/sizeof(devfn);
		for (unsigned i=0;i<n;i++) if (!slots[i]) slots[i] = (devfn)DEV_generic_stub;
	}
	g_theDevice.lpVtbl=&g_devVtbl;

	g_d3dVtbl.AddRef=(ULONG(*)(IDirect3D7*))generic_addref;
	g_d3dVtbl.Release=(ULONG(*)(IDirect3D7*))generic_release;
	g_d3dVtbl.QueryInterface=D3D_QueryInterface;
	g_d3dVtbl.EnumDevices=D3D_EnumDevices; g_d3dVtbl.CreateDevice=D3D_CreateDevice;
	g_d3dVtbl.CreateVertexBuffer=D3D_CreateVertexBuffer;
	g_d3dVtbl.EnumZBufferFormats=D3D_EnumZBufferFormats;
	g_d3dVtbl.EvictManagedTextures=D3D_EvictManagedTextures;
	g_theD3D.lpVtbl=&g_d3dVtbl;

	g_ddVtbl.AddRef=DD_AddRef;
	g_ddVtbl.Release=DD_Release;
	g_ddVtbl.QueryInterface=DD_QueryInterface;
	g_ddVtbl.CreateSurface=DD_CreateSurface;
	g_ddVtbl.SetCooperativeLevel=DD_SetCooperativeLevel;
	g_ddVtbl.SetDisplayMode=DD_SetDisplayMode; g_ddVtbl.RestoreDisplayMode=DD_RestoreDisplayMode;
	g_ddVtbl.GetCaps=DD_GetCaps; g_ddVtbl.GetAvailableVidMem=DD_GetAvailableVidMem;
	g_ddVtbl.GetDeviceIdentifier=DD_GetDeviceIdentifier; g_ddVtbl.GetDisplayMode=DD_GetDisplayMode;
	g_ddVtbl.EnumDisplayModes=DD_EnumDisplayModes;
	g_ddVtbl.CreateClipper=DD_CreateClipper; g_ddVtbl.CreatePalette=DD_CreatePalette;
}

/* ============================ creation entry points ====================== */
extern "C" HRESULT DirectDrawCreateEx(GUID*, LPVOID* lplpDD, REFIID, IUnknown*)
{
	init_vtbls_once();
	GLDD7* dd = (GLDD7*)calloc(1, sizeof(GLDD7));
	dd->lpVtbl = &g_ddVtbl;
	dd->ref = 1;
	if (lplpDD) *lplpDD = dd;
	VLOG("DirectDrawCreateEx -> %p\n", (void*)dd);
	return DD_OK;
}

extern "C" HRESULT DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA cb, LPVOID ctx, DWORD)
{
	/* report a single primary display driver (NULL GUID = primary) */
	if (cb) cb(NULL, (LPSTR)"Primary Display Driver", (LPSTR)"display", ctx, NULL);
	return DD_OK;
}

/* ====================================================================== *
 * DirectInput (PHASE 1: non-fatal stub -- reports no input).             *
 * Real keyboard/mouse/joystick via SDL events is a later phase; for now  *
 * the game must be able to create the input devices so it proceeds to    *
 * SetDriverAndMode (which brings the window up) and into the game loop.  *
 * ====================================================================== */
static IDirectInputVtbl       g_diVtbl;
static IDirectInputDeviceVtbl g_didevVtbl;
static IDirectInputDeviceA    g_diKeyboard, g_diMouse, g_diJoystick, g_diGeneric;

static HRESULT DIDEV_GetDeviceState(IDirectInputDeviceA* This, DWORD cb, LPVOID buf) {
	if (buf && cb) memset(buf,0,cb);
	/* keyboard immediate state: 256-byte DIK array, 0x80 = down (some code uses this) */
	if (This==&g_diKeyboard && buf && cb>=256) {
		int n; const Uint8* st=SDL_GetKeyboardState(&n);
		unsigned char* d=(unsigned char*)buf;
		for (int sc=0;sc<n;sc++) if (st[sc]) { int dik=sdl_to_dik(sc); if(dik&&dik<256) d[dik]=0x80; }
	}
	/* joystick immediate state: fill DIJOYSTATE from SDL. DInput axes (no SetProperty
	   RANGE applied by the compat) read as unsigned 0..65535, centre 32768; the game
	   scales (ULong)js.lX/4 -> SWord position and calibrates from there. */
	if (This==&g_diMouse && buf && cb>=sizeof(DIMOUSESTATE)) {
		DIMOUSESTATE* ms=(DIMOUSESTATE*)buf;
		ms->lX=g_mouseRelX; ms->lY=g_mouseRelY; ms->lZ=0;
		ms->rgbButtons[0]=(g_mouseBtns&1)?0x80:0; ms->rgbButtons[1]=(g_mouseBtns&2)?0x80:0;
		ms->rgbButtons[2]=(g_mouseBtns&4)?0x80:0; ms->rgbButtons[3]=0;
		g_mouseRelX=0; g_mouseRelY=0;
	}
	if (This==&g_diJoystick && buf && cb>=sizeof(DIJOYSTATE)) {
		joy_open_once();
		DIJOYSTATE* js=(DIJOYSTATE*)buf;
		for (int i=0;i<4;i++) { js->rgdwPOV[i]=0xFFFFFFFF; }   /* POV centred = -1 */
		if (g_sdlJoy) {
			SDL_JoystickUpdate();
			/* Logitech Extreme 3D Pro: 0=X(roll) 1=Y(pitch) 2=twist(rudder) 3=throttle slider */
			long ax[6]={0,0,0,0,0,0};
			for (int i=0;i<g_joyAxes && i<6;i++) ax[i]=(long)SDL_JoystickGetAxis(g_sdlJoy,i)+32768; /* 0..65535 */
			js->lX  = ax[0];
			js->lY  = ax[1];
			js->lRz = ax[2];   /* twist -> rudder */
			js->lZ  = ax[3];   /* throttle slider */
			for (int b=0;b<g_joyButtons && b<32;b++) js->rgbButtons[b]= SDL_JoystickGetButton(g_sdlJoy,b)?0x80:0x00;
			if (g_joyHats>0) {
				Uint8 h=SDL_JoystickGetHat(g_sdlJoy,0); DWORD pov=0xFFFFFFFF;
				switch(h){ case SDL_HAT_UP:pov=0; break; case SDL_HAT_RIGHTUP:pov=4500; break;
					case SDL_HAT_RIGHT:pov=9000; break; case SDL_HAT_RIGHTDOWN:pov=13500; break;
					case SDL_HAT_DOWN:pov=18000; break; case SDL_HAT_LEFTDOWN:pov=22500; break;
					case SDL_HAT_LEFT:pov=27000; break; case SDL_HAT_LEFTUP:pov=31500; break; }
				js->rgdwPOV[0]=pov;
			}
			if (getenv("MA_TRACE_JOY")) { static int _t=0; if((_t++%30)==0)
				fprintf(stderr,"[joy] X=%ld Y=%ld Rz=%ld Z=%ld btn0=%d pov=%lu\n",
					(long)js->lX,(long)js->lY,(long)js->lRz,(long)js->lZ,(int)js->rgbButtons[0],(unsigned long)js->rgdwPOV[0]); }
		}
	}
	return 0;
}
static HRESULT DIDEV_QueryInterface(IDirectInputDeviceA* This, REFIID, void** ppv) {
	/* IDirectInputDevice2 == IDirectInputDeviceA (same struct) -> hand back self */
	if (ppv) { *ppv = This; return DI_OK; }
	return E_FAIL;
}
/* Synthetic joystick deflection for deterministic validation / CI (mirrors the
   keyboard-side BOB_AUTOFLY). When BOB_AUTOJOY is set, the physical SDL axis read is
   replaced by a scripted triangle-wave sweep, so the full
   joy -> DI GetDeviceData -> Analogue::PollPosition -> axisvalues -> flight-model chain
   can be exercised without a hand on the stick. Modes (first char):
     roll   -> sweep axis 0 (aileron), others centred
     pitch  -> sweep axis 1 (elevator), others centred
     sweep  -> sweep axes 0 and 1 together
     center -> force every axis to centre (drift baseline)
     left/right -> HOLD axis 0 at full deflection (aileron, for A/B frame compare)
     up/down    -> HOLD axis 1 at full deflection (elevator)
   Returns a 0..65535 absolute value (same encoding as a physical axis). */
static int g_autoJoyTick=0;
static int autojoy_axis_value(int inst, int physical) {
	const char* m=getenv("BOB_AUTOJOY");
	if (!m || !*m) return physical;
	const int CENTRE=32768, LO=2768, HI=62768; /* ~ +/-30000 about centre */
	if (m[0]=='c') return CENTRE;                      /* center */
	/* constant-hold modes (steady deflection for deterministic A/B comparison) */
	if (!strcmp(m,"left"))  return inst==0?LO:CENTRE;
	if (!strcmp(m,"right")) return inst==0?HI:CENTRE;
	if (!strcmp(m,"up"))    return inst==1?LO:CENTRE;
	if (!strcmp(m,"down"))  return inst==1?HI:CENTRE;
	/* triangle wave: tick%120 -> 0..60..0 -> +/-30000 about centre */
	int phase = g_autoJoyTick % 120;
	int tri   = phase<60 ? phase : 120-phase;          /* 0..60..0 */
	int defl  = CENTRE + (tri-30)*1000;                /* ~ +/-30000 */
	if (defl<0) defl=0; if (defl>65535) defl=65535;
	int doRoll  = (m[0]=='r'||m[0]=='s');              /* roll | sweep */
	int doPitch = (m[0]=='p'||m[0]=='s');              /* pitch| sweep */
	if (inst==0) return doRoll  ? defl : CENTRE;
	if (inst==1) return doPitch ? defl : CENTRE;
	return CENTRE;
}
/* Read one physical SDL object's current value, given the dwType (axis/button/POV +
   instance) the game assigned in EnumObjects. Axes -> 0..65535 absolute. */
static DWORD joy_obj_value(DWORD dwType) {
	int type = dwType & 0xFF;          /* DIDFT_GETTYPE */
	int inst = (dwType >> 8) & 0xFFFF; /* DIDFT_GETINSTANCE */
	if (!g_sdlJoy) return 0;
	if (type & (DIDFT_ABSAXIS|DIDFT_RELAXIS)) {
		int phys = (inst < g_joyAxes) ? (SDL_JoystickGetAxis(g_sdlJoy,inst)+32768) : 32768;
		return (DWORD)autojoy_axis_value(inst, phys); /* 0..65535 (passthrough unless BOB_AUTOJOY) */
	}
	if (type & DIDFT_POV) {
		if (inst < g_joyHats) { Uint8 h=SDL_JoystickGetHat(g_sdlJoy,inst);
			switch(h){ case SDL_HAT_UP:return 0; case SDL_HAT_RIGHTUP:return 4500; case SDL_HAT_RIGHT:return 9000;
				case SDL_HAT_RIGHTDOWN:return 13500; case SDL_HAT_DOWN:return 18000; case SDL_HAT_LEFTDOWN:return 22500;
				case SDL_HAT_LEFT:return 27000; case SDL_HAT_LEFTUP:return 31500; default:return 0xFFFFFFFF; } }
		return 0xFFFFFFFF;
	}
	/* button */
	if (inst < g_joyButtons) return SDL_JoystickGetButton(g_sdlJoy,inst)?0x80:0x00;
	return 0;
}
/* mouse object value for the game buffered format: relative axes -> accumulated SDL
   delta (consumed per poll), buttons -> current state. inst 0=X, 1=Y. */
static DWORD mouse_obj_value(DWORD dwType) {
	int type = dwType & 0xFF; int inst = (dwType >> 8) & 0xFFFF;
	if (type & DIDFT_RELAXIS) {
		const char* am=getenv("BOB_AUTOMOUSE");   /* synthetic steady motion for headless A/B */
		if (am && *am) return (DWORD)(inst==0 ? (am[0]=='a'?6:10) : 0);
		return (DWORD)(inst==0 ? g_mouseRelX : inst==1 ? g_mouseRelY : 0);
	}
	if (type & DIDFT_PSHBUTTON) return (g_mouseBtns & (1<<inst)) ? 0x80 : 0x00;
	return 0;
}
static HRESULT DIDEV_GetDeviceData(IDirectInputDeviceA* This, DWORD, LPDIDEVICEOBJECTDATA buf, LPDWORD inout, DWORD flags) {
	if (!inout) return 0;
	if (This==&g_diKeyboard) {
		DWORD want=*inout, got=0;
		while (got<want && g_kbHead!=g_kbTail) {
			if (buf) { memset(&buf[got],0,sizeof(buf[got]));
				buf[got].dwOfs=g_kbq[g_kbHead].ofs; buf[got].dwData=g_kbq[g_kbHead].data;
				buf[got].dwSequence=++g_kbSeq; }
			if (!(flags & 0x1 /*DIGDD_PEEK*/)) g_kbHead=(g_kbHead+1)%BOB_KBQ;
			got++;
		}
		*inout=got;
		return 0;
	}
	if (This==&g_diJoystick && g_sdlJoy && g_joyFmt) {
		/* Buffered model: one event per object in the game's data format, carrying the
		   object's current value at the dwOfs the game assigned. The game only emits
		   change-driven deltas in real DI; emitting current values each poll is
		   idempotent for the absolute/scaled axes + button states it tracks. We only
		   send on the FIRST drain call of a poll (loop drains until numelts==0). */
		static int g_joyDrained=0;
		SDL_JoystickUpdate();
		DWORD want=*inout, got=0;
		if (!g_joyDrained) {
			g_autoJoyTick++;   /* advance scripted-deflection phase once per poll */
			DWORD n = g_joyFmt->dwNumObjs;
			for (DWORD i=0; i<n && got<want; i++) {
				const DIOBJECTDATAFORMAT* od = &g_joyFmt->rgodf[i];
				if (!od->dwType) continue;
				if (buf) { memset(&buf[got],0,sizeof(buf[got]));
					buf[got].dwOfs = od->dwOfs;
					buf[got].dwData = joy_obj_value(od->dwType);
					buf[got].dwSequence=++g_kbSeq; }
				got++;
			}
			if (!(flags & 0x1)) g_joyDrained=1;
		} else {
			if (!(flags & 0x1)) g_joyDrained=0;   /* next poll re-arms */
		}
		if (getenv("MA_TRACE_JOY") && got) { static int _t=0; if((_t++%60)==0) {
			fprintf(stderr,"[joy] axes:");
			for (int a=0;a<g_joyAxes && a<6;a++) fprintf(stderr," ax%d=%d",a,SDL_JoystickGetAxis(g_sdlJoy,a));
			int bm=0; for (int b=0;b<g_joyButtons && b<12;b++) if(SDL_JoystickGetButton(g_sdlJoy,b)) bm|=(1<<b);
			fprintf(stderr," btnmask=0x%03x hat=%d (events=%lu)\n",bm,g_joyHats?SDL_JoystickGetHat(g_sdlJoy,0):-1,(unsigned long)got); } }
		*inout=got;
		return 0;
	}
	if (This==&g_diMouse && g_mouseFmt) {
		DWORD want=*inout, got=0;
		if (!g_mouseDrained) {
			DWORD n = g_mouseFmt->dwNumObjs;
			for (DWORD i=0; i<n && got<want; i++) {
				const DIOBJECTDATAFORMAT* od = &g_mouseFmt->rgodf[i];
				if (!od->dwType) continue;
				if (buf) { memset(&buf[got],0,sizeof(buf[got]));
					buf[got].dwOfs=od->dwOfs; buf[got].dwData=mouse_obj_value(od->dwType); buf[got].dwSequence=++g_kbSeq; }
				got++;
			}
			if (getenv("MA_TRACE_MOUSE") && got) { static int _t=0; if((_t++%30)==0)
				fprintf(stderr,"[mouse] poll dX=%ld relX=%d btns=0x%x objs=%lu\n",(long)mouse_obj_value(DIDFT_RELAXIS|DIDFT_MAKEINSTANCE(0)),g_mouseRelX,g_mouseBtns,(unsigned long)got); }
			g_mouseRelX=0; g_mouseRelY=0;
			if (!(flags & 0x1)) g_mouseDrained=1;
		} else { if (!(flags & 0x1)) g_mouseDrained=0; }
		*inout=got; return 0;
	}
	*inout=0; return 0;
}
static void mouse_set_capture(int on) {
	on = on?1:0; if (on==g_mouseCaptured) return; g_mouseCaptured=on;
	if (!getenv("MA_NO_MOUSE_GRAB")) SDL_SetRelativeMouseMode(on?SDL_TRUE:SDL_FALSE);
	if (on) { g_mouseRelX=0; g_mouseRelY=0; }
	if (getenv("MA_TRACE_MOUSE")) fprintf(stderr,"[mouse] capture %s\n", on?"ON":"OFF");
}
static HRESULT DIDEV_Acquire(IDirectInputDeviceA* This) {
	if (This==&g_diKeyboard) { if(!g_diKbAcquired && getenv("MA_TRACE_KEY")) fprintf(stderr,"[di] keyboard ACQUIRED\n"); g_diKbAcquired=1; }
	else if (This==&g_diMouse) { if (getenv("MA_TRACE_MOUSE")) fprintf(stderr,"[mouse] DI mouse ACQUIRED\n"); mouse_set_capture(1); }
	return 0;
}
static HRESULT DIDEV_Unacquire(IDirectInputDeviceA* This) { if (This==&g_diMouse) mouse_set_capture(0); return 0; }
static HRESULT DIDEV_SetEventNotify(IDirectInputDeviceA* This, HANDLE h) { if (This==&g_diKeyboard) g_diKbNotify=(void*)h; return 0; }
static HRESULT DIDEV_ok(IDirectInputDeviceA*) { return 0; }
static HRESULT DIDEV_SetProperty(IDirectInputDeviceA*, REFGUID, LPCDIPROPHEADER) { return 0; }
static HRESULT DIDEV_GetProperty(IDirectInputDeviceA*, REFGUID, LPDIPROPHEADER) { return 0; }
static HRESULT DIDEV_SetDataFormat(IDirectInputDeviceA* This, LPCDIDATAFORMAT fmt) {
	if (This==&g_diJoystick) {
		if (fmt && fmt->rgodf) {
			DWORD n = fmt->dwNumObjs; if (n>64) n=64;
			for (DWORD i=0;i<n;i++) g_joyObjs[i]=fmt->rgodf[i];
			g_joyFmtCopy = *fmt; g_joyFmtCopy.dwNumObjs=n; g_joyFmtCopy.rgodf=g_joyObjs;
			g_joyFmt=&g_joyFmtCopy;
		} else g_joyFmt=NULL;
		if (getenv("MA_TRACE_JOY")) fprintf(stderr,"[joy] SetDataFormat numObjs=%lu (copied)\n", fmt?(unsigned long)fmt->dwNumObjs:0); }
	if (This==&g_diMouse) {
		if (fmt && fmt->rgodf) {
			DWORD n = fmt->dwNumObjs; if (n>16) n=16;
			for (DWORD i=0;i<n;i++) g_mouseObjs[i]=fmt->rgodf[i];
			g_mouseFmtCopy = *fmt; g_mouseFmtCopy.dwNumObjs=n; g_mouseFmtCopy.rgodf=g_mouseObjs;
			g_mouseFmt=&g_mouseFmtCopy;
		} else g_mouseFmt=NULL;
		if (getenv("MA_TRACE_MOUSE")) fprintf(stderr,"[mouse] SetDataFormat numObjs=%lu\n", fmt?(unsigned long)fmt->dwNumObjs:0);
	}
	return 0;
}
static HRESULT DIDEV_SetCoop(IDirectInputDeviceA*, HWND, DWORD) { return 0; }
/* Enumerate joystick objects so the game builds its data format (DIEnumDeviceObjectsProc
   assigns each a dwOfs by axis-usage). Order: axes 0..N, buttons, POV. */
static HRESULT DIDEV_EnumObjects(IDirectInputDeviceA* This, LPDIENUMDEVICEOBJECTSCALLBACKA cb, LPVOID ref, DWORD flags) {
	if (This==&g_diMouse) {
		if (!cb) return 0;
		int wAx = (flags & DIDFT_AXIS) || flags==DIDFT_ALL || flags==0;
		int wBt = (flags & DIDFT_BUTTON) || flags==DIDFT_ALL || flags==0;
		DIDEVICEOBJECTINSTANCEA oi; const GUID* ag[2]={&GUID_XAxis,&GUID_YAxis};
		if (wAx) for (int a=0;a<2;a++) { memset(&oi,0,sizeof(oi)); oi.dwSize=sizeof(oi);
			oi.guidType=*ag[a]; oi.dwOfs=a*4; oi.dwType=DIDFT_RELAXIS | DIDFT_MAKEINSTANCE(a);  /* RELAXIS -> game flags mouse axis */
			snprintf(oi.tszName,sizeof(oi.tszName),a?"Y-Axis":"X-Axis");   /* S57: SController shows these ("active mouse : X-Axis & Y-Axis") */
			if (cb(&oi,ref)==DIENUM_STOP) return 0; }
		if (wBt) for (int b=0;b<3;b++) { memset(&oi,0,sizeof(oi)); oi.dwSize=sizeof(oi);
			oi.guidType=GUID_Button; oi.dwOfs=64+b; oi.dwType=DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(b);
			snprintf(oi.tszName,sizeof(oi.tszName),"Button %d",b);
			if (cb(&oi,ref)==DIENUM_STOP) return 0; }
		return 0;
	}
	if (This!=&g_diJoystick || !cb) return 0;
	joy_open_once();
	/* GUID per SDL axis index, chosen so the game's axis-usage classifier maps a typical
	   4-axis flight stick (Logitech Extreme 3D Pro SDL order: 0=X roll, 1=Y pitch,
	   2=twist yaw, 3=throttle slider) to aileron/elevator/rudder/throttle:
	     X  -> stick pair first  -> AU_AILERON
	     Y  -> pair partner      -> AU_ELEVATOR
	     Rz -> xtype, not-first  -> AU_RUDDER   (twist)
	     Z  -> 'else'            -> AU_THROTTLE (slider)
	   (matches real DirectInput's X,Y,Rz,Slider enumeration for this stick). */
	const GUID* axisGuid[6]={&GUID_XAxis,&GUID_YAxis,&GUID_RzAxis,&GUID_ZAxis,&GUID_RyAxis,&GUID_RxAxis};
	int wantAxes = (flags & DIDFT_AXIS) || flags==DIDFT_ALL || flags==0;
	int wantBtn  = (flags & DIDFT_BUTTON) || flags==DIDFT_ALL || flags==0;
	int wantPov  = (flags & DIDFT_POV) || flags==DIDFT_ALL || flags==0;
	DIDEVICEOBJECTINSTANCEA oi;
	if (wantAxes) for (int a=0; a<g_joyAxes && a<6; a++) {
		memset(&oi,0,sizeof(oi)); oi.dwSize=sizeof(oi);
		oi.guidType=*axisGuid[a]; oi.dwOfs=a*4;
		oi.dwType=DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(a);
		snprintf(oi.tszName,sizeof(oi.tszName),"Axis %d",a);   /* S57: SController's per-axis combo text ("active joystick : Axis 0 & Axis 1"); was empty -> "… : &" (parity #7) */
		if (cb(&oi,ref)==DIENUM_STOP) return 0;
	}
	if (wantBtn) for (int b=0; b<g_joyButtons && b<32; b++) {
		memset(&oi,0,sizeof(oi)); oi.dwSize=sizeof(oi);
		oi.guidType=GUID_Button; oi.dwOfs=64+b;
		oi.dwType=DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(b);
		snprintf(oi.tszName,sizeof(oi.tszName),"Button %d",b);
		if (cb(&oi,ref)==DIENUM_STOP) return 0;
	}
	if (wantPov) for (int h=0; h<g_joyHats && h<4; h++) {
		memset(&oi,0,sizeof(oi)); oi.dwSize=sizeof(oi);
		oi.guidType=GUID_POV; oi.dwOfs=32+h*4;
		oi.dwType=DIDFT_POV | DIDFT_MAKEINSTANCE(h);
		snprintf(oi.tszName,sizeof(oi.tszName),"Hat Switch %d",h);
		if (cb(&oi,ref)==DIENUM_STOP) return 0;
	}
	return 0;
}
static HRESULT DIDEV_GetCaps(IDirectInputDeviceA* This, LPDIDEVCAPS c) { if (c) { DWORD sz=c->dwSize; memset(c,0,sz?sz:sizeof(*c)); c->dwSize=sz?sz:sizeof(*c);
	if (This==&g_diMouse) { c->dwDevType=DIDEVTYPE_MOUSE; c->dwAxes=2; c->dwButtons=3; }
	if (This==&g_diJoystick) { joy_open_once(); c->dwDevType=DIDEVTYPE_JOYSTICK;
		c->dwAxes=g_joyAxes>0?g_joyAxes:4; c->dwButtons=g_joyButtons>0?g_joyButtons:12; c->dwPOVs=g_joyHats>0?g_joyHats:1; } } return 0; }
static ULONG   DIDEV_addref(IDirectInputDeviceA*) { return 1; }
static ULONG   DIDEV_release(IDirectInputDeviceA*) { return 0; }

static HRESULT DI_CreateDevice(IDirectInputA*, REFGUID rguid, LPDIRECTINPUTDEVICE* out, IUnknown*) {
	if (!out) return E_FAIL;
	/* hand back a shared dummy device (any of them is fine -- all no-op) */
	if      (rguid == GUID_SysKeyboard) *out = &g_diKeyboard;
	else if (rguid == GUID_Joystick)    *out = &g_diJoystick;
	else if (rguid == GUID_SysMouse)    *out = &g_diMouse;
	else                                *out = &g_diGeneric;
	return 0;
}
static HRESULT DI_GetDeviceStatus(IDirectInputA*, REFGUID) { return DI_OK; }
/* Enumerate input devices. The game asks for DIDEVTYPE_JOYSTICK (ANALWIN/ANALOGUE);
   when a physical stick is present, report one instance with guidInstance=GUID_Joystick
   so InitJoystickInput -> CreateDevice(GUID_Joystick) runs. */
static HRESULT DI_EnumDevices(IDirectInputA*, DWORD devType, LPDIENUMDEVICESCALLBACKA cb, LPVOID ref, DWORD) {
	if (!cb) return DI_OK;
	if (devType==DIDEVTYPE_JOYSTICK || devType==0) {
		joy_open_once();
		if (g_sdlJoy) {
			DIDEVICEINSTANCEA di; memset(&di,0,sizeof(di)); di.dwSize=sizeof(di);
			di.guidInstance=GUID_Joystick; di.guidProduct=GUID_Joystick;
			di.dwDevType=DIDEVTYPE_JOYSTICK;            /* LOBYTE -> GET_DIDEVICE_TYPE */
			strncpy(di.tszProductName, SDL_JoystickName(g_sdlJoy)?SDL_JoystickName(g_sdlJoy):"Joystick", sizeof(di.tszProductName)-1);
			cb(&di, ref);                               /* InitJoystickInput */
		}
	}
	if (devType==DIDEVTYPE_MOUSE || devType==0) {
		/* S59: report the system mouse UNCONDITIONALLY (Windows: GUID_SysMouse always
		   exists). This was gated on g_win, but under SDL_VIDEODRIVER=dummy the OPENGL
		   window never exists -> no mouse device -> the prefs-Controls "3d Pointer" row
		   read "Keyboard" headless vs "active mouse : X-Axis & Y-Axis" on GL — an
		   environment-dependent screen, caught by the S58 dummy==GL cmp bar. Device
		   PRESENCE must not depend on the video backend; capture/motion still no-op
		   windowless. */
		{
			DIDEVICEINSTANCEA di; memset(&di,0,sizeof(di)); di.dwSize=sizeof(di);
			di.guidInstance=GUID_SysMouse; di.guidProduct=GUID_SysMouse; di.dwDevType=DIDEVTYPE_MOUSE;
			strncpy(di.tszProductName,"Mouse",sizeof(di.tszProductName)-1);
			if (getenv("MA_TRACE_MOUSE")) fprintf(stderr,"[mouse] DI_EnumDevices -> reporting 1 mouse\n");
			cb(&di, ref);
		}
	}
	return DI_OK;
}
static ULONG   DI_addref(IDirectInputA*) { return 1; }
static ULONG   DI_release(IDirectInputA*) { return 0; }

static IDirectInputA g_theDI;

static void init_dinput_once(void) {
	static int done=0; if (done) return; done=1;
	g_didevVtbl.AddRef=DIDEV_addref; g_didevVtbl.Release=DIDEV_release;
	g_didevVtbl.GetDeviceState=DIDEV_GetDeviceState; g_didevVtbl.GetDeviceData=DIDEV_GetDeviceData;
	g_didevVtbl.Acquire=DIDEV_Acquire; g_didevVtbl.Unacquire=DIDEV_Unacquire; g_didevVtbl.Poll=DIDEV_ok;
	g_didevVtbl.SetEventNotification=DIDEV_SetEventNotify;
	g_didevVtbl.SetProperty=DIDEV_SetProperty; g_didevVtbl.GetProperty=DIDEV_GetProperty;
	g_didevVtbl.SetDataFormat=DIDEV_SetDataFormat; g_didevVtbl.SetCooperativeLevel=DIDEV_SetCoop;
	g_didevVtbl.EnumObjects=DIDEV_EnumObjects; g_didevVtbl.GetCapabilities=DIDEV_GetCaps;
	g_didevVtbl.QueryInterface=DIDEV_QueryInterface;
	g_diKeyboard.lpVtbl=g_diMouse.lpVtbl=g_diJoystick.lpVtbl=g_diGeneric.lpVtbl=&g_didevVtbl;

	g_diVtbl.AddRef=DI_addref; g_diVtbl.Release=DI_release;
	g_diVtbl.CreateDevice=DI_CreateDevice; g_diVtbl.EnumDevices=DI_EnumDevices;
	g_diVtbl.GetDeviceStatus=DI_GetDeviceStatus;
	g_theDI.lpVtbl=&g_diVtbl;
}

extern "C" HRESULT DirectInputCreateA(HINSTANCE, DWORD, LPDIRECTINPUT* ppDI, IUnknown*)
{
	init_dinput_once();
	if (ppDI) *ppDI = &g_theDI;
	return 0;
}

/* Phase 1a render smoke test (BOB_RENDER_SMOKETEST=1): exercise the full 2D path
   through the real D3D7 device API -- create a texture surface + fill it, create a
   vertex buffer + fill a TLVERTEX quad, then SetTexture + DrawPrimitiveVB(FAN) and
   present. Verifies the device->GL backend independently of the (not-yet-driven)
   game overlay. The textured quad covers screen centre over a dark clear. */
#ifndef DDSCAPS_TEXTURE
#define DDSCAPS_TEXTURE 0x00001000
#endif
extern "C" int bob_render_smoketest(void)
{
	ensure_window(800,600);
	if (!g_win) { fprintf(stderr,"[vid] render smoketest: no window\n"); return 0; }
	init_vtbls_once();
	IDirectDraw7* dd=NULL; DirectDrawCreateEx(0,(LPVOID*)&dd,IID_IDirectDraw7,0);

	DDSURFACEDESC2 td; memset(&td,0,sizeof(td)); td.dwSize=sizeof(td);
	td.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
	td.dwWidth=64; td.dwHeight=64; td.ddsCaps.dwCaps=DDSCAPS_TEXTURE;
	td.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT); td.ddpfPixelFormat.dwFlags=DDPF_RGB;
	td.ddpfPixelFormat.dwRGBBitCount=16; td.ddpfPixelFormat.dwRBitMask=0xF800;
	td.ddpfPixelFormat.dwGBitMask=0x07E0; td.ddpfPixelFormat.dwBBitMask=0x001F;
	IDirectDrawSurface7* tex=NULL; dd->CreateSurface(&td,&tex,0);
	DDSURFACEDESC2 lk; memset(&lk,0,sizeof(lk)); lk.dwSize=sizeof(lk);
	tex->Lock(0,&lk,0,0);
	unsigned short* px=(unsigned short*)lk.lpSurface;
	for (int y=0;y<64;y++) for (int x=0;x<64;x++)
		px[y*64+x] = ((x^y)&8) ? 0xF800 /*red*/ : 0x07E0 /*green*/;   /* RGB565 checker */
	tex->Unlock(0);

	IDirect3D7* d3d=NULL; dd->QueryInterface(IID_IDirect3D7,(void**)&d3d);
	IDirect3DDevice7* dev=NULL; d3d->CreateDevice(IID_IDirect3DHALDevice,(LPDIRECTDRAWSURFACE7)tex,&dev);

	struct TLV { float x,y,z,rhw; unsigned argb; float u,v; };
	D3DVERTEXBUFFERDESC vd; memset(&vd,0,sizeof(vd)); vd.dwSize=sizeof(vd);
	vd.dwFVF=0x004|0x040|0x100; vd.dwNumVertices=4;   /* XYZRHW|DIFFUSE|TEX1 */
	IDirect3DVertexBuffer7* vb=NULL; d3d->CreateVertexBuffer(&vd,&vb,0);
	TLV* v=NULL; DWORD vsz=0; vb->Lock(0,(LPVOID*)&v,&vsz);
	v[0].x=200;v[0].y=150; v[1].x=600;v[1].y=150; v[2].x=600;v[2].y=450; v[3].x=200;v[3].y=450;
	for(int i=0;i<4;i++){ v[i].z=0; v[i].rhw=1; v[i].argb=0xffffffff; }
	v[0].u=0;v[0].v=0; v[1].u=1;v[1].v=0; v[2].u=1;v[2].v=1; v[3].u=0;v[3].v=1;
	vb->Unlock();

	for (int f=0; f<60; f++) {
		dev->BeginScene();
		dev->Clear(0,NULL,0x1,0x00203040,1,0);              /* D3DCLEAR_TARGET, dark */
		dev->SetRenderState((D3DRENDERSTATETYPE)27,0);      /* ALPHABLENDENABLE off */
		dev->SetTexture(0,tex);
		dev->DrawPrimitiveVB((D3DPRIMITIVETYPE)6,vb,0,4,0); /* TRIANGLEFAN */
		dev->EndScene();
		SDL_GL_SwapWindow(g_win); pump_events(); SDL_Delay(16);
	}
	unsigned char mid[3]={0,0,0};
	glReadPixels(400,300,1,1,GL_RGB,GL_UNSIGNED_BYTE,mid);
	fprintf(stderr,"[vid] render smoketest: centre(400,300) rgb=(%d,%d,%d) [expect red/green checker, not 32,48,64] glErr=%d\n",
		mid[0],mid[1],mid[2],(int)glGetError());
	return 1;
}

/* Phase 2 input smoke test (BOB_INPUT_SMOKETEST=1): drive the keyboard device the
   way the game does -- CreateDevice(GUID_SysKeyboard), SetDataFormat,
   SetEventNotification, Acquire -- then simulate key events into the queue and
   verify GetDeviceData drains them as DIK buffered events and that
   MsgWaitForMultipleObjects (bob_msg_wait) wakes on the keyboard handle. */
extern "C" int bob_input_smoketest(void)
{
	init_dinput_once();
	LPDIRECTINPUT di=NULL; DirectInputCreateA(0,0,&di,0);
	LPDIRECTINPUTDEVICE kb=NULL; di->CreateDevice(GUID_SysKeyboard,&kb,0);
	void* fakeEvent=(void*)0xABCDul;
	kb->SetEventNotification((HANDLE)fakeEvent);
	kb->Acquire();
	kb_push(0x1E,1); kb_push(0x1E,0); kb_push(0xC8,1);   /* A down, A up, UP down */

	DIDEVICEOBJECTDATA ev[16]; DWORD n=16;
	kb->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),ev,&n,0);
	fprintf(stderr,"[input] GetDeviceData drained %u events (expect 3): ",(unsigned)n);
	for (DWORD i=0;i<n;i++) fprintf(stderr,"{ofs=%02x data=%02x} ",(unsigned)ev[i].dwOfs,(unsigned)ev[i].dwData);
	fprintf(stderr,"\n");

	kb_push(0x39,1);                                    /* SPACE down -> input pending */
	void* handles[3]={(void*)1,fakeEvent,(void*)3};
	unsigned long r=bob_msg_wait(3,handles,0);
	fprintf(stderr,"[input] bob_msg_wait -> %lu (expect 1 = keyboard handle index)\n", r);
	return 1;
}

#endif /* FF_LINUX */
