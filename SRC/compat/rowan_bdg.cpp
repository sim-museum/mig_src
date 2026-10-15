/* bdg.txt reader for the Rowan ports (MiG Alley, Battle of Britain). The BDG community patches (BDG MiG Alley 0.85F,
   BDG BoB 0.99) keep their switches in bdg.txt in the game folder, "KEY = VALUE" one per line, '#' comments; the
   PO's installs carry it, and the AppImages ship it with the game data. The pre-patch source we compile never read
   it (PATCH-PORT-1, HEADTRACK-1). Values: ON/OFF (1/0), integers, or text.
   This file is identical in ~/ma and ~/bob. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

namespace {
struct BdgEntry { char key[48]; char val[80]; };
BdgEntry g_bdg[256];
int g_nBdg = -1;   // -1: not read yet

void bdg_load()
{
    g_nBdg = 0;
    FILE* f = fopen("bdg.txt", "r");
    if (!f) f = fopen("BDG.TXT", "r");
    if (!f) f = fopen("Bdg.txt", "r");
    if (!f) return;
    char line[256];
    while (g_nBdg < 256 && fgets(line, sizeof line, f)) {
        char* p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '#' || *p == ';' || *p == 0) continue;
        char* eq = strchr(p, '=');
        if (!eq) continue;
        char* ke = eq;
        while (ke > p && isspace((unsigned char)ke[-1])) ke--;
        char* v = eq + 1;
        while (isspace((unsigned char)*v)) v++;
        char* ve = v + strlen(v);
        while (ve > v && isspace((unsigned char)ve[-1])) ve--;
        BdgEntry& e = g_bdg[g_nBdg++];
        snprintf(e.key, sizeof e.key, "%.*s", (int)(ke - p), p);
        snprintf(e.val, sizeof e.val, "%.*s", (int)(ve - v), v);
    }
    fclose(f);
    fprintf(stderr, "[bdg] bdg.txt: %d settings\n", g_nBdg);
}
}

/* The text of setting KEY, or NULL. */
const char* rowan_bdg_text(const char* key)
{
    /* test/override hook: environment BDG_<KEY> wins over bdg.txt (e.g. BDG_FIX_KEYBOARD_RUDDER=ON) */
    char envname[64];
    snprintf(envname, sizeof envname, "BDG_%s", key);
    if (const char* e = getenv(envname)) return e;
    if (g_nBdg < 0) bdg_load();
    for (int i = 0; i < g_nBdg; i++)
        if (!strcasecmp(g_bdg[i].key, key)) return g_bdg[i].val;
    return NULL;
}

/* Setting KEY as a number: ON = 1, OFF = 0, else the leading integer; dflt when absent. */
int rowan_bdg_int(const char* key, int dflt)
{
    const char* v = rowan_bdg_text(key);
    if (!v || !*v) return dflt;
    if (!strcasecmp(v, "ON")) return 1;
    if (!strcasecmp(v, "OFF")) return 0;
    return atoi(v);
}

/* Setting KEY as a floating-point number; dflt when absent. */
double rowan_bdg_float(const char* key, double dflt)
{
    const char* v = rowan_bdg_text(key);
    return (v && *v) ? atof(v) : dflt;
}
