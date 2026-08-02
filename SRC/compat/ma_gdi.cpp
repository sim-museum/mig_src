/* ma_gdi.cpp — GDI software-canvas backend for the Linux port.
 *
 * The Win32 front-end (RDialog panels + the CRListBoxCtrl OCX) draws through GDI:
 * memory DCs, CreateCompatibleBitmap/SelectObject, FillRect, CPen lines, BitBlt,
 * SetDIBitsToDevice and TextOut. On Windows each HDC is a device context backed by
 * a bitmap; here every HDC is an MaDC backed by a 32-bit BGRA software surface.
 *
 * The SCREEN dc is the special handle (void*)1 (the value the compat BeginPaint and
 * CDC stubs already hand out). It is backed by a single screen canvas that the whole
 * front-end composes into; ma_gdi_present_screen() uploads it to GL once per frame.
 *
 * Pixel format: 0xAARRGGBB packed little-endian -> bytes B,G,R,A == GL_BGRA, matching
 * ma_gl_blit_bgra() in bob_video.cpp. COLORREF is Win32 0x00BBGGRR. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>      /* S58 MA_SHOT: raw open() for the canvas dump */
#include <unistd.h>

#pragma pack(push, 8)          /* keep stb's structs native-ABI despite the global -fpack-struct=1 */
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"
#pragma pack(pop)

extern "C" {
void ma_gl_blit_bgra(const void* px, int w, int h);
void ma_ddraw_ensure_window(int w, int h);
}

/* ---- TrueType glyph rendering (stb_truetype) ---------------------------------
 * Replaces the built-in 8x8 bitmap font with the real game/system TTF, antialiased,
 * so the front-end text matches the original. The game's own frontend faces live under
 * <DRIVE_C>/windows/Fonts/ (Mig Alley ships Intel.ttf, a non-standard TTF stb can't parse,
 * so we fall through to a system serif close to the Win menu face). Override via MA_FONT. */
static unsigned char* g_ttfBuf = NULL;
static stbtt_fontinfo  g_ttf;
static int             g_ttfState = 0;   /* 0 unloaded, 1 ok, -1 failed */

static int ttf_try(const char* p) {
	if (!p || !*p) return 0;
	FILE* f = fopen(p, "rb"); if (!f) return 0;
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	if (n <= 0) { fclose(f); return 0; }
	unsigned char* buf = (unsigned char*)malloc(n);
	size_t got = fread(buf, 1, n, f); fclose(f);
	if ((long)got != n || !stbtt_InitFont(&g_ttf, buf, stbtt_GetFontOffsetForIndex(buf, 0))) {
		free(buf); return 0;
	}
	g_ttfBuf = buf; fprintf(stderr, "[gdifont] loaded %s\n", p); return 1;
}
static int ttf_load(void) {
	if (g_ttfState) return g_ttfState > 0;
	char path[1200];
	const char* drive = getenv("BOB_DRIVE_C");
	const char* gameFonts[] = { "Intel.ttf", "g101016_.ttf", "FUSION_B.TTF", NULL };
	if (ttf_try(getenv("MA_FONT"))) { g_ttfState = 1; return 1; }
	for (int i = 0; drive && gameFonts[i]; i++) {
		snprintf(path, sizeof(path), "%s/windows/Fonts/%s", drive, gameFonts[i]);
		if (ttf_try(path)) { g_ttfState = 1; return 1; }
	}
	const char* sys[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf",
		"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
		"/usr/share/fonts/truetype/liberation/LiberationSerif-Bold.ttf",
		"/usr/share/fonts/TTF/DejaVuSerif-Bold.ttf", NULL };
	for (int i = 0; sys[i]; i++) if (ttf_try(sys[i])) { g_ttfState = 1; return 1; }
	fprintf(stderr, "[gdifont] no TTF found -> 8x8 bitmap fallback\n");
	g_ttfState = -1; return 0;
}

typedef unsigned int  u32;
typedef unsigned char u8;

/* COLORREF (0x00BBGGRR) -> canvas pixel (0xFFRRGGBB) */
static inline u32 cr2px(u32 cr) {
	u32 r = cr & 0xFF, g = (cr >> 8) & 0xFF, b = (cr >> 16) & 0xFF;
	return 0xFF000000u | (r << 16) | (g << 8) | b;
}

/* ---- built-in 8x8 bitmap font (printable ASCII 0x20..0x7E) -------------- */
/* font8x8_basic (public domain, Daniel Hepper / dhepper). Each glyph is 8 rows;
   bit 0 (LSB) = leftmost pixel. Glyphs are scaled to the requested font height. */
