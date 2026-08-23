#if !defined(AFX_MAPDLG_H__DED33C01_22B7_11D1_A1F0_444553540000__INCLUDED_)
#define AFX_MAPDLG_H__DED33C01_22B7_11D1_A1F0_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MapDlg.h : header file
//
#ifndef	NDEBUG
struct	DebugLineStats
{
	int	debuglineRGB[8];
	int	lastline,lastbox;

	struct	DebugBoxMem
	{
		SWord	sx,ex;
		SLong	sy:12,ey:12,
				linenum:3,
				spare:5;
	}
	boxmem[300];
	void	Set(CRect& rect)
	{
		boxmem[lastbox].sx=rect.left;boxmem[lastbox].ex=rect.right;
		boxmem[lastbox].sy=rect.top;boxmem[lastbox].ey=rect.bottom;
		boxmem[lastbox].linenum=lastline;
		if (lastbox<300)	lastbox++;
	}
};
extern	DebugLineStats	debugboxmem;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapDlg dialog

class CMapDlg : public CDialog
{
// Construction
public:
	UINT	maptimer;
	void InvalidateLine(CPoint point1,CPoint point2);
	void InvalidateMovedLine(UniqueID thiswp,UniqueID nextwp,BOOL japanwp=FALSE);
	void InvalidateLine(UniqueID thiswp,UniqueID nextwp);
	void InvalidateRoute(int	pack, int wave, int	group);
	void InvalidateRoutes(UniqueID onlythrough=UID_NULL);
	int		FindMapItem(CPoint point);
#ifdef MA_LINUX
	/* S95 (PO-3): the port has no Windows message queue, so the map idle loop must
	   call the map's own click handlers directly -- and they are protected. This is
	   the one public seam for that; it does nothing the mouse would not do. */
	void	MaDriveClick(CPoint point);
	/* S172 (K8): the same seam for a DRAG. A drag is not a click with a longer press --
	   CMapDlg::OnMouseMove is where the waypoint's world position is recomputed, and it only
	   engages once the pointer leaves a 3px box around the press point. MaDriveClick issues
	   down+up in one tick precisely to keep m_bDragging FALSE; this issues the moves. */
	void	MaDriveDrag(CPoint from, CPoint to);
	/* S189 (PO-55): the interactive drag, one edge per tick, driven from the front-end loop.
	   MaDriveDrag above synthesises a whole drag for the gates; these deliver a real one. */
	int		MaCanDragAt(CPoint point);
	void	MaDriveDown(CPoint point);
	void	MaDriveMove(CPoint point);
	int		MaDriveUp(CPoint point);
#endif
	void	InvalidateIconText(CDC* pDC,int px,int py,CString s);
	void	InvalidateDragIcon(int ox,int oy,int px,int py);
	void	InvalidateIcon(int px,int py);
	int m_buttonid;
	int m_hintid;
	CPoint m_point;
	CPoint m_oldpoint;
	BOOL m_LButtonDown;
	BOOL m_bDragging;
//	BOOL Create( CWnd* pParentWnd);
	CMapDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMapDlg)
	enum { IDD = IDD_MAP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	void	ResetIcon(UniqueID wpuid);
	void	RedrawIcon(int oldbuttonid);
	RDialog*	TargetSelectSink;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	bool	AllowDragItem(int buttonid);
	void	OnClickItem(int buttonid);
	void	OnDragItem(int buttonid);
protected:

	// Generated message map functions
	//{{AFX_MSG(CMapDlg)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPDLG_H__DED33C01_22B7_11D1_A1F0_444553540000__INCLUDED_)
