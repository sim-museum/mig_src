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
};

#ifdef __cplusplus
extern "C" {
#endif
int		ma_red_active(void);					/* the player commands the Communist side */
void	ma_red_set_active(int on);
void	ma_red_save(const char* savepath);		/* sidecar "<savepath>.red" */
void	ma_red_load(const char* savepath);
#ifdef __cplusplus
}
#endif
#endif
