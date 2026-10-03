/* EPIC-MA-RED (port addition): the single-player Communist campaign -- state, sidecar save, UN AI strategy.
   #included at the end of MISSINIT.CPP under MA_LINUX (it needs that TU's MissMan/DirControl/SupplyTree context).

   The player commands the Communist air forces in the Spring Offensive (SO51, 5 Jan - 1 Apr 1951). The UN side is
   planned by the computer through MA's own Directives (DirControl::AllocateAc / SetPackagesFromDirectives, the same
   engine that fills the UN player's Mission Folder); this file decides HOW MANY strike and fighter aircraft the UN
   commits to each directive type each session -- its strategy. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "MA_RED.H"

/* R4: per-regiment state, persistent across sessions (BoB RAF model: readiness, sustained effort). */
enum RedReadiness	{ RED_RR_RELEASED = 0, RED_RR_AVAILABLE, RED_RR_READINESS, RED_RR_STANDBY };
enum RedTask		{ RED_RT_NONE = 0, RED_RT_INTERCEPT, RED_RT_BARCAP };
struct RedRegimentState
{
	signed char		readiness;		/* RedReadiness */
	signed char		task;			/* RedTask for the coming session */
	signed char		contact;		/* RED_RT_INTERCEPT: index into red_contacts */
	unsigned char	fatigue;		/* 0 fresh .. 100 exhausted */
	short			sorties;		/* flown this campaign */
	short			spare;
};
enum { RED_MAXREG = 12 };			/* == MAXMIGSQUADRONS (MISSSUB.H) */
struct RedCampaignState
{
	int					magic;			/* 'MRED' */
	int					version;		/* 2: regiments added */
	int					active;			/* the player is the Communist commander */
	RedRegimentState	reg[RED_MAXREG];
	int					reserved[40];	/* later sprints (rosters, milestones) -- the loader tolerates shorter files */
};
/* NOT static: MISSINIT.CPP is compiled twice (standalone in obj2 and inside the _MISS.CPP unity) and linked with
   --allow-multiple-definition. A static would exist once per object, and a function resolved from one object
   could read the other's copy; with external linkage every reference lands on one definition. */
RedCampaignState g_red = { 0x4445524d, 2, 0 };

extern "C" int	ma_red_active(void)			{ return g_red.active; }
extern "C" void	ma_red_set_active(int on)	{ g_red.active = on ? 1 : 0; }

/* The savegame cannot carry new fields: Campaign is written as one raw blob and PackageList::LoadGame reads the
   stream to EOF ("must be last thing in savegame"). So the Red state lives in a sidecar beside the save. A save
   without one is a UN campaign; a UN save written over a Red one removes the stale sidecar. */
extern "C" void ma_red_save(const char* savepath)
{
	if (!savepath || !*savepath) return;
	char p[512]; snprintf(p, sizeof p, "%s.red", savepath);
	if (!g_red.active) { unlink(p); return; }
	FILE* f = fopen(p, "wb");
	if (!f) { fprintf(stderr, "[red] cannot write %s\n", p); return; }
	fwrite(&g_red, sizeof g_red, 1, f);
	fclose(f);
	if (getenv("MA_TRACE_RED")) fprintf(stderr, "[red] saved %s (active=%d)\n", p, g_red.active);
}

extern "C" void ma_red_load(const char* savepath)
{
	RedCampaignState s; memset(&s, 0, sizeof s);
	char p[512]; snprintf(p, sizeof p, "%s.red", savepath ? savepath : "");
	FILE* f = fopen(p, "rb");
	size_t n = 0;
	if (f) { n = fread(&s, 1, sizeof s, f); fclose(f); }
	RedCampaignState z; memset(&z, 0, sizeof z); z.magic = 0x4445524d; z.version = 2;
	if (n >= 3 * sizeof(int) && s.magic == 0x4445524d)
	{
		memcpy(&z, &s, n < sizeof z ? n : sizeof z);	/* an older, shorter sidecar keeps defaults for the rest */
		z.version = 2;
	}
	g_red = z;
	if (getenv("MA_TRACE_RED")) fprintf(stderr, "[red] load %s -> active=%d (sidecar %s)\n", p, g_red.active, f ? "found" : "absent");
}

