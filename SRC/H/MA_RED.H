#ifndef MA_RED_INCLUDED
#define MA_RED_INCLUDED
/* EPIC-MA-RED (port addition, MA_LINUX): the single-player Communist campaign.
   Ids in 0xEF00-0xEFFF are the port's own: strings are served by bob_load_string's extra table
   (bob_resources.cpp), controls are injected into the installed dialog templates (ma_dlgtmpl.cpp). */
enum
{
	IDS_RED_EXTRA_FIRST		= 0xEF00,
	IDS_L_UNSTRAT			= 0xEF00,	/* +0 "Historical", +1 "Spring Offensive Tutorial" -- RESCOMBO(UNSTRAT,2) */
	IDS_RED_LABEL_UNSTRAT	= 0xEF02,	/* "UN Strategy" */
	IDS_RED_EXTRA_LAST		= 0xEFFF,

	IDC_RSTATIC_UNSTRATEGY	= 0xEF10,	/* Preferences > Game (IDD_SGAME 274): label */
	IDC_CBO_UNSTRATEGY		= 0xEF11,	/* Preferences > Game: Historical / Spring Offensive Tutorial */

	IDD_REDCOMMAND			= 0xEF20,	/* the Red command panel (template built entirely by ma_dlgtmpl) */
	IDC_RED_TITLE			= 0xEF30,
	IDC_RED_RAIDHDR			= 0xEF31,
	IDC_RED_REGHDR			= 0xEF32,
	IDC_RED_SUMMARY			= 0xEF33,
	IDC_RED_RAID0			= 0xEF40,	/* .. +5 */
	IDC_RED_REGNAME0		= 0xEF50,	/* .. +4 */
	IDC_RED_READY0			= 0xEF60,	/* .. +5 combos */
	IDC_RED_TASK0			= 0xEF70,	/* .. +5 combos */
	IDC_RED_OUTCOME0		= 0xEF80,	/* .. +5 */
	IDC_RED_AUTOPLAN		= 0xEF90,	/* button */
	RED_PANEL_RAIDS			= 6,
	RED_PANEL_REGS			= 5,
	IDC_RED_HDR_READY		= 0xEF34,
	IDC_RED_HDR_TASK		= 0xEF35,
	IDC_RED_HDR_OUTCOME		= 0xEF36,
};

#ifdef __cplusplus
extern "C" {
#endif
int		ma_red_active(void);					/* the player commands the Communist side */
void	ma_red_set_active(int on);
void	ma_red_save(const char* savepath);		/* sidecar "<savepath>.red" */
void	ma_red_load(const char* savepath);
int		ma_red_fatigue_sum(void);				/* test aid */
/* R5: the Red command panel */
int		ma_red_nraids(void);
int		ma_red_raid_text(int i, char* buf, int n);
int		ma_red_regiment_usable(int r);
int		ma_red_regiment_text(int r, char* buf, int n);
int		ma_red_get_readiness(int r);
void	ma_red_set_readiness(int r, int v);
int		ma_red_get_task(int r);
void	ma_red_set_task(int r, int v);
int		ma_red_outcome_text(int r, char* buf, int n);
void	ma_red_auto_plan_c(void);
void	ma_red_session_title(char* buf, int n);
/* R6: the plan in the battle (TANK.CPP / Persons3.cpp) */
int		ma_red_regiment_flies(int r);
void	ma_red_plan_flags(int* barcap, int* intercepts);
int		ma_red_next_intercept_pack(void);
int		ma_red_player_regiment(void);
int		ma_red_serviceable_c(int r);
int		ma_red_player_target(void);
int		ma_red_regiment_skill(int r);
/* R7: identities, rosters, report */
const char*	ma_red_regiment_name(int r);
const char*	ma_red_pilot_name(int r, int i);
int		ma_red_last_report(char* buf, int n);
int		ma_red_player_squad(void);
#ifdef __cplusplus
}
#endif
#endif
