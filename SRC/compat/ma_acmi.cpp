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
#include <math.h>

extern "C" {

static FILE* g_acmi = 0;
static char  g_acmi_path[512];
static int   g_acmi_objects = 0;
static double g_acmi_lastT = -1.0;

/* Korea theatre origin -- CALIBRATED (S305), no longer a guess.
   The reference is now the sim's OWN map origin (0,0) (MAPS.H KOREAMAPORIGINX/Y = 0), so U/V
   convert with no offset subtraction. Solved from two airfield placements in the shipped
   battlefield source SRC/BFIELDS/MAINMIG.BFI, whose `Posn { Abs { X, Z } }` blocks are absolute
   world CENTIMETRES -- the same frame Replay.cpp samples as `_ac->World.X / 100`:

       UID_AfBlKimpo    X=61615953 Z=61326673  ->  37.558N 126.791E
       UID_AfRdSinuiju  X=41409080 Z=89304918  ->  40.100N 124.400E

   Two equations, two unknowns per axis (offset + scale). X is EAST, Z is NORTH. */
static const double ACMI_REF_LON = 119.500224;   /* lon at world X = 0 */
static const double ACMI_REF_LAT =  31.986085;   /* lat at world Z = 0 */

/* Solved scale. NOT the textbook constants, and that difference is the point: the map is a
   hand-drawn theatre, not a projection, so its degrees are its own. The fit lands within 1% of
   the true 111132 m/deg latitude, which is the evidence it is a real solution rather than two
   points joined by a line -- a wrong pairing would have produced no such agreement. */
static const double ACMI_M_PER_DEG_LAT = 110063.9;
static const double ACMI_M_PER_DEG_LON =  84512.2;

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
/* PO-79: the export needs its OWN monotonic frame counter. The game side was feeding
   `replayframecount`, which is the WITHIN-BLOCK counter (0..FRAMESINBLOCK-1, reset when a block
   rolls), so the exported clock restarted every 51.2 s. ma_acmi_time drops any non-increasing
   timestamp, so every marker after the first block vanished while the OBJECT writes continued --
   producing exactly 1024 markers however long the sortie, with all later positions stacked under
   the last one. The counter lives HERE, with the file it belongs to, so it cannot go out of step
   with begin/end the way a static in the caller would across two sorties in one process. */
static unsigned long g_acmi_frame = 0;

void ma_acmi_reset_clock(void) { g_acmi_frame = 0; g_acmi_lastT = -1.0; }

unsigned long ma_acmi_next_frame(void) { return g_acmi_frame++; }

void ma_acmi_time(double seconds)
{
    if (!g_acmi) return;
    if (seconds <= g_acmi_lastT) return;
    fprintf(g_acmi, "#%.2f\r\n", seconds);
    g_acmi_lastT = seconds;
}

/* One object's state at the current time marker.
   u/v/alt are METRES (callers convert from the sim's centimetres); roll/pitch/yaw in DEGREES. */
void ma_acmi_object_ias(unsigned long id, double u, double v, double alt,
                        double roll, double pitch, double yaw,
                        const char* name, const char* type, const char* color, int isPlayer,
                        double ias);

void ma_acmi_object(unsigned long id, double u, double v, double alt,
                    double roll, double pitch, double yaw,
                    const char* name, const char* type, const char* color, int isPlayer)
{
    ma_acmi_object_ias(id, u, v, alt, roll, pitch, yaw, name, type, color, isPlayer, -1.0);
}

/* L4: same, plus indicated airspeed in METRES PER SECOND (the ACMI spec is metric throughout).
   Pass a negative ias to omit the property. */
void ma_acmi_object_ias(unsigned long id, double u, double v, double alt,
                        double roll, double pitch, double yaw,
                        const char* name, const char* type, const char* color, int isPlayer,
                        double ias)
{
    if (!g_acmi || !id) return;
    /* S284: EMIT REAL LON/LAT, not just U/V.
       This wrote "T=||alt|roll|pitch|yaw|u|v|hdg" -- Lon and Lat deliberately blank, on the belief
       that Tacview would position objects from the native flat-world U/V. It does not: Lon/Lat are
       the authoritative spherical position and U/V are supplementary. With both blank every object
       stayed pinned at the reference origin for the whole recording while its attitude and altitude
       kept updating.
       ⭐ THE PO DIAGNOSED THIS FROM THE PICTURE, and the report is worth preserving because of how
       precise it was: "each aircraft seems constrained to stay at the same X,Y location - it can
       rotate and move up and down, but not translate in the X-Y plane". That splits the transform
       exactly along the line between the fields written into non-Lon/Lat slots (Alt, Roll, Pitch,
       Yaw -- all working) and the one field encoded ONLY as U/V (X-Y -- dead). No other fault has
       that shape.
       It also explains two earlier reports I had misattributed to file truncation: "nothing makes
       aircraft start moving" (MA) and "I don't see motion, though there is a slow motion of the z
       axis" (BoB). The truncation was real and separate; this is why motion was missing even in the
       part of the file that survived. A fix that makes a symptom smaller is not proof it was the
       cause -- S278 shortened these files' visible span and I read the remaining stillness as "not
       enough file", when the aircraft were never going to move at any length.
       Flat-earth conversion about the reference point: at these scales (a theatre a few hundred km
       across) the error from ignoring curvature is far below what a debrief can perceive. U/V are
       still emitted so the native coordinates remain available. */
    {
    /* S305: THE THEATRE IS NOW PINNED. S296 left this arbitrary and said so; the missing piece was
       an assumption, not data. S296 concluded "the positions live in the world item data, reachable
       only at runtime" -- which is true of the RUNTIME positions and false of the placements. The
       world is BUILT from SRC/BFIELDS/*.BFI, checked-in text in which every item carries an
       absolute Posn. No runtime dump was needed; the coordinates had been in the repo all along.

       VALIDATION -- four airfields NOT used in the fit, so they could have falsified it:

           Suwon AB         predicted 37.2418N 127.0012E   vs  37.239N 127.007E    0.6 km
           Pyongyang        predicted 39.0461N 125.7713E   vs  39.030N 125.750E    2.6 km
           Antung/Dandong   predicted 40.0202N 124.2442E   vs  40.025N 124.286E    3.7 km
           Taegu K-2        predicted 35.8840N 128.7242E   vs  35.894N 128.659E    5.8 km

       Every one lands within 6 km on a theatre ~800 km across. The residuals are about the size of
       the ambiguity in the pairing itself (Pyongyang and Taegu each have more than one field), so
       they bound the game's own placement error, not a defect in the solve.

       CROSS-CHECK: the existing ma-1v1-long.acmi opens at U=412525 V=892952, which is Sinuiju's
       placement (414091, 893049) to ~1.5 km -- the quick mission does start over MiG Alley. That
       is independent confirmation that U=World.X/100 and V=World.Z/100 share the BFI frame, which
       is the one assumption the whole solve rests on.

       The knobs stay, now as overrides on a measured default rather than substitutes for one:
       MA_ACMI_REF="lat,lon", MA_ACMI_ORIGIN="u,v", MA_ACMI_MPERDEG="lat,lon". */
    double _refLat = ACMI_REF_LAT, _refLon = ACMI_REF_LON, _oU = 0.0, _oV = 0.0;
    { const char* r = getenv("MA_ACMI_REF");
      if (r) sscanf(r, "%lf,%lf", &_refLat, &_refLon);
      const char* o = getenv("MA_ACMI_ORIGIN");
      if (o) sscanf(o, "%lf,%lf", &_oU, &_oV); }
    /* The scale is the map's, not the Earth's, so it does NOT derive from _refLat -- moving the
       reference with MA_ACMI_REF must not silently rescale the theatre, which the old
       cos(_refLat) form did. */
    double _mPerDegLat = ACMI_M_PER_DEG_LAT, _mPerDegLon = ACMI_M_PER_DEG_LON;
    { const char* m = getenv("MA_ACMI_MPERDEG");
      if (m) sscanf(m, "%lf,%lf", &_mPerDegLat, &_mPerDegLon); }
    double _lat = _refLat + (v - _oV) / _mPerDegLat;
    double _lon = _refLon + (u - _oU) / _mPerDegLon;
    fprintf(g_acmi, "%lx,T=%.7f|%.7f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f",
            id, _lon, _lat, alt, roll, pitch, yaw, u, v, yaw);
    }
    if (name  && *name)  fprintf(g_acmi, ",Name=%s", name);
    if (type  && *type)  fprintf(g_acmi, ",Type=%s", type);
    if (color && *color) fprintf(g_acmi, ",Color=%s", color);
    if (isPlayer)        fprintf(g_acmi, ",Pilot=Player");
    if (ias >= 0.0)      fprintf(g_acmi, ",IAS=%.2f", ias);
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
    /* S259 (L5): copy WHOLE LINES ONLY -- the working file is written continuously, so its last
       line can be half-formed at the moment of a save, and a published .acmi must never carry a
       truncated record.

       S278: REWRITTEN, because the first version silently dropped ~78% of the file. The PO opened
       the export in Tacview and it froze after ~4 seconds; the working file held 1024 time markers
       (51 s) while the published copy held 80 (4 s). The old chunked implementation carried a
       partial tail between iterations and lost it on the next read -- a fix for a ONE-LINE
       truncation that introduced a THREE-QUARTERS truncation.
       The L5 gate passed it throughout: it checks that the file is well-formed, and a file
       truncated at a line boundary IS well-formed. STRUCTURAL VALIDITY IS NOT COMPLETENESS.
       This version reads the file whole and writes up to the final newline -- obviously correct at
       a glance, which the previous one was not. */
    if (fseek(in, 0, SEEK_END) != 0) { fclose(in); fclose(o); return 0; }
    long total = ftell(in);
    if (total < 0) { fclose(in); fclose(o); return 0; }
    rewind(in);
    char* all = (char*)malloc((size_t)total + 1);
    if (!all) { fclose(in); fclose(o); return 0; }
    size_t got = fread(all, 1, (size_t)total, in);
    long cut = (long)got;
    while (cut > 0 && all[cut - 1] != '\n') cut--;   /* drop any partial final line */
    if (cut > 0) fwrite(all, 1, (size_t)cut, o);
    free(all);
    fclose(o); fclose(in);
    if (getenv("MA_TRACE_ACMI"))
        fprintf(stderr, "[acmi] wrote %s (%d object samples)\n", out, g_acmi_objects);
    return 1;
}

} /* extern "C" */