/* ---------------------------------------------------------------------------------------------------------------
   UN strategy. Directive types (SupplyTree, NODE.H): 0 air superiority, 1 choke points (bridges), 2 supply
   (depots, marshalling yards), 3 airfields, 4 rail, 5 road, 6 army (close support). strike = aircraft from
   the F-80/F-84/F-51/B-26/B-29 pool; fighter = F-86s (sweeps under type 0, escorts under the others).

   HISTORICAL (default): FEAF's effort over the game's window, by phase. Sources: USAF, "The U.S. Air Force in Korea:
   Campaigns, Units, and Stations" (CCF Intervention 3 Nov 50-24 Jan 51; First UN Counteroffensive 25 Jan-21 Apr
   51); R. F. Futrell, "The United States Air Force in Korea 1950-1953" (close support against the January and
   February Chinese offensives; FEAF Interdiction Campaign No. 4 from mid-January against bridges and marshalling
   yards); the 4th Fighter-Interceptor Wing left Kimpo for Japan on 1 Jan 1951 and its Sabres were back over MiG
   Alley by 10 Mar, flying cover for Bomber Command's B-29s (USAF Korea campaigns; Air & Space Forces Magazine,
   "MiG Alley"). Proportions are the period's emphasis, not counted sorties.
   TUTORIAL (Preferences > Game > UN Strategy): the Spring Offensive strategy guide shipped in DOC/
   (springOffensiveSummary.md, migAlleySpringOffensiveTutorial.pdf): airfields first under a large escort and a
   BARCAP, then bridges, depots/yards, armed recon of rail and road; close support optional (none here).
   --------------------------------------------------------------------------------------------------------------- */
struct UNPhase { int fromday; const char* name; signed char strike[7], fighter[7]; };
static const UNPhase un_historical[] =
{	/* fromday = day of 1951 (1 = 1 Jan)		  AS  CH  SU  AF  RA  RO  AR		  AS  CH  SU  AF  RA  RO  AR */
	{  5, "CCF Third Phase offensive",			{  0, 16,  0,  0,  0, 16, 32 },	{  0,  0,  0,  0,  0,  0,  0 } },
	{ 15, "Interdiction Campaign No. 4",		{  0, 32, 16,  0,  0, 16, 16 },	{  0,  0,  0,  0,  0,  0,  0 } },
	{ 25, "Thunderbolt / Roundup",				{  0, 24, 16,  0, 16,  0, 24 },	{  0,  0,  0,  0,  0,  0,  0 } },
	{ 42, "CCF Fourth Phase offensive",			{  0, 16,  0,  0,  0, 16, 48 },	{  0,  0,  0,  0,  0,  0,  0 } },
	{ 52, "Killer",								{  0, 24, 16,  0,  0, 16, 24 },	{  0,  0,  0,  0,  0,  0,  0 } },
	{ 69, "Ripper -- Sabres back in MiG Alley",	{  0, 32, 16, 16,  0,  0, 16 },	{ 32,  0,  0,  0,  0,  0,  0 } },
};
/* Every non-zero strike entry is >= 16: a smaller allocation is "insufficient aircraft" for a package
   (measured: choke 8 -> note M_INSUFFAC, no mission). */
static const UNPhase un_tutorial =
	{  0, "Spring Offensive tutorial",			{  0, 24, 16, 32,  0, 16,  0 },	{ 16,  0,  0, 16,  0,  0,  0 } };

static const UNPhase& ma_red_un_phase(int* dayout)
{
	int day = (int)((MMC.currdate - (int)JAN1951) / (int)SECSPERDAY) + 1;
	if (dayout) *dayout = day;
	const char* ov = getenv("MA_RED_UNSTRAT");	/* test override: historical | tutorial */
	if (ov ? !strcmp(ov, "tutorial") : (bool)Save_Data.gamedifficulty[GD_UNTUTORIALSTRATEGY]) return un_tutorial;
	int k = 0;
	for (int i = 0; i < (int)(sizeof un_historical / sizeof un_historical[0]); i++)
		if (day >= un_historical[i].fromday) k = i;
	return un_historical[k];
}

/* Called before DirControl::AllocateAc() wherever the UN folder is generated (campaign start, each EndDebrief).
   Only in a Red campaign: in the UN campaign the Directives are the player's own. */
void ma_red_apply_un_strategy(DirControl& dc)
{
	if (!g_red.active) return;
	int day = 0;
	const UNPhase& ph = ma_red_un_phase(&day);
	dc.autogen = true;
	for (int t = 0; t < 7; t++)
	{
		dc.directives[t].strikeac = ph.strike[t];
		dc.directives[t].fighterac = ph.fighter[t];
	}
	/* Close support needs a ground battle: with none in progress the army directive finds no targets
	   (measured at the 5 Jan start: targets=0). FEAF then put that effort into armed reconnaissance and
	   interdiction behind the front, so move it there: half to road recon, half to choke points. */
	int movedcas = 0;
	if (dc.directives[6].strikeac > 0)
	{
		dc.FindTargets(6);
		if (dc.directives[6].targets == 0)
		{
			movedcas = dc.directives[6].strikeac;
			dc.directives[5].strikeac += movedcas / 2;
			dc.directives[1].strikeac += movedcas - movedcas / 2;
			dc.directives[6].strikeac = 0;
		}
	}
	if (getenv("MA_TRACE_UNFOLDER") || getenv("MA_TRACE_RED"))
		fprintf(stderr, "[unstrat] day %d of 1951: %s (%s)%s strike={%d,%d,%d,%d,%d,%d,%d} fighter={%d,%d,%d,%d,%d,%d,%d}\n",
		        day, ph.name, Save_Data.gamedifficulty[GD_UNTUTORIALSTRATEGY] ? "tutorial" : "historical",
		        movedcas ? " [no ground battle: CAS -> recon/interdiction]" : "",
		        dc.directives[0].strikeac, dc.directives[1].strikeac, dc.directives[2].strikeac, dc.directives[3].strikeac,
		        dc.directives[4].strikeac, dc.directives[5].strikeac, dc.directives[6].strikeac,
		        dc.directives[0].fighterac, dc.directives[1].fighterac, dc.directives[2].fighterac, dc.directives[3].fighterac,
		        dc.directives[4].fighterac, dc.directives[5].fighterac, dc.directives[6].fighterac);
}

