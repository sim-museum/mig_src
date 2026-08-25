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

// MscTlbr.cpp : implementation file
//

#include "stdafx.h"
#include "mig.h"
#include "MscTlbr.h"
#include "MigView.h"
#include "thumnail.h"
#include "savegame.h"
#include "FILING.H"	// Linux port: CFiling
#include "Load.h"	// Linux port: CLoad

#ifdef _DEBUG
#define new DEBUG_NEW
#ifndef THIS_FILE_DEFINED
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif
/////////////////////////////////////////////////////////////////////////////
// CMiscToolbar dialog


CMiscToolbar::CMiscToolbar(CWnd* pParent /*=NULL*/)
	: CRToolBar(pParent)
{
	//{{AFX_DATA_INIT(CMiscToolbar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMiscToolbar::DoDataExchange(CDataExchange* pDX)
{
	CRToolBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMiscToolbar)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX,IDC_ZOOMIN,m_IDC_ZOOMIN);
	DDX_Control(pDX,IDC_ZOOMOUT,m_IDC_ZOOMOUT);
	DDX_Control(pDX,IDC_REPLAY,m_IDC_REPLAY);
	DDX_Control(pDX,IDC_READYROOM,m_IDC_READYROOM);
#if defined(MA_LINUX)
	/* S242b (PO-71): the swap must happen HERE, not in OnShowWindow.
	   S242 put it in CMiscToolbar::OnShowWindow -- correct on Windows, useless here: that is a
	   WM_SHOWWINDOW handler and the compat message dispatch never delivers it, so the trace line
	   never printed and the fix never ran. DoDataExchange demonstrably DOES run (these very
	   DDX_Control calls are what create the buttons), so the swap goes here.

	   THE GENERAL TRAP, and it has bitten this port before (BoB S158: SendMessage was an allowlist
	   of three, and 16 of the 20 WM_* routes the game sends silently returned 0):
	   A FIX PLACED IN A HOOK THE PORT DOES NOT CALL IS INDISTINGUISHABLE FROM NO FIX. Put the
	   trace in first, and if it never prints, move the code -- do not assume the hook ran.

	   IDC_REPLAY and IDC_READYROOM share one rect ("WS_TABSTOP,144,0,32,30", slot 5), neither is
	   marked invisible, and the code that swapped them was commented out on 19/03/99. READYROOM is
	   declared last so it draws on top and buries Replay. Ready Room is the MULTIPLAYER control
	   (OnClickedReadyRoom drives _DPlay), and multiplayer is out of scope for this port (StartComms
	   returns FALSE, section 8) -- so Replay is the one that should be visible.
	   MA_NO_REPLAY_SLOT_FIX=1 reverts. */
	if (!getenv("MA_NO_REPLAY_SLOT_FIX"))
	{
		m_IDC_READYROOM.ShowWindow(SW_HIDE);
		m_IDC_REPLAY.ShowWindow(SW_SHOW);
		/* S242c: OnShowWindow did TWO jobs -- the swap AND the enable
		   (`but->SetDisabled(incomms)`). Since that hook never fires here, the button was left
		   visible but DISABLED, and the toolbar dispatcher swallows a click on a disabled control
		   ("[tbclick] id=%d is DISABLED -- click swallowed"). That is why the PO reported the
		   button as present-but-dead. Do the enable here too, on the same condition the original
		   used. Multiplayer is out of scope for this port, so this is effectively always enabled. */
		m_IDC_REPLAY.SetDisabled(RFullPanelDial::incomms ? true : false);
		if (getenv("MA_TRACE_OLE")) {
			/* S243: id=1049 never appeared in ANY click or filter trace, so before theorising about
			   why it is not clickable, establish whether it EXISTS and WHERE. Print the rect and
			   visibility the port actually gave each slot-5 candidate, next to a known-good
			   neighbour (FILES, id=10, which the PO can click). */
			fprintf(stderr,"[misctlbr] slot5 REPLAY   : vis=%d rect=(%d,%d,%d,%d)\n",
			        (int)m_IDC_REPLAY.m_maVisible, m_IDC_REPLAY.m_maX, m_IDC_REPLAY.m_maY,
			        m_IDC_REPLAY.m_maW, m_IDC_REPLAY.m_maH);
			fprintf(stderr,"[misctlbr] slot5 READYROOM: vis=%d rect=(%d,%d,%d,%d)\n",
			        (int)m_IDC_READYROOM.m_maVisible, m_IDC_READYROOM.m_maX, m_IDC_READYROOM.m_maY,
			        m_IDC_READYROOM.m_maW, m_IDC_READYROOM.m_maH);
			fprintf(stderr,"[misctlbr] ref   ZOOMOUT  : vis=%d rect=(%d,%d,%d,%d)  (incomms=%d)\n",
			        (int)m_IDC_ZOOMOUT.m_maVisible, m_IDC_ZOOMOUT.m_maX, m_IDC_ZOOMOUT.m_maY,
			        m_IDC_ZOOMOUT.m_maW, m_IDC_ZOOMOUT.m_maH, (int)RFullPanelDial::incomms);
			fflush(stderr);
		}
	}
