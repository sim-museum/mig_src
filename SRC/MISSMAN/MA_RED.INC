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

struct RedCampaignState
{
	int		magic;			/* 'MRED' */
	int		version;
	int		active;			/* the player is the Communist commander */
	int		reserved[61];	/* later sprints: plan, detection, rosters -- the sidecar is versioned by size */
};
static RedCampaignState g_red = { 0x4445524d, 1, 0, { 0 } };

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
	if (n >= 3 * sizeof(int) && s.magic == 0x4445524d) g_red = s;
	else { RedCampaignState z = { 0x4445524d, 1, 0, { 0 } }; g_red = z; }
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
static RedContact red_contacts[RED_MAXCONTACTS];
static int red_ncontacts = 0;

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