/* ---------------------------------------------------------------------------------------------------------------
   R3 -- Communist detection (PO 2026-10-02: the Red player sees only what radar picks up; "always make the sim as
   realistic as possible to the actual historical situation").

   MA plans a session before it is flown, so the Red commander plans against the UN raids that WILL be detected
   during the session, each with the time it is detected; readiness then decides whether MiGs can scramble and
   reach it (R4). Undetected raids are invisible to the plan.

   RADAR. The Soviet 64th Fighter Aviation Corps brought air-surveillance radar and GCI to the Yalu with its MiG
   regiments; in the game's window (Jan-Apr 1951) that coverage is centred on Antung/Sinuiju -- the chain reached
   13 sets between Sinuiju and Sariwon only by December 1951. Sets: P-3 "Dumbo" (120-150 km) and RUS-2 (120 km,
   targets up to ~7 km). Sources: Wikipedia "P-3 radar", "64th Fighter Aviation Corps"; Air & Space Forces Magazine,
   "The Russians in MiG Alley". Model: one site at Antung, 150 km against high targets, cut to the radar horizon for
   low ones (antenna ~30 m: d = 4.12 (sqrt 30 + sqrt h) km, h in metres -- ~94 km at 300 m).
   OBSERVERS. Chinese/North Korean visual air-observation posts along the roads and coast of Communist-held
   territory, reporting by telephone. MODEL ASSUMPTIONS (no figure found yet, flagged): a raid is sighted when it
   crosses the front into Communist territory and reaches the air command 12 minutes later; large formations are
   almost always reported, small or low ones less often.
   --------------------------------------------------------------------------------------------------------------- */
struct RedContact
{
	int		pack;			/* Todays_Packages index */
	int		detected;		/* 1 = this raid is in the Red picture */
	int		source;			/* 1 radar, 2 observers */
	int		dettime;		/* seconds after midnight the report reaches the command */
	int		tot;			/* the raid's time over target (truth, used by the intercept model -- not shown) */
	int		warnmin;		/* minutes between report and ToT */
	int		aircraft;		/* truth */
	int		sizeband;		/* what the report says: 0 few (<8), 1 medium (8-23), 2 large (24+) */
	int		bombers;		/* observers can tell heavy bombers; radar cannot */
	int		altm;			/* modelled cruise altitude, metres */
	SLong	detx, detz;		/* where it was first detected (world) */
	SLong	tgtx, tgtz;		/* target (world) -- R4 uses it for the intercept geometry */
};
enum { RED_MAXCONTACTS = Profile::MAX_PACKS };
RedContact red_contacts[RED_MAXCONTACTS];	/* external linkage: see g_red */
int red_ncontacts = 0;

static double red_km(SLong dx, SLong dz)	/* world units -> km; MA world units are centimetres (calibrated below) */
{
	double x = (double)dx, z = (double)dz;
	return sqrt(x * x + z * z) / 100000.0;
}

/* modelled cruise altitude (m) and speed (km/h) of a package, from its main group's aircraft and its duty */
static void red_profile_of(Profile& pk, int* altm, int* kmh, int* bombers)
{
	int sq = (int)pk.wave[0].group[Profile::GRPSTRIKE].squadron;
	*bombers = 0;
	switch (sq)
	{
	case SQ_B29:	*altm = 6000; *kmh = 350; *bombers = 1; break;
	case SQ_B26:	*altm = 3000; *kmh = 380; *bombers = 1; break;
	case SQ_F86A:
	case SQ_F86B:	*altm = 9000; *kmh = 800; break;
	case SQ_F51:	*altm = 2000; *kmh = 450; break;
	default:		*altm = 4000; *kmh = 650; break;		/* F-80 / F-84 */
	}
	/* armed reconnaissance of roads and rail is flown low over the target area */
	if (((int)pk.duty & 0xF0) == 0x30) *altm = 500;	/* DC_AR (FLYINIT.H) */
}

static int red_front_z(SLong x)	/* front-line north coordinate nearest to this east coordinate */
{
	SupplyLine* lines[3] = { &SupplyTree::WestRoute, &SupplyTree::CentralRoute, &SupplyTree::EastRoute };
	SLong best = 0; SLong bestdx = 0x7fffffff; int have = 0;
	for (int i = 0; i < 3; i++)
	{
		if (!lines[i]->frontline || !lines[i]->frontline->associated_item) continue;
		ItemBasePtr it = Persons2::ConvertPtrUID(lines[i]->frontline->associated_item);
		if (!it) continue;
		SLong dx = it->World.X - x; if (dx < 0) dx = -dx;
		if (!have || dx < bestdx) { best = it->World.Z; bestdx = dx; have = 1; }
	}
	return have ? (int)best : 0x7fffffff;
}