static const unsigned char FONT8X8[95][8] = {
{0,0,0,0,0,0,0,0},{0x18,0x3C,0x3C,0x18,0x18,0,0x18,0},{0x36,0x36,0,0,0,0,0,0},
{0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0},{0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0},
{0,0x63,0x33,0x18,0x0C,0x66,0x63,0},{0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0},
{0x06,0x06,0x03,0,0,0,0,0},{0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0},
{0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0},{0,0x66,0x3C,0xFF,0x3C,0x66,0,0},
{0,0x0C,0x0C,0x3F,0x0C,0x0C,0,0},{0,0,0,0,0,0x0C,0x0C,0x06},
{0,0,0,0x3F,0,0,0,0},{0,0,0,0,0,0x0C,0x0C,0},{0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0},
{0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0},{0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0},
{0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0},{0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0},
{0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0},{0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0},
{0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0},{0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0},
{0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0},{0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0},
{0,0x0C,0x0C,0,0,0x0C,0x0C,0},{0,0x0C,0x0C,0,0,0x0C,0x0C,0x06},
{0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0},{0,0,0x3F,0,0,0x3F,0,0},
{0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0},{0x1E,0x33,0x30,0x18,0x0C,0,0x0C,0},
{0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0},{0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0},
{0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0},{0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0},
{0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0},{0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0},
{0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0},{0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0},
{0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0},{0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0},
{0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0},{0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0},
{0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0},{0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0},
{0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0},{0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0},
{0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0},{0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0},
{0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0},{0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0},
{0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0},{0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0},
{0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0},{0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0},
{0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0},{0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0},
{0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0},{0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0},
{0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0},{0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0},
{0x08,0x1C,0x36,0x63,0,0,0,0},{0,0,0,0,0,0,0,0xFF},{0x0C,0x0C,0x18,0,0,0,0,0},
{0,0,0x1E,0x30,0x3E,0x33,0x6E,0},{0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0},
{0,0,0x1E,0x33,0x03,0x33,0x1E,0},{0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0},
{0,0,0x1E,0x33,0x3f,0x03,0x1E,0},{0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0},
{0,0,0x6E,0x33,0x33,0x3E,0x30,0x1F},{0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0},
{0x0C,0,0x0E,0x0C,0x0C,0x0C,0x1E,0},{0x30,0,0x30,0x30,0x30,0x33,0x33,0x1E},
{0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0},{0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0},
{0,0,0x33,0x7F,0x7F,0x6B,0x63,0},{0,0,0x1F,0x33,0x33,0x33,0x33,0},
{0,0,0x1E,0x33,0x33,0x33,0x1E,0},{0,0,0x3B,0x66,0x66,0x3E,0x06,0x0F},
{0,0,0x6E,0x33,0x33,0x3E,0x30,0x78},{0,0,0x3B,0x6E,0x66,0x06,0x0F,0},
{0,0,0x3E,0x03,0x1E,0x30,0x1F,0},{0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0},
{0,0,0x33,0x33,0x33,0x33,0x6E,0},{0,0,0x33,0x33,0x33,0x1E,0x0C,0},
{0,0,0x63,0x6B,0x7F,0x7F,0x36,0},{0,0,0x63,0x36,0x1C,0x36,0x63,0},
{0,0,0x33,0x33,0x33,0x3E,0x30,0x1F},{0,0,0x3F,0x19,0x0C,0x26,0x3F,0},
{0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0},{0x18,0x18,0x18,0,0x18,0x18,0x18,0},
{0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0},{0x6E,0x3B,0,0,0,0,0,0}
};

struct MaFont { int height, weight, italic; int cw, ch; };

/* ---- surfaces ----------------------------------------------------------- */
struct MaBitmap { int w, h; u32* px; };

struct MaDC {
	int   isScreen;
	int   w, h;
	u32*  px;            /* points at canvas (screen) or selected bitmap's pixels */
	MaBitmap* selBmp;    /* currently selected bitmap (memory DC) */
	/* GDI state */
	u32   penColor;  int penWidth; int penNull;
	u32   brushColor; int brushNull;
	u32   textColor;
	u32   bkColor;   int bkMode;     /* Win32: TRANSPARENT=1, OPAQUE=2 */
	int   curX, curY;
	void* font;
	int   ox, oy;        /* viewport origin: added to all destination coords */
};

/* the one screen canvas */
static u32* g_canvas = 0;
static int  g_cw = 0, g_ch = 0;
static MaDC g_screenDC;
static int  g_screenInit = 0;

static void screen_init() {
	if (g_screenInit) return;
	g_screenInit = 1;
	memset(&g_screenDC, 0, sizeof(g_screenDC));
	g_screenDC.isScreen = 1;
	g_screenDC.penWidth = 1; g_screenDC.bkMode = 2 /*OPAQUE default*/; g_screenDC.bkColor = 0xFFFFFFFFu;
}

static void ensure_canvas(int w, int h) {
	screen_init();
	if (w <= g_cw && h <= g_ch && g_canvas) return;
	int nw = w > g_cw ? w : g_cw, nh = h > g_ch ? h : g_ch;
	u32* nc = (u32*)calloc((size_t)nw * nh, 4);
	if (!nc) return;
	if (g_canvas) {                          /* preserve existing pixels */
		for (int y = 0; y < g_ch; y++)
			memcpy(nc + (size_t)y * nw, g_canvas + (size_t)y * g_cw, (size_t)g_cw * 4);
		free(g_canvas);
	}
	g_canvas = nc; g_cw = nw; g_ch = nh;
	g_screenDC.px = g_canvas; g_screenDC.w = g_cw; g_screenDC.h = g_ch;
	if (getenv("MA_TRACE_CANVAS")) fprintf(stderr, "[canvas] grow -> %dx%d (requested %dx%d)\n", nw, nh, w, h);
}