#endif
	//}}AFX_DATA_MAP
	for (int id=0;dialids[id];id++)
		DDX_Control(pDX,dialids[id],m_dialids[id]);

}


BEGIN_MESSAGE_MAP(CMiscToolbar, CRToolBar)
	//{{AFX_MSG_MAP(CMiscToolbar)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMiscToolbar message handlers
				

BEGIN_EVENTSINK_MAP(CMiscToolbar, CRToolBar)
    //{{AFX_EVENTSINK_MAP(CMiscToolbar)
	ON_EVENT(CMiscToolbar, IDC_REPLAY, 1 /* Clicked */, OnClickedReplay, VTS_NONE)
	ON_EVENT(CMiscToolbar, IDC_READYROOM, 1 /* Clicked */, OnClickedReadyRoom, VTS_NONE)
	ON_EVENT(CMiscToolbar, IDC_ZOOMIN, 1 /* Clicked */, OnClickedZoomin, VTS_NONE)
	ON_EVENT(CMiscToolbar, IDC_ZOOMOUT, 1 /* Clicked */, OnClickedZoomout, VTS_NONE)
	ON_EVENT(CMiscToolbar, IDC_THUMBNAIL, 1 /* Clicked */, OnClickedThumbnail, VTS_NONE)
	ON_EVENT(CMiscToolbar, IDC_FILES, 1 /* Clicked */, OnClickedFiles, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()
int CMiscToolbar::dialids[]={
		IDC_THUMBNAIL, IDC_FILES,0};

void CMiscToolbar::OnClickedReplay() 
{
	Todays_Packages.SaveBin();
	if (MMC.indebrief)
	{
		if (_Replay.badrecord)									//AMM 09Mar99
			RFullPanelDial::RMessageBox(IDS_MIGALLEY,IDS_BADREPLAY,NULL,IDS_OK);//AMM 09Mar99
		else
		{
			_Replay.MapPlayback=true;
			RDialog::m_pView->LaunchFullPane(&RFullPanelDial::replaysave);
		}
//DeadCode AMM 10Mar99 		RDialog::m_pView->LaunchFullPane(&RFullPanelDial::replaysave);
	}
	else
	{
		_Replay.MapPlayback=true;
//DeadCode AMM 26Feb99 		Todays_Packages.SaveBin();
//	if (!RFullPanelDial::incomms)								//rdh 10/1/99
		RDialog::m_pView->LaunchFullPane(&RFullPanelDial::replayload);
//	else
//		RDialog::m_pView->LaunchFullPane(&RFullPanelDial::SelectReadyRoom);
	}
		
}
void CMiscToolbar::OnClickedReadyRoom() 
{
	if (_DPlay.UIPlayerType==PLAYER_HOST)							//AMM 03Mar99
		_DPlay.UpdateMission=TRUE;								//AMM 03Mar99

	FullScreen* fs;
	RFullPanelDial::SelectReadyRoomStatic(fs);
	RDialog::m_pView->LaunchFullPane(fs);
		
}

void CMiscToolbar::OnClickedZoomin() 
{
	RDialog::m_pView->OnZoomIn();
}

void CMiscToolbar::OnClickedZoomout() 
{
	RDialog::m_pView->OnZoomOut();
}

void CMiscToolbar::OnClickedThumbnail() 
{
	typedef	HideEdgeControl	EDGE;
	if (!LoggedChild(THUMBNAIL))
	{
		CMIGView::m_thumbnail=new CThumbnail;
		LogChild(THUMBNAIL,
			RDialog::MakeTopDialog(
			//CRect(140,140,0,0),
			Place(POSN_MIN,POSN_MIN),
								DialBox(FIL_NULL,CMIGView::m_thumbnail,Edges(EDGE::ALIGN_MIN + EDGE::ACTIONS_SPLITTER,EDGE::ALIGN_MIN,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE))
				)					);
	}
	else
		CloseLoggedChild(THUMBNAIL);
}

void CMiscToolbar::ChildDialClosed(int dialnum,RDialog*,int rv)
{
	GETDLGITEM(dialids[dialnum])->SetPressed(FALSE);
	if (dialnum==0)
		CMIGView::m_thumbnail=NULL;
}

void CMiscToolbar::OnClickedFiles() 
{
	typedef	HideEdgeControl	EDGE; 
	CString	wildcard="*.sav";
	CFiling::selectedfile=Save_Data.lastsavegame;
//	CFiling::selectedfile="test.sav";
	if (!LoggedChild(FILES))
	{
		LogChild(FILES,
			RDialog::MakeTopDialog(Place(POSN_CENTRE,POSN_CENTRE),
								DialList(DialBox(FIL_NULL/*IDB_BBC3*/,new CFiling,Edges(EDGE::ALIGN_MIN,EDGE::ALIGN_MIN,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE)), //rdh
									HTabBox(FIL_NULL,IdList(IDS_SAVE,IDS_LOAD),Edges(EDGE::ALIGN_MIN,EDGE::ALIGN_MIN,EDGE::ALIGN_MAX,EDGE::ALIGN_MAX), 
									DialBox(FIL_MAP_PLAYER_LOG/*IDB_BBC3*/,new CLoad(IDD_SAVE,LSD_SAVE,			FIL_SAVEGAMEDIR,wildcard,CFiling::selectedfile)),
			  						DialBox(FIL_MAP_PLAYER_LOG/*IDB_BBC3*/,new CLoad(IDD_LOAD,LSD_LOAD_DESELECT,FIL_SAVEGAMEDIR,wildcard,CFiling::selectedfile))
				)					)	)	);
	}
	else
		CloseLoggedChild(FILES);
	
}

void CMiscToolbar::OpenSaveOnly(bool quitafter) 
{
	typedef	HideEdgeControl	EDGE;
	CString	wildcard="*.sav";
	CFiling::selectedfile=Save_Data.lastsavegame;
	if (!LoggedChild(FILES))
		LogChild(FILES,
			RDialog::MakeTopDialog(Place(POSN_CENTRE,POSN_CENTRE),
								DialList(DialBox(FIL_NULL/*IDB_BBC3*/,new CFiling(true),Edges(EDGE::ALIGN_MIN,EDGE::ALIGN_MIN,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE)), //rdh
									HTabBox(FIL_NULL,IdList(IDS_QUITSAVE),Edges(EDGE::ALIGN_MIN,EDGE::ALIGN_MIN,EDGE::ALIGN_MAX,EDGE::ALIGN_MAX), 
									DialBox(FIL_MAP_PLAYER_LOG/*IDB_BBC3*/,new CLoad(IDD_SAVE,LSD_SAVE,			FIL_SAVEGAMEDIR,wildcard,CFiling::selectedfile))
				)					)	)	);
	else
		;//bring to front...
}

void CMiscToolbar::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CRToolBar::OnShowWindow(bShow, nStatus);
//DEADCODE RDH 19/03/99 	if (!RFullPanelDial::incomms)								//rdh 10/1/99
//DEADCODE RDH 19/03/99 	{
//DEADCODE RDH 19/03/99 		CRButton*   b;
//DEADCODE RDH 19/03/99 		b=GETDLGITEM(IDC_REPLAY);
//DEADCODE RDH 19/03/99 		b->ShowWindow(SW_SHOW);
//DEADCODE RDH 19/03/99 		b=GETDLGITEM(IDC_READYROOM);
//DEADCODE RDH 19/03/99 		b->ShowWindow(SW_HIDE);
//DEADCODE RDH 19/03/99 	}else
//DEADCODE RDH 19/03/99 	{
//DEADCODE RDH 19/03/99 		CRButton*   b;
//DEADCODE RDH 19/03/99 		b=GETDLGITEM(IDC_REPLAY);
//DEADCODE RDH 19/03/99 		b->ShowWindow(SW_HIDE);
//DEADCODE RDH 19/03/99 		b=GETDLGITEM(IDC_READYROOM);
//DEADCODE RDH 19/03/99 		b->ShowWindow(SW_SHOW);
//DEADCODE RDH 19/03/99 	}

#if defined(MA_LINUX)
	/* S242 (PO-71): THE REPLAY BUTTON IS HIDDEN UNDER THE READY ROOM BUTTON.
	   IDD_MISCTOOLBAR gives IDC_REPLAY and IDC_READYROOM the SAME RECT -- both are
	   "WS_TABSTOP,144,0,32,30", slot 5 of the row. Neither is marked invisible in the template, and
	   the code that used to swap them was commented out on 19/03/99 (immediately above). So both
	   are created, both are drawn, and READYROOM -- declared LAST in the template -- lands on top.
	   The PO could not find the Replay button on the campaign map because they were looking at the
	   Ready Room button sitting over it.

	   Reinstating the author's own swap, on the author's own condition. This is well-supported
	   rather than inferred: the two controls demonstrably share a rect; the dead code performs
	   exactly this swap; and the LIVE code immediately below branches on the SAME `incomms` flag to
	   disable Replay in multiplayer. Ready Room is the multiplayer control (OnClickedReadyRoom
	   drives _DPlay), Replay is the single-player one -- so they are alternatives, not neighbours.

	   NB the caution from S225: restoring commented-out code is only as good as the evidence for
	   it. Here the evidence is the shared rect, which is a fact about the template, not a story.
	   MA_NO_REPLAY_SLOT_FIX=1 reverts. */
	if (!getenv("MA_NO_REPLAY_SLOT_FIX"))
	{
		CRButton* _rp = GETDLGITEM(IDC_REPLAY);
		CRButton* _rr = GETDLGITEM(IDC_READYROOM);
		const bool _mp = RFullPanelDial::incomms;
		if (_rp) _rp->ShowWindow(_mp ? SW_HIDE : SW_SHOW);
		if (_rr) _rr->ShowWindow(_mp ? SW_SHOW : SW_HIDE);
		if (getenv("MA_TRACE_OLE"))
			fprintf(stderr,"[misctlbr] slot 5: incomms=%d -> REPLAY=%s READYROOM=%s\n",
			        (int)_mp, _mp?"hidden":"SHOWN", _mp?"SHOWN":"hidden");
	}
#endif

	CRButton*	but = GETDLGITEM(IDC_REPLAY);
	but->SetPressed(FALSE);		//always lift replay button, might have been pressed
	if (RFullPanelDial::incomms)
	{
		but->SetDisabled(true);
		but->SetHintString(RESSTRING(NOTAVAIL));

	}else
	{
		but->SetDisabled(false);
		but->SetHintString(RESSTRING(REPLAY));

	}



}