/* deterministic 0..99 per package and day, so a session's picture is reproducible */
static int red_roll(int pack, int salt)
{
	unsigned h = (unsigned)(MMC.currdate / 3600) * 2654435761u ^ (unsigned)(pack * 40503 + salt * 9176);
	h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
	return (int)(h % 100);
}

void ma_red_build_intel(const char* where)
{
	red_ncontacts = 0;
	if (!g_red.active) return;
	ItemBasePtr antung = Persons2::ConvertPtrUID(UID_AfRdAntung);
	if (!antung) return;
	const int dbg = getenv("MA_TRACE_RED") || getenv("MA_TRACE_UNFOLDER");
	if (dbg)
	{
		ItemBasePtr kimpo = Persons2::ConvertPtrUID(UID_AfBlKimpo);
		if (kimpo) fprintf(stderr, "[redintel] calibration: Antung-Kimpo = %.0f km (real ~340)\n",
		                   red_km(kimpo->World.X - antung->World.X, kimpo->World.Z - antung->World.Z));
	}
	for (int p = 0; p < Profile::MAX_PACKS && red_ncontacts < RED_MAXCONTACTS; p++)
	{
		Profile& pk = Todays_Packages.pack[p];
		int ac = 0;
		for (int w = 0; w < Profile::MAX_WAVES; w++)
			for (int g = 0; g < Profile::MAX_GROUPS; g++)
				ac += 4 * (int)pk.wave[w].group[g].numflights;
		if (!ac || pk.packagetarget == UID_NULL) continue;
		ItemBasePtr tk = Persons2::ConvertPtrUID(pk.packagetakeoff), tg = Persons2::ConvertPtrUID(pk.packagetarget);
		if (!tk || !tg) continue;
		RedContact& c = red_contacts[red_ncontacts++];
		memset(&c, 0, sizeof c);
		c.pack = p; c.aircraft = ac; c.tot = pk.wave[0].targettime;
		c.tgtx = tg->World.X; c.tgtz = tg->World.Z;
		int kmh; red_profile_of(pk, &c.altm, &kmh, &c.bombers);
		c.sizeband = ac < 8 ? 0 : ac < 24 ? 1 : 2;
		/* the route, approximated as the straight line takeoff -> target; s = km back from the target */
		double lx = (double)(tk->World.X - tg->World.X), lz = (double)(tk->World.Z - tg->World.Z);
		double L = sqrt(lx * lx + lz * lz) / 100000.0;
		double ux = L > 0 ? lx / (L * 100000.0) : 0, uz = L > 0 ? lz / (L * 100000.0) : 0;
		/* radar: farthest point (largest s) of the route inside the site's range */
		double R = 4.12 * (sqrt(30.0) + sqrt((double)c.altm)); if (R > 150.0) R = 150.0;
		double rs = -1;
		for (double s = L; s >= 0; s -= 2.0)
		{
			double px = tg->World.X + ux * s * 100000.0, pz = tg->World.Z + uz * s * 100000.0;
			if (red_km((SLong)(px - antung->World.X), (SLong)(pz - antung->World.Z)) <= R) { rs = s; break; }
		}
		int tradar = rs >= 0 ? c.tot - (int)(rs / kmh * 3600.0) : 0x7fffffff;
		/* observers: the point where the route crosses into Communist-held territory (north of the front) */
		int tobs = 0x7fffffff; double os = -1;
		for (double s = 0; s <= L; s += 2.0)
		{
			double px = tg->World.X + ux * s * 100000.0, pz = tg->World.Z + uz * s * 100000.0;
			if (pz < red_front_z((SLong)px)) { os = s; break; }	/* first point (from the target) south of the front */
		}
		int seen = c.sizeband == 2 ? 95 : c.sizeband == 1 ? 85 : 70;
		if (c.altm <= 500) seen -= 15;
		if (os > 0 && red_roll(p, 1) < seen)
			tobs = c.tot - (int)(os / kmh * 3600.0) + 12 * 60;
		if (tradar <= tobs && tradar != 0x7fffffff)
		{
			c.detected = 1; c.source = 1; c.dettime = tradar;
			c.detx = (SLong)(tg->World.X + ux * rs * 100000.0); c.detz = (SLong)(tg->World.Z + uz * rs * 100000.0);
			c.bombers = 0;	/* a radar plot does not tell type */
		}
		else if (tobs != 0x7fffffff)
		{
			c.detected = 1; c.source = 2; c.dettime = tobs;
			c.detx = (SLong)(tg->World.X + ux * os * 100000.0); c.detz = (SLong)(tg->World.Z + uz * os * 100000.0);
		}
		c.warnmin = c.detected ? (c.tot - c.dettime) / 60 : 0;
		if (dbg)
			fprintf(stderr, "[redintel] %s pack %d: %d ac (alt %d m, %d km/h) tot %02d:%02d route %.0f km -> %s%s report %02d:%02d, warning %d min%s\n",
			        where, p, ac, c.altm, kmh, c.tot / 3600, (c.tot / 60) % 60, L,
			        c.detected ? (c.source == 1 ? "RADAR" : "OBSERVERS") : "NOT DETECTED",
			        c.detected ? (c.sizeband == 2 ? " (large)" : c.sizeband == 1 ? " (medium)" : " (few)") : "",
			        c.detected ? c.dettime / 3600 : 0, c.detected ? (c.dettime / 60) % 60 : 0, c.warnmin,
			        c.detected && c.bombers ? ", bombers reported" : "");
	}
	if (dbg)
	{
		int nd = 0; for (int i = 0; i < red_ncontacts; i++) nd += red_contacts[i].detected;
		fprintf(stderr, "[redintel] %s: %d of %d UN raids in the Red picture\n", where, nd, red_ncontacts);
	}
}

