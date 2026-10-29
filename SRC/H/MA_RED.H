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
	IDS_RED_BEGIN			= 0xEF03,	/* campaign screen button */
	IDS_RED_END_DECISIVE	= 0xEF04,	/* end-of-campaign texts, Red perspective */
	IDS_RED_END_WIN			= 0xEF05,
	IDS_RED_END_LOSS		= 0xEF06,
	IDS_RED_END_COLLAPSE	= 0xEF07,
	IDS_RED_ONLY_SO51		= 0xEF08,	/* MA-CAMPAIGN-SIDE-1: "Begin as Communists" on a UN-only campaign */
	IDS_RED_EXTRA_LAST		= 0xEFFF,

	IDC_RSTATIC_UNSTRATEGY	= 0xEF10,	/* Preferences > Game (IDD_SGAME 274): label */
	IDC_CBO_UNSTRATEGY		= 0xEF11,	/* Preferences > Game: Historical / Spring Offensive Tutorial */

	IDD_REDFRAG				= 0xEF21,	/* MA-RED-ALL-1 (a2): the Red ready room -- the player's MiG flight, names and seats */
	IDC_RFR_TITLE			= 0xEFA0,
	IDC_RFR_SUB				= 0xEFA1,
	IDC_RFR_SEAT0			= 0xEFB0,	/* .. +3 */
	IDC_RFR_NAME0			= 0xEFC0,	/* .. +3 */
	IDC_RFR_PICK0			= 0xEFD0,	/* .. +3 */
	IDD_REDCOMMAND			= 0xEF20,	/* the Red command panel (template built entirely by ma_dlgtmpl) */
	IDC_RED_TITLE			= 0xEF30,
	IDC_RED_RAIDHDR			= 0xEF31,
	IDC_RED_REGHDR			= 0xEF32,
	IDC_RED_SUMMARY			= 0xEF33,
	IDC_RED_UNITLBL			= 0xEF37,	/* MA-RED-ALL-1 (a): "Your squadron" */
	IDC_RED_UNIT			= 0xEF38,	/* Automatic / Soviet / Chinese / North Korean */
	IDC_RED_POS				= 0xEF39,	/* the player's place in his flight: leader, No. 2, No. 3, No. 4 */
	IDC_RED_RAID0			= 0xEF40,	/* .. +5 */
	IDC_RED_REGNAME0		= 0xEF50,	/* .. +4 */
	IDC_RED_READY0			= 0xEF60,	/* .. +5 combos */
	IDC_RED_TASK0			= 0xEF70,	/* .. +5 combos */
	IDC_RED_OUTCOME0		= 0xEF80,	/* .. +5 */
	IDC_RED_AUTOPLAN		= 0xEF90,	/* button */
	RED_PANEL_RAIDS			= 5,	/* MA-RED-ALL-1 (a): the 6th line is the "Your squadron" row */
	RED_PANEL_REGS			= 5,
	IDC_RED_HDR_READY		= 0xEF34,
	IDC_RED_HDR_TASK		= 0xEF35,
	IDC_RED_HDR_OUTCOME		= 0xEF36,
};

#ifdef __cplusplus
extern "C" {
#endif
int		ma_red_active(void);					/* the player commands the Communist side */
void		ma_red_request_scripted(int);
void		ma_red_take_scripted_request(void);
int		ma_red_scripted(void);				/* MA-RED-ALL-1 (c): a scripted squadron campaign flown in a MiG flight */
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
/* MA-RED-ALL-1 (a): the player's flight nationality (0 automatic, 1 Soviet, 2 Chinese, 3 North Korean) and its skill */
int		ma_red_get_player_nation(void);
void	ma_red_set_player_nation(int n);
int		ma_red_player_flight_skill(int seq);
int		ma_red_get_player_pos(void);			/* 0 leader .. 3 No. 4 */
void	ma_red_set_player_pos(int p);
int		ma_red_player_seat(int pr);				/* the position, clamped to the flight he will actually fly */
/* MA-RED-ALL-1 (a2): the Red ready room */
void	ma_red_flight_title(char* buf, int n);		/* "<regiment> -- <task>" */
void	ma_red_flight_sub(char* buf, int n);		/* nationality and skill line */
void	ma_red_flight_pilot(int seat, char* buf, int n);	/* seat 0..3: the pilot's name (the player's own seat: his name) */
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
/* R8: entry and outcome */
void	ma_red_request(int on);
int		ma_red_take_request(void);
int		ma_red_outcome(int* held, char* names, int n);
/* EPIC-MP-CAMPAIGN */
int		ma_mp_red_humans(void);
#ifdef __cplusplus
}
#endif
#endif
