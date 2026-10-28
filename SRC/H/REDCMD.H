#ifndef REDCMD_INCLUDED
#define REDCMD_INCLUDED
/* EPIC-MA-RED R5: the Red command panel -- what the Communist air commander sees and decides each session.
   Opened from the map toolbar's Directives button when the campaign is Red (MAINTBAR.CPP); the template is
   port-defined (ma_dlgtmpl.cpp, IDD_REDCOMMAND); the model lives in MA_RED.INC behind the MA_RED.H C API. */
#include "MA_RED.H"

class CRedCommand : public RowanDialog
{
public:
	CRedCommand(CWnd* pParent = NULL);
	enum { IDD = IDD_REDCOMMAND };
	CRStatic	m_title, m_raidhdr, m_reghdr, m_summary, m_hdrready, m_hdrtask, m_hdroutcome;
	CRStatic	m_raid[RED_PANEL_RAIDS];
	CRStatic	m_regname[RED_PANEL_REGS], m_outcome[RED_PANEL_REGS];
	CRCombo		m_ready[RED_PANEL_REGS], m_task[RED_PANEL_REGS];
	CREdtBt		m_auto;
	CRStatic	m_unitlbl;	/* MA-RED-ALL-1 (a) */
	CRCombo		m_unit, m_pos;
protected:
	int		m_rowreg[RED_PANEL_REGS];	/* panel row -> regiment index, -1 empty */
	int		m_filling;					/* ignore combo events while the panel writes them */
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void	Refresh(int rebuild);
	afx_msg void OnComboChanged(int id);
	afx_msg void OnClickedAutoplan();
	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()
};
/* MA-RED-ALL-1 (a2): the Red ready room -- replaces the UN frag list in a Red campaign (FULLPANE FragInit) */
class CRedFrag : public RowanDialog
{
public:
	CRedFrag(CWnd* pParent = NULL);
	enum { IDD = IDD_REDFRAG };
	CRStatic	m_title, m_sub, m_seat[4], m_name[4];
	CREdtBt		m_pick[4];
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void	Refresh();
	afx_msg void OnClickedPick0();
	afx_msg void OnClickedPick1();
	afx_msg void OnClickedPick2();
	afx_msg void OnClickedPick3();
	void	Pick(int seat);
	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()
};
#endif