/* ---------------------------------------------------------------------------------------------------------------
   R4 -- the Red plan (BoB RAF model, PO 2026-10-02: "the initiative is on the other side; early detection, readiness,
   and sustained, not unsustainable, effort to protect their military and infrastructure assets while causing
   attrition").

   Regiments = MA's own MiG units (MMC.migsquad[], historical type/start date/strength/skill), each in a READINESS
   state for the session and given a TASK: intercept a detected raid, fly a standing patrol over MiG Alley, or
   nothing. Feasibility is geometry and time: the report reaches the command, the regiment needs its readiness
   delay to get airborne, climbs, and flies from the Yalu bases (Antung, inside the Manchurian sanctuary) to the
   raid's target. Effort costs FATIGUE, which cuts serviceable aircraft and pilot skill next session; resting
   (Released) recovers it. MODEL ASSUMPTIONS, flagged for tuning against the game's balance: readiness delays
   (Released: none; Available 15 min; Readiness 5 min; Standby 2 min -- the BoB-style ladder), MiG-15 cruise
   850 km/h plus 4 min climb, combat radius 350 km from Antung (MiG operations stayed north of Pyongyang in
   early 1951), fatigue +5/+10/+15 for Available/Readiness/Standby, +25 per intercept, +35 for a patrol, -30 rest.
   --------------------------------------------------------------------------------------------------------------- */
static const int red_delay_s[4]	= { 999999, 15 * 60, 5 * 60, 2 * 60 };
static const int red_fat_ready[4]	= { -30, 5, 10, 15 };
static const char* const red_ready_name[4] = { "Released", "Available", "Readiness", "Standby" };

static int red_regiment_usable(int r)
{
	if (r < 0 || r >= RED_MAXREG) return 0;
	const auto& m = MMC.migsquad[r];
	return m.ac_total_avail > 0 && m.startdate <= MMC.currdate;
}
int ma_red_serviceable(int r)	/* aircraft a regiment can put up this session */
{
	if (!red_regiment_usable(r)) return 0;
	int f = g_red.reg[r].fatigue;
	int n = MMC.migsquad[r].ac_total_avail * (200 - f) / 200;	/* exhausted regiment: half strength */
	return n < 0 ? 0 : n;
}

enum RedFeas { RED_RF_NONE = 0, RED_RF_BEFORE_TARGET, RED_RF_ON_EGRESS, RED_RF_TOO_LATE, RED_RF_OUT_OF_RANGE, RED_RF_NOT_READY };
static const char* const red_feas_name[] = { "-", "before target", "on egress", "too late", "out of range", "released" };

/* when could regiment r meet contact k, and is that before the bombs fall? */
static int red_feasibility(int r, int k, int* arrive)
{
	*arrive = 0;
	if (k < 0 || k >= red_ncontacts || !red_contacts[k].detected) return RED_RF_NONE;
	int rd = g_red.reg[r].readiness;
	if (rd == RED_RR_RELEASED) return RED_RF_NOT_READY;
	ItemBasePtr antung = Persons2::ConvertPtrUID(UID_AfRdAntung);
	if (!antung) return RED_RF_NONE;
	const RedContact& c = red_contacts[k];
	double km = red_km(c.tgtx - antung->World.X, c.tgtz - antung->World.Z);
	if (km > 350.0) return RED_RF_OUT_OF_RANGE;
	int t = c.dettime + red_delay_s[rd] + 4 * 60 + (int)(km / 850.0 * 3600.0);
	*arrive = t;
	if (t <= c.tot) return RED_RF_BEFORE_TARGET;
	if (t <= c.tot + 10 * 60) return RED_RF_ON_EGRESS;
	return RED_RF_TOO_LATE;
}

/* The automatic Red plan (the suggestion the player starts from, and the plan R5-R9 tests fly):
   - standing patrol over MiG Alley by the freshest regiment once Sabres are back (they would otherwise sweep it),
   - each detected raid, largest warning first, gets the freshest regiment that can meet it before its target,
   - a regiment above 70 fatigue is released to recover -- sustained, not unsustainable, effort. */
