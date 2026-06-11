/*
 * FreeFalcon Linux Port - wingdi.h compatibility
 *
 * GDI types and stub functions.
 */

#ifndef FF_COMPAT_WINGDI_H
#define FF_COMPAT_WINGDI_H

#ifdef FF_LINUX

#include "compat_types.h"

/* ============================================================
 * Color helpers
 * ============================================================ */
#ifndef RGB
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g)) << 8) | ((DWORD)((BYTE)(b)) << 16)))
#endif
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))
#define PALETTERGB(r, g, b)   (0x02000000 | RGB(r, g, b))
#define PALETTEINDEX(i)       ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))

/* ============================================================
 * Palette
 * ============================================================ */
typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;

#define PC_RESERVED   0x01
#define PC_EXPLICIT   0x02
#define PC_NOCOLLAPSE 0x04

typedef struct tagLOGPALETTE {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, *LPLOGPALETTE;

/* ============================================================
 * Bitmaps
 * ============================================================ */
typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagRGBTRIPLE {
    BYTE rgbtBlue;
    BYTE rgbtGreen;
    BYTE rgbtRed;
} RGBTRIPLE;

#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER, *LPBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

typedef struct tagBITMAP {
    LONG   bmType;
    LONG   bmWidth;
    LONG   bmHeight;
    LONG   bmWidthBytes;
    WORD   bmPlanes;
    WORD   bmBitsPixel;
    LPVOID bmBits;
} BITMAP, *PBITMAP, *LPBITMAP;

#define BI_RGB       0
#define BI_RLE8      1
#define BI_RLE4      2
#define BI_BITFIELDS 3

#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1

/* ============================================================
 * Fonts / text
 * ============================================================ */
#define LF_FACESIZE 32

typedef struct tagLOGFONTA {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA, *PLOGFONTA, *LPLOGFONTA;
typedef LOGFONTA LOGFONT, *PLOGFONT, *LPLOGFONT;

typedef struct tagTEXTMETRICA {
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
    BYTE tmFirstChar;
    BYTE tmLastChar;
    BYTE tmDefaultChar;
    BYTE tmBreakChar;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
} TEXTMETRICA, *PTEXTMETRICA, *LPTEXTMETRICA;
typedef TEXTMETRICA TEXTMETRIC, *PTEXTMETRIC, *LPTEXTMETRIC;

/* Font weights / charsets */
#define FW_DONTCARE   0
#define FW_THIN       100
#define FW_EXTRALIGHT 200
#define FW_ULTRALIGHT 200
#define FW_LIGHT      300
#define FW_NORMAL     400
#define FW_REGULAR    400
#define FW_MEDIUM     500
#define FW_SEMIBOLD   600
#define FW_DEMIBOLD   600
#define FW_BOLD       700
#define FW_EXTRABOLD  800
#define FW_ULTRABOLD  800
#define FW_HEAVY      900
#define FW_BLACK      900
#define ANSI_CHARSET      0
#define DEFAULT_CHARSET   1
#define SYMBOL_CHARSET    2
#define SHIFTJIS_CHARSET  128
#define HANGEUL_CHARSET   129
#define HANGUL_CHARSET    129
#define JOHAB_CHARSET     130
#define GB2312_CHARSET    134
#define CHINESEBIG5_CHARSET 136
#define GREEK_CHARSET     161
#define TURKISH_CHARSET   162
#define VIETNAMESE_CHARSET 163
#define HEBREW_CHARSET    177
#define ARABIC_CHARSET    178
#define BALTIC_CHARSET    186
#define RUSSIAN_CHARSET   204
#define THAI_CHARSET      222
#define EASTEUROPE_CHARSET 238
#define OEM_CHARSET       255
#define MAC_CHARSET       77
#define OUT_DEFAULT_PRECIS   0
#define OUT_STRING_PRECIS    1
#define OUT_CHARACTER_PRECIS 2
#define OUT_STROKE_PRECIS    3
#define OUT_TT_PRECIS        4
#define OUT_DEVICE_PRECIS    5
#define OUT_RASTER_PRECIS    6
#define OUT_TT_ONLY_PRECIS   7
#define OUT_OUTLINE_PRECIS   8
#define CLIP_DEFAULT_PRECIS  0
#define CLIP_CHARACTER_PRECIS 1
#define CLIP_STROKE_PRECIS   2
#define CLIP_MASK            0xf
#define CLIP_LH_ANGLES       (1 << 4)
#define CLIP_TT_ALWAYS       (2 << 4)
#define CLIP_DFA_DISABLE     (4 << 4)
#define CLIP_EMBEDDED        (8 << 4)
#define DEFAULT_QUALITY    0
#define DRAFT_QUALITY      1
#define PROOF_QUALITY      2
#define NONANTIALIASED_QUALITY 3
#define ANTIALIASED_QUALITY 4
#define DEFAULT_PITCH 0
#define FIXED_PITCH   1
#define VARIABLE_PITCH 2
#define FF_DONTCARE   (0 << 4)
#define FF_ROMAN      (1 << 4)
#define FF_SWISS      (2 << 4)
#define FF_MODERN     (3 << 4)
#define FF_SCRIPT     (4 << 4)
#define FF_DECORATIVE (5 << 4)

/* Text alignment / bk modes */
#define TRANSPARENT 1
#define OPAQUE      2
#define TA_LEFT     0
#define TA_RIGHT    2
#define TA_CENTER   6
#define TA_TOP      0
#define TA_BOTTOM   8
#define TA_NOUPDATECP 0
#define TA_UPDATECP   1
#define TA_BASELINE   24
/* StretchBlt modes */
#define BLACKONWHITE  1
#define WHITEONBLACK  2
#define COLORONCOLOR  3
#define HALFTONE      4
/* Linux port: invoke the enumeration callback once, reporting that the requested
   font family exists. CreatePointFont() (MIG.cpp) loops over '\n'-separated font
   names calling this and only breaks when the callback sets its flag -- a stub
   that never calls back spins forever. The callback only reads lParam, so passing
   the logfont back with NULL metrics is sufficient. */
typedef int (*BOB_FONTENUMPROC)(const void*, const void*, unsigned long, LPARAM);
static inline int EnumFontFamiliesExA(HDC, void* lpLogfont, void* lpProc, LPARAM lParam, DWORD) {
    if (lpProc) ((BOB_FONTENUMPROC)lpProc)(lpLogfont, 0, 0, lParam);
    return 0;
}
#define EnumFontFamiliesEx EnumFontFamiliesExA

/* Stock objects */
#define WHITE_BRUSH   0
#define LTGRAY_BRUSH  1
#define GRAY_BRUSH    2
#define DKGRAY_BRUSH  3
#define BLACK_BRUSH   4
#define NULL_BRUSH    5
#define WHITE_PEN     6
#define BLACK_PEN     7
#define NULL_PEN      8
#define SYSTEM_FONT   13
#define DEFAULT_PALETTE 15

/* ROP codes */
#define SRCCOPY     0x00CC0020
#define SRCPAINT    0x00EE0086
#define SRCAND      0x008800C6
#define SRCINVERT   0x00660046
#define BLACKNESS   0x00000042
#define WHITENESS   0x00FF0062

/* Pen styles */
#ifndef PS_SOLID
#define PS_SOLID        0
#define PS_DASH         1
#define PS_DOT          2
#define PS_DASHDOT      3
#define PS_DASHDOTDOT   4
#define PS_NULL         5
#define PS_INSIDEFRAME  6
#endif
/* ExtCreatePen geometric-pen type/endcap/join styles + hatch styles + LOGBRUSH */
#ifndef PS_GEOMETRIC
#define PS_COSMETIC      0x00000000
#define PS_GEOMETRIC     0x00010000
#define PS_ENDCAP_ROUND  0x00000000
#define PS_ENDCAP_SQUARE 0x00000100
#define PS_ENDCAP_FLAT   0x00000200
#define PS_JOIN_ROUND    0x00000000
#define PS_JOIN_BEVEL    0x00001000
#define PS_JOIN_MITER    0x00002000
#endif
#ifndef HS_HORIZONTAL
#define HS_HORIZONTAL    0
#define HS_VERTICAL      1
#define HS_FDIAGONAL     2
#define HS_BDIAGONAL     3
#define HS_CROSS         4
#define HS_DIAGCROSS     5
#endif
#ifndef _LOGBRUSH_DEFINED
#define _LOGBRUSH_DEFINED
typedef struct tagLOGBRUSH { UINT lbStyle; COLORREF lbColor; LONG lbHatch; }
        LOGBRUSH, *PLOGBRUSH, *LPLOGBRUSH;
#endif
/* SetSystemPaletteUse (full-screen palette ownership) */
#ifndef SYSPAL_STATIC
#define SYSPAL_ERROR    0
#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2
#define SYSPAL_NOSTATIC256 3
static inline UINT SetSystemPaletteUse(HDC, UINT) { return SYSPAL_STATIC; }
#endif
/* GetGlyphOutline glyph-rasterising API (OVERLAY renders overlay text via font
   glyphs). Stubbed for bring-up: returns 0 (no glyph bitmap) -> blank text now;
   a real path can rasterise via FreeType/SDL_ttf later. */
#ifndef GGO_GRAY8_BITMAP
#define GGO_METRICS       0
#define GGO_BITMAP        1
#define GGO_NATIVE        2
#define GGO_GRAY2_BITMAP  4
#define GGO_GRAY4_BITMAP  5
#define GGO_GRAY8_BITMAP  6
#ifndef GDI_ERROR
#define GDI_ERROR ((DWORD)0xFFFFFFFF)
#endif
typedef struct _FIXED { WORD fract; short value; } FIXED;
typedef struct _MAT2 { FIXED eM11, eM12, eM21, eM22; } MAT2, *LPMAT2;
typedef struct _GLYPHMETRICS {
    UINT  gmBlackBoxX, gmBlackBoxY;
    POINT gmptGlyphOrigin;
    short gmCellIncX, gmCellIncY;
} GLYPHMETRICS, *LPGLYPHMETRICS;
static inline DWORD GetGlyphOutlineA(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, const MAT2*) { return 0; }
#define GetGlyphOutline GetGlyphOutlineA
#endif
/* Background / mapping / ROP2 modes */
#ifndef TRANSPARENT
#define TRANSPARENT     1
#define OPAQUE          2
#endif
#ifndef MM_TEXT
#define MM_TEXT         1
#endif
#ifndef R2_COPYPEN
#define R2_BLACK        1
#define R2_NOT          6
#define R2_COPYPEN      13
#define R2_XORPEN       7
#endif
/* GetDeviceCaps indices */
#ifndef HORZRES
#define HORZRES         8
#define VERTRES         10
#define BITSPIXEL       12
#define LOGPIXELSX      88
#define LOGPIXELSY      90
#endif
/* DrawText flags */
#ifndef DT_LEFT
#define DT_LEFT         0x0000
#define DT_CENTER       0x0001
#define DT_RIGHT        0x0002
#define DT_TOP          0x0000
#define DT_VCENTER      0x0004
#define DT_BOTTOM       0x0008
#define DT_WORDBREAK    0x0010
#define DT_SINGLELINE   0x0020
#define DT_NOCLIP       0x0100
#define DT_CALCRECT     0x0400
#endif

/* ============================================================
 * GDI function stubs
 * ============================================================ */
static inline HDC GetDC(HWND hWnd) { (void)hWnd; return NULL; }
static inline int ReleaseDC(HWND hWnd, HDC hDC) { (void)hWnd; (void)hDC; return 1; }
static inline HDC CreateCompatibleDC(HDC hdc) { (void)hdc; return NULL; }
static inline BOOL DeleteDC(HDC hdc) { (void)hdc; return TRUE; }
static inline HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h) { (void)hdc; (void)h; return NULL; }
static inline BOOL DeleteObject(HGDIOBJ h) { (void)h; return TRUE; }
static inline HGDIOBJ GetStockObject(int i) { (void)i; return NULL; }
static inline COLORREF SetPixel(HDC hdc, int x, int y, COLORREF color) { (void)hdc; (void)x; (void)y; return color; }
static inline COLORREF GetPixel(HDC hdc, int x, int y) { (void)hdc; (void)x; (void)y; return 0; }
static inline BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop) {
    (void)hdc; (void)x; (void)y; (void)cx; (void)cy; (void)hdcSrc; (void)x1; (void)y1; (void)rop; return TRUE;
}
static inline int SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, UINT, UINT, const void*, const void*, UINT) { return 0; }
static inline HRGN CreateRectRgn(int, int, int, int) { return NULL; }
static inline HRGN CreatePolygonRgn(const POINT*, int, int) { return NULL; }
static inline HRGN CreateRectRgnIndirect(LPCRECT) { return NULL; }
static inline int  CombineRgn(HRGN, HRGN, HRGN, int) { return 0; }
static inline HBITMAP CreateDIBitmap(HDC, const void*, DWORD, const void*, const void*, UINT) { return NULL; }
static inline HBITMAP CreateDIBSection(HDC, const void*, UINT, void**, HANDLE, DWORD) { return NULL; }
#ifndef CBM_INIT
#define CBM_INIT 0x04
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1
#endif
/* polygon fill modes */
#ifndef ALTERNATE
#define ALTERNATE 1
#define WINDING   2
#endif
static inline int SetStretchBltMode(HDC, int) { return 0; }
static inline int SetPolyFillMode(HDC, int) { return 0; }
static inline int GetDIBits(HDC, HBITMAP, UINT, UINT, void*, void*, UINT) { return 0; }
static inline int SetDIBits(HDC, HBITMAP, UINT, UINT, const void*, const void*, UINT) { return 0; }
static inline BOOL StretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop) {
    (void)hdcDest; (void)xDest; (void)yDest; (void)wDest; (void)hDest; (void)hdcSrc; (void)xSrc; (void)ySrc; (void)wSrc; (void)hSrc; (void)rop; return TRUE;
}
static inline HBITMAP CreateCompatibleBitmap(HDC hdc, int cx, int cy) { (void)hdc; (void)cx; (void)cy; return NULL; }
static inline HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO *pbmi, UINT usage, void **ppvBits, HANDLE hSection, DWORD offset) {
    (void)hdc; (void)pbmi; (void)usage; (void)hSection; (void)offset;
    if (ppvBits) *ppvBits = NULL;
    return NULL;
}
static inline HFONT CreateFontA(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic,
                                DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
                                DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName) {
    (void)cHeight; (void)cWidth; (void)cEscapement; (void)cOrientation; (void)cWeight; (void)bItalic;
    (void)bUnderline; (void)bStrikeOut; (void)iCharSet; (void)iOutPrecision; (void)iClipPrecision;
    (void)iQuality; (void)iPitchAndFamily; (void)pszFaceName;
    return NULL;
}
#define CreateFont CreateFontA
static inline HFONT CreateFontIndirectA(const LOGFONTA *lplf) { (void)lplf; return NULL; }
#define CreateFontIndirect CreateFontIndirectA
static inline BOOL GetTextMetricsA(HDC hdc, LPTEXTMETRICA lptm) {
    (void)hdc; if (lptm) memset(lptm, 0, sizeof(*lptm)); return FALSE;
}
#define GetTextMetrics GetTextMetricsA
static inline BOOL TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c) {
    (void)hdc; (void)x; (void)y; (void)lpString; (void)c; return TRUE;
}
#define TextOut TextOutA
static inline COLORREF SetTextColor(HDC hdc, COLORREF color) { (void)hdc; return color; }
static inline COLORREF SetBkColor(HDC hdc, COLORREF color) { (void)hdc; return color; }
static inline int SetBkMode(HDC hdc, int mode) { (void)hdc; (void)mode; return 0; }
static inline BOOL GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int c, LPSIZE psizl) {
    (void)hdc; (void)lpString;
    if (psizl) { psizl->cx = c * 8; psizl->cy = 16; }
    return TRUE;
}
#define GetTextExtentPoint32 GetTextExtentPoint32A
static inline HPALETTE CreatePalette(const LOGPALETTE *plpal) { (void)plpal; return NULL; }
static inline HBRUSH CreateSolidBrush(COLORREF color) { (void)color; return NULL; }
static inline HPEN CreatePen(int iStyle, int cWidth, COLORREF color) { (void)iStyle; (void)cWidth; (void)color; return NULL; }
static inline BOOL Rectangle(HDC hdc, int left, int top, int right, int bottom) {
    (void)hdc; (void)left; (void)top; (void)right; (void)bottom; return TRUE;
}
static inline BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT lppt) { (void)hdc; (void)x; (void)y; (void)lppt; return TRUE; }
static inline BOOL LineTo(HDC hdc, int x, int y) { (void)hdc; (void)x; (void)y; return TRUE; }
static inline int GetDeviceCaps(HDC hdc, int index) { (void)hdc; (void)index; return 0; }
static inline int GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage) {
    (void)hdc; (void)hbm; (void)start; (void)cLines; (void)lpvBits; (void)lpbmi; (void)usage; return 0;
}
static inline int SetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, const void *lpBits, const BITMAPINFO *lpbmi, UINT colorUse) {
    (void)hdc; (void)hbm; (void)start; (void)cLines; (void)lpBits; (void)lpbmi; (void)colorUse; return 0;
}

/* GetDeviceCaps indices */
#define HORZRES     8
#define VERTRES     10
#define BITSPIXEL   12
#define PLANES      14
#define RASTERCAPS  38
#define RC_PALETTE  0x0100
#define SIZEPALETTE 104
#define NUMRESERVED 106

#endif /* FF_LINUX */
#endif /* FF_COMPAT_WINGDI_H */
