/* ma_dlgtmpl.cpp — RT_DIALOG template parser for the Linux port.
 *
 * Real MFC's CDialog::Create(IDD) loads the dialog's RT_DIALOG template from the module
 * and creates+positions each control. Our compat CDialog::Create is otherwise a no-op, so
 * the OCX controls have no position. This parses the standard DLGTEMPLATE (from miglang.dll
 * via bob_res_get) and records each control's client-relative rect (dialog units -> pixels)
 * keyed by (dialog, control-id); DDX_Control then MoveWindow's the control into place.
 *
 * Only the classic (non-extended) DLGTEMPLATE is handled — that's what Mig Alley's dialogs
 * use (IDD_CREDITS etc.: dlgVer/sig != 1/0xFFFF). */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <map>
#include <string>
#include <utility>

extern "C" {
const void* bob_res_get(void* h, unsigned type, unsigned id, unsigned* outSize);
void  ma_dlg_load_template(unsigned idd, void* dlg);
int   ma_dlg_rect(void* dlg, int id, int* x, int* y, int* w, int* h);
int   ma_dlg_label(void* dlg, int id, char* out, int outsz);
}

struct Rect4 { int x, y, w, h; };
typedef unsigned char u8;   /* (also defined below for the RT_DIALOG parser; same type, legal) */
static std::map<std::pair<void*, int>, std::string>& labelmap() {
    static std::map<std::pair<void*, int>, std::string> m; return m;
}

/* Parse the dialog's RT_DLGINIT (240) resource and record each control's label text. Each entry
   is { WORD id; WORD msg(0x0376); DWORD size; BYTE data[size] } where data = a license string
   (DWORD wchar-count + UTF-16) followed by the control's persisted properties. Rowan's RStatic
   labels persist two ANSI length-prefixed strings: "IDS_<symbol>" then the human label text
   (e.g. "Display Driver:"). We extract the non-IDS_ string and key it by (dlg,id). */
static void parse_dlginit(unsigned idd, void* dlg) {
    unsigned sz = 0;
    const u8* d = (const u8*)bob_res_get(0, 240 /*RT_DLGINIT*/, idd, &sz);
    if (!d || !sz) return;
    unsigned p = 0;
    while (p + 8 <= sz) {
        unsigned id   = (unsigned)(d[p] | (d[p+1] << 8));
        unsigned size = (unsigned)(d[p+4] | (d[p+5]<<8) | (d[p+6]<<16) | ((unsigned)d[p+7]<<24));
        const u8* data = d + p + 8;
        if (p + 8 + size > sz) break;
        p += 8 + size;
        /* skip the license string: DWORD wchar-count + count*2 bytes */
        unsigned off = 0;
        if (size >= 4) { unsigned slen = data[0] | (data[1]<<8) | (data[2]<<16) | ((unsigned)data[3]<<24);
                         off = 4 + (slen > size ? 0 : slen * 2); }
        /* scan the remaining bytes for length-prefixed ANSI strings; keep the last non-"IDS_" one */
        std::string label;
        for (unsigned i = off; i < size; ) {
            unsigned L = data[i];
            if (L >= 1 && L <= 64 && i + 1 + L <= size) {
                int printable = 1;
                for (unsigned k = 0; k < L; k++) { u8 c = data[i+1+k]; if (c < 32 || c >= 127) { printable = 0; break; } }
                if (printable) { std::string s((const char*)data + i + 1, L);
                                 if (s.compare(0, 4, "IDS_") != 0) label = s;   /* the human label */
                                 i += 1 + L; continue; }
            }
            i++;
        }
        if (!label.empty()) labelmap()[std::make_pair(dlg, (int)id)] = label;
    }
}

extern "C" int ma_dlg_label(void* dlg, int id, char* out, int outsz) {
    std::map<std::pair<void*, int>, std::string>& m = labelmap();
    std::map<std::pair<void*, int>, std::string>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end() || !out || outsz <= 0) return 0;
    strncpy(out, it->second.c_str(), outsz - 1); out[outsz - 1] = 0;
    return 1;
}
static std::map<std::pair<void*, int>, Rect4>& dlgmap() {
    static std::map<std::pair<void*, int>, Rect4> m; return m;
}

/* Dialog-base-units for the template font (MS Sans Serif 8pt ~ 6x13). MapDialogRect:
   px_x = dlu_x * baseX / 4, px_y = dlu_y * baseY / 8. */
enum { DLG_BASE_X = 6, DLG_BASE_Y = 13 };
static inline int dlu_x(int v) { return v * DLG_BASE_X / 4; }
static inline int dlu_y(int v) { return v * DLG_BASE_Y / 8; }

typedef unsigned char u8;
static inline unsigned short rd16(const u8* p) { return (unsigned short)(p[0] | (p[1] << 8)); }
static inline unsigned rd32(const u8* p) { return (unsigned)(p[0] | (p[1]<<8) | (p[2]<<16) | ((unsigned)p[3]<<24)); }

/* advance past a sz_Or_Ord field (0x0000 = none, 0xFFFF + WORD ordinal, else WCHAR* nul-term) */
static const u8* skip_sz_or_ord(const u8* p, const u8* end) {
    if (p + 2 > end) return end;
    unsigned short w = rd16(p);
    if (w == 0x0000) return p + 2;
    if (w == 0xFFFF) return p + 4;                 /* ordinal */
    while (p + 2 <= end && rd16(p) != 0) p += 2;    /* unicode string */
    return p + 2;                                   /* skip terminator */
}
static const u8* align4(const u8* base, const u8* p) {
    size_t off = (size_t)(p - base);
    off = (off + 3) & ~((size_t)3);
    return base + off;
}

