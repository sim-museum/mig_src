/*
	 MiG Alley
	 Copyright (C) 1998, 1999, 2000, 2001 Empire Interactive (Europe) Ltd,
	 677 High Road, North Finchley, London N12 0DA

	 Please see the document licence.doc for the full licence agreement

2. LICENCE
 2.1 	
 	Subject to the provisions of this Agreement we now grant to you the 
 	following rights in respect of the Source Code:
  2.1.1 
  	the non-exclusive right to Exploit  the Source Code and Executable 
  	Code on any medium; and 
  2.1.2 
  	the non-exclusive right to create and distribute Derivative Works.
 2.2 	
 	Subject to the provisions of this Agreement we now grant you the
	following rights in respect of the Object Code:
  2.2.1 
	the non-exclusive right to Exploit the Object Code on the same
	terms and conditions set out in clause 3, provided that any
	distribution is done so on the terms of this Agreement and is
	accompanied by the Source Code and Executable Code (as
	applicable).

3. GENERAL OBLIGATIONS
 3.1 
 	In consideration of the licence granted in clause 2.1 you now agree:
  3.1.1 
	that when you distribute the Source Code or Executable Code or
	any Derivative Works to Recipients you will also include the
	terms of this Agreement;
  3.1.2 
	that when you make the Source Code, Executable Code or any
	Derivative Works ("Materials") available to download, you will
	ensure that Recipients must accept the terms of this Agreement
	before being allowed to download such Materials;
  3.1.3 
	that by Exploiting the Source Code or Executable Code you may
	not impose any further restrictions on a Recipient's subsequent
	Exploitation of the Source Code or Executable Code other than
	those contained in the terms and conditions of this Agreement;
  3.1.4 
	not (and not to allow any third party) to profit or make any
	charge for the Source Code, or Executable Code, any
	Exploitation of the Source Code or Executable Code, or for any
	Derivative Works;
  3.1.5 
	not to place any restrictions on the operability of the Source 
	Code;
  3.1.6 
	to attach prominent notices to any Derivative Works stating
	that you have changed the Source Code or Executable Code and to
	include the details anddate of such change; and
  3.1.7 
  	not to Exploit the Source Code or Executable Code otherwise than
	as expressly permitted by  this Agreement.

questions about this file may be asked at http://www.simhq.com/
*/

// MapDlg.cpp : implementation file
//

#include "stdafx.h"
#include "comms.h"
#include "MIG.h"
#include "MapDlg.h"
#include "messages.h"
#include "fileman.h"
#include "MIGView.h"
#include "node.h"
#include "MILES.H"	// Linux port
#include "PROFILE.H"	// Linux port: CProfile
// Linux port: instantiate the UniqueIDField bitfield class (MIGVIEW.CPP:108 does the same;
// each TU that uses UniqueIDField must invoke MAKEFIELD at file scope)
MAKEFIELD(UniqueID,UID_NULL,IllegalSepID);
// Linux port: prelude defines F_SOUNDS (suppresses F_SOUNDS.G); supply the sound ID this file needs
static const FileNum FIL_SFX_OFFICE_PAPER3 = (FileNum)0x749b;

#ifdef _DEBUG
#define new DEBUG_NEW
#ifndef	THIS_FILE_DEFINED
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapDlg dialog

CMapDlg::CMapDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMapDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_LButtonDown=FALSE;
	m_buttonid=0;
	m_hintid=0;
	TargetSelectSink=NULL;
}


void CMapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapDlg, CDialog)
	//{{AFX_MSG_MAP(CMapDlg)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	ON_WM_DESTROY()
//	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapDlg message handlers

/* Attempt to create a template

BOOL CMapDlg::Create(CWnd * pParentWnd)
{
	int	nItems=1;
	
	int	nBufferSize;

	nBufferSize = sizeof(DLGTEMPLATE) + 6 + nItems * (sizeof(DLGITEMTEMPLATE) + 10);

	unsigned char * pBuffer = new unsigned char [nBufferSize];

	DLGTEMPLATE*	lpTmplt = (DLGTEMPLATE*)pBuffer;

	lpTmplt->style = WS_CHILD | WS_CAPTION;
	lpTmplt->dwExtendedStyle = 0;
	lpTmplt->cdit = nItems;
	lpTmplt->x = 0;
	lpTmplt->y = 0;
	lpTmplt->cx = 100;
	lpTmplt->cy = 100;

	WORD * lpZeros = (WORD*)(pBuffer+sizeof(DLGTEMPLATE));

	lpZeros[0] = lpZeros[1] = lpZeros[2] = 0;

	DLGITEMTEMPLATE * lpDIT;

	lpDIT = (DLGITEMTEMPLATE *)&lpZeros[3];

	lpDIT->style = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE;
	lpDIT->dwExtendedStyle = 0;
	lpDIT->x = 50;
	lpDIT->y = 50;
	lpDIT->cx = 15;
	lpDIT->cy = 15;
	lpDIT->id = IDC_BUTTON1;
	
//	lpDit = (DLGITEMTEMPLATE*)((unsigned char *)lpDIT+10);

	WORD * lpMoreZeros = (WORD*)((unsigned char *)lpDIT+sizeof(DLGTEMPLATE));
	lpMoreZeros[0] = 0xFFFF;
	lpMoreZeros[1] = 0x0080;
	lpMoreZeros[2] = lpMoreZeros[3] = lpMoreZeros[4] = 0x0000;

	ASSERT( CreateIndirect(lpTmplt, pParentWnd));
	return TRUE;

}
*/

