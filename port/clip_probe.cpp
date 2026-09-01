/* port/clip_probe.cpp -- PO-77 (S401): does ETO_CLIPPED actually clip, and does an EMPTY rect
 * correctly mean "do not clip"?
 *
 * Headless on purpose. S399 wired ETO_CLIPPED into 40 call sites across 10 live files, and S400
 * found that a degenerate rect would have blanked a control's text entirely. Neither could be
 * checked without the display, because parity_2d needs a real screen -- but the clip itself is
 * pure software: ma_gdi renders into a bitmap we can read back. So test the exact chain
 * ExtTextOutA now runs (set_clip_logical -> text_out -> restore_clip) against a pixel buffer.
 *
 * Links the SHIPPED ma_gdi object, not a copy of it.
 */
#include <stdio.h>
#include <string.h>

extern "C" {
    void* ma_gdi_create_dc(void);
    void  ma_gdi_delete_dc(void*);
    void* ma_gdi_create_bitmap(int w, int h);
    void  ma_gdi_delete_bitmap(void*);
    void* ma_gdi_select_bitmap(void* hdc, void* hbmp);
    void  ma_gdi_text_out(void* hdc, int x, int y, const char* s, int n);
    void  ma_gdi_set_clip_logical(void*, int, int, int, int, int*);
    void  ma_gdi_restore_clip(void*, const int*);
    void  ma_gdi_set_text_color(void* hdc, unsigned c);
    void  ma_gdi_set_bk_mode(void* hdc, int mode);
    void  ma_gdi_bitblt(void* hdst, int dx, int dy, int w, int h, void* hsrc, int sx, int sy, unsigned rop);
    void  ma_gdi_fill_rect(void*, int, int, int, int, unsigned);
}

/* Stubs for the few externs ma_gdi.cpp references on paths this probe never runs (screen
   presentation and icon loading). Linking the SHIPPED object matters more than linking a small
   one, so satisfy the linker rather than compile a trimmed copy that could drift from it. */
extern "C" {
    void ma_gl_blit_bgra(const void*, int, int) {}
    void* bob_LoadLibrary(const char*) { return 0; }
    void* bob_GetProcAddress(void*, const char*) { return 0; }
    int   bob_FreeLibrary(void*) { return 0; }
    const void* bob_res_get(int, int, unsigned*) { return 0; }
}

static const int W = 400, H = 60;

/* count pixels that differ from the (zeroed) background, split by a vertical cut at `cut` */
static void count(unsigned* px, int cut, long* left, long* right) {
    *left = *right = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (px[y * W + x] & 0x00FFFFFFu) { if (x < cut) (*left)++; else (*right)++; }
}

