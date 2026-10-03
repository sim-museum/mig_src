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
#endif