void ma_red_auto_plan(const char* where)
{
	if (!g_red.active) return;
	int order[RED_MAXREG], n = 0;
	for (int r = 0; r < RED_MAXREG; r++)
	{
		g_red.reg[r].task = RED_RT_NONE; g_red.reg[r].contact = -1;
		if (!red_regiment_usable(r)) continue;
		g_red.reg[r].readiness = g_red.reg[r].fatigue > 70 ? RED_RR_RELEASED : RED_RR_READINESS;
		if (g_red.reg[r].readiness != RED_RR_RELEASED) order[n++] = r;
	}
	for (int i = 1; i < n; i++)		/* freshest first */
		for (int j = i; j > 0 && g_red.reg[order[j]].fatigue < g_red.reg[order[j - 1]].fatigue; j--)
		{ int t = order[j]; order[j] = order[j - 1]; order[j - 1] = t; }
	int used[RED_MAXREG] = { 0 };
	int sabres = 0;
	for (int k = 0; k < red_ncontacts; k++)
	{
		int sq = (int)Todays_Packages.pack[red_contacts[k].pack].wave[0].group[Profile::GRPSTRIKE].squadron;
		if (sq == SQ_F86A || sq == SQ_F86B) sabres = 1;
	}
	if (sabres && n > 1) { g_red.reg[order[0]].task = RED_RT_BARCAP; used[0] = 1; }
	int ks[RED_MAXCONTACTS], nk = 0;
	for (int k = 0; k < red_ncontacts; k++) if (red_contacts[k].detected) ks[nk++] = k;
	for (int i = 1; i < nk; i++)
		for (int j = i; j > 0 && red_contacts[ks[j]].warnmin > red_contacts[ks[j - 1]].warnmin; j--)
		{ int t = ks[j]; ks[j] = ks[j - 1]; ks[j - 1] = t; }
	for (int i = 0; i < nk; i++)
		for (int o = 0; o < n; o++)
		{
			if (used[o]) continue;
			int arr; int f = red_feasibility(order[o], ks[i], &arr);
			if (f == RED_RF_BEFORE_TARGET || f == RED_RF_ON_EGRESS)
			{
				g_red.reg[order[o]].task = RED_RT_INTERCEPT; g_red.reg[order[o]].contact = (signed char)ks[i]; used[o] = 1;
				break;
			}
		}
	/* untasked regiments stand at Available: cheaper than Readiness, still a 15-minute reserve */
	for (int o = 0; o < n; o++)
		if (!used[o]) g_red.reg[order[o]].readiness = RED_RR_AVAILABLE;
	if (getenv("MA_TRACE_RED"))
	{
		for (int r = 0; r < RED_MAXREG; r++)
		{
			if (!red_regiment_usable(r)) continue;
			const RedRegimentState& g = g_red.reg[r];
			int arr = 0, f = g.task == RED_RT_INTERCEPT ? red_feasibility(r, g.contact, &arr) : RED_RF_NONE;
			fprintf(stderr, "[redplan] %s regiment %d (%s, %d ac, %d serviceable, fatigue %d): %s, %s%s",
			        where, r, MMC.migsquad[r].actype == PT_MIG15BIS ? "MiG-15bis" : "MiG-15", MMC.migsquad[r].ac_total_avail,
			        ma_red_serviceable(r), g.fatigue, red_ready_name[(int)g.readiness],
			        g.task == RED_RT_BARCAP ? "BARCAP over MiG Alley" : g.task == RED_RT_INTERCEPT ? "intercept raid " : "no task",
			        g.task == RED_RT_INTERCEPT ? "" : "\n");
			if (g.task == RED_RT_INTERCEPT)
				fprintf(stderr, "%d (pack %d) -> %s at %02d:%02d (ToT %02d:%02d)\n", g.contact, red_contacts[(int)g.contact].pack,
				        red_feas_name[f], arr / 3600, (arr / 60) % 60, red_contacts[(int)g.contact].tot / 3600, (red_contacts[(int)g.contact].tot / 60) % 60);
		}
	}
}

/* After a session is flown: effort becomes fatigue (or rest recovers it). Called before the next autogen. */
void ma_red_end_session(void)
{
	if (!g_red.active) return;
	for (int r = 0; r < RED_MAXREG; r++)
	{
		if (!red_regiment_usable(r)) continue;
		RedRegimentState& g = g_red.reg[r];
		int d = red_fat_ready[(int)g.readiness];
		if (g.task == RED_RT_INTERCEPT) { d += 25; g.sorties++; }
		if (g.task == RED_RT_BARCAP)    { d += 35; g.sorties++; }
		int f = (int)g.fatigue + d; if (f < 0) f = 0; if (f > 100) f = 100;
		g.fatigue = (unsigned char)f;
		if (getenv("MA_TRACE_RED"))
			fprintf(stderr, "[redplan] end of session: regiment %d %s %s -> fatigue %d (sorties %d)\n", r,
			        red_ready_name[(int)g.readiness], g.task == RED_RT_BARCAP ? "patrol" : g.task == RED_RT_INTERCEPT ? "intercept" : "idle",
			        g.fatigue, g.sorties);
	}
}