void CMapDlg::OnClickItem(int m_buttonid)
{
#ifndef	MIG_DEMO_VER

//	RDialog::MakeTopDialog(CRect(100,100,0,0),
//		DialList(DialBox(FIL_NULL,
	typedef	HideEdgeControl::EdgeCtrl	Edge;
	typedef	HideEdgeControl	EDGE;
	_Miles.UIPlaySample(FIL_SFX_OFFICE_PAPER3);					//RDH 12Apr99
//DeadCode RJS 12Mar99 	_Miles.ProcessInterfaceSpot();		//RJS 17Nov98				  //RDH 03/03/99

	if (TargetSelectSink)
	{
		CMainFrame* main=(CMainFrame*)AfxGetMainWnd();
		Save_Data.mapfilters |= FILT_MAINWP;
		Save_Data.mapfilters |= FILT_SUBWP;
		main->m_toolbar1.SetFiltersFromSaveGame();
		Invalidate();

		TargetSelectSink->RefreshData(m_buttonid);
	}
	else
	if (m_buttonid>=WayPointBAND && m_buttonid<WayPointBANDEND)
	{
		int pack, wave, g;
		if (Profile::GetPackageFromWP((UniqueID)m_buttonid, pack, wave, g))
		{
//DEADCODE RDH 19/03/99 			RDialog::m_pView->SetHiLightInfo(pack,  wave, g, (UniqueID)m_buttonid);

			//we need to reset the highlight before opening dialogue
			//so that correct wp info is shown
			if (!MMC.indebrief)										  //RDH 24/06/99
			{
				RDialog::m_pView->SetHiLightInfo(pack,  wave, g, (UniqueID)m_buttonid); //RDH 25/03/99
				CMissionFolder::Make();
				CMissionFolder::OpenOneProfile(pack);
			}
//DEADCODE RDH 25/03/99 			RDialog::m_pView->SetHiLightInfo(pack,  wave, g, (UniqueID)m_buttonid); //RDH 19/03/99
//			CProfile* p;
//			RDialog::MakeTopDialog(CRect(50,450,0,0),
//				DialBox(FIL_MAP_PROFILE,p=new CProfile(true, pack))//,Edges(EDGE::ALIGN_MIN + EDGE::ACTIONS_SPLITTER,EDGE::ALIGN_MIN,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE))
//				);
//			p->SetWaveTabs(wave, true, -1);
		}

	}else
	{
		Invalidate();
		MainToolBar().OpenDossier((UniqueID)m_buttonid);
//		CTargetDossier::MakeSheet(CRect(180,180,0,0),(UniqueID)m_buttonid);
	}
#endif #ifndef	MIG_DEMO_VER

}

#ifdef MA_LINUX
/* S95 (PO-3): deliver a click to the map. Down+Up in one call, which is what a real
   click without intervening motion is -- m_bDragging stays FALSE, so we take the
   OnClickItem() path (dossier/recon) and never the drag path, and we never enter
   OnMouseMove(), whose GetDC() result is dereferenced unchecked in this port. */
void CMapDlg::MaDriveClick(CPoint point)
{
	OnLButtonDown(0, point);
	if (getenv("MA_TRACE_CLICK"))
		fprintf(stderr,"[mapclick] (%d,%d) hit id=%d(0x%x) band=%d\n",
		        (int)point.x,(int)point.y,m_buttonid,m_buttonid,
		        m_buttonid ? Persons2::getbandfromUID((UniqueID)m_buttonid) : -1);
	OnLButtonUp(0, point);
}

/* S189 (PO-55): the INTERACTIVE drag path -- the three edges of a real mouse drag, delivered one
   per tick from the front-end loop, as opposed to MaDriveDrag below which synthesises a whole drag
   in one call for the gates.
   The S95 comment above says down+up in one tick "never enters OnMouseMove(), whose GetDC() result
   is dereferenced unchecked in this port". That was a reasonable caution in S95 and it is now
   obsolete: S172's MaDriveDrag has been driving OnMouseMove through the very same path in
   route_drag.sh ever since, on every suite run, without a crash. pDC is used only by
   InvalidateIconText, and it survives. Dodging the handler cost the player the entire feature.
   MA_NO_MAP_DRAG=1 reverts to click-only behaviour. */
/* Is there a DRAGGABLE item under this point? A pure probe: FindMapItem returns an id and
   shadows m_hintid with a local, so it touches no member, and AllowDragItem only reads. This
   exists so the front-end can decide whether a gesture is a waypoint drag WITHOUT driving
   OnLButtonDown first -- driving it on every press would double-fire the down edge for an
   ordinary tap, which still goes through MaDriveClick. */
int CMapDlg::MaCanDragAt(CPoint point)
{
	int id = FindMapItem(point);
	return (id && AllowDragItem(id)) ? id : 0;
}

