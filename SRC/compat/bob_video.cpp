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

#define VLOG(...) do{ if(g_traceVid) fprintf(stderr,"[vid] " __VA_ARGS__); }while(0)

static void ensure_window(int w, int h)
{
	if (w > 0 && h > 0) { g_scrW = w; g_scrH = h; }
	if (g_win) {
		SDL_SetWindowSize(g_win, g_scrW, g_scrH);
		return;
	}
	g_traceVid = getenv("BOB_TRACE_VID") ? 1 : 0;
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "[vid] SDL_Init failed: %s\n", SDL_GetError());
		return;
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	g_win = SDL_CreateWindow("Rowan's Battle of Britain (Linux native port)",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		g_scrW, g_scrH, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!g_win) { fprintf(stderr, "[vid] SDL_CreateWindow failed: %s\n", SDL_GetError()); return; }
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
static void kb_push(unsigned dik, int down) {
	if (!dik) return;
	int nt=(g_kbTail+1)%BOB_KBQ;
	if (nt==g_kbHead) return;          /* full: drop oldest-style */
	g_kbq[g_kbTail].ofs=dik; g_kbq[g_kbTail].data=down?0x80:0x00;
	g_kbTail=nt;
}

/* Pump the SDL event queue: window close + keyboard -> DIK queue. */
static void pump_events(void)
{
	if (!g_win) return;
	/* Synthetic input for headless testing (no physical keyboard).
	   BOB_AUTOFLY=sweep : press every DIK in turn (verify key->command dispatch).
	   BOB_AUTOFLY=throttle (or 1): tap '0' (DIK 0x0B = RPM_00 = 100% throttle) a few
	   times so the parked aircraft should spool up and accelerate down the runway. */
	if (getenv("BOB_AUTOFLY") && g_diKbAcquired) {
		const char* mode=getenv("BOB_AUTOFLY");
		static int cnt=0; cnt++;
		if (mode && mode[0]=='s') { static int sweep=1;
			if ((cnt%4)==0) { kb_push(sweep,1); kb_push(sweep,0); if(++sweep>0xD8) sweep=1; } }
		else { if ((cnt%30)==0 && cnt<600) { kb_push(0x0B,1); kb_push(0x0B,0); } }  /* full throttle */
	}
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) { fprintf(stderr,"[vid] window closed -> exit\n"); SDL_Quit(); _exit(0); }
		else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
			int dik = sdl_to_dik(e.key.keysym.scancode);
			if (dik && g_diKbAcquired && !e.key.repeat) kb_push(dik, e.type==SDL_KEYDOWN);
			/* a hard exit hatch while the UI loop isn't wired: Ctrl+ESC quits */
			if (e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_ESCAPE && (e.key.keysym.mod & KMOD_CTRL)) {
				SDL_Quit(); _exit(0);
			}
		}
	}
}

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
struct GLDD7      { IDirectDraw7Vtbl* lpVtbl; HWND hwnd; DWORD coopFlags; };

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
	if (!getenv("BOB_TRACE_PRESENT") && !getenv("BOB_DUMP_FRAME")) return;
	static int frames=0; frames++;
	if (getenv("BOB_TRACE_PRESENT") && (frames<=3 || (frames%60)==0)) {
		unsigned char px[3]={0,0,0};
		glReadPixels(g_scrW/2,g_scrH/2,1,1,GL_RGB,GL_UNSIGNED_BYTE,px);
		fprintf(stderr,"[present] frame %d via %s centre rgb=(%d,%d,%d) glErr=%d\n",
			frames,path,px[0],px[1],px[2],(int)glGetError());
	}
	const char* df = getenv("BOB_DUMP_FRAME");
	if (df && frames == atoi(df)) {
		int w=g_scrW,h=g_scrH; unsigned char* buf=(unsigned char*)malloc(w*h*3);
		glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,buf);
		/* raw POSIX open() to bypass the game's redirected fopen */
		int fd=::open("/tmp/bobframe.ppm",O_WRONLY|O_CREAT|O_TRUNC,0644);
		if (fd>=0){ char hdr[64]; int n=snprintf(hdr,sizeof(hdr),"P6\n%d %d\n255\n",w,h);
			if (write(fd,hdr,n)<0){} for (int y=h-1;y>=0;y--) if(write(fd,buf+y*w*3,w*3)<0){}
			close(fd); fprintf(stderr,"[present] dumped frame %d to /tmp/bobframe.ppm (%dx%d) glErr=%d\n",frames,w,h,(int)glGetError()); }
		else fprintf(stderr,"[present] dump open failed errno path\n");
		free(buf);
		if (getenv("BOB_EXIT_AFTER_DUMP")) { fflush(stderr); _exit(0); }
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
static ULONG   SURF_Release(IDirectDrawSurface7* This) { GLSurface7* s=(GLSurface7*)This; if(s->bits) free(s->bits); free(s); return 0; }
static HRESULT SURF_GetCaps(IDirectDrawSurface7* This, LPDDSCAPS2 c) { GLSurface7* s=(GLSurface7*)This; if(c)*c=s->desc.ddsCaps; return DD_OK; }

