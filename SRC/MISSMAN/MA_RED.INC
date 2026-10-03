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