int main(void) {
    int fails = 0;
    void* dc = ma_gdi_create_dc();
    void* bm = ma_gdi_create_bitmap(W, H);
    ma_gdi_select_bitmap(dc, bm);
    ma_gdi_set_bk_mode(dc, 1 /*TRANSPARENT*/);
    ma_gdi_set_text_color(dc, 0x00FFFFFFu);
    /* The DC writes into the bitmap's pixel array (ma_gdi_select_bitmap hands the DC b->px), and
       MaBitmap's layout starts w, h, px -- so read it back through a matching view rather than
       adding an accessor to shipping code for a probe's benefit. */
    struct MaBitmapLite { int w, h; unsigned* px; };
    unsigned* px = ((MaBitmapLite*)bm)->px;

    const char* msg = "CLIPPING TEST STRING WIDE ENOUGH TO CROSS THE CUT";
    const int CUT = 150;

    /* 1. UNCLIPPED: text must appear on both sides of the cut. */
    memset(px, 0, (size_t)W * H * 4);
    long l0, r0;
    ma_gdi_text_out(dc, 4, 20, msg, (int)strlen(msg));
    count(px, CUT, &l0, &r0);
    printf("  unclipped:            left=%ld right=%ld\n", l0, r0);
    if (!(l0 > 0 && r0 > 0)) { printf("  FAIL: the unclipped draw did not cross the cut -- test is invalid\n"); fails++; }

    /* 2. CLIPPED to the left of the cut: nothing may appear to the right. */
    memset(px, 0, (size_t)W * H * 4);
    int saved[5];
    ma_gdi_set_clip_logical(dc, 0, 0, CUT, H, saved);
    ma_gdi_text_out(dc, 4, 20, msg, (int)strlen(msg));
    ma_gdi_restore_clip(dc, saved);
    long l1, r1; count(px, CUT, &l1, &r1);
    printf("  ETO_CLIPPED to x<%d:   left=%ld right=%ld\n", CUT, l1, r1);
    if (r1 != 0)      { printf("  FAIL: text drew OUTSIDE the clip rect\n"); fails++; }
    if (l1 == 0)      { printf("  FAIL: text vanished INSIDE the clip rect\n"); fails++; }

    /* 3. EMPTY rect must mean NO CLIP, not "clip everything" (S400). */
    memset(px, 0, (size_t)W * H * 4);
    ma_gdi_set_clip_logical(dc, 0, 0, 0, 0, saved);
    ma_gdi_text_out(dc, 4, 20, msg, (int)strlen(msg));
    ma_gdi_restore_clip(dc, saved);
    long l2, r2; count(px, CUT, &l2, &r2);
    printf("  empty rect (0,0,0,0): left=%ld right=%ld\n", l2, r2);
    if (l2 + r2 == 0) { printf("  FAIL: an empty rect blanked the text -- S400's regression\n"); fails++; }

    /* 4. the clip must be RESTORED: a later unclipped draw is unaffected. */
    memset(px, 0, (size_t)W * H * 4);
    ma_gdi_set_clip_logical(dc, 0, 0, CUT, H, saved);
    ma_gdi_text_out(dc, 4, 20, msg, (int)strlen(msg));
    ma_gdi_restore_clip(dc, saved);
    memset(px, 0, (size_t)W * H * 4);
    ma_gdi_text_out(dc, 4, 20, msg, (int)strlen(msg));
    long l3, r3; count(px, CUT, &l3, &r3);
    printf("  after restore:        left=%ld right=%ld\n", l3, r3);
    if (r3 == 0)      { printf("  FAIL: the clip leaked past its restore\n"); fails++; }

    /* 5. THE OTHER WRITERS. S401 fixed blendpx (TTF glyphs) after it was found not to clip, and an
       audit then showed ma_gdi.cpp has FOUR canvas writers -- putpx, blendpx, bitblt, stretchblt --
       plus icons, which go through putpx. Text alone would not have caught blendpx; nor would it
       catch a blit that ignores the clip. Cover a second writer here so the audit is enforced
       rather than merely recorded. */
    {
        void* srcbm = ma_gdi_create_bitmap(W, H);
        void* srcdc = ma_gdi_create_dc();
        ma_gdi_select_bitmap(srcdc, srcbm);
        struct MaBitmapLite2 { int w, h; unsigned* px; };
        unsigned* spx = ((MaBitmapLite2*)srcbm)->px;
        for (int i = 0; i < W * H; i++) spx[i] = 0x00FFFFFFu;      /* solid source */

        memset(px, 0, (size_t)W * H * 4);
        ma_gdi_set_clip_logical(dc, 0, 0, CUT, H, saved);
        ma_gdi_bitblt(dc, 0, 0, W, H, srcdc, 0, 0, 0x00CC0020u /*SRCCOPY*/);
        ma_gdi_restore_clip(dc, saved);
        long l4, r4; count(px, CUT, &l4, &r4);
        printf("  BITBLT clipped x<%d:  left=%ld right=%ld\n", CUT, l4, r4);
        if (r4 != 0) { printf("  FAIL: bitblt drew OUTSIDE the clip rect\n"); fails++; }
        if (l4 == 0) { printf("  FAIL: bitblt drew nothing INSIDE the clip rect\n"); fails++; }

        ma_gdi_delete_dc(srcdc); ma_gdi_delete_bitmap(srcbm);
    }

    ma_gdi_delete_bitmap(bm); ma_gdi_delete_dc(dc);
    printf(fails == 0 ? "\nPASS: text and blits clip, an empty rect does not, and the clip restores\n"
                      : "\nFAIL (%d)\n", fails);
    return fails == 0 ? 0 : 1;
}