static MaDC* resolve(void* hdc) {
	screen_init();
	if (hdc == (void*)0) return 0;
	if (hdc == (void*)1) { g_screenDC.px = g_canvas; g_screenDC.w = g_cw; g_screenDC.h = g_ch; return &g_screenDC; }
	return (MaDC*)hdc;
}

/* ---- public C API (called from compat CDC / wingdi) --------------------- */
extern "C" {

void* ma_gdi_screen_dc(void) { screen_init(); return (void*)1; }

void ma_gdi_screen_resize(int w, int h) { if (w > 0 && h > 0) ensure_canvas(w, h); }

void* ma_gdi_create_dc(void) {
	MaDC* dc = (MaDC*)calloc(1, sizeof(MaDC));
	if (!dc) return 0;
	dc->penWidth = 1; dc->bkMode = 2 /*OPAQUE default*/; dc->bkColor = 0xFFFFFFFFu;
	return dc;
}
void ma_gdi_delete_dc(void* hdc) {
	MaDC* dc = resolve(hdc);
	if (dc && !dc->isScreen) free(dc);
}

void* ma_gdi_create_bitmap(int w, int h) {
	if (w <= 0) w = 1; if (h <= 0) h = 1;
	MaBitmap* b = (MaBitmap*)calloc(1, sizeof(MaBitmap));
	if (!b) return 0;
	b->w = w; b->h = h; b->px = (u32*)calloc((size_t)w * h, 4);
	return b;
}
void ma_gdi_delete_bitmap(void* hbmp) {
	MaBitmap* b = (MaBitmap*)hbmp;
	if (b) { free(b->px); free(b); }
}
void ma_gdi_bitmap_size(void* hbmp, int* w, int* h) {
	MaBitmap* b = (MaBitmap*)hbmp;
	if (b) { if (w) *w = b->w; if (h) *h = b->h; }
}

/* select a bitmap into a (memory) DC: the DC then draws into the bitmap */
void* ma_gdi_select_bitmap(void* hdc, void* hbmp) {
	MaDC* dc = resolve(hdc); MaBitmap* b = (MaBitmap*)hbmp;
	if (!dc) return 0;
	MaBitmap* prev = dc->selBmp;
	dc->selBmp = b;
	if (b) { dc->px = b->px; dc->w = b->w; dc->h = b->h; }
	return prev;
}

void ma_gdi_set_pen(void* hdc, int width, u32 colorref, int isnull) {
	MaDC* dc = resolve(hdc); if (!dc) return;
	dc->penColor = cr2px(colorref); dc->penWidth = width > 0 ? width : 1; dc->penNull = isnull;
}
void ma_gdi_set_brush(void* hdc, u32 colorref, int isnull) {
	MaDC* dc = resolve(hdc); if (!dc) return;
	dc->brushColor = cr2px(colorref); dc->brushNull = isnull;
}
void ma_gdi_set_text_color(void* hdc, u32 colorref) { MaDC* dc = resolve(hdc); if (dc) dc->textColor = cr2px(colorref); }
void ma_gdi_set_bk_color(void* hdc, u32 colorref)   { MaDC* dc = resolve(hdc); if (dc) dc->bkColor = cr2px(colorref); }
void ma_gdi_set_bk_mode(void* hdc, int mode)        { MaDC* dc = resolve(hdc); if (dc) dc->bkMode = mode; }
void ma_gdi_set_font(void* hdc, void* font)         { MaDC* dc = resolve(hdc); if (dc) dc->font = font; }
void* ma_gdi_get_font(void* hdc)                    { MaDC* dc = resolve(hdc); return dc ? dc->font : 0; }
/* set viewport origin; returns the old origin packed (oldx in low, oldy in high 16) */
void ma_gdi_set_viewport_org(void* hdc, int x, int y, int* oldx, int* oldy) {
	MaDC* dc = resolve(hdc); if (!dc) return;
	if (oldx) *oldx = dc->ox; if (oldy) *oldy = dc->oy;
	dc->ox = x; dc->oy = y;
}

static inline void putpx(MaDC* dc, int x, int y, u32 p) {
	x += dc->ox; y += dc->oy;
	if (x < 0 || y < 0 || x >= dc->w || y >= dc->h || !dc->px) return;
	dc->px[(size_t)y * dc->w + x] = p;
}

void ma_gdi_fill_solid(void* hdc, int l, int t, int r, int b, u32 colorref) {
	MaDC* dc = resolve(hdc); if (!dc || !dc->px) return;
	u32 p = cr2px(colorref);
	l += dc->ox; r += dc->ox; t += dc->oy; b += dc->oy;
	if (l < 0) l = 0; if (t < 0) t = 0; if (r > dc->w) r = dc->w; if (b > dc->h) b = dc->h;
	for (int y = t; y < b; y++) { u32* row = dc->px + (size_t)y * dc->w; for (int x = l; x < r; x++) row[x] = p; }
}

/* FillRect(rect, brush) — brush color passed in as colorref */
void ma_gdi_fill_rect(void* hdc, int l, int t, int r, int b, u32 colorref) { ma_gdi_fill_solid(hdc, l, t, r, b, colorref); }

/* Rectangle(): brush fill interior + pen border */
void ma_gdi_rectangle(void* hdc, int l, int t, int r, int b) {
	MaDC* dc = resolve(hdc); if (!dc || !dc->px) return;
	if (!dc->brushNull) {
		u32 p = dc->brushColor;
		for (int y = t; y < b; y++) for (int x = l; x < r; x++) putpx(dc, x, y, p);
	}
	if (!dc->penNull) {
		u32 p = dc->penColor;
		for (int x = l; x < r; x++) { putpx(dc, x, t, p); putpx(dc, x, b - 1, p); }
		for (int y = t; y < b; y++) { putpx(dc, l, y, p); putpx(dc, r - 1, y, p); }
	}
}

void ma_gdi_move_to(void* hdc, int x, int y) { MaDC* dc = resolve(hdc); if (dc) { dc->curX = x; dc->curY = y; } }
void ma_gdi_line_to(void* hdc, int x1, int y1) {
	MaDC* dc = resolve(hdc); if (!dc || !dc->px) return;
	if (dc->penNull) { dc->curX = x1; dc->curY = y1; return; }
	int x0 = dc->curX, y0 = dc->curY;
	int dx = abs(x1 - x0), dy = -abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
	u32 p = dc->penColor; int hw = dc->penWidth / 2;
	for (;;) {
		for (int oy = -hw; oy <= hw; oy++) for (int ox = -hw; ox <= hw; ox++) putpx(dc, x0 + ox, y0 + oy, p);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
	dc->curX = x1; dc->curY = y1;
}

void ma_gdi_set_pixel(void* hdc, int x, int y, u32 colorref) { MaDC* dc = resolve(hdc); if (dc) putpx(dc, x, y, cr2px(colorref)); }
u32  ma_gdi_get_pixel(void* hdc, int x, int y) {
	MaDC* dc = resolve(hdc);
	if (!dc || !dc->px || x < 0 || y < 0 || x >= dc->w || y >= dc->h) return 0xFFFFFFFFu;
	u32 p = dc->px[(size_t)y * dc->w + x];
	return ((p >> 16) & 0xFF) | (((p >> 8) & 0xFF) << 8) | ((p & 0xFF) << 16);   /* -> COLORREF */
}

/* SRCCOPY BitBlt between DCs */
void ma_gdi_bitblt(void* hdst, int dx, int dy, int w, int h, void* hsrc, int sx, int sy, unsigned long rop) {
	(void)rop;
	MaDC* d = resolve(hdst); MaDC* s = resolve(hsrc);
	if (!d || !s || !d->px || !s->px) return;
	dx += d->ox; dy += d->oy;            /* dest viewport origin */
	sx += s->ox; sy += s->oy;            /* src origin (usually 0) */
	for (int y = 0; y < h; y++) {
		int syy = sy + y, dyy = dy + y;
		if (dyy < 0 || dyy >= d->h || syy < 0 || syy >= s->h) continue;
		for (int x = 0; x < w; x++) {
			int sxx = sx + x, dxx = dx + x;
			if (dxx < 0 || dxx >= d->w || sxx < 0 || sxx >= s->w) continue;
			d->px[(size_t)dyy * d->w + dxx] = s->px[(size_t)syy * s->w + sxx];
		}
	}
}

/* nearest-neighbour StretchBlt */
void ma_gdi_stretchblt(void* hdst, int dx, int dy, int dw, int dh, void* hsrc, int sx, int sy, int sw, int sh, unsigned long rop) {
	(void)rop;
	MaDC* d = resolve(hdst); MaDC* s = resolve(hsrc);
	if (!d || !s || !d->px || !s->px || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return;
	dx += d->ox; dy += d->oy; sx += s->ox; sy += s->oy;
	for (int y = 0; y < dh; y++) {
		int syy = sy + (int)((long long)y * sh / dh), dyy = dy + y;
		if (dyy < 0 || dyy >= d->h || syy < 0 || syy >= s->h) continue;
		for (int x = 0; x < dw; x++) {
			int sxx = sx + (int)((long long)x * sw / dw), dxx = dx + x;
			if (dxx < 0 || dxx >= d->w || sxx < 0 || sxx >= s->w) continue;
			d->px[(size_t)dyy * d->w + dxx] = s->px[(size_t)syy * s->w + sxx];
		}
	}
}

/* SetDIBitsToDevice: decode a packed DIB (8/24/32 bpp, bottom-up unless biHeight<0)
   into the DC surface at (dx,dy). Reads the BITMAPINFO header by byte offset to stay
   independent of struct packing. */
void ma_gdi_set_dibits(void* hdc, int dx, int dy, int destW, int destH,
                       const void* bits, const void* bmiv) {
	MaDC* dc = resolve(hdc);
	const u8* bmi = (const u8*)bmiv;
	if (!dc || !bits || !bmi) return;
	int biSize  = *(const int*)(bmi + 0);
	int biW     = *(const int*)(bmi + 4);
	int biH     = *(const int*)(bmi + 8);
	int bpp     = *(const unsigned short*)(bmi + 14);
	int H = biH < 0 ? -biH : biH, W = biW;
	if (W <= 0 || H <= 0) return;
	int comp = *(const int*)(bmi + 16);              /* biCompression: 0=BI_RGB 1=BI_RLE8 */
	if (getenv("MA_TRACE_DIB")) { static int n=0; if(n++<8)
		fprintf(stderr,"[setdib] dst=(%d,%d) %dx%d bpp=%d comp=%d\n", dx,dy,W,H,bpp,comp); }
	int topdown = biH < 0;
	int srcpitch = ((W * bpp + 31) / 32) * 4;
	const u8* palb = bmi + (biSize ? biSize : 40);   /* RGBQUAD[] : B,G,R,0 */
	const u8* src8 = (const u8*)bits;
	/* BI_RLE8: Win32 SetDIBitsToDevice decompresses; we must too. Decode the RLE stream into a
	   raw-index buffer (scanline 0 = bottom row, BMP order) so the existing bottom-up loop works. */
	if (comp == 1 && bpp == 8) {
		static u8* rle = 0; static int rlecap = 0;
		if (rlecap < W*H) { free(rle); rle = (u8*)malloc((size_t)W*H); rlecap = W*H; }
		if (rle) {
			memset(rle, 0, (size_t)W*H);
			unsigned sizeImage = *(const unsigned*)(bmi + 20);
			const u8* p = src8;
			const u8* pend = src8 + (sizeImage ? sizeImage : (unsigned)(W*H*2));
			int x = 0, row = 0;
			while (p + 1 < pend && row < H) {
				u8 cnt = *p++;
				if (cnt) {                       /* encoded run: cnt copies of one index */
					u8 v = *p++;
					while (cnt-- && x < W) rle[(size_t)row*W + x++] = v;
				} else {                         /* escape */
					u8 sec = *p++;
					if (sec == 0) { x = 0; row++; }                    /* end of line */
					else if (sec == 1) break;                          /* end of bitmap */
					else if (sec == 2) { if (p + 1 < pend) { x += *p++; row += *p++; } }  /* delta */
					else {                                             /* absolute: sec literals */
						for (int i = 0; i < sec; i++) { if (p >= pend) break; u8 v = *p++;
							if (x < W && row < H) rle[(size_t)row*W + x++] = v; }
						if (sec & 1) p++;                              /* word-align */
					}
				}
			}
			src8 = rle; srcpitch = W; topdown = 0;   /* row 0 = bottom, like uncompressed BMP */
		}
	}
	if (dc->isScreen) {
		int needW = dx + W, needH = dy + H;
		if (destW > needW) needW = destW;
		if (destH > needH) needH = destH;
		if (getenv("MA_TRACE_CANVAS") && (needH > g_ch || needW > g_cw))
			fprintf(stderr, "[canvas] set_dibits at(%d,%d) dib=%dx%d dest=%dx%d ox=%d oy=%d\n",
			        dx, dy, W, H, destW, destH, dc->ox, dc->oy);
		ensure_canvas(needW, needH);
	}
	int copyW = W, copyH = H;
	for (int y = 0; y < copyH; y++) {
		int srcrow = topdown ? y : (H - 1 - y);
		const u8* s = src8 + (size_t)srcrow * srcpitch;
		int dyy = dy + y;
		for (int x = 0; x < copyW; x++) {
			u32 px;
			if (bpp == 8)       { const u8* c = palb + (size_t)s[x] * 4; px = 0xFF000000u | (c[2] << 16) | (c[1] << 8) | c[0]; }
			else if (bpp == 24) { const u8* p = s + x * 3; px = 0xFF000000u | (p[2] << 16) | (p[1] << 8) | p[0]; }
			else                { const u8* p = s + x * 4; px = 0xFF000000u | (p[2] << 16) | (p[1] << 8) | p[0]; }
			putpx(dc, dx + x, dyy, px);
		}
	}
}

/* StretchDIBits: decode a packed DIB and nearest-neighbour scale a source sub-rect
   (sx,sy,sw,sh) into a dest rect (dx,dy,dw,dh). Used by the campaign operational map
   (CMIGView::UpdateBitmaps blits the scrolled/zoomed Korea MIDMAP tiles through this). */
void ma_gdi_stretch_dibits(void* hdc, int dx, int dy, int dw, int dh,
                           int sx, int sy, int sw, int sh,
                           const void* bits, const void* bmiv) {
	MaDC* dc = resolve(hdc);
	const u8* bmi = (const u8*)bmiv;
	if (!dc || !bits || !bmi || dw <= 0 || dh <= 0) return;
	int biSize = *(const int*)(bmi + 0);
	int biW    = *(const int*)(bmi + 4);
	int biH    = *(const int*)(bmi + 8);
	int bpp    = *(const unsigned short*)(bmi + 14);
	int comp   = *(const int*)(bmi + 16);
	int H = biH < 0 ? -biH : biH, W = biW;
	if (W <= 0 || H <= 0) return;
	int topdown = biH < 0;
	int srcpitch = ((W * bpp + 31) / 32) * 4;
	const u8* palb = bmi + (biSize ? biSize : 40);
	const u8* src8 = (const u8*)bits;
	/* RLE8 -> raw index buffer (row 0 = bottom), same as ma_gdi_set_dibits */
	if (comp == 1 && bpp == 8) {
		static u8* rle = 0; static int rlecap = 0;
		if (rlecap < W*H) { free(rle); rle = (u8*)malloc((size_t)W*H); rlecap = W*H; }
		if (!rle) return;
		memset(rle, 0, (size_t)W*H);
		unsigned sizeImage = *(const unsigned*)(bmi + 20);
		const u8* p = src8, *pend = src8 + (sizeImage ? sizeImage : (unsigned)(W*H*2));
		int x = 0, row = 0;
		while (p + 1 < pend && row < H) {
			u8 cnt = *p++;
			if (cnt) { u8 v = *p++; while (cnt-- && x < W) rle[(size_t)row*W + x++] = v; }
			else { u8 sec = *p++;
				if (sec == 0) { x = 0; row++; }
				else if (sec == 1) break;
				else if (sec == 2) { if (p+1 < pend) { x += *p++; row += *p++; } }
				else { for (int i=0;i<sec;i++){ if(p>=pend)break; u8 v=*p++; if(x<W&&row<H) rle[(size_t)row*W+x++]=v; } if (sec&1) p++; }
			}
		}
		src8 = rle; srcpitch = W; topdown = 0;
	}
	if (dc->isScreen) {
		int needW = dx + dw, needH = dy + dh;
		if (getenv("MA_TRACE_CANVAS") && (needH > g_ch || needW > g_cw))
			fprintf(stderr, "[canvas] stretch_dibits at(%d,%d) dest=%dx%d src=%dx%d ox=%d oy=%d\n",
			        dx, dy, dw, dh, W, H, dc->ox, dc->oy);
		ensure_canvas(needW > 0 ? needW : 1, needH > 0 ? needH : 1);
	}
	if (sw <= 0) sw = W; if (sh <= 0) sh = H;
	for (int Y = 0; Y < dh; Y++) {
		int spy = sy + (int)((long long)Y * sh / dh);
		if (spy < 0 || spy >= H) continue;
		int srcrow = topdown ? spy : (H - 1 - spy);
		const u8* s = src8 + (size_t)srcrow * srcpitch;
		int dyy = dy + Y;
		for (int X = 0; X < dw; X++) {
			int spx = sx + (int)((long long)X * sw / dw);
			if (spx < 0 || spx >= W) continue;
			u32 px;
			if (bpp == 8)       { const u8* c = palb + (size_t)s[spx] * 4; px = 0xFF000000u | (c[2]<<16) | (c[1]<<8) | c[0]; }
			else if (bpp == 24) { const u8* p = s + spx*3; px = 0xFF000000u | (p[2]<<16) | (p[1]<<8) | p[0]; }
			else                { const u8* p = s + spx*4; px = 0xFF000000u | (p[2]<<16) | (p[1]<<8) | p[0]; }
			putpx(dc, dx + X, dyy, px);
		}
	}
}

/* ---- fonts + text ------------------------------------------------------- */
void* ma_gdi_font_create(int height, int weight, int italic, const char* face) {
	(void)face;
	MaFont* f = (MaFont*)calloc(1, sizeof(MaFont));
	if (!f) return 0;
	int h = height < 0 ? -height : height;
	if (h < 6) h = 12; if (h > 64) h = 64;
	f->height = h; f->weight = weight; f->italic = italic;
	f->ch = h;
	f->cw = (h * 6 + 4) / 8;   /* ~0.75*height, leaves a little tracking */
	if (f->cw < 4) f->cw = 4;
	return f;
}
void ma_gdi_font_delete(void* hf) { free(hf); }

static MaFont* dc_font(MaDC* dc) {
	static MaFont s_default = { 12, 0, 0, 9, 12 };
	if (dc && dc->font) return (MaFont*)dc->font;
	return &s_default;
}

/* alpha-blend an RGB colour over the dc pixel at (x,y) (viewport-relative, clipped) */
static inline void blendpx(MaDC* dc, int x, int y, int r, int g, int b, int a) {
	x += dc->ox; y += dc->oy;
	if (x < 0 || y < 0 || x >= dc->w || y >= dc->h || !dc->px) return;
	if (a >= 255) { dc->px[(size_t)y*dc->w + x] = 0xFF000000u | ((u32)r<<16) | ((u32)g<<8) | (u32)b; return; }
	u32* d = &dc->px[(size_t)y*dc->w + x];
	int db = (*d)&0xff, dg=((*d)>>8)&0xff, dr=((*d)>>16)&0xff;
	int rr=(r*a+dr*(255-a))/255, gg=(g*a+dg*(255-a))/255, bb=(b*a+db*(255-a))/255;
	*d = 0xFF000000u | ((u32)rr<<16) | ((u32)gg<<8) | (u32)bb;
}
/* advance width of the first n chars at pixel height pixelH (stb), 0 if no TTF */
static int ttf_width(const char* s, int n, int pixelH) {
	if (!ttf_load() || !s || pixelH <= 0) return 0;
	float scale = stbtt_ScaleForPixelHeight(&g_ttf, (float)pixelH), penx = 0;
	for (int i = 0; i < n; i++) {
		int aw; stbtt_GetCodepointHMetrics(&g_ttf, (unsigned char)s[i], &aw, NULL); penx += aw*scale;
		if (i+1 < n) penx += stbtt_GetCodepointKernAdvance(&g_ttf, (unsigned char)s[i], (unsigned char)s[i+1])*scale;
	}
	return (int)(penx + 0.5f);
}

void ma_gdi_text_out(void* hdc, int x, int y, const char* s, int n) {
	MaDC* dc = resolve(hdc); if (!dc || !dc->px || !s) return;
	if (getenv("MA_TRACE_TEXT")) { static int c=0; if(c++<24) fprintf(stderr,"[text] hdc=%p screen=%d @(%d,%d)+org(%d,%d) col=%06x bk=%d n=%d \"%.*s\"\n", hdc, dc->isScreen, x, y, dc->ox, dc->oy, dc->textColor&0xFFFFFF, dc->bkMode, n, n, s); }
	/* S63: trap non-ASCII text draws -- the uninit-garbage hunt (MA_TRACE_GARBAGE). */
	if (getenv("MA_TRACE_GARBAGE")) {
		int bad = 0; for (int i = 0; i < n; i++) { unsigned char ch = (unsigned char)s[i]; if (ch < 0x20 || ch >= 0x7f) { bad = 1; break; } }
		if (bad) { static int gc=0; if (gc++<10) { fprintf(stderr,"[garbage] @(%d,%d)+org(%d,%d) n=%d bytes:", x, y, dc->ox, dc->oy, n);
			for (int i = 0; i < n && i < 24; i++) fprintf(stderr," %02x", (unsigned char)s[i]); fprintf(stderr,"\n"); fflush(stderr);
			if (getenv("MA_TRACE_GARBAGE_ABORT")) abort();   /* opt-in: abort under gdb to get the caller */ } }
	}
	MaFont* f = dc_font(dc);
	u32 fg = dc->textColor;
	int opaque = (dc->bkMode == 2 /*OPAQUE; TRANSPARENT==1*/);
	u32 bg = dc->bkColor;
	if (ttf_load()) {
		int pixelH = f->ch > 0 ? f->ch : 12;
		float scale = stbtt_ScaleForPixelHeight(&g_ttf, (float)pixelH);
		int ascent; stbtt_GetFontVMetrics(&g_ttf, &ascent, NULL, NULL);
		int baseline = y + (int)(ascent*scale + 0.5f);
		int fr=(fg>>16)&0xff, fgc=(fg>>8)&0xff, fb=fg&0xff;
		if (opaque) {              /* fill the text cell with the background first */
			int w = ttf_width(s, n, pixelH);
			for (int yy = 0; yy < pixelH; yy++) for (int xx = 0; xx < w; xx++) putpx(dc, x+xx, y+yy, bg);
		}
		float penx = (float)x;
		for (int i = 0; i < n; i++) {
			unsigned char c = (unsigned char)s[i];
			int x0,y0,x1,y1;
			stbtt_GetCodepointBitmapBox(&g_ttf, c, scale, scale, &x0,&y0,&x1,&y1);
			int gw=x1-x0, gh=y1-y0;
			if (gw > 0 && gh > 0) {
				unsigned char* glyph = (unsigned char*)malloc((size_t)gw*gh);
				if (glyph) {
					stbtt_MakeCodepointBitmap(&g_ttf, glyph, gw, gh, gw, scale, scale, c);
					int gox=(int)penx+x0, goy=baseline+y0;
					for (int gy = 0; gy < gh; gy++) for (int gx = 0; gx < gw; gx++) {
						int a = glyph[gy*gw+gx]; if (a) blendpx(dc, gox+gx, goy+gy, fr, fgc, fb, a);
					}
					free(glyph);
				}
			}
			int aw; stbtt_GetCodepointHMetrics(&g_ttf, c, &aw, NULL); penx += aw*scale;
			if (i+1 < n) penx += stbtt_GetCodepointKernAdvance(&g_ttf, c, (unsigned char)s[i+1])*scale;
		}
		return;
	}
	/* fallback: built-in 8x8 bitmap font */
	int cw = f->cw, ch = f->ch;
	for (int i = 0; i < n; i++) {
		unsigned char c = (unsigned char)s[i];
		int gx = x + i * cw;
		const unsigned char* glyph = (c >= 0x20 && c <= 0x7E) ? FONT8X8[c - 0x20] : FONT8X8[0];
		for (int py = 0; py < ch; py++) {
			int srow = py * 8 / ch;
			unsigned char bits = glyph[srow];
			int dyy = y + py;
			for (int px = 0; px < cw; px++) {
				int scol = px * 8 / cw;
				int on = (bits >> scol) & 1;
				if (on) putpx(dc, gx + px, dyy, fg);
				else if (opaque) putpx(dc, gx + px, dyy, bg);
			}
		}
	}
}

void ma_gdi_get_text_metrics(void* hdc, void* tmv) {
	MaDC* dc = resolve(hdc); MaFont* f = dc_font(dc);
	if (!tmv) return;
	/* TEXTMETRIC fields are LONG, in order: tmHeight, tmAscent, tmDescent,
	   tmInternalLeading, tmExternalLeading, tmAveCharWidth, tmMaxCharWidth, ... */
	long* tm = (long*)tmv;
	if (ttf_load()) {
		int pixelH = f->ch > 0 ? f->ch : 12;
		float scale = stbtt_ScaleForPixelHeight(&g_ttf, (float)pixelH);
		int ascent, descent, linegap; stbtt_GetFontVMetrics(&g_ttf, &ascent, &descent, &linegap);
		int aw; stbtt_GetCodepointHMetrics(&g_ttf, 'x', &aw, NULL);
		tm[0] = pixelH;                                    /* tmHeight */
		tm[1] = (long)(ascent * scale + 0.5f);             /* tmAscent */
		tm[2] = (long)(-descent * scale + 0.5f);           /* tmDescent */
		tm[3] = 0;
		tm[4] = (long)(linegap * scale + 0.5f);            /* tmExternalLeading */
		tm[5] = (long)(aw * scale + 0.5f);                 /* tmAveCharWidth */
		tm[6] = (long)(aw * scale + 0.5f) * 2;             /* tmMaxCharWidth (approx) */
		return;
	}
	tm[0] = f->ch;                 /* tmHeight */
	tm[1] = (f->ch * 4) / 5;       /* tmAscent */
	tm[2] = f->ch - tm[1];         /* tmDescent */
	tm[3] = 0;                     /* tmInternalLeading */
	tm[4] = 0;                     /* tmExternalLeading */
	tm[5] = f->cw;                 /* tmAveCharWidth */
	tm[6] = f->cw;                 /* tmMaxCharWidth */
}

void ma_gdi_get_text_extent(void* hdc, const char* s, int n, int* cx, int* cy) {
	MaDC* dc = resolve(hdc); MaFont* f = dc_font(dc);
	int pixelH = f->ch > 0 ? f->ch : 12;
	if (cx) *cx = (ttf_load() && s) ? ttf_width(s, n, pixelH) : n * f->cw;
	if (cy) *cy = f->ch;
}

void ma_gdi_present_screen(void) {
	if (g_canvas && g_cw > 0 && g_ch > 0) ma_gl_blit_bgra(g_canvas, g_cw, g_ch);
}

/* Clear the screen canvas to opaque black. Used on the map->panel transition: the campaign
   map fills the whole canvas, and a panel whose art doesn't cover it (e.g. singlefrag) would
   otherwise let the map show through. */
void ma_gdi_clear_screen(void) {
	if (g_canvas && g_cw > 0 && g_ch > 0)
		for (int i = 0; i < g_cw * g_ch; i++) g_canvas[i] = 0xFF000000u;
}

/* expose the screen canvas (for the frame-dump path / debugging) */
const void* ma_gdi_canvas(int* w, int* h) { if (w) *w = g_cw; if (h) *h = g_ch; return g_canvas; }

/* S58 (MA_SHOT): one-shot canvas dump to a named P6 PPM -- the GL-free capture path
 * (BoB BOB_SHOT / bob_gdi_dump_to recipe). Reads the BGRA screen canvas directly, so it
 * works under SDL_VIDEODRIVER=dummy with no window/GL context at all. Returns the
 * nonblack pixel count (capture sanity metric), -1 if no canvas exists yet. */
int ma_gdi_dump_to(const char* path) {
	if (!g_canvas || g_cw <= 0 || g_ch <= 0) return -1;
	int nz = 0;
	for (size_t i = 0; i < (size_t)g_cw * g_ch; i++) if (g_canvas[i] & 0xFFFFFF) nz++;
	/* raw POSIX open() to bypass the game's redirected fopen (same as the GL dump path) */
	int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) { fprintf(stderr, "[shot] ma_gdi_dump_to: cannot open %s\n", path); return -1; }
	char hdr[64]; int n = snprintf(hdr, sizeof(hdr), "P6\n%d %d\n255\n", g_cw, g_ch);
	if (write(fd, hdr, n) < 0) {}
	for (size_t i = 0; i < (size_t)g_cw * g_ch; i++) {
		u32 p = g_canvas[i];
		unsigned char rgb[3] = { (unsigned char)(p >> 16), (unsigned char)(p >> 8), (unsigned char)p };
		if (write(fd, rgb, 3) < 0) {}
	}
	close(fd);
	fprintf(stderr, "[shot] canvas %dx%d nonblack=%d/%lu -> %s\n",
		g_cw, g_ch, nz, (unsigned long)((size_t)g_cw * g_ch), path);
	return nz;
}

} /* extern "C" */