extern "C" int ma_red_fatigue_sum(void)	/* test aid: total regiment fatigue (save/load round-trip check) */
{
	int t = 0; for (int r = 0; r < RED_MAXREG; r++) t += g_red.reg[r].fatigue; return t;
}

/* ---------------------------------------------------------------------------------------------------------------
   R5 -- C API for the Red command panel (CRedCommand, COMIT_E.CPP). Raids are numbered in the order the Red side
   learnt of them; only detected raids exist for the panel (PO: only what radar picks up).
   --------------------------------------------------------------------------------------------------------------- */
static int red_detected_order(int* ks)	/* detected contacts sorted by report time */
{
	int nk = 0;
	for (int k = 0; k < red_ncontacts; k++) if (red_contacts[k].detected) ks[nk++] = k;
	for (int i = 1; i < nk; i++)
		for (int j = i; j > 0 && red_contacts[ks[j]].dettime < red_contacts[ks[j - 1]].dettime; j--)
		{ int t = ks[j]; ks[j] = ks[j - 1]; ks[j - 1] = t; }
	return nk;
}
extern "C" int ma_red_nraids(void) { int ks[RED_MAXCONTACTS]; return red_detected_order(ks); }

extern "C" int ma_red_raid_text(int i, char* buf, int n)
{
	int ks[RED_MAXCONTACTS]; int nk = red_detected_order(ks);
	if (i < 0 || i >= nk) { if (n) buf[0] = 0; return 0; }
	const RedContact& c = red_contacts[ks[i]];
	/* the report: who saw it, when, how many (a band, not a count), what kind if observers could tell, which way */
	ItemBasePtr antung = Persons2::ConvertPtrUID(UID_AfRdAntung);
	double km = antung ? red_km(c.tgtx - antung->World.X, c.tgtz - antung->World.Z) : 0;
	snprintf(buf, n, "%d  %02d:%02d  %s  %s%s  target %d km",
	         i + 1, c.dettime / 3600, (c.dettime / 60) % 60, c.source == 1 ? "radar" : "observers",
	         c.sizeband == 2 ? "large" : c.sizeband == 1 ? "medium" : "a few aircraft",
	         c.bombers ? ", bombers" : "", (int)(km + 0.5));
	return 1;
}

extern "C" int ma_red_regiment_usable(int r) { return red_regiment_usable(r); }
extern "C" int ma_red_regiment_text(int r, char* buf, int n)
{
	if (!red_regiment_usable(r)) { if (n) buf[0] = 0; return 0; }
	snprintf(buf, n, "%d  %s  %d/%d  fat %d", r + 1,
	         MMC.migsquad[r].actype == PT_MIG15BIS ? "MiG-15bis" : "MiG-15",
	         ma_red_serviceable(r), MMC.migsquad[r].ac_total_avail, g_red.reg[r].fatigue);
	return 1;
}
extern "C" int  ma_red_get_readiness(int r)        { return r >= 0 && r < RED_MAXREG ? g_red.reg[r].readiness : 0; }
extern "C" void ma_red_set_readiness(int r, int v) { if (r >= 0 && r < RED_MAXREG && v >= 0 && v <= 3) g_red.reg[r].readiness = (signed char)v; }

/* task as the panel's combo index: 0 none, 1 BARCAP, 2+i intercept raid i (panel numbering) */
extern "C" int ma_red_get_task(int r)
{
	if (r < 0 || r >= RED_MAXREG) return 0;
	const RedRegimentState& g = g_red.reg[r];
	if (g.task == RED_RT_BARCAP) return 1;
	if (g.task == RED_RT_INTERCEPT)
	{
		int ks[RED_MAXCONTACTS]; int nk = red_detected_order(ks);
		for (int i = 0; i < nk; i++) if (ks[i] == g.contact) return 2 + i;
	}
	return 0;
}
extern "C" void ma_red_set_task(int r, int v)
{
	if (r < 0 || r >= RED_MAXREG) return;
	RedRegimentState& g = g_red.reg[r];
	int ks[RED_MAXCONTACTS]; int nk = red_detected_order(ks);
	g.task = RED_RT_NONE; g.contact = -1;
	if (v == 1) g.task = RED_RT_BARCAP;
	else if (v >= 2 && v - 2 < nk) { g.task = RED_RT_INTERCEPT; g.contact = (signed char)ks[v - 2]; }
	if (g.task != RED_RT_NONE && g.readiness == RED_RR_RELEASED) g.readiness = RED_RR_AVAILABLE;	/* a tasked regiment cannot be stood down */
}
extern "C" int ma_red_outcome_text(int r, char* buf, int n)
{
	if (!red_regiment_usable(r)) { if (n) buf[0] = 0; return 0; }
	const RedRegimentState& g = g_red.reg[r];
	if (g.task == RED_RT_BARCAP) { snprintf(buf, n, "patrol"); return 1; }
	if (g.task != RED_RT_INTERCEPT) { snprintf(buf, n, g.readiness == RED_RR_RELEASED ? "resting" : "in reserve"); return 1; }
	int arr = 0, f = red_feasibility(r, g.contact, &arr);
	static const char* const shortname[] = { "-", "in time", "egress", "too late", "no range", "released" };
	if (arr && (f == RED_RF_BEFORE_TARGET || f == RED_RF_ON_EGRESS)) snprintf(buf, n, "%s %02d:%02d", shortname[f], arr / 3600, (arr / 60) % 60);
	else if (f >= 0 && f <= 5) snprintf(buf, n, "%s", shortname[f]);
	return 1;
}
extern "C" void ma_red_auto_plan_c(void) { ma_red_auto_plan("panel"); }
extern "C" void ma_red_session_title(char* buf, int n)
{
	static const char* const mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int day = (int)((MMC.currdate - (int)JAN1951) / (int)SECSPERDAY);	/* 0 = 1 Jan 1951 */
	static const int mdays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int m = 0; while (m < 11 && day >= mdays[m]) { day -= mdays[m]; m++; }
	static const char* const per[] = { "Morning", "Midday", "Afternoon" };
	int p = MMC.debrief.currperiod; if (p < 0 || p > 2) p = 0;
	snprintf(buf, n, "AIR DEFENCE COMMAND    %d %s 1951, %s", day + 1, mon[m], per[p]);
}

