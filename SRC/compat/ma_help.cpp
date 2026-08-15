/* S112 (PO-10): the documentation the "?" button is supposed to open.
 *
 * The game ships its help as a WinHelp 4 file, English/TEXT/MIG.HLP. S98 fixed the four broken
 * links in the chain that gets a "?" click to CMainFrame::OnCommandHelp, and S99 decoded four of
 * the five stages needed to read a topic's TEXT -- container, LZ77, phrase table and the topic link
 * chain -- leaving the fifth (the Hall opcode table for the phrase-compressed text stream)
 * unsolved. S112 searched 800 candidate opcode layouts against S99's title oracle (a correctly
 * decoded topic contains its own |TTLBTREE title) and the best scored 2/39, so the layout is not in
 * that family: still unsolved, and still not something to guess at, because a wrong decoder emits
 * real dictionary words in the wrong order.
 *
 * What IS verified is the topic INDEX: |TTLBTREE holds 44 topic titles -- "Map Screen", "Main
 * Toolbar", "Filter Toolbar", "Bases", "Dossier", "Squadron Information", "Weather", "Target
 * List"... -- exactly the screens the play-tester was pressing "?" on. This file reads that index at
 * runtime so the "?" can show what the game documents, and say plainly that the body text is not
 * decoded yet. A "?" that shows the topic list is honest; a "?" that shows plausible nonsense is
 * worse than one that does nothing (S99's rule).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MA_HELP_MAX 64

static char  g_titles[MA_HELP_MAX][64];
static int   g_count = -1;      /* -1 = not parsed yet, 0 = parsed and empty */

static unsigned short rd16(const unsigned char* p) { return (unsigned short)(p[0] | (p[1] << 8)); }
static int            rd32(const unsigned char* p) { return (int)(p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24)); }

/* Walk a WinHelp B-tree's root page, which for a file this small holds every leaf entry.
   Layout (helpdeco): 9-byte file header, then BTREEHEADER whose pagesize is at +4 and rootpage at
   +26; pages start at +38. A leaf page is (unk, nentries, prev, next) then entries. */
static int btree_entries(const unsigned char* d, long dsize, int off,
                         void (*emit)(const char* name, int val))
{
    int o = off + 9;
    if (o + 38 > dsize) return 0;
    int pagesize = rd16(d + o + 4);
    short rootpage = (short)rd16(d + o + 26);
    long p = (long)o + 38 + (long)rootpage * pagesize;
    if (p + 8 > dsize) return 0;
    int n = (short)rd16(d + p + 2);
    long q = p + 8;
    int got = 0;
    for (int i = 0; i < n && q < dsize; i++)
    {
        const unsigned char* e = (const unsigned char*)memchr(d + q, 0, (size_t)(dsize - q));
        if (!e) break;
        const char* name = (const char*)(d + q);
        long namelen = (long)(e - (d + q));
        q += namelen + 1;
        if (q + 4 > dsize) break;
        int v = rd32(d + q); q += 4;
        emit(name, v);
        got++;
    }
    return got;
}

static void emit_title(const char* name, int /*val*/)
{
    if (g_count >= MA_HELP_MAX) return;
    if (!name || !*name) return;
    strncpy(g_titles[g_count], name, sizeof(g_titles[0]) - 1);
    g_titles[g_count][sizeof(g_titles[0]) - 1] = 0;
    g_count++;
}

static void ma_help_parse(void)
{
    g_count = 0;
    /* The compat layer maps fopen -> fopen_nocase, which resolves game paths under BOB_DRIVE_C. */
    FILE* f = fopen("English\\TEXT\\MIG.HLP", "rb");
    if (!f) f = fopen("English/TEXT/MIG.HLP", "rb");
    if (!f) { if (getenv("MA_TRACE_HELP")) fprintf(stderr, "[help] MIG.HLP not found\n"); return; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 16 || n > (16 << 20)) { fclose(f); return; }
    unsigned char* d = (unsigned char*)malloc((size_t)n);
    if (!d) { fclose(f); return; }
    if (fread(d, 1, (size_t)n, f) != (size_t)n) { free(d); fclose(f); return; }
    fclose(f);

    if (rd32(d) != 0x00035F3F)   /* WinHelp magic */
    {
        if (getenv("MA_TRACE_HELP")) fprintf(stderr, "[help] not a WinHelp file\n");
        free(d); return;
    }
    int dirstart = rd32(d + 4);

    /* The directory B-tree maps internal file NAMES to offsets; find |TTLBTREE in it. Walked
       inline rather than through btree_entries() because this walk needs to keep a result. */
    int s_ttl = 0;
    {
        int o = dirstart + 9;
        if (o + 38 <= n) {
            int pagesize = rd16(d + o + 4);
            short rootpage = (short)rd16(d + o + 26);
            long p = (long)o + 38 + (long)rootpage * pagesize;
            if (p + 8 <= n) {
                int cnt = (short)rd16(d + p + 2);
                long q = p + 8;
                for (int i = 0; i < cnt && q < n; i++) {
                    const unsigned char* e = (const unsigned char*)memchr(d + q, 0, (size_t)(n - q));
                    if (!e) break;
                    const char* name = (const char*)(d + q);
                    long len = (long)(e - (d + q));
                    q += len + 1;
                    if (q + 4 > n) break;
                    int v = rd32(d + q); q += 4;
                    if (!strcmp(name, "|TTLBTREE")) s_ttl = v;
                }
            }
        }
    }
    if (s_ttl > 0) btree_entries(d, n, s_ttl, emit_title);
    if (getenv("MA_TRACE_HELP"))
        fprintf(stderr, "[help] MIG.HLP: %ld bytes, |TTLBTREE @%d, %d topic titles\n", n, s_ttl, g_count);
    free(d);
}

extern "C" int ma_help_topic_count(void)
{
    if (g_count < 0) ma_help_parse();
    return g_count;
}

extern "C" const char* ma_help_topic(int i)
{
    if (g_count < 0) ma_help_parse();
    if (i < 0 || i >= g_count) return "";
    return g_titles[i];
}

/* ---- the panel's open/closed state (drawn by the campaign-map idle) --------------------------
 * Deliberately just a flag plus the context id: the "?" is a toggle in the game, and the panel is
 * composited by the map idle like every other piece of map chrome, so no dialog machinery is
 * needed for what is currently an index listing. */
static int g_open = 0, g_ctx = 0;
extern "C" void ma_help_open(int on)        { g_open = on ? 1 : 0; }
extern "C" int  ma_help_is_open(void)       { return g_open; }
extern "C" void ma_help_set_context(int c)  { g_ctx = c; }
extern "C" int  ma_help_context(void)       { return g_ctx; }
