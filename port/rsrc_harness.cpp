/* port/rsrc_harness.cpp — headless harness (S57): exercise the PRODUCTION
 * bob_resources.cpp + ma_dlgtmpl.cpp against the installed build's PE modules
 * (miglang.dll / Mig.exe) with no GL. This is the sprint-57 DoD evidence path
 * for the BoB-note-14 PE parity-oracle layer while GLX was wedged.
 *
 * Build (native, no -m32 needed — the parsers are offset-based):
 *   g++ -O1 -DFF_LINUX '-DMA_SRC_DIR="/home/admin/ma/SRC"' -o /tmp/rsrc_harness \
 *       port/rsrc_harness.cpp SRC/compat/bob_resources.cpp SRC/compat/ma_dlgtmpl.cpp
 * Run:
 *   MIG=/home/admin/sgl/TUE/MigAlley/WP/drive_c/rowan/mig
 *   /tmp/rsrc_harness "$MIG/English/TEXT/miglang.dll" "$MIG/Mig.exe" 271 958
 *   MA_NO_PE_RSRC=1 ... reruns the S56-baseline behaviour (A/B).
 * Usage: rsrc_harness <miglang.dll> <Mig.exe|-> <idd> [idd...]
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" void* bob_LoadLibrary(const char* path);
extern "C" void  bob_SetResourceHandle(void* h);
extern "C" void  bob_set_main_module(void* h);
extern "C" const void* bob_res_get(void* h, unsigned type, unsigned id, unsigned* outSize);
extern "C" int   bob_load_string(void* h, unsigned id, char* buf, int maxlen);
extern "C" void  ma_dlg_load_template(unsigned idd, void* dlg);
extern "C" int   ma_dlg_rect(void* dlg, int id, int* x, int* y, int* w, int* h);
extern "C" int   ma_dlg_label(void* dlg, int id, char* out, int outsz);
extern "C" int   ma_dlg_in_template(void* dlg, int id) __attribute__((weak));
extern "C" int   ma_dlg_enum_statics(void* dlg, int* ids, int maxn) __attribute__((weak));
extern "C" int   ma_dlg_artnum(void* dlg, int id, long* outFn) __attribute__((weak));

/* fopen_nocase stub for the harness (paths given verbatim) */
extern "C" FILE* fopen_nocase(const char* path, const char* mode) { return fopen(path, mode); }

/* enumerators (may not exist pre-S57; declared weak so baseline links) */
extern "C" int bob_res_enum_dialog_items(
        void (*itemcb)(void*, int, int, int, int, int, int, const char*), void*)
        __attribute__((weak));
extern "C" int bob_res_enum_dlginit(
        void (*initcb)(void*, int, int, const unsigned char*, int), void*)
        __attribute__((weak));

static int g_items = 0, g_lastdlg = -1, g_dialogs = 0;
static void itemcb(void*, int dlgId, int ctrlId, int x, int y, int w, int h, const char* cls) {
    g_items++;
    if (dlgId != g_lastdlg) { g_dialogs++; g_lastdlg = dlgId; }
    if (getenv("DUMP_ITEMS")) {
        int want = atoi(getenv("DUMP_ITEMS"));
        if (!want || want == dlgId)
            printf("  [enum] dlg=%d ctrl=%d dlu=(%d,%d %dx%d) cls=%s\n", dlgId, ctrlId, x, y, w, h, cls);
    }
}
static int g_inits = 0;
static void initcb(void*, int dlgId, int ctrlId, const unsigned char*, int len) {
    g_inits++;
    if (getenv("DUMP_INITS")) {
        int want = atoi(getenv("DUMP_INITS"));
        if (!want || want == dlgId)
            printf("  [dlginit] dlg=%d ctrl=%d len=%d\n", dlgId, ctrlId, len);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s miglang.dll Mig.exe|- idd...\n", argv[0]); return 2; }
    void* lang = bob_LoadLibrary(argv[1]);
    printf("miglang: %s\n", lang ? "LOADED" : "FAILED");
    if (!lang) return 1;
    bob_SetResourceHandle(lang);
    if (strcmp(argv[2], "-") != 0) {
        void* exe = bob_LoadLibrary(argv[2]);
        printf("Mig.exe: %s\n", exe ? "LOADED" : "FAILED");
        if (exe) bob_set_main_module(exe);
    }
    if (bob_res_enum_dialog_items) {
        g_items = 0; g_lastdlg = -1; g_dialogs = 0;
        bob_res_enum_dialog_items(itemcb, 0);
        bob_res_enum_dlginit(initcb, 0);
        printf("enumerators: %d dialogs, %d dialog items, %d DLGINIT records\n",
               g_dialogs, g_items, g_inits);
    } else {
        printf("enumerators: not present (baseline)\n");
    }
    /* string-table smoke test */
    { char s[128]; int n = bob_load_string(0, 1, s, sizeof s);
      printf("string id 1: %s \"%s\"\n", n ? "ok" : "none", s); }

    for (int a = 3; a < argc; a++) {
        unsigned idd = (unsigned)atoi(argv[a]);
        unsigned sz = 0;
        const void* p = bob_res_get(0, 5, idd, &sz);
        printf("\n=== IDD %u: RT_DIALOG %s (%u bytes) ===\n", idd, p ? "found" : "MISSING", sz);
        void* key = (void*)(size_t)(0x1000 + idd);
        ma_dlg_load_template(idd, key);
        int nrect = 0, nlab = 0;
        for (int id = 0; id < 65536; id++) {
            int x, y, w, h; char lbl[128]; long fn = 0;
            int hr = ma_dlg_rect(key, id, &x, &y, &w, &h);
            int hl = ma_dlg_label(key, id, lbl, sizeof lbl);
            int ha = ma_dlg_artnum ? ma_dlg_artnum(key, id, &fn) : 0;
            if (hr || hl) {
                printf("  id=%-5d rect=%s", id, hr ? "" : "MISSING        ");
                if (hr) printf("(%4d,%4d %4dx%3d)", x, y, w, h);
                printf("  label=%s", hl ? lbl : "-");
                if (ha) printf("  art=0x%lx", fn);
                printf("\n");
                if (hr) nrect++; if (hl) nlab++;
            }
        }
        printf("  totals: %d rects, %d labels\n", nrect, nlab);
        if (ma_dlg_enum_statics) {
            int sids[160]; int ns = ma_dlg_enum_statics(key, sids, 160);
            printf("  template statics (%d):", ns);
            for (int i = 0; i < ns; i++) printf(" %d", sids[i]);
            printf("\n");
        }
        if (ma_dlg_in_template)
            printf("  in_template: known-id=%d absent-id(9999)=%d unknown-dlg=%d\n",
                   ma_dlg_in_template(key, 2024), ma_dlg_in_template(key, 9999),
                   ma_dlg_in_template((void*)0xdead, 2024));
    }
    return 0;
}