/* ---------------------------------------------------------------------------------------------------------------
   R6 -- the Red plan flies. MA builds the Communist air battle in Persons2::FindNextBf (TANK.CPP): regiment 0
   (squad SQ_M15_0) holds over China and flies the China patrols, regiment 1 (SQ_M15_1) launches from the Red
   airfields; a "MiG day type" decides direct attacks vs. barrier patrols, and LaunchMigPatrol aims each patrol at a
   RANDOM UN package. In a Red campaign the plan decides instead: the day type follows the tasks, patrols go after
   the raids regiments were assigned, a Released regiment stays on the ground, formations are the serviceable
   strength, and the player leads the regiment he tasked (only the two active regiments have battlefields).
   --------------------------------------------------------------------------------------------------------------- */
extern "C" int ma_red_regiment_flies(int r)
{
	return red_regiment_usable(r) && g_red.reg[r].readiness != RED_RR_RELEASED;
}
extern "C" void ma_red_plan_flags(int* barcap, int* intercepts)
{
	*barcap = *intercepts = 0;
	for (int r = 0; r < RED_MAXREG; r++)
	{
		if (!ma_red_regiment_flies(r)) continue;
		if (g_red.reg[r].task == RED_RT_BARCAP) (*barcap)++;
		if (g_red.reg[r].task == RED_RT_INTERCEPT) (*intercepts)++;
	}
}
/* the package the next MiG patrol should go after: the planned intercepts in turn (-1: none planned) */
extern "C" int ma_red_next_intercept_pack(void)
{
	static int turn = 0;
	int packs[RED_MAXREG], n = 0;
	for (int r = 0; r < RED_MAXREG; r++)
		if (ma_red_regiment_flies(r) && g_red.reg[r].task == RED_RT_INTERCEPT && g_red.reg[r].contact >= 0
		    && g_red.reg[r].contact < red_ncontacts)
			packs[n++] = red_contacts[(int)g_red.reg[r].contact].pack;
	if (!n) return -1;
	return packs[(turn++) % n];
}
/* which of the two battle regiments (0/1) the player leads: a tasked one that can reach its raid in time first,
   then any tasked one, then one that is not released; -1 if neither flies */
extern "C" int ma_red_player_regiment(void)
{
	int best = -1, score = -1;
	for (int r = 0; r < 2; r++)
	{
		if (!ma_red_regiment_flies(r)) continue;
		int s = 1;
		if (g_red.reg[r].task == RED_RT_BARCAP) s = 2;
		if (g_red.reg[r].task == RED_RT_INTERCEPT)
		{
			int arr; int f = red_feasibility(r, g_red.reg[r].contact, &arr);
			s = (f == RED_RF_BEFORE_TARGET || f == RED_RF_ON_EGRESS) ? 4 : 3;
		}
		if (s > score) { score = s; best = r; }
	}
	return best;
}
extern "C" int ma_red_serviceable_c(int r) { return ma_red_serviceable(r); }
/* where the player's flight goes: the target of his regiment's assigned raid, or MiG Alley (Sinanju) for a patrol */
extern "C" int ma_red_player_target(void)
{
	int pr = ma_red_player_regiment();
	if (pr >= 0 && g_red.reg[pr].task == RED_RT_INTERCEPT && g_red.reg[pr].contact >= 0 && g_red.reg[pr].contact < red_ncontacts)
		return (int)Todays_Packages.pack[red_contacts[(int)g_red.reg[pr].contact].pack].packagetarget;
	return (int)UID_AfRdSinanju;
}
extern "C" int ma_red_regiment_skill(int r) { return (r >= 0 && r < RED_MAXREG) ? (int)MMC.migsquad[r].generalskill : 0; }