static GLSurface7* make_surface(const DDSURFACEDESC2* in, int defW, int defH)
{
	GLSurface7* s = (GLSurface7*)calloc(1, sizeof(GLSurface7));
	s->lpVtbl = &g_surfVtbl;
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
static HRESULT DD_SetDisplayMode(IDirectDraw7*, DWORD w, DWORD h, DWORD, DWORD, DWORD) { ensure_window((int)w,(int)h); return DD_OK; }
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
static ULONG DD_Release(IDirectDraw7* This) { free((void*)This); return 0; }
/* Report a couple of display modes so EnumerateDriverModes builds a list. */
static HRESULT DD_EnumDisplayModes(IDirectDraw7*, DWORD, LPDDSURFACEDESC2, LPVOID ctx, LPDDENUMMODESCALLBACK2 cb) {
	if (!cb) return DD_OK;
	static const int modes[][2] = {{1024,768},{1280,1024},{800,600},{1280,720},{1920,1080}};
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

	g_surfVtbl.AddRef=(ULONG(*)(IDirectDrawSurface7*))generic_addref;
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

	g_ddVtbl.AddRef=(ULONG(*)(IDirectDraw7*))generic_addref;
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
	return 0;
}
static HRESULT DIDEV_GetDeviceData(IDirectInputDeviceA* This, DWORD, LPDIDEVICEOBJECTDATA buf, LPDWORD inout, DWORD flags) {
	if (!inout) return 0;
	if (This!=&g_diKeyboard) { *inout=0; return 0; }
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
static HRESULT DIDEV_Acquire(IDirectInputDeviceA* This) { if (This==&g_diKeyboard) g_diKbAcquired=1; return 0; }
static HRESULT DIDEV_SetEventNotify(IDirectInputDeviceA* This, HANDLE h) { if (This==&g_diKeyboard) g_diKbNotify=(void*)h; return 0; }
static HRESULT DIDEV_ok(IDirectInputDeviceA*) { return 0; }
static HRESULT DIDEV_SetProperty(IDirectInputDeviceA*, REFGUID, LPCDIPROPHEADER) { return 0; }
static HRESULT DIDEV_GetProperty(IDirectInputDeviceA*, REFGUID, LPDIPROPHEADER) { return 0; }
static HRESULT DIDEV_SetDataFormat(IDirectInputDeviceA*, LPCDIDATAFORMAT) { return 0; }
static HRESULT DIDEV_SetCoop(IDirectInputDeviceA*, HWND, DWORD) { return 0; }
static HRESULT DIDEV_EnumObjects(IDirectInputDeviceA*, LPDIENUMDEVICEOBJECTSCALLBACKA, LPVOID, DWORD) { return 0; }
static HRESULT DIDEV_GetCaps(IDirectInputDeviceA*, LPDIDEVCAPS c) { if (c) { DWORD sz=c->dwSize; memset(c,0,sz?sz:sizeof(*c)); c->dwSize=sz?sz:sizeof(*c); } return 0; }
static ULONG   DIDEV_addref(IDirectInputDeviceA*) { return 1; }
static ULONG   DIDEV_release(IDirectInputDeviceA*) { return 0; }

static HRESULT DI_CreateDevice(IDirectInputA*, REFGUID rguid, LPDIRECTINPUTDEVICE* out, IUnknown*) {
	if (!out) return E_FAIL;
	/* hand back a shared dummy device (any of them is fine -- all no-op) */
	if      (rguid == GUID_SysKeyboard) *out = &g_diKeyboard;
	else                                *out = &g_diGeneric;
	return 0;
}
static HRESULT DI_EnumDevices(IDirectInputA*, DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD) { return 0; }
static ULONG   DI_addref(IDirectInputA*) { return 1; }
static ULONG   DI_release(IDirectInputA*) { return 0; }

static IDirectInputA g_theDI;

static void init_dinput_once(void) {
	static int done=0; if (done) return; done=1;
	g_didevVtbl.AddRef=DIDEV_addref; g_didevVtbl.Release=DIDEV_release;
	g_didevVtbl.GetDeviceState=DIDEV_GetDeviceState; g_didevVtbl.GetDeviceData=DIDEV_GetDeviceData;
	g_didevVtbl.Acquire=DIDEV_Acquire; g_didevVtbl.Unacquire=DIDEV_ok; g_didevVtbl.Poll=DIDEV_ok;
	g_didevVtbl.SetEventNotification=DIDEV_SetEventNotify;
	g_didevVtbl.SetProperty=DIDEV_SetProperty; g_didevVtbl.GetProperty=DIDEV_GetProperty;
	g_didevVtbl.SetDataFormat=DIDEV_SetDataFormat; g_didevVtbl.SetCooperativeLevel=DIDEV_SetCoop;
	g_didevVtbl.EnumObjects=DIDEV_EnumObjects; g_didevVtbl.GetCapabilities=DIDEV_GetCaps;
	g_diKeyboard.lpVtbl=g_diMouse.lpVtbl=g_diJoystick.lpVtbl=g_diGeneric.lpVtbl=&g_didevVtbl;

	g_diVtbl.AddRef=DI_addref; g_diVtbl.Release=DI_release;
	g_diVtbl.CreateDevice=DI_CreateDevice; g_diVtbl.EnumDevices=DI_EnumDevices;
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
