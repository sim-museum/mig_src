#if !defined(AFX_TITLEBAR_H__891B54A3_4D5E_11D2_83A5_00004760763D__INCLUDED_)
#define AFX_TITLEBAR_H__891B54A3_4D5E_11D2_83A5_00004760763D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TitleBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// TitleBar dialog

class TitleBar : public CRToolBar
{
// Construction
public:
	TitleBar(CWnd* pParent = NULL);   // standard constructor
	void Redraw();
	int m_xoffset;
	int m_yoffset;

// Dialog Data
	//{{AFX_DATA(TitleBar)
	enum { IDD = IDD_TITLEBAR };
		// NOTE: the ClassWizard will add data members here
	CRButton	m_IDC_TITLE;
	CRButton	m_IDC_DATE;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TitleBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(TitleBar)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	afx_msg FileNum OnGetArt();
	afx_msg long OnGetXYOffset();
	afx_msg CFont* OnGetGlobalFont(int fontnum);
	afx_msg BYTE* OnGetFile(int filenum);
	afx_msg void OnReleaseLastFile();
	afx_msg BOOL OnGetX2Flag();
#if defined(MA_LINUX)
public:
	/* GOLDSCREENS-MA-1 S10: the plate. OnEraseBkgnd below decodes FIL_TOOL_HORIZONTAL and
	   blits it, and S9 found this port never calls it -- ON_WM_ERASEBKGND() is #defined to
	   nothing, so all 13 registrations vanish and the base stub returns TRUE ("already
	   erased"). A public entry so the map's draw can call the real handler behind a flag,
	   rather than reimplementing a paint the game already has. */
	BOOL MaPaintBkgnd(CDC* pDC) { return OnEraseBkgnd(pDC); }
protected:
	/* GOLDSCREENS-MA-1 S8: CRToolBar::OnRowanMessage answers WM_GETGLOBALFONT with 0
	   ("icon buttons need no font") -- true for the filter/main toolbars, false for THIS
	   one, whose two buttons are the words "MIG ALLEY" and the date. Without an override
	   TitleBar::OnGetGlobalFont below has never run. Same shape as RMdlDlg.h's note. */
	virtual LRESULT OnRowanMessage(UINT msg, WPARAM w, LPARAM l);
#endif
	MSG2_0(OnGetArt);
	MSG2_0(OnGetXYOffset);
	MSG2_1(OnGetGlobalFont);
//	MSG2_1(OnGetFile);
//	MSG2_0v(OnReleaseLastFile);
//	MSG2_0(OnGetX2Flag);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TITLEBAR_H__891B54A3_4D5E_11D2_83A5_00004760763D__INCLUDED_)
