/* ma_acmi.cpp — Tacview ACMI export (EPIC L / L1).
 *
 * PO: "when you save a replay .cam file, also save an equivalent tacview replay file of the same
 * material ... it allows the user to review their performance precisely."
 *
 * WHY IT TEES FROM THE SIM INSTEAD OF CONVERTING THE .cam
 * ------------------------------------------------------
 * L0 (S211) answered this: a REPLAYPACKET is 11 packed bytes of DELTAS against a reconstructed
 * world, not absolute state. Converting a .cam would mean re-implementing the playback integrator
 * and inheriting every one of its alignment bugs -- the very bugs PO-61/PO-69 spent a dozen sprints
 * on. Teeing live state while the flight records is both simpler and independent of the reader.
 *
 * ADDITIVE ONLY -- THE .cam MUST BE BYTE-UNCHANGED
 * ------------------------------------------------
 * Nothing here touches the replay stream. The export writes its own file and is skipped entirely
 * unless enabled. That is also what makes the epic safely testable: the existing replay path is its
 * own control, so "did I break the recording?" is answered by `cmp`, not by judgement.
 *
 * TRANSFORM SYNTAX #4 -- T=Lon|Lat|Alt|Roll|Pitch|Yaw|U|V|Heading
 * ---------------------------------------------------------------
 * U and V are NATIVE FLAT-WORLD METRES. MA's theatre is a flat Korea map in centimetres, so we emit
 * U/V directly from sim coordinates and pick one ReferenceLongitude/Latitude for the origin --
 * no geodetic projection to get wrong, and every number stays checkable against the sim's own
 * coordinates. Lon/Lat are left empty (legal in syntax #4); Tacview derives them from the reference.
 *
 * MA_ACMI=0 disables the export.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {

static FILE* g_acmi = 0;
static char  g_acmi_path[512];
static int   g_acmi_objects = 0;
static double g_acmi_lastT = -1.0;

/* Korea theatre origin. MA's map is flat and local; one reference point anchors the whole file.
   Roughly Seoul -- the exact value only shifts where Tacview draws the map under the track, and
   U/V carry the real geometry. */
static const double ACMI_REF_LON = 127.0;
static const double ACMI_REF_LAT =  37.5;

int ma_acmi_enabled(void)
{
    const char* e = getenv("MA_ACMI");
    return (e && e[0] == '0') ? 0 : 1;
}

/* Begin a recording. `title` names the sortie. Safe to call repeatedly; a second call closes the
   first. Returns 1 if a file is open. */
int ma_acmi_begin(const char* title)
{
    if (!ma_acmi_enabled()) return 0;
    if (g_acmi) { fclose(g_acmi); g_acmi = 0; }
    g_acmi_objects = 0; g_acmi_lastT = -1.0;
    snprintf(g_acmi_path, sizeof(g_acmi_path), "%s", "acmi_current.txt");
    g_acmi = fopen(g_acmi_path, "wb");
    if (!g_acmi) return 0;
    fprintf(g_acmi, "FileType=text/acmi/tacview\r\n");
    fprintf(g_acmi, "FileVersion=2.2\r\n");
    /* Global properties live on object 0. ReferenceTime is an ISO-8601 instant; MA's campaign is
       Korea 1950-53, and the mission date is not plumbed here yet (L4), so a fixed epoch keeps the
       file valid and the RELATIVE timeline -- which is what a debrief reads -- exact. */
    fprintf(g_acmi, "0,ReferenceTime=1950-06-25T00:00:00Z\r\n");
    fprintf(g_acmi, "0,ReferenceLongitude=%.6f\r\n", ACMI_REF_LON);
    fprintf(g_acmi, "0,ReferenceLatitude=%.6f\r\n",  ACMI_REF_LAT);
    fprintf(g_acmi, "0,DataSource=MiG Alley (Rowan, 1999) -- Linux port\r\n");
    fprintf(g_acmi, "0,DataRecorder=ma_acmi (EPIC L)\r\n");
    fprintf(g_acmi, "0,Title=%s\r\n", (title && *title) ? title : "MiG Alley sortie");
    fflush(g_acmi);
    return 1;
}

/* Advance the timeline. ACMI requires time markers to be MONOTONIC; emit one only when the time
   actually moves forward, so a repeated or stale call cannot corrupt the file. */