extern "C" void ma_dlg_load_template(unsigned idd, void* dlg) {
    if (getenv("MA_TRACE_DLGINIT")) {
        unsigned isz = 0; const u8* di = (const u8*)bob_res_get(0, 240 /*RT_DLGINIT*/, idd, &isz);
        fprintf(stderr, "[dlginit] idd=%u DLGINIT %s sz=%u: ", idd, di?"FOUND":"none", isz);
        for (unsigned j=0; di && j<48 && j<isz; j++) fprintf(stderr, "%02x ", di[j]);
        fprintf(stderr, "\n");
        const char* want = getenv("MA_DUMP_DLGINIT");
        if (want && di && idd == (unsigned)atoi(want)) {
            FILE* f = fopen("/tmp/dlginit.bin", "wb"); if (f) { fwrite(di, 1, isz, f); fclose(f);
                fprintf(stderr, "[dlginit] wrote %u bytes for idd=%u to /tmp/dlginit.bin\n", isz, idd); }
        }
    }
    unsigned sz = 0;
    const u8* d = (const u8*)bob_res_get(0, 5 /*RT_DIALOG*/, idd, &sz);
    int trace = getenv("MA_TRACE_DLG") ? 1 : 0;
    if (!d || sz < 18) { if (trace) fprintf(stderr, "[dlg] IDD %u dlg=%p NOT FOUND in resource module (sz=%u)\n", idd, dlg, sz); return; }
    const u8* end = d + sz;

    unsigned style = rd32(d);
    /* extended template (DLGTEMPLATEEX) starts with dlgVer=1, signature=0xFFFF — not used here */
    if (rd16(d) == 1 && rd16(d + 2) == 0xFFFF) { if (trace) fprintf(stderr, "[dlg] %u: extended template (unsupported)\n", idd); return; }
    int cdit = rd16(d + 8);
    const u8* p = d + 18;                          /* after style,exstyle,cdit,x,y,cx,cy */
    p = skip_sz_or_ord(p, end);                    /* menu */
    p = skip_sz_or_ord(p, end);                    /* window class */
    p = skip_sz_or_ord(p, end);                    /* title */
    if (style & 0x40 /*DS_SETFONT*/) {
        p += 2;                                    /* point size */
        while (p + 2 <= end && rd16(p) != 0) p += 2;
        p += 2;                                    /* font name terminator */
    }
    if (trace) fprintf(stderr, "[dlg] IDD %u: %d items (sz=%u)\n", idd, cdit, sz);
    for (int i = 0; i < cdit && p < end; i++) {
        p = align4(d, p);
        if (p + 18 > end) break;
        short x = (short)rd16(p + 8), y = (short)rd16(p + 10);
        short cx = (short)rd16(p + 12), cy = (short)rd16(p + 14);
        unsigned short id = rd16(p + 16);
        p += 18;
        const u8* clsp = p;
        p = skip_sz_or_ord(p, end);                /* class */
        const u8* titlep = p;
        p = skip_sz_or_ord(p, end);                /* title */
        /* creation data: WORD byte-count (0 = none), then that many bytes */
        unsigned short cd = 0; const u8* cdp = 0;
        if (p + 2 <= end) { cd = rd16(p); p += 2; cdp = p; if (cd) p += cd; }
        if (getenv("MA_TRACE_DLGCTL")) {
            char cls[64]={0}, ttl[128]={0}; int k=0;
            const u8* cp=clsp; if(rd16(cp)!=0xFFFF){ while(rd16(cp)!=0 && k<63){cls[k++]=(char)cp[0];cp+=2;} } else { cls[0]='#'; }
            k=0; const u8* tp=titlep; if(rd16(tp)!=0xFFFF){ while(rd16(tp)!=0 && k<127){ttl[k++]=(char)tp[0];tp+=2;} } else { ttl[0]='#'; }
            fprintf(stderr,"[dlgctl] id=%u class=\"%s\" title=\"%s\" cdlen=%u cd[0..7]=", id, cls, ttl, cd);
            for(unsigned j=0;j<cd&&j<8;j++) fprintf(stderr,"%02x ", cdp[j]);
            fprintf(stderr,"\n");
        }
        Rect4 r; r.x = dlu_x(x); r.y = dlu_y(y); r.w = dlu_x(cx); r.h = dlu_y(cy);
        dlgmap()[std::make_pair(dlg, (int)id)] = r;
        if (trace) fprintf(stderr, "[dlg]   id=%u dlu(%d,%d,%d,%d) -> px(%d,%d,%d,%d)\n",
                           id, x, y, cx, cy, r.x, r.y, r.w, r.h);
    }
    parse_dlginit(idd, dlg);     /* also record per-control label text from RT_DLGINIT */
}

extern "C" int ma_dlg_rect(void* dlg, int id, int* x, int* y, int* w, int* h) {
    std::map<std::pair<void*, int>, Rect4>& m = dlgmap();
    std::map<std::pair<void*, int>, Rect4>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return 0;
    if (x) *x = it->second.x; if (y) *y = it->second.y;
    if (w) *w = it->second.w; if (h) *h = it->second.h;
    return 1;
}