/* S190: the WORLD position at press, so the interactive path carries the same oracle the gate
   path does. Screen coordinates move with zoom and scroll; info_waypoint::World is what the
   flight actually flies, and it is the only honest evidence that a drag DID something. Held
   across the gesture rather than re-read at the drop: OnLButtonUp -> OnDragItem repaints and
   re-hit-tests, and m_buttonid can point at a different item by then (S172 lost a measurement
   to exactly that). */
static int  ma_dragOracleId = 0;
static long ma_dragOracleX  = 0, ma_dragOracleZ = 0;

void CMapDlg::MaDriveDown(CPoint point)
{
	OnLButtonDown(0, point);
	ma_dragOracleId = m_buttonid; ma_dragOracleX = 0; ma_dragOracleZ = 0;
	if (ma_dragOracleId) {
		ItemBasePtr _p = Persons2::ConvertPtrUID(UniqueID(ma_dragOracleId));
		if (_p) { ma_dragOracleX = (long)_p->World.X; ma_dragOracleZ = (long)_p->World.Z; }
	}
	if (getenv("MA_TRACE_CLICK"))
		fprintf(stderr,"[mapdrag] press (%d,%d) -> id=%d(0x%x) \"%s\" allowdrag=%d world=(%ld,%ld)\n",
		        (int)point.x,(int)point.y,m_buttonid,m_buttonid,
		        m_buttonid ? (const char*)(LPCTSTR)GetTargName((UniqueID)m_buttonid) : "",
		        m_buttonid ? (int)AllowDragItem(m_buttonid) : 0,
		        ma_dragOracleX, ma_dragOracleZ);
}
void CMapDlg::MaDriveMove(CPoint point)
{
	OnMouseMove(0x0001 /*MK_LBUTTON*/, point);
}
int CMapDlg::MaDriveUp(CPoint point)
{
	/* m_bDragging is the one bit separating a drag from a click, and OnLButtonUp clears it --
	   so read it BEFORE, or the caller can never tell which path ran. */
	int wasdragging = (int)m_bDragging;
	int id = m_buttonid;
	OnLButtonUp(0, point);
	long wx1 = ma_dragOracleX, wz1 = ma_dragOracleZ;
	if (ma_dragOracleId) {
		ItemBasePtr _p = Persons2::ConvertPtrUID(UniqueID(ma_dragOracleId));
		if (_p) { wx1 = (long)_p->World.X; wz1 = (long)_p->World.Z; }
	}
	/* NOT gated on MA_TRACE_CLICK: this line is the gate's evidence, and a gate that has to turn
	   on a debug flag to see whether the feature worked will one day be run without it. */
	fprintf(stderr,"[mapdrag] release (%d,%d) id=%d(0x%x) wasdragging=%d world (%ld,%ld) -> (%ld,%ld) moved=%d\n",
	        (int)point.x,(int)point.y,ma_dragOracleId,ma_dragOracleId,wasdragging,
	        ma_dragOracleX, ma_dragOracleZ, wx1, wz1,
	        (int)(wx1 != ma_dragOracleX || wz1 != ma_dragOracleZ));
	(void)id;
	ma_dragOracleId = 0;
	return wasdragging;
}

/* S172 (K8): drive a real waypoint DRAG -- press, move, release -- through the map's own
   handlers. Every step matters and none of it is re-implemented here:
     OnLButtonDown  sets m_buttonid via FindMapItem
     OnMouseMove    starts the drag only once the pointer leaves a 3px box around the press
                    point AND AllowDragItem() says the item is a waypoint, then recomputes
                    the item's World position from the cursor on EVERY move
     OnLButtonUp    calls OnDragItem(), which clamps the waypoint into the theatre, recalcs
                    the route and the waypoint times, and re-checks fuel (it can raise the
                    BINGO warning -- that is the game telling you the route is too long)
   Intermediate steps are driven rather than one jump, because the world position is
   recomputed per move: a single-step drag would exercise the drop but not the dragging. */