void ma_acmi_time(double seconds)
{
    if (!g_acmi) return;
    if (seconds <= g_acmi_lastT) return;
    fprintf(g_acmi, "#%.2f\r\n", seconds);
    g_acmi_lastT = seconds;
}

/* One object's state at the current time marker.
   u/v/alt are METRES (callers convert from the sim's centimetres); roll/pitch/yaw in DEGREES. */
void ma_acmi_object(unsigned long id, double u, double v, double alt,
                    double roll, double pitch, double yaw,
                    const char* name, const char* type, const char* color, int isPlayer)
{
    if (!g_acmi || !id) return;
    fprintf(g_acmi, "%lx,T=||%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f",
            id, alt, roll, pitch, yaw, u, v, yaw);
    if (name  && *name)  fprintf(g_acmi, ",Name=%s", name);
    if (type  && *type)  fprintf(g_acmi, ",Type=%s", type);
    if (color && *color) fprintf(g_acmi, ",Color=%s", color);
    if (isPlayer)        fprintf(g_acmi, ",Pilot=Player");
    fprintf(g_acmi, "\r\n");
    g_acmi_objects++;
}

/* An object left the world (destroyed or despawned). */
void ma_acmi_remove(unsigned long id)
{
    if (!g_acmi || !id) return;
    fprintf(g_acmi, "-%lx\r\n", id);
}

int ma_acmi_active(void)   { return g_acmi ? 1 : 0; }
int ma_acmi_count(void)    { return g_acmi_objects; }

void ma_acmi_end(void)
{
    if (!g_acmi) return;
    fclose(g_acmi); g_acmi = 0;
}

/* Publish the recording beside the .cam the game just wrote. `camname` is what SaveReplayData was
   given (e.g. "260825test6.cam"); the .acmi takes the same stem. Returns 1 on success.
   The current recording stays open -- saving mid-flight must not stop the tee. */
int ma_acmi_save_as(const char* camname)
{
    if (!ma_acmi_enabled() || !camname || !*camname) return 0;
    if (g_acmi) fflush(g_acmi);
    char stem[512]; snprintf(stem, sizeof(stem), "%s", camname);
    char* dot = strrchr(stem, '.'); if (dot) *dot = 0;
    char out[600]; snprintf(out, sizeof(out), "Videos/%s.acmi", stem);
    FILE* in = fopen(g_acmi_path, "rb");
    if (!in) return 0;
    FILE* o = fopen(out, "wb");
    if (!o) { fclose(in); return 0; }
    /* S259 (L5): COPY WHOLE LINES ONLY.
       The working file is written continuously while the sim runs, so at the instant of a save --
       or of a kill -- its last line can be half-formed. The L5 gate found exactly that: 171 object
       lines with a valid 9-field transform and ONE trailing line cut mid-number
       ("2,T=||1555.24|71.65|14.56|145.76|413327.26|8"). Harmless in the scratch file, NOT harmless
       in a published .acmi: a debrief tool should never be handed a truncated record, and a partial
       line is exactly the sort of thing that makes a parser reject an otherwise good file.
       So the copy stops at the last newline. Whatever follows it is an incomplete sample and is
       dropped -- one lost frame out of thousands, against a file that is always well-formed. */
    char buf[8192]; size_t n; long carry = 0;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        size_t last = 0, i;
        for (i = 0; i < n; i++) if (buf[i] == '\n') last = i + 1;
        if (last) { fwrite(buf, 1, last, o); carry = 0; }
        if (last < n) {
            /* tail without a newline: hold it -- if more data follows it completes a line, and if
               this was the final read it is the partial sample we deliberately drop. */
            memmove(buf, buf + last, n - last);
            carry = (long)(n - last);
            size_t got = fread(buf + carry, 1, sizeof(buf) - carry, in);
            if (got == 0) break;
            n = carry + got;
            size_t l2 = 0; for (i = 0; i < n; i++) if (buf[i] == '\n') l2 = i + 1;
            if (l2) fwrite(buf, 1, l2, o);
            if (l2 >= n) continue;
            memmove(buf, buf + l2, n - l2); carry = (long)(n - l2);
        }
    }
    fclose(o); fclose(in);
    if (getenv("MA_TRACE_ACMI"))
        fprintf(stderr, "[acmi] wrote %s (%d object samples)\n", out, g_acmi_objects);
    return 1;
}

} /* extern "C" */
