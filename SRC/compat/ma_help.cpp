/* S114 (PO-10): the documentation the "?" button opens — the game's own help text.
 *
 * Route here: S98 fixed four broken links so a "?" click reaches CWinApp::WinHelp; S112 made that
 * raise a panel instead of doing nothing. What the panel could SHOW was the open question, because
 * the shipped help file (English/TEXT/MIG.HLP) is Hall-compressed and two sprints failed to decode
 * its topic text -- S99 solved four of five stages, S112 eliminated an 800-candidate opcode family
 * against the title oracle, and neither would ship a decoder that emits plausible nonsense.
 *
 * The PO pointed at the tree and the answer was in it: the help SOURCE ships with the source.
 * SRC/<LANG>/HELP/MIG.RTF is the WinHelp RTF that MIG.HLP was compiled FROM (MIG.HPJ even records
 * COMPRESS=12 Hall Zeck), and MIG.HM maps the HID_/HIDD_ symbols to the context numbers the game
 * passes to WinHelp. The documentation never needed decompressing -- it needed reading.
 *
 * port/tools/rtf_help.py turns those two files into one flat, inspectable data file
 * (port/data/mig_help.txt, installed beside the game data):
 *
 *     #TOPIC <context symbol>|<title>
 *     <body line>
 *     ...
 *     #MAP <symbol> <0xNNNN>
 *
 * This reads it, resolves the context id the game passed, and serves the topic's real text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MA_HELP_MAXTOPIC 64
#define MA_HELP_MAXLINE  600
#define MA_HELP_MAXMAP   256

struct MaHelpTopic { char ctx[48]; char title[64]; int first, count; };

static char*        g_buf;
static const char*  g_line[MA_HELP_MAXLINE];
static int          g_lines;
static MaHelpTopic  g_topic[MA_HELP_MAXTOPIC];
static int          g_topics = -1;               /* -1 = not loaded yet */
static struct { char sym[48]; unsigned id; } g_map[MA_HELP_MAXMAP];
static int          g_maps;
static int          g_cur = -1;                  /* selected topic, -1 = show the index */
static int          g_open;

static void ma_help_load(void)
{
    g_topics = 0; g_maps = 0; g_lines = 0; g_cur = -1;
    /* The port runs with the game directory as cwd (S30 bare launch). Try the install first, then
       the repo copy, so a developer tree works with no install step. */
    const char* tries[] = { "mig_help.txt", "Docs\\mig_help.txt", "Docs/mig_help.txt",
                            "/home/admin/ma/port/data/mig_help.txt", 0 };
    FILE* f = 0;
    for (int i = 0; tries[i] && !f; i++) f = fopen(tries[i], "rb");
    if (!f) { if (getenv("MA_TRACE_HELP")) fprintf(stderr, "[help] mig_help.txt not found\n"); return; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > (4 << 20)) { fclose(f); return; }
    g_buf = (char*)malloc((size_t)n + 1);
    if (!g_buf) { fclose(f); return; }
    size_t got = fread(g_buf, 1, (size_t)n, f);
    fclose(f);
    g_buf[got] = 0;

    char* p = g_buf;
    while (*p && g_lines < MA_HELP_MAXLINE)
    {
        char* e = strchr(p, '\n');
        if (e) *e = 0;
        size_t L = strlen(p);
        while (L && (p[L-1] == '\r' || p[L-1] == ' ')) p[--L] = 0;

        if (!strncmp(p, "#TOPIC ", 7))
        {
            if (g_topics < MA_HELP_MAXTOPIC)
            {
                MaHelpTopic& t = g_topic[g_topics];
                const char* s = p + 7;
                const char* bar = strchr(s, '|');
                size_t cl = bar ? (size_t)(bar - s) : strlen(s);
                if (cl >= sizeof(t.ctx)) cl = sizeof(t.ctx) - 1;
                memcpy(t.ctx, s, cl); t.ctx[cl] = 0;
                strncpy(t.title, bar ? bar + 1 : "", sizeof(t.title) - 1);
                t.title[sizeof(t.title) - 1] = 0;
                t.first = g_lines; t.count = 0;
                g_topics++;
            }
        }
        else if (!strncmp(p, "#MAP ", 5))
        {
            char sym[48]; unsigned id = 0;
            if (sscanf(p + 5, "%47s %i", sym, &id) == 2 && g_maps < MA_HELP_MAXMAP)
            {
                strncpy(g_map[g_maps].sym, sym, sizeof(g_map[0].sym) - 1);
                g_map[g_maps].sym[sizeof(g_map[0].sym) - 1] = 0;
                g_map[g_maps].id = id;
                g_maps++;
            }
        }
        else if (*p && g_topics > 0)
        {
            g_line[g_lines++] = p;
            g_topic[g_topics - 1].count++;
        }
        if (!e) break;
        p = e + 1;
    }
    if (getenv("MA_TRACE_HELP"))
        fprintf(stderr, "[help] mig_help.txt: %d topics, %d lines, %d context ids\n",
                g_topics, g_lines, g_maps);
}

static void ma_help_ensure(void) { if (g_topics < 0) ma_help_load(); }

extern "C" void ma_help_open(int on)  { g_open = on ? 1 : 0; }
extern "C" int  ma_help_is_open(void) { return g_open; }

extern "C" void ma_help_set_context(int ctx)
{
    ma_help_ensure();
    g_cur = -1;
    if (!g_topics) return;
    /* id -> symbol (MIG.HM) -> topic (the RTF's # footnote). Only some symbols are mapped and the
       map "?" passes HID_BASE_RESOURCE+IDD_*, so a miss is normal: fall back to the index, which is
       still the game's own documentation rather than an empty window. */
    const char* sym = 0;
    for (int i = 0; i < g_maps; i++)
        if (g_map[i].id == (unsigned)ctx) { sym = g_map[i].sym; break; }
    if (sym)
        for (int i = 0; i < g_topics; i++)
            if (!strcmp(g_topic[i].ctx, sym)) { g_cur = i; break; }
    if (getenv("MA_TRACE_HELP"))
        fprintf(stderr, "[help] context 0x%x -> %s -> topic %d\n", (unsigned)ctx,
                sym ? sym : "(unmapped)", g_cur);
}

extern "C" int         ma_help_topic_count(void) { ma_help_ensure(); return g_topics; }
extern "C" const char* ma_help_topic(int i)      { ma_help_ensure(); return (i >= 0 && i < g_topics) ? g_topic[i].title : ""; }
extern "C" int         ma_help_current(void)     { ma_help_ensure(); return g_cur; }
extern "C" void        ma_help_select(int i)     { ma_help_ensure(); g_cur = (i >= -1 && i < g_topics) ? i : -1; }

extern "C" int ma_help_body_lines(void)
{
    ma_help_ensure();
    if (g_cur < 0 || g_cur >= g_topics) return 0;
    return g_topic[g_cur].count;
}

extern "C" const char* ma_help_body_line(int i)
{
    ma_help_ensure();
    if (g_cur < 0 || g_cur >= g_topics) return "";
    if (i < 0 || i >= g_topic[g_cur].count) return "";
    return g_line[g_topic[g_cur].first + i];
}