void CMapDlg::MaDriveDrag(CPoint from, CPoint to)
{
	enum { MA_DRAG_STEPS = 8 };
	OnLButtonDown(0, from);
	/* The WORLD position is the oracle, not the screen position: screen coordinates move with
	   zoom and scroll, and the thing that decides what the flight flies is w->World. Read it
	   before and after so a gate can assert the route CHANGED rather than that some pixels did. */
	long _wx0 = 0, _wz0 = 0; int _haveworld = 0;
	/* Capture the uid, do NOT re-read m_buttonid after the drop: OnLButtonUp -> OnDragItem
	   repaints and recalculates the route, and anything in there that hit-tests the map can
	   leave m_buttonid pointing at a different item. Reading the "after" position through a
	   member the drop is free to change measures whichever waypoint happened to be under the
	   cursor last. */
	const int _dragid = m_buttonid;
	if (_dragid) {
		ItemBasePtr _p = Persons2::ConvertPtrUID(UniqueID(_dragid));
		if (_p) { _wx0 = (long)_p->World.X; _wz0 = (long)_p->World.Z; _haveworld = 1; }
	}
	fprintf(stderr,"[mapdrag] press (%d,%d) -> id=%d(0x%x) \"%s\" allowdrag=%d world=(%ld,%ld)\n",
	        (int)from.x,(int)from.y,m_buttonid,m_buttonid,
	        m_buttonid ? (const char*)(LPCTSTR)GetTargName((UniqueID)m_buttonid) : "",
	        (int)AllowDragItem(m_buttonid), _wx0, _wz0);
	for (int _i = 1; _i <= MA_DRAG_STEPS; _i++)
	{
		CPoint _p((int)from.x + (int)(to.x - from.x) * _i / MA_DRAG_STEPS,
		          (int)from.y + (int)(to.y - from.y) * _i / MA_DRAG_STEPS);
		OnMouseMove(0x0001 /*MK_LBUTTON*/, _p);
	}
	/* m_bDragging is the one bit that separates a DRAG from a click: MaDriveClick keeps it
	   FALSE on purpose. A gate that only checked "the waypoint moved" could pass on a drag
	   that never engaged, because OnLButtonUp would then have called OnClickItem instead. */
	int _wasdragging = (int)m_bDragging;
	fprintf(stderr,"[mapdrag] moved to (%d,%d) dragging=%d\n",(int)to.x,(int)to.y,_wasdragging);
	OnLButtonUp(0, to);
	long _wx1 = _wx0, _wz1 = _wz0;
	if (_haveworld) {
		ItemBasePtr _p = Persons2::ConvertPtrUID(UniqueID(_dragid));
		if (_p) { _wx1 = (long)_p->World.X; _wz1 = (long)_p->World.Z; }
	}
	fprintf(stderr,"[mapdrag] released id=%d(0x%x) dragging=%d world (%ld,%ld) -> (%ld,%ld) moved=%d\n",
	        _dragid, _dragid, _wasdragging, _wx0, _wz0, _wx1, _wz1,
	        (int)(_wx1 != _wx0 || _wz1 != _wz0));
}
#endif

void CMapDlg::OnDragItem(int m_buttonid)
{
#ifndef	MIG_DEMO_VER

//	RDialog::MakeTopDialog(CRect(100,100,0,0),
//		DialList(DialBox(FIL_NULL,
	ItemBasePtr p=Persons2::ConvertPtrUID(UniqueID(m_buttonid));
	switch (p->Status.size)
	{
	case WAYPOINTSIZE:
		{
			int pack, wave, g;
			if (Profile::GetPackageFromWP((UniqueID)m_buttonid, pack, wave, g))
			{
				info_waypoint* w=*p;
				if (w->World.X<METRES250KM)								w->World.X=METRES250KM;
				if (w->World.X>METRES500KM+METRES500KM-METRES20KM)		w->World.X=METRES500KM+METRES500KM-METRES20KM;
				if (w->World.Z<METRES100KM+METRES20KM)					w->World.Z=METRES100KM+METRES20KM;
				if (w->World.Z>METRES500KM+METRES500KM+METRES150KM)		w->World.Z=METRES500KM+METRES500KM+METRES150KM;

				Coords3D c=w->World;
				Todays_Packages[pack].SetTargetGlobrefs();			  //JIM 16/03/99
				Todays_Packages[pack].RecalcPrimaryPositions();		  //JIM 16/03/99
				if (wave)
					Todays_Packages[pack].RecalcSecondaryPositions(wave);
				if (w->target.Evaluate()!=ENABLE_COMPLEX_VAL)
					c-=Persons2::ConvertPtrUID(UniqueID(w->target.Evaluate()));
				Todays_Packages[pack].SetTargetGlobrefs();
				Todays_Packages[pack].RecalcPrimaryPositions();
				w->position.RecalcDeltasToMatch(c);
				if (!wave)
					Todays_Packages[pack].RecalcRoutePositions();
				else
					Todays_Packages[pack].RecalcSecondaryPositions(wave);
				int fuel;
				Invalidate();										  //JIM 16/03/99

				Todays_Packages[pack].CalcWPTimes(wave);
				if (w->applygroup==ENABLE_COMPLEX_VAL)
				{
					for (g = 0; g < 3; g++)
					{
						info_airgrp* ag;
						ag=*Persons2::ConvertPtrUID(Todays_Packages[pack][wave][g].uid);
						if (ag)
							if (!(Todays_Packages[pack].CalcFuel(wave,g,UID_NULL, fuel)))
								RDialog::RMessageBox(IDS_BINGOWARNINGTITLE, IDS_BINGOWARNING,NULL,IDS_OK);
					}
				}else if (!(Todays_Packages[pack].CalcFuel(wave,w->applygroup,UID_NULL, fuel)))
								RDialog::RMessageBox(IDS_BINGOWARNINGTITLE, IDS_BINGOWARNING,NULL,IDS_OK);
				if (CProfile::wavetabs)
				{
					CMissionFolder::Make();
					CMissionFolder::OpenOneProfile(pack);
				}
			}
		}
	break;
	}
#endif #ifndef	MIG_DEMO_VER

//	typedef	HideEdgeControl::EdgeCtrl	Edge;
//	typedef	HideEdgeControl	EDGE;
//	CTargetDossier::MakeSheet(CRect(180,180,0,0),(UniqueID)m_buttonid);
}

bool CMapDlg::AllowDragItem(int m_buttonid)
{
//	RDialog::MakeTopDialog(CRect(100,100,0,0),
//		DialList(DialBox(FIL_NULL,
	if (TargetSelectSink)
		return false;
	return (m_buttonid>=WayPointBAND && m_buttonid<WayPointBANDEND);
}



