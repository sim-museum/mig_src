#pragma optimize( "g", off )
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
#define THIS_FILE __FILE__
#define	THIS_FILE_DEFINED

#endif
#define F_SOUNDS
#include "dosdefs.h"
#include "files.g"

#include	<afxmt.h>
#include	<dinput.h>
#include	<mmsystem.h>
#include	"stub3d.h" 
#include "display.h"  
#include "stdafx.h"
#include	"worldinc.h"	// Linux/GCC port: defines UniqueID etc. needed by DEBRIEF.h/infoitem.h
#include	"node.h"	// Linux/GCC port: SupplyTree etc. used by BASES.H/the UI headers
#include	"bitfield.h"	// Linux/GCC port: MAKEFIELD macro used by misssub.h/DEBRIEF.h
#include	"enumbits.m"	// Linux/GCC port: MATHABLE macro used below + by DEBRIEF.h
// Linux/GCC port: Rowan custom MFC controls, in dependency order (controls before
// rdialog.h which uses CRListBox; FullPane.h/MISSFLDR.H/etc. use CRButton/CRListBox).
#include	"RBUTTON.H"
#include	"rlistbox.h"
#include	"rcombo.h"
#include	"redit.h"
#include	"redtbt.h"
#include	"rradio.h"
#include	"rstatic.h"
#include	"rspinbut.h"
#include	"rtabs.h"
#include	"rdialog.h"
// Linux/GCC port: common game headers the fragments cross-reference (PCH-equiv).
#include "savegame.h"
#include "persons2.h"
#include "RCOMBOX.H"
#include "../mfc/resource.h"
#include "squick1.h"
#include "rtoolbar.h"
#include "rtitle.h"
#include "fullpane.h"
#include "mapfltrs.h"
#include "TitleBar.h"
#include "FLT_TASK.H"
#include "MISSFLDR.H"
#include "RTESTSH1.H"
#include "WPDETAIL.H"
#include "bfnumber.h"	// Linux/GCC port: sets BFNUMBER_Included so infoitem.h gives info_waypoint its full def
#include "infoitem.h"
#include "MESSAGES.H"
#include "Mytime.h"
#include "CURREMBL.H"
#include "../mfc/MAINTBAR.H"	// Linux/GCC port: defines MAINTBAR_INCLUDED + CMainToolbar (the SRC/H one does not)
#include "mainfrm.h"
#include "winmove.h"	// Linux/GCC port: DPlay class + _DPlay global + PLAYER_/CPS_
#include "replay.h"	// Linux/GCC port: _Replay global
#include	"missman2.h"	// MMC etc. (self-contained here; mapfltrs.h/fullpane.h are NOT
				// self-contained at this point — fragments include them themselves)
#include	"flyinit.h"
#include	"DEBRIEF.h"
#include "misssub.h"	// Linux/GCC port
CString	MakePilotName(PilotName& name);

MATHABLE(SquadSelect)
MATHABLE(Debrief::Types)
MATHABLE(Debrief::PlayerTypes)
#ifndef _DEBUG
#pragma message(__HERE__ "Non-debug version: global optimisation enabled when safe!")
#pragma optimize( "g", on )
#endif