BOOL CMapDlg::OnEraseBkgnd(CDC* pDC) 
{
//	CRect rect;
//	pDC->GetBoundsRect(&rect,NULL);
//	if (rect.Width()<30 && rect.Height()<30) return TRUE;
	RDialog::m_pView->UpdateBitmaps(pDC);
	return TRUE;
}

/*void CMapDlg::OnPaint() 
{
	PAINTSTRUCT paintstruct;
	CDC* pDC;
	pDC=this->BeginPaint(&paintstruct);
	this->EndPaint(&paintstruct);
}
*/

void	CMapDlg::InvalidateIcon(int px,int py)
{
		InvalidateRect(CRect(px-RDialog::m_pView->m_iconradius-1,py-RDialog::m_pView->m_iconradius-1,px+RDialog::m_pView->m_iconradius+1,py+RDialog::m_pView->m_iconradius+1));
}
void	CMapDlg::InvalidateDragIcon(int ox,int oy,int px,int py)
{	//waypoint is somewhere in there...
		px=min(ox,px);
		py=min(oy,py);
		InvalidateRect(CRect(px-RDialog::m_pView->m_iconradius*2,py-RDialog::m_pView->m_iconradius*2,px+RDialog::m_pView->m_iconradius*2,py+RDialog::m_pView->m_iconradius*2));
}
void	CMapDlg::InvalidateIconText(CDC* pDC,int px,int py,CString s)
{
	CSize size=pDC->GetTextExtent(s);
	InvalidateRect(CRect(px+RDialog::m_pView->m_iconradius,py-RDialog::m_pView->m_iconradius+1,px+RDialog::m_pView->m_iconradius+1+size.cx,py-RDialog::m_pView->m_iconradius+1+size.cy+2));
}

int	CMapDlg::FindMapItem(CPoint point)
{
	int i;	// Linux/GCC port: for-scope hoist
	int	m_hintid=0;
	int	mwx,mwy;													  //JIM 16/02/99
//	int mwx=		65536*384 +((point.x+RDialog::m_pView->m_scrollpoint.x)*65536)/RDialog::m_pView->m_zoom; //JIM 16/02/99
//	int mwy=		65536*1792-((point.y+RDialog::m_pView->m_scrollpoint.y)*65536)/RDialog::m_pView->m_zoom; //JIM 16/02/99
	int worldicon=RDialog::m_pView->WorldXY(mwx,mwy,point.x,point.y);//m_iconradius*65536/RDialog::m_pView->m_zoom; //JIM 16/02/99
	int mwl=mwx-worldicon;
	int	mwr=mwx+worldicon;
	int	mwt=mwy+worldicon;
	int	mwb=mwy-worldicon;
	UniqueIDField redUIDs;
	SupplyTree::FillNatForUID(redUIDs);
	FileNum  FL_Icon[AmberBattleNodeBANDEND-AmberBattleNodeBAND];
	SupplyTree::FillFLS_Icons(FL_Icon);

	ItemBasePtr p;
	FileNum  f;
	for (i=UID_Null;i<WayPointBAND;i++)
	{
		p=Persons2::ConvertPtrUID(UniqueID(i));
		if (p)
		{
			f = RDialog::m_pView->DrawIconTest(p, UniqueID(i), false,redUIDs,FL_Icon);
			if (f)
				if (	p->World.X>mwl && p->World.X<mwr &&	p->World.Z>mwb && p->World.Z<mwt )
					m_hintid=i;
		}
	}
	for (i=WayPointBAND;i<WayPointBANDEND;i++)
	{
		p=Persons2::ConvertPtrUID(UniqueID(i));
		if (p)
		{
			f = RDialog::m_pView->DrawIconTest(p, UniqueID(i), false,redUIDs,FL_Icon);
			if (f)
			{
				int wx=p->World.X;
				int wz=p->World.Z;
				SWord s,c;
				UWord ang=(i-WayPointBAND)*ANGLES_50Deg;
				Math_Lib.sin_cos((Angles)ang,s,c);
				wx+=MULSHSIN(s,worldicon,15);
				wz-=MULSHSIN(c,worldicon,15);
				if (	wx>mwl && wx<mwr &&	wz>mwb && wz<mwt )
					m_hintid=i;
			}
		}
	}
	for (i=WayPointBANDEND;i<IllegalSepID;i++)
	{
		p=Persons2::ConvertPtrUID(UniqueID(i));
		if (p)
		{
			f = RDialog::m_pView->DrawIconTest(p, UniqueID(i), false,redUIDs,FL_Icon);
			if (f)
				if (	p->World.X>mwl && p->World.X<mwr &&	p->World.Z>mwb && p->World.Z<mwt )
					m_hintid=i;
		}
	}
	return m_hintid;
}
void CMapDlg::ResetIcon(UniqueID wpuid)
{
	int	oldbuttonid = m_buttonid;
	m_buttonid = wpuid;
	RedrawIcon(oldbuttonid);
}
void CMapDlg::RedrawIcon(int oldbuttonid)
{// doing 2 invalidaterects here removes the very slight flicker...

	ItemBasePtr p;
	int px,py;
	if (m_buttonid)
	{
		p=Persons2::ConvertPtrUID(UniqueID(m_buttonid));
		if (p)
		{
			RDialog::m_pView->ScreenXY(p,px,py,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
			InvalidateIcon(px,py);

		}
		else m_buttonid=UID_NULL;									  //DAW 07/05/99

	}
	if (oldbuttonid)
	{
		p=Persons2::ConvertPtrUID(UniqueID(oldbuttonid));
		if (p)
		{
			RDialog::m_pView->ScreenXY(p,px,py,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
			InvalidateIcon(px,py);
		}
		else
			oldbuttonid=UID_NULL;									  //DAW 07/05/99
	}
}
void CMapDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	m_LButtonDown=TRUE;
	m_bDragging=FALSE;
	m_point=point;
	
	int oldbuttonid=m_buttonid;
	m_buttonid=NULL;
	m_buttonid=FindMapItem(point);
	if (m_buttonid == NULL)										//RDH 17Jun99
		m_buttonid = oldbuttonid;								//RDH 17Jun99
	RedrawIcon(oldbuttonid);
	CDialog::OnLButtonDown(nFlags, point);
}


void CMapDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	RDialog::m_pView->m_popupPoint=point;
	int px,py;
	ItemBasePtr p;
	CDC* pDC=GetDC();
	if (m_LButtonDown && !m_bDragging && m_buttonid)
	{
		if (!CRect(m_point.x-3,m_point.y-3,m_point.x+3,m_point.y+3).PtInRect(point))
			if (FindMapItem(m_point) && AllowDragItem(m_buttonid))
		{
			m_bDragging=TRUE;
			SetCapture();
			p=Persons2::ConvertPtrUID(UniqueID(m_buttonid));
			RDialog::m_pView->ScreenXY(p,px,py,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
			m_point.x=px;
			m_point.y=py;
			InvalidateIconText(pDC,px,py,GetTargName(UniqueID(m_hintid)));
			m_hintid=NULL; // dont display hints while dragging...
		}
	}
	if (m_LButtonDown && m_bDragging && m_buttonid)
	{
		ItemBasePtr p=Persons2::ConvertPtrUID(UniqueID(m_buttonid));
//DEADCODE JIM 16/02/99 		RDialog::m_pView->ScreenXY(p,px,py,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
		InvalidateRoutes(UniqueID(m_buttonid));

		RDialog::m_pView->WorldXY(p->World,point.x,point.y); //JIM 16/02/99

//DEADCODE JIM 16/02/99 		p->World.X=(65536*(point.x+RDialog::m_pView->m_scrollpoint.x+(384*RDialog::m_pView->m_zoom)))/RDialog::m_pView->m_zoom;
//DEADCODE JIM 16/02/99  		p->World.Z=-65536*(point.y+RDialog::m_pView->m_scrollpoint.y-(1792*RDialog::m_pView->m_zoom))/RDialog::m_pView->m_zoom;
		InvalidateDragIcon(m_point.x,m_point.y,point.x,point.y);
		m_oldpoint=m_point;
		m_point=point;
		InvalidateRoutes(UniqueID(m_buttonid));
	}
	if (!m_LButtonDown)
	{
		// check if mouse pointer is over an icon and if so display
		// the hint text for it
		// first find if we are over one
		int oldhintid=m_hintid;
		m_hintid=NULL;
		m_hintid=FindMapItem(point);
		// doing 2 invalidaterects here removes the very slight flicker...
		if (oldhintid!=m_hintid)
		{
			if (m_hintid)
			{
				p=Persons2::ConvertPtrUID(UniqueID(m_hintid));
				RDialog::m_pView->ScreenXY(p,px,py,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
				InvalidateIconText(pDC,px,py,GetTargName(UniqueID(m_hintid)));
			}
			if (oldhintid)
			{
				p=Persons2::ConvertPtrUID(UniqueID(oldhintid));
				if (p)
				{
					RDialog::m_pView->ScreenXY(p,px,py,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
					InvalidateIconText(pDC,px,py,GetTargName(UniqueID(oldhintid)));
				}
			}
		}
	}
	CDialog::OnMouseMove(nFlags, point);
}

void CMapDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	if (m_LButtonDown)
	{
		m_LButtonDown=FALSE;
		if (m_buttonid)
		{
			if (!m_bDragging)
			{
				if(FindMapItem(point))
					OnClickItem(m_buttonid);
			}
			else
				OnDragItem(m_buttonid);
		}
	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CMapDlg::InvalidateRoutes(UniqueID onlythrough)
{
	RDialog::m_pView->PlotRoutes(NULL,onlythrough);
/*	bool	seesubwp = true;
	ItemBasePtr pt1,pt2;

	// invalidates any routes coming in or out of m_buttonid
	// doesnt work
	for (int pack=0;pack<Profile::MAX_PACKS;pack++)
	{
		for (int wave=0;wave<Profile::MAX_WAVES;wave++)
		{
			for (int group=0;group<Profile::MAX_GROUPS;group++)
			{
				UniqueID start=Todays_Packages[pack][wave][group].takeoffAF;
				if (start)
				{
					UniqueID nextwp=Todays_Packages[pack][wave].firstwp;
					InvalidateMovedLine(start,nextwp);
					info_waypoint* wp=*Persons2::ConvertPtrUID(nextwp);
					UniqueID oldwp = nextwp;
					while ((nextwp=wp->nextwp)!=NULL)
					{
						wp=*Persons2::ConvertPtrUID(nextwp);
						if  (		
								(wp->uid !=ENABLE_COMPLEX_VAL) ||
								(wp->applygroup==ENABLE_COMPLEX_VAL) ||
								(		
									(seesubwp) &&
									(wp->applygroup == Todays_Packages.higroup)
								)
							)
						{
							//display subgroup wps as spurs
							InvalidateMovedLine(oldwp,nextwp);
							if  (wp->applyflight==ENABLE_COMPLEX_VAL)
							{
								oldwp=nextwp;
							}
						}
					}
					InvalidateMovedLine(nextwp,start);
				}
			}
		}
	}
	*/
}

void CMapDlg::InvalidateRoute(int	pack, int wave, int	group)
{
	UniqueID start=Todays_Packages[pack][wave][group].takeoffAF;
	bool	seesubwp = true;
	if (start)
	{
		UniqueID nextwp=Todays_Packages[pack][wave].firstwp;
		InvalidateLine(start,nextwp);
		info_waypoint* wp=*Persons2::ConvertPtrUID(nextwp);
		UniqueID oldwp = nextwp;
		while ((nextwp=wp->nextwp)!=NULL)
		{
			wp=*Persons2::ConvertPtrUID(nextwp);
			if  (		
					(wp->uid !=ENABLE_COMPLEX_VAL) ||
					(wp->applygroup==ENABLE_COMPLEX_VAL) ||
					(		
						(seesubwp) &&
						(wp->applygroup == Todays_Packages.higroup)
					)
				)
			{
				//display subgroup wps as spurs
				InvalidateLine(oldwp,nextwp);
				if  (wp->applyflight==ENABLE_COMPLEX_VAL)
				{
					oldwp=nextwp;
				}
			}
		}
		InvalidateLine(nextwp,start);
	}
}

void CMapDlg::InvalidateLine(UniqueID thiswp,UniqueID nextwp)
{
	int px1,py1,px2,py2;
	ItemBasePtr pt1,pt2;
	pt1=Persons2::ConvertPtrUID(UniqueID(thiswp));
	pt2=Persons2::ConvertPtrUID(UniqueID(nextwp));
	if (!pt1 || !pt2) return;
//DEADCODE RDH 18/04/99 	if (thiswp==UID_Japan)
//DEADCODE RDH 18/04/99 		RDialog::m_pView->ScreenXY(pt1->World.X + 40000000,pt1->World.Z - 20000000,px1,py1,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
//DEADCODE RDH 18/04/99 	else
		RDialog::m_pView->ScreenXY(pt1,px1,py1,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
//DEADCODE RDH 18/04/99 	if (nextwp==UID_Japan)
//DEADCODE RDH 18/04/99 		RDialog::m_pView->ScreenXY(pt2->World.X + 40000000,pt2->World.Z - 20000000,px2,py2,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
//DEADCODE RDH 18/04/99 	else
		RDialog::m_pView->ScreenXY(pt2,px2,py2,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
	InvalidateLine(CPoint(px1,py1),CPoint(px2,py2));
	InvalidateIcon(px1,py1);										  //JIM 02/06/99
}

//DEADCODE JIM 16/03/99 void CMapDlg::InvalidateMovedLine(UniqueID thiswp,UniqueID nextwp,BOOL japanwp)
//DEADCODE JIM 16/03/99 {
//DEADCODE JIM 16/03/99 
//DEADCODE JIM 16/03/99 	if (thiswp!=m_buttonid && nextwp!=m_buttonid)
//DEADCODE JIM 16/03/99 		return;
//DEADCODE JIM 16/03/99 	if (thiswp!=m_buttonid)
//DEADCODE JIM 16/03/99 	{
//DEADCODE JIM 16/03/99 		UniqueID swap;
//DEADCODE JIM 16/03/99 		swap=thiswp;
//DEADCODE JIM 16/03/99 		thiswp=nextwp;
//DEADCODE JIM 16/03/99 		nextwp=swap;
//DEADCODE JIM 16/03/99 	}
//DEADCODE JIM 16/03/99 	int px2,py2;
//DEADCODE JIM 16/03/99 	ItemBasePtr pt2;
//DEADCODE JIM 16/03/99 	pt2=Persons2::ConvertPtrUID(UniqueID(nextwp));
//DEADCODE JIM 16/03/99 	if (japanwp)
//DEADCODE JIM 16/03/99 		RDialog::m_pView->ScreenXY(pt2->World.X + 40000000,pt2->World.Z - 20000000,px2,py2,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
//DEADCODE JIM 16/03/99 	else
//DEADCODE JIM 16/03/99 		RDialog::m_pView->ScreenXY(pt2,px2,py2,-RDialog::m_pView->m_scrollpoint.x,-RDialog::m_pView->m_scrollpoint.y);
//DEADCODE JIM 16/03/99 	InvalidateLine(m_point,CPoint(px2,py2));
//DEADCODE JIM 16/03/99 	InvalidateLine(m_oldpoint,CPoint(px2,py2));
//DEADCODE JIM 16/03/99 }

#ifndef	NDEBUG

DebugLineStats	debugboxmem
={{0x00,0x20,0x40,0x60,0x80,0xA0,0xC0,0xE0},0,0};
#endif
void CMapDlg::InvalidateLine(CPoint point1,CPoint point2)
{	// invalidates a line from point1 to point2
	// ideally youd have thought you could use invalidate region
	// however this is very very slow...

	// so instead we invalidate millions of little rectangles...

//	CRect rect(point1.x-1,point1.y-1,point2.x+1,point2.y+1);
//	rect.NormalizeRect();
//	InvalidateRect(rect);
	const int RECTSIZE=50;
	CRect clientrect;
	CRect rect;
	CRect intersectrect;
	GetClientRect(clientrect);
	double dx=point2.x-point1.x;
	double dy=point2.y-point1.y;
	int steps;
	if (abs(dx)>abs(dy))
	{
		steps=abs(dx/RECTSIZE)+1;
		if (steps>1)
		{
			dy=dy/steps;
			dx=dx/steps;
		}
	}
	else
	{
		steps=abs(dy/RECTSIZE)+1;
		if (steps>1)
		{
			dy=dy/steps;
			dx=dx/steps;
		}
	}
	int radius=2;
	double x1,y1,x2,y2;
	if (dx>0)
	{
		x1=point1.x-dx/2-radius;
		x2=point1.x+dx/2+radius;
	}
	else
	{
		x2=point1.x-dx/2+radius;
		x1=point1.x+dx/2-radius;
	}
	if (dy>0)
	{
		y1=point1.y-dy/2-radius;
		y2=point1.y+dy/2+radius;
	}
	else
	{
		y2=point1.y-dy/2+radius;
		y1=point1.y+dy/2-radius;
	}
	for (int i=0;i<=steps;i++)
	{
		CRect rect(x1,y1,x2,y2);
		if (intersectrect.IntersectRect(clientrect,rect))
		{
			InvalidateRect(rect);
#ifndef NDEBUG
			debugboxmem.Set(rect);
#endif
		}
		x1+=dx;
		x2+=dx;
		y1+=dy;
		y2+=dy;
	}
#ifndef NDEBUG
	debugboxmem.lastline++;
#endif
}

/*
	// the polygon technique:
	// this isnt perfect
	CPoint points[6];
	int orientation;
	// figure out the orientation of point1 to point2
	// 0=topright 1=bottomright 2=bottomleft 3=topleft
	if (point1.x<=point2.x && point1.y>=point2.y) orientation=0;
	if (point1.x<=point2.x && point1.y<=point2.y) orientation=1;
	if (point1.x>=point2.x && point1.y<=point2.y) orientation=2;
	if (point1.x>=point2.x && point1.y>=point2.y) orientation=3;
	int i=0;
	if (orientation!=0) 
	{
		points[i]=CPoint(point1.x+2,point1.y-2);
		i++;
	}
	if (orientation!=1) 
	{
		points[i]=CPoint(point1.x+2,point1.y+2);
		i++;
	}
	if (orientation!=2) 
	{
		points[i]=CPoint(point1.x-2,point1.y+2);
		i++;
	}
	if (orientation!=3) 
	{
		points[i]=CPoint(point1.x-2,point1.y-2);
		i++;
	}

	if (orientation!=2) 
	{
		points[i]=CPoint(point2.x+2,point2.y-2);
		i++;
	}
	if (orientation!=3) 
	{
		points[i]=CPoint(point2.x+2,point2.y+2);
		i++;
	}
	if (orientation!=0) 
	{
		points[i]=CPoint(point2.x-2,point2.y+2);
		i++;
	}
	if (orientation!=1) 
	{
		points[i]=CPoint(point2.x-2,point2.y-2);
		i++;
	}
	CRgn region;
	region.CreatePolygonRgn(points,8,WINDING);
	InvalidateRgn(&region);
}
*/




void CMapDlg::OnTimer(UINT nIDEvent) 
{
//DeadCode AMM 03Mar99 	static ULong nextsendtime=timeGetTime()+20000; // every 20 secs

	if (nIDEvent==maptimer && _DPlay.PlayerCreated)
	{
		if (DPlay::H2H_Player[_DPlay.mySlot].status==CPS_MAP)
		{
//DeadCode AMM 03Mar99 			if (timeGetTime()>nextsendtime && _DPlay.Host)
//DeadCode AMM 03Mar99 			{
//DeadCode AMM 03Mar99 				nextsendtime=timeGetTime()+20000;
//DeadCode AMM 03Mar99 				_DPlay.SendSaveGame();
//DeadCode AMM 03Mar99 			}

			_DPlay.UIUpdateMainSheet();
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}

BOOL CMapDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
 	maptimer=SetTimer(TIMER_MAP,0,NULL);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMapDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	KillTimer(maptimer);
	
}

//DEADCODE JIM 29/05/99 BOOL CMapDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
//DEADCODE JIM 29/05/99 {
//DEADCODE JIM 29/05/99 	// TODO: Add your message handler code here and/or call default
//DEADCODE JIM 29/05/99 	
//DEADCODE JIM 29/05/99 	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
//DEADCODE JIM 29/05/99 }
