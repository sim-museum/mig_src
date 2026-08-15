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

// MIG.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <typeinfo>
extern "C" int ma_hardware_available(void);
//DeadCode DAW 07Dec00 #include "locale.h"
#include "MIG.h"

#include "MainFrm.h"
#include "MIGDoc.h"
#include "MIGView.h"
#include "rdemptyd.h"
#include	"stub3d.h"
//#include "Targets.h"
#include "WaveTabs.h"
//#include "Wave.h"
//#include "Chain.h"
//#include "Filters.h"
//#include "Title.h"
#include "gameset.h"
#include "smack.h"
#include "listbx.h"
#include "monotxt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#ifndef	THIS_FILE_DEFINED
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

//extern inline	int	LockExchange(int* loc,int newval=0);
//inline	int	LockExchange(int* loc,int newval=0)
//{
//	int rv;
//	_asm	{	mov		eax,newval
//				mov		ebx,loc
//				xchg	ds:[ebx],eax
//				mov		rv,eax
//			}
//	return	rv;
//}
/////////////////////////////////////////////////////////////////////////////
// CMIGApp

BEGIN_MESSAGE_MAP(CMIGApp, CWinApp)
	//{{AFX_MSG_MAP(CMIGApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMIGApp construction

CMIGApp::CMIGApp()
{
   resourceInst=NULL;
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

GameSettings gameSettings;

class RCommandLineInfo:public CCommandLineInfo
{
public:
	GameSettings m_gameSettings;
	RCommandLineInfo() {};
	~RCommandLineInfo() {}
	void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );
};

extern	int introsmk;
//������������������������������������������������������������������������������
//Procedure		ParseParam
//Author		Paul.   
//Date			Thu 25 Jun 1998
//------------------------------------------------------------------------------
void RCommandLineInfo::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{
	if (bFlag)
	{
		if (strcmpi(lpszParam,"USE16")==0)
		{
			//force use of 16 bit textures
			m_gameSettings.m_bUse16=true;
		}
		else if (strcmpi(lpszParam,"NOMEM")==0)
		{
			//ignore video RAM test results
			m_gameSettings.m_bBypassMemCheck=true;
		}
		else if (strcmpi(lpszParam,"NOINTRO")==0)
		{
			//ignore video RAM test results
			introsmk=1;
		}
		else if (strcmpi(lpszParam,"WINDOWED")==0)
			m_gameSettings.m_bFullScreenMode=false;
		else if (strcmpi(lpszParam,"HOST")==0)
		{
			// lobby host
			_DPlay.UIPlayerType=PLAYER_HOST;
		}
		else if (strcmpi(lpszParam,"GUEST")==0)
		{
			// lobby guest
			_DPlay.UIPlayerType=PLAYER_GUEST;
		}
		else CCommandLineInfo::ParseParam(lpszParam,bFlag,bLast);
	}
	else CCommandLineInfo::ParseParam(lpszParam,bFlag,bLast);
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CMIGApp object

CMIGApp theApp;

#if defined(MA_LINUX)
// Linux/GCC port: expose the real app to the compat AfxGetApp() and give the ELF
// entry (bob_main.cpp) C-callable hooks to drive the MFC boot path.
CWinApp* g_pBobApp = &theApp;
extern "C" void ma_gdi_present_screen(void);   /* GDI software-canvas present (ma_gdi.cpp) */
extern "C" int  ma_gdi_dump_to(const char* path); /* S58 MA_SHOT: one-shot canvas->PPM dump (ma_gdi.cpp) */
extern "C" void _exit(int) __attribute__((noreturn));
extern "C" void ma_gdi_clear_screen(void);     /* clear the canvas (map->panel transition) */
extern "C" int bob_init_instance(void) { return theApp.InitInstance() ? 1 : 0; }
extern "C" int bob_run(void)           { return theApp.Run(); }
extern "C" void ma_request_flight_exit(void);   /* STUB3D.CPP: raise OverLay.quit3d (graceful exit) */
/* S80 G2 campaign-loop drive (MAINTBAR.CPP — MMC/Miss_Man and the toolbars live there):
   read the post-mission debrief state, and drive the debrief toolbar's Next Period button. */
extern "C" int  ma_camp_indebrief(void);
extern "C" void ma_camp_next_period(void);
extern "C" const char* ma_camp_state(void);
extern "C" int  ma_ole_dialog_extent(void* dialog, int* outw, int* outh);  /* S94 */
/* S94: where the system box was last DRAWN, so the click router hits exactly what paint put
   on screen (the S84 rule). -1 = not drawn yet. */
static int g_sysbox_ox = -1, g_sysbox_oy = -1;
static int g_misc_ox = -1, g_misc_oy = -1;   /* S109: where the misc toolbar was drawn, for clicks */
static int g_filt_ox = -1, g_filt_oy = -1;   /* S109: ditto for the filter rows */   /* S109: where the misc toolbar was drawn, for clicks */     /* "6/25/50: Morning, planning" (the map's own readout) */
/* S80: idles-until-capture, armed by the MA_CAMP_LOOP drive when it finishes its mission
   target. The plain MA_SHOT=N capture fires at a hand-picked idle number, which cannot be
   made to land at the end of a multi-mission loop (the idle count depends on how long two
   flights took); arming it from the drive itself keeps the recipe free of magic numbers,
   the same rule S62/S63 adopted for click coordinates. -1 = disarmed. */
static long ma_loop_shot_countdown = -1;
/* operational-map navigation input (bob_video.cpp): held pan-dir bits, one-shot action
   queue (1=zoomin 2=zoomout 3=exit 4=fly), mouse-wheel ticks, and the canvas size. */
extern "C" int ma_map_nav_held(void);
extern "C" int ma_map_nav_take(void);
extern "C" int ma_mouse_wheel(void);
extern "C" const void* ma_gdi_canvas(int*, int*);
extern "C" void* ma_gdi_screen_dc(void);
extern "C" void* ma_gdi_get_font(void* hdc);
extern "C" void  ma_gdi_set_font(void* hdc, void* font);
extern "C" void  ma_ole_set_parent_scoped(void* dialog);
/* Apply a new map zoom about screen point (cx,cy), replicating CMIGView::Zoom()'s math
   (MIGVIEW.CPP:888-925) WITHOUT its m_pScaleBar/scrollbar/m_mapdlg side-effects, which
   aren't wired in the port (m_pScaleBar can be NULL -> the real Zoom would crash). */
static void ma_map_apply_zoom(CMIGView* v, float newzoom, int cx, int cy) {
	if (newzoom < ZOOMMIN) newzoom = ZOOMMIN;
	if (newzoom > ZOOMMAX) newzoom = ZOOMMAX;
	float ratio = newzoom / v->m_zoom;
	v->m_scrollpoint.x = (int)((v->m_scrollpoint.x + cx) * ratio) - cx;
	v->m_scrollpoint.y = (int)((v->m_scrollpoint.y + cy) * ratio) - cy;
	v->m_zoom = newzoom; v->m_oldzoom = newzoom;
	v->m_size.cx = (long)(256*4*newzoom) - 5;
	v->m_size.cy = (long)(256*7*newzoom) - 5;
	v->m_iconradius = (newzoom > ZOOMTHRESHOLD2) ? 12 : 3;
}

/* ---- Flight -> menu round-trip (M1) ----------------------------------------------------
   In flight, EXITKEY (Alt+X) / a prefs exit calls View3d::CloseWindow on the DRAW thread,
   which on Win32 posts WM_COMMAND(IDOK|IDCANCEL) to the flight dialog; the pump then routes
   it to Rtestsh1::OnOK/OnCancel (Paused + delete View3d/Inst3d) and the parent panel's
   OnFlyingClosed (-> LaunchScreen back to the front-end / debrief). Our compat PostMessage is
   a no-op so that hop is dead. CloseWindow (STUB3D, MA_LINUX) instead calls ma_capture_flight_close
   to stash the id; the main idle loop drains it here, on the MAIN thread, so the view/inst
   teardown (which joins the draw thread in ~View3d) is race-free. Mirrors BoB's recipe. */
static volatile int g_maFlightCloseId = -1;   /* pending WM_COMMAND id, -1 = none */
void ma_capture_flight_close(int id)
{
	if (Rtestsh1::THISTHIS && Inst3d::InThe3D())
		g_maFlightCloseId = id;
}
static void ma_process_flight_close(void)
{
	if (g_maFlightCloseId < 0) return;
	int id = g_maFlightCloseId; g_maFlightCloseId = -1;
	if (getenv("MA_TRACE_3D"))
		fprintf(stderr,"[3d] flight close (id=%d) -> %s + OnFlyingClosed\n",
			id, id==IDOK ? "OnOK" : "OnCancel");
	if (Rtestsh1::THISTHIS) {
		/* The game's OWN teardown + navigation: Rtestsh1::OnOK/OnCancel pauses+deletes the
		   view/inst, then (unlike BoB) the MA RDialog::EndDialog->DialExitFix->ChildDialClosed
		   chain fires the parent panel's OnFlyingClosed naturally -> LaunchScreen back to the
		   front-end (debrief for OnOK, options3d for OnCancel). So DON'T call OnFlyingClosed
		   again here -- that would double-navigate. */
		if (id==IDOK) Rtestsh1::THISTHIS->OnOK();
		else          Rtestsh1::THISTHIS->OnCancel();
		/* compat CDialog::OnOK/OnCancel doesn't destruct the dialog, so THISTHIS can linger with
		   setup3dstatus==GOING + tmpinst==NULL -> the idle MaDriveLaunch driver would re-launch
		   flight on the stale dialog. Disarm it; the next StartFlying makes a fresh Rtestsh1. */
		Rtestsh1::THISTHIS = NULL;
	}
	if (getenv("MA_TRACE_3D"))
		fprintf(stderr,"[3d] back in front-end (InThe3D=%d)\n", (int)Inst3d::InThe3D());
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CMIGApp initialization

CFont* (g_AllFonts[MAXFONTS][4])={{NULL}};
CDC g_OffScreenDC;
static int __stdcall EnumFontFamProc(const LOGFONT *lpelf,const TEXTMETRIC *lpntm,ULong FontType,LPARAM lParam )
{
	*(int*)lParam=true;
	return 1;//0;
}
 

void CreatePointFont(int ind, int point,char* name,bool all4)
{
	//input is 10ths of a point
	//a point is 1/72 inches
	//units are 1/720 inches
	//96 pixels per inch
	//multiply by 96/720=6*16/3*3*5*16=2/15
	enum	{POINT2PIXMUL=2,POINT2PIXDIV=15};
	point*=-POINT2PIXMUL;
	
	g_AllFonts[ind][0]=new CFont;
	g_AllFonts[ind][0]->CreateFont(point/POINT2PIXDIV,0,	0,0,0,	0,0,0,
						DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_CHARACTER_PRECIS,
						DEFAULT_QUALITY,DEFAULT_PITCH,
						name);
	g_AllFonts[ind][3]=new CFont;
	g_AllFonts[ind][3]->CreateFont(point*2/POINT2PIXDIV,0,	0,0,0,	0,0,0,
						DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_CHARACTER_PRECIS,
						DEFAULT_QUALITY,DEFAULT_PITCH,
						name);
	if (all4)
	{
		g_AllFonts[ind][1]=new CFont;
		g_AllFonts[ind][1]->CreateFont((point*4)/(3*POINT2PIXDIV),0,	0,0,0,	0,0,0,
						DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_CHARACTER_PRECIS,
						DEFAULT_QUALITY,DEFAULT_PITCH,
						name);
		g_AllFonts[ind][2]=new CFont;
		g_AllFonts[ind][2]->CreateFont((point*5)/(3*POINT2PIXDIV),0,	0,0,0,	0,0,0,
						DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_CHARACTER_PRECIS,
						DEFAULT_QUALITY,DEFAULT_PITCH,
						name);
	}
}

__inline HWND FindMSWheel( 
      PUINT puiMsh_MsgMouseWheel,
      PUINT puiMsh_Msg3DSupport=NULL,
      PUINT puiMsh_MsgScrollLines=NULL,
      PBOOL pf3DSupport=NULL,
      PINT  piScrollLines=NULL
)
{
   HWND hdlMsWheel;

   hdlMsWheel = FindWindow(MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);
   if (puiMsh_MsgMouseWheel)
	   *puiMsh_MsgMouseWheel=0;
   if (hdlMsWheel)
   {
	   int	Msh_MsgMouseWheel = RegisterWindowMessage(MSH_MOUSEWHEEL);
	   int	Msh_Msg3DSupport = RegisterWindowMessage(MSH_WHEELSUPPORT);
	   int	Msh_MsgScrollLines = RegisterWindowMessage(MSH_SCROLL_LINES);

	   if (pf3DSupport)
	   if (*puiMsh_Msg3DSupport)
		  *pf3DSupport = (BOOL)SendMessage(hdlMsWheel, Msh_Msg3DSupport, 0, 0);
	   else
		  *pf3DSupport = FALSE;  // default to FALSE

	   if (piScrollLines)
	   if (*puiMsh_MsgScrollLines )
		  *piScrollLines = (int)SendMessage(hdlMsWheel, Msh_MsgScrollLines, 0, 0);
	   else
		  *piScrollLines = 3;  // default
	   if (puiMsh_MsgMouseWheel)
		   *puiMsh_MsgMouseWheel=Msh_MsgMouseWheel;
	   if (puiMsh_Msg3DSupport)
		   *puiMsh_Msg3DSupport=Msh_Msg3DSupport;
	   if (puiMsh_MsgScrollLines)
		   *puiMsh_MsgScrollLines=Msh_MsgScrollLines;
   }
   return(hdlMsWheel);
}


BOOL CMIGApp::InitInstance()
{
	INT		sysnames[]={COLOR_SCROLLBAR,COLOR_3DDKSHADOW};
	DWORD	sysvals[]={0x0000ff00,0x00ff0000};
//	SetSysColors(2,sysnames,sysvals);

	TRACE0("InitInstance starting...\n");

	FindMSWheel(&wheelmessagenum);

//	SmackSetSystemRes(SMACKRES800X600);
	// Initialise global fonts

	// The syntax is g_AllFonts[font number].CreatePointFont(pointsize*10,"Name of Font");
	// so to create a 10 point MS Sans Serif font and set it to be font number 6
	// g_AllFonts[6].CreatePointFont(100,"MS Sans Serif");
// *** SYNTAX HAS CHANGED
	// Make sure you alter the maximum size of the global array if you add fonts
	// ie change the line 
	//
	// CFont g_AllFonts[2];
	// to 
	// CFont g_AllFonts[3];
	//
	// which is just above these comments
	//
	// *** Use caution when changing or deleting existing fonts ***
	// Make sure each font has a unique number
	// And make sure that a control doesn't use a font that does not exist
	// *** Odd Numbered fonts should be double the size of the previous even numbered font
	// These are used for when a dialog box is doubled in size
/*
	g_AllFonts[0].CreatePointFont(110,"Free"); // data font (small frontend and map)
	g_AllFonts[1].CreatePointFont(200,"Intel"); // title bar font (map only)
	g_AllFonts[2].CreatePointFont(110,"Header"); // Header font (small frontend and map)
	g_AllFonts[3].CreatePointFont(220,"Free"); // data font (large frontend)
	g_AllFonts[4].CreatePointFont(220,"Header"); // the default listbox header font
	g_AllFonts[5].CreatePointFont(220,"Intel"); // Head font (large frontend)
	g_AllFonts[6].CreatePointFont(320,"Intel"); // the default title screen font

	g_AllFonts[10].CreatePointFont(240,"Intel"); // 640x480 front screen listbox font
	g_AllFonts[11].CreatePointFont(260,"Intel"); // 800x600 front screen listbox font
	g_AllFonts[12].CreatePointFont(280,"Intel"); // 1024x768 front screen listbox font
	g_AllFonts[13].CreatePointFont(320,"Intel"); // 1280x1024 front screen listbox font
*/

	if (!RDialog::actscrw)										//RJS 08Sep98
	{	//1st time in read registry...
		HKEY k;
		DWORD type;
		unsigned char buff[10];
		DWORD size=9;
#ifdef MA_LINUX
		/* S61 (parity #15, I4): `type` and `buff` are read back below WITHOUT checking
		   RegQueryValueEx's return code. On Windows that is harmless — the
		   Control Panel\desktop\WindowMetrics values always exist — but this port's
		   RegQueryValueEx is a stub that returns ERROR_FILE_NOT_FOUND and touches
		   neither output. So `if (type==REG_SZ)` tested uninitialised stack and
		   `*(int*)buff` read uninitialised stack, giving RDialog::borderwidth a
		   different garbage value on every run.
		   borderwidth feeds MakeParentDialog's dialog sizing
		   (`dialsize.left + rect.right - 1 + borderwidth*2`), so every top-level dialog
		   tree got a garbage origin — measured on the Player Log as
		   m_ma=(978990,978859) one run and (979004,978793) the next. That is the
		   run-to-run variance that distinguishes it from a logic bug.
		   Zeroing both makes the failed query deterministic: type != REG_SZ, the else
		   branch reads 0, and borderwidth lands on `2 - 0` = a sane 2px border.
		   (Same uninitialised-read class as the S58/S59 R* ctor audit — one level up,
		   in a local fed by a stubbed Win32 API rather than a skipped ctor member.) */
		type = 0;
		memset(buff, 0, sizeof buff);
#endif
		RegOpenKeyEx( 
				HKEY_CURRENT_USER,
				"Control Panel\\desktop\\WindowMetrics",
				0, KEY_ALL_ACCESS, &k);
		RegQueryValueEx(k,"ScrollWidth",NULL,&type,buff,&size);
		if (type==REG_SZ)
			RDialog::actscrw=atoi((char*)buff)/12;
		else
			RDialog::actscrw=*(int*)buff/12;
		size=9;
		RegQueryValueEx(k,"ScrollHeight",NULL,&type,buff,&size);
		if (type==REG_SZ)
			RDialog::actscrh=atoi((char*)buff)/12;
		else
			RDialog::actscrh=*(int*)buff/12;
		size=9;
		RegQueryValueEx(k,"ScrollWidth",NULL,&type,buff,&size);
		if (type==REG_SZ)
			RDialog::actscrw=atoi((char*)buff)/12;
		else
			RDialog::actscrw=*(int*)buff/12;
		size=9;
		RegQueryValueEx(k,"BorderWidth",NULL,&type,buff,&size);
		if (type==REG_SZ)
			RDialog::borderwidth=atoi((char*)buff)/15;
		else
			RDialog::borderwidth=*(int*)buff/15;
		RDialog::borderwidth=2-RDialog::borderwidth;
		RegCloseKey(k);
//DeadCode DAW 29Oct99 		RegOpenKeyEx( 
//DeadCode DAW 29Oct99 				HKEY_CURRENT_CONFIG,
//DeadCode DAW 29Oct99 				"Display\\Settings",
//DeadCode DAW 29Oct99 				0, KEY_ALL_ACCESS, &k);
//DeadCode DAW 29Oct99 		size=9;
//DeadCode DAW 29Oct99 		RegQueryValueEx(k,"DPILogicalX",NULL,&type,buff,&size);
//DeadCode DAW 29Oct99 		if (type==REG_SZ)
//DeadCode DAW 29Oct99 			RDialog::fontdpi=atoi((char*)buff);
//DeadCode DAW 29Oct99 		else
//DeadCode DAW 29Oct99 			RDialog::fontdpi=*(int*)buff;
//DeadCode DAW 29Oct99 		RegCloseKey(k);
	}															//RJS 08Sep98
	RDialog::fontdpi=RDialog::GetFontScaling();


     // one of the first things in the init code
	//Load alternate language resources
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Rowan Software Ltd"));				
	//This is not being used as we still have a settings.mig :(
	//I think for BoB we should progress to using the registry ...
	//however - I will use this to indicate that the program is running

	LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMIGDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CMIGView));
	pDocTemplate->SetContainerInfo(IDR_CNTR_INPLACE);
	AddDocTemplate(pDocTemplate);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	//CCommandLineInfo cmdInfo;
	RCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	gameSettings=cmdInfo.m_gameSettings;

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

#if defined(MA_LINUX)
	// Linux/GCC port: the compat ProcessShellCommand is a no-op, so the MFC doc/view
	// framework never builds the SDI doc/frame/view. Do it here (the natural creation
	// order — NOT a cold-start), then drive the engine's own init in sequence so the
	// front-end launches the way the first WM_PAINT would on Windows:
	//   CMIGView::OnInitialUpdate (map dialog + scrollbars)
	//   CMainFrame::Initialise    (builds toolbars + system box, then LaunchFullPane ->
	//                              RFullPanelDial::introsmack/demotitle = the title screen)
	// CMainFrame::Initialise gets the view via GetActiveView(), so wire SetActiveView.
	if (!m_pMainWnd) {
		CMIGDoc*    pDoc       = new CMIGDoc();
		CMIGView*   pView      = new CMIGView();   // ctor sets RDialog::m_pView = this
		pView->m_pDocument     = pDoc;
		CMainFrame* pMainFrame = new CMainFrame();
		pMainFrame->m_hWnd     = (HWND)1;          // placeholder; GL/DDraw uses the global SDL window
		pMainFrame->MoveWindow(0, 0, 800, 600);    // 800x600 canvas: GetCurrentRes() -> res 800 (dial800
		                                           // art exists; dial640 isn't installed) via GetWindowRect
		pMainFrame->SetActiveView(pView);
		m_pMainWnd = pMainFrame;
		{ extern CWnd* g_pBobMainWnd; g_pBobMainWnd = pMainFrame; }  // AfxGetMainWnd()

		pView->OnInitialUpdate();
		pMainFrame->InitialiseSafe();              // sets havesafe; Initialise() runs once havedrawn
		pMainFrame->Initialise();                  // build the toolbar/dialog UI (title launch deferred)
	}
#endif
//	Rtestsh1::tmpinst=new Inst3d;
//	if (!Rtestsh1::tmpview)
//	{
//		Rtestsh1::tmpview=new View3d(Rtestsh1::tmpinst,m_pMainWnd->m_hWnd,m_pMainWnd);
//		Rtestsh1::tmpview->MakeInteractive(WinMode::WIN);
//	}

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ModifyStyle(m_pMainWnd->GetStyle(),WS_THICKFRAME); // Bye bye title bar
//	m_pMainWnd->ModifyStyle(m_pMainWnd->GetStyle(),NULL); // Bye bye title bar and edges
	m_pMainWnd->SetMenu(NULL); // Bye bye menu  Status bar removed in MainFrm.cpp
	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);

//MOVED THIS DOWN A BIT....
//DeadCode DAW 29Oct99 	CreatePointFont(0,110,"Arial Italic",FALSE); // data font (small frontend and map)
//DAVE DO NOT TAKE THESE FONT CHANGES FROM MACCA's MACHINE

//	CreatePointFont(1,200,"Intel",FALSE); // title bar font (map only)
//	CreatePointFont(2,110,"Arial",FALSE); // Arial font (small frontend and map) was 110
//	CreatePointFont(3,220,"Arial Italic",FALSE); // data font (large frontend) was 220
//	CreatePointFont(4,220,"Arial",FALSE); // the default listbox Arial font was 220
//	CreatePointFont(5,220,"Intel",FALSE); // Head font (large frontend) was 220
//	CreatePointFont(6,320,"Intel",FALSE); // the default title screen font
//	CreatePointFont(7,110,"Arial Italic",FALSE); // data font (small frontend and map) (again)
//	CreatePointFont(10,150,"Intel",TRUE); // front screen listbox font

	Master_3d.Init(m_hInstance,m_pMainWnd->m_hWnd);
//#ifndef	NDEBUG
//In normal development, we don't want to load this DLL because of the risk of orphan DLLs
//It is too late in MiG Alley for this sort of garbage;
//If you do get a orphan DLL you will have to exit DevStudio
	char buffer[80];
 	resourceInst = LoadLibrary(File_Man.NameNumberedFile(FIL_LANGRESOURCEDLL,buffer));//"c:\\mig\\src\\english\\debug\\miglang.dll");
	if (resourceInst != NULL)
		AfxSetResourceHandle(resourceInst);

#if defined(MA_LINUX)
	// Now that the language resource DLL is loaded (menu strings resolvable) and the dir
	// list is fully registered, launch the deferred title front-end (see MAINFRM.CPP).
	// F4 (Sprint 4): launch the FULL `title` (Preferences / Single Player / Comms / Load /
	// Replay / Credits / Quit) instead of `demotitle` (the cut-down 5-item demo menu) so the
	// single-player path (Quick Mission / Campaign) is reachable. The data install is the full
	// game; CheckForDemo (the intro->title onselect) would pick demotitle only for demo data,
	// but it isn't on this direct-launch path, so we go straight to `title`.
	/* S103 (PO-8): run the startup preferences init the port had been skipping.
	   On Windows the game reaches its front-end through `RFullPanelDial::IntroSmackInit`
	   (FULLPANE.CPP), which opens with `Save_Data.InitPreferences()` + `_Replay.TruncateTempFiles()`
	   BEFORE anything about the intro Smacker is decided -- so those run on every launch, intro
	   present or not. The port launches the title directly (Smacker is stubbed, E1) and therefore
	   never ran either. The only other call site is inside `#ifdef MIG_DEMO_VER`.
	   InitPreferences is what establishes the shipped default profile, including
	   `infoLineCount=1` ("want to see messages" -- SAVEGAME.CPP), which is why the in-flight info
	   line never drew: `DrawInfoBar` returns early on `infoLineCount==0`. Two consequences of the
	   same missing call had already been patched LOCALLY without asking who was supposed to set
	   them (STUB3D.CPP: `SetUnits()` for the zeroed unit factors, `GD_HUDINSTACTIVE` for the HUD
	   instruments) -- a third symptom is a good moment to fix the cause instead.
	   `MA_NO_INITPREFS=1` reverts, so the before/after is one env var. */
	if (!getenv("MA_NO_INITPREFS")) {
		Save_Data.InitPreferences((int)Master_3d.winst);
		_Replay.TruncateTempFiles();
		/* S108 (PO-11): MA_FORCE_RES=WxH selects a display mode without driving the Preferences
		   UI, writing exactly the two fields the resolution combo writes (SDETAIL.CPP sets
		   Save_Data.displayW/H; DDRWINIT picks the matching 16-bit mode from them). Needed
		   because the PO's gold recordings are 1280x1024 and this engine picks its panel ART SET
		   by resolution -- comparing a 800x600 native capture against a 1280x1024 gold is the
		   exact mistake S64 recorded. Offer the same modes the combo offers
		   (ma_populate_software_modes): 640x480 800x600 1024x768 1280x960 1600x1200 1920x1080. */
		/* S110 (PO-12): the hardware-path measurement flag has to be applied HERE, before the
		   display is initialised -- DDRWINIT decides whether to construct Direct3D from
		   Save_Data.fSoftware, and by flight time it is far too late. Note the third source of
		   this value: since S103 the port really loads settings.mig, and that file was written
		   while the port pinned fSoftware=true, so the preference itself now says "software".
		   A real Preferences option (the PO's ask) will have to set this the same way. */
		/* S118 (PO-12 phase 4): the value that matters here is the PLAYER's, loaded from
		   settings.mig a few lines above; DDRWINIT builds Direct3D from it and by flight time it
		   is far too late to change. MA_TRY_HARDWARE remains as a developer override that forces
		   hardware on regardless of the saved preference, and MA_NO_HARDWARE forces it off. */
		if (getenv("MA_TRY_HARDWARE")) Save_Data.fSoftware = false;
		if (!ma_hardware_available())  Save_Data.fSoftware = true;
		/* The port offers one hardware driver, the primary, which dddriver names as -1. A saved
		   0 selects a driver that does not exist (driverCount is 0) and puts the Preferences combo
		   index out of range. Normalise on load so a settings file written by an earlier build --
		   or by the MA_TRY_HARDWARE override -- cannot carry an unreachable driver number in. */
		if (!Save_Data.fSoftware && Save_Data.dddriver != -1) Save_Data.dddriver = -1;
		if (getenv("MA_TRACE_3D"))
			fprintf(stderr, "[hw] display init: fSoftware=%d dddriver=%d\n",
			        (int)Save_Data.fSoftware, (int)Save_Data.dddriver);
		/* S122 (PO: "set all graphics preferences to max -> the window is a translucent shadow").
		   The PO's settings live only in memory until a clean exit, so a crash/kill loses them and
		   the configuration cannot be reproduced from a script. MA_FORCE_DETAIL=max turns every 3D
		   detail flag on and maxes filtering, which is what "all settings to max" means. */
		if (const char* det = getenv("MA_FORCE_DETAIL")) {
			if (det[0] == 'm') {
				for (int d = DETAIL3D_MIN; d < DETAIL3D_MAX; ++d)
					if (d != DETAIL3D_PADLOCKCHEAT) Save_Data.detail_3d |= (SG_Detail_3D)d;
				Save_Data.filtering = 1;
				fprintf(stderr, "[detail] MA_FORCE_DETAIL=max: all 3D detail flags on, filtering=1\n");
			}
		}
		if (const char* fr = getenv("MA_FORCE_RES")) {
			int w=0,h=0;
			if (sscanf(fr, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
				Save_Data.displayW = w; Save_Data.displayH = h;
				fprintf(stderr, "[prefs] MA_FORCE_RES -> displayW/H = %dx%d\n", w, h);
			}
		}
		if (getenv("MA_TRACE_PREFS"))
			fprintf(stderr, "[prefs] InitPreferences() run at startup: infoLineCount=%d\n",
			        (int)Save_Data.infoLineCount);
	}
	if (RDialog::m_pView)
		RDialog::m_pView->LaunchFullPane(&RFullPanelDial::title);
#endif

/////	{	//Debugs to generate character usage statistics:
/////		int mybigletterbuffer[65536];
/////		for (int i=0;i<65536;i++)
/////			mybigletterbuffer[i]=0;
/////		for (i=0;i<65536;i++)
/////		{
/////			CString resi;
/////			resi.LoadString(i);
/////			const unsigned char * getter=(const unsigned char *)(const char *)resi;
/////			while (*getter)
/////			{
/////				int c=_mbsnextc(getter);
/////				mybigletterbuffer[c]++;
/////				getter=_mbsinc(getter);
/////			}
/////		}
/////		FILE* f=fopen("usage.txt","wt");
/////		int	usedchars=0;
/////		{for (int i=0,k=64;i<65536;i=k,k+=64)
/////		{
/////			fprintf(f,"\n%04x ",i);
/////			int uc=0;
/////			for (int j=i;j<k;j++)
/////				if (mybigletterbuffer[j])
/////					uc++;
/////			if (!(i&255))
/////				fprintf(f,"P %2i: ",uc);
/////			else
/////				fprintf(f,"  %2i: ",uc);
/////			for (j=i;j<k;j++)
/////			{
/////				int l=mybigletterbuffer[j];
/////				if (l)
/////					usedchars++;
/////				if (l==0)
/////					fputc(' ',f);
/////				else if (l<10)
/////					fputc('0'+l,f);
/////				else if (l<200)
/////					fputc('a'+l/10,f);
/////				else if (l<4000)
/////					fputc('A'+l/200,f);
/////				else if (l<7*4000)
/////					fputc('S'+l/4000,f);
/////				else
/////					fputc('#',f);
/////			}
/////		}}
/////		fprintf(f,"\n\nTotal used glyphs=%i\n",usedchars);
/////		fclose(f);
/////	}
//#endif
	oldhelppath=m_pszHelpFilePath ;
	if (File_Man.existnumberedfile(FIL_LANGRESOURCEHELPTABSANDPANELS))
	{
		m_pszHelpFilePath =  File_Man.NameNumberedFile(FIL_LANGRESOURCEHELPTABSANDPANELS,helpfilepath);
		((CMainFrame*)(m_pMainWnd))->disablehelp=0;
	}
	else
	if (File_Man.existnumberedfile(FIL_LANGRESOURCEHELPPANELS))
	{
		m_pszHelpFilePath =  File_Man.NameNumberedFile(FIL_LANGRESOURCEHELPPANELS,helpfilepath);
		((CMainFrame*)(m_pMainWnd))->disablehelp=1;
	}
	else
	if (File_Man.existnumberedfile(FIL_LANGRESOURCEHELPSEPARATETABS)) 
	{
		m_pszHelpFilePath =  File_Man.NameNumberedFile(FIL_LANGRESOURCEHELPSEPARATETABS,helpfilepath);
		((CMainFrame*)(m_pMainWnd))->disablehelp=2;
	}
	else
	if (File_Man.existnumberedfile(FIL_LANGRESOURCEHELP)) 
	{
		m_pszHelpFilePath =  File_Man.NameNumberedFile(FIL_LANGRESOURCEHELP,helpfilepath);
		((CMainFrame*)(m_pMainWnd))->disablehelp=3;

	}
//DeadCode DAW 29Oct99 	((CMainFrame*)(m_pMainWnd))->InitialiseSafe();
	CDC* pdc=m_pMainWnd->GetDC();
	g_OffScreenDC.CreateCompatibleDC(pdc);
	g_OffScreenDC.SetMapMode(MM_TEXT);
	m_pMainWnd->ReleaseDC(pdc);
	char* curlyfont="�l�r ����";
	char* straightfont="�l�r �S�V�b�N";
	int	gotfont=false;
	char*	myfont="Intel";										//DAW 29Oct99

	EnumFontFamilies(GetDC(m_pMainWnd->m_hWnd),curlyfont,EnumFontFamProc, (LPARAM)&gotfont);
	if (gotfont)
	{	//must be doing japanese!
		myfont=curlyfont;
	}
	else
	{
		straightfont="Arial";
		curlyfont="Arial Italic";
		EnumFontFamilies(GetDC(m_pMainWnd->m_hWnd),myfont,EnumFontFamProc, (LPARAM)&gotfont);
 		if (!gotfont)
			myfont="Times New Roman Bold";
		EnumFontFamilies(GetDC(m_pMainWnd->m_hWnd),myfont,EnumFontFamProc, (LPARAM)&gotfont);
 		if (!gotfont)
			myfont="MS Serif";
	}
 	
	CreatePointFont(0,110,curlyfont,FALSE); // data font (small frontend and map)
	CreatePointFont(1,200,myfont,FALSE); // title bar font (map only)
	CreatePointFont(2,110,straightfont,FALSE); // Arial font (small frontend and map) was 110
	CreatePointFont(3,200,curlyfont,FALSE); // data font (large frontend) was 220
	CreatePointFont(4,200,straightfont,FALSE); // the default listbox Arial font was 220
	CreatePointFont(5,200,myfont,FALSE); // Head font (large frontend) was 220
	CreatePointFont(6,320,myfont,FALSE); // the default title screen font
	CreatePointFont(7,110,curlyfont,FALSE); // data font (small frontend and map) (again)
	CreatePointFont(8,140,straightfont,FALSE); // Arial font (small frontend and map) was 110
	CreatePointFont(9,140,curlyfont,FALSE); // data font (small frontend and map) (again)
	CreatePointFont(10,150,myfont,TRUE); // front screen listbox font

	((CMainFrame*)(m_pMainWnd))->InitialiseSafe();
	m_pMainWnd->UpdateWindow();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

//	SetSysColors(2,sysnames,sysvals);

	return TRUE;
}

int CMIGApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CWinApp::ExitInstance();
}
CMIGApp::~CMIGApp()
{
	m_pszHelpFilePath= oldhelppath;
    if (resourceInst != NULL)
        FreeLibrary(resourceInst);
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CMIGApp::OnAppAbout()
{
	RDEmptyD aboutDlg;
//	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

#if defined(MA_LINUX)
/* S53 OOB render (cross-port from BoB S113/S114/S116): the OOB-info dialogs (Squads/... opened
   from a map-toolbar click) BUILD their fchild tree (S52) but nothing paints them. Mirror BoB:
   walk each open logged-child's fchild chain and, per art-bearing node, OnPaint the background
   art (self-positioned via OnGetXYOffset) + ma_ole_draw_toolbar its hosted controls. */
extern "C" void ma_ole_draw_toolbar(void* dialog, void* screenHdc, int ox, int oy);
static void ma_oob_dump(RDialog* n, int depth, int via)
{
	if (!n || depth > 8) return;
	if (getenv("MA_TRACE_OOB")) { static int _c=0; if(_c++<60)
		fprintf(stderr,"[oobtree] d=%d via=%c node=%p art=%d fchild=%p dchild=%p sibling=%p\n",
			depth, via, (void*)n, (int)n->artnum, (void*)n->fchild, (void*)n->dchild, (void*)n->sibling); }
	ma_oob_dump(n->fchild, depth+1, 'f');
	ma_oob_dump(n->dchild, depth+1, 'd');
	ma_oob_dump(n->sibling, depth, 's');
}
static void ma_oob_render_node(RDialog* n)
{
	if (!n) return;
	if (n->artnum) n->MaOnPaint();                       /* background art (self-positioned) */
	long off = n->MaXYOffset();
	ma_ole_draw_toolbar((void*)n, (void*)1, (short)(off & 0xFFFF), (short)((off >> 16) & 0xFFFF));
}
/* S60 (parity #15, I4): render the WHOLE visible OOB tree, not just the first art-bearing
   node.
   The S53 walk descended n->fchild until it found a node with background art and rendered
   only that node plus its immediate children. That was enough for Squads (whose tab page
   carries the art AND the controls) but it silently drops any node whose artnum is 0 —
   and the Player Log has two of them, both load-bearing:

     MakeTopDialog                       artnum 0
       +- RDEmptyP   (HTabBox host)      artnum 0   <- owns IDJ_TABCTRL, the tab bar
       |    +- CCareer                   artnum FIL_MAP_PLAYER_LOG   <- the only art node
       |    +- CMisn_log / CLastMissionLog          (SW_HIDEd by AddChildren)
       +- CPlyr_log                      artnum 0   <- owns IDJ_TITLE, the title bar

   CPlyr_log is a SIBLING of the tab host (AddChildren pushes each child onto fchild), so
   the old fchild-only descent could not have reached it at any artnum. Result: no title
   bar, no tab bar — two of #15's four named deviations, from one walk.

   Now: recurse over siblings and children, rendering every VISIBLE node. Visibility is
   what keeps the non-selected tab pages off the screen — RDialog::AddChildren calls
   ShowWindow(SW_SHOW) on the first tab and SW_HIDE on the rest (RDIALOG.CPP:984-992), so
   honouring m_maVisible reproduces the old "first tab only" outcome by the engine's own
   mechanism instead of by stopping the walk early. Art stays gated on artnum inside
   ma_oob_render_node; a node with no art still gets its controls drawn.
   Outer-to-inner order preserves the painter's algorithm. */
static void ma_oob_paint_tree_rec(RDialog* n, int depth)
{
	if (!n || depth > 8) return;
	for (; n; n = n->sibling)
	{
		if (getenv("MA_TRACE_OOB")) { static int _r=0; if (_r++<40)
			fprintf(stderr,"[oobpaint] d=%d node=%p art=%d vis=%d view=(%ld,%ld %ldx%ld) m_ma=(%d,%d %dx%d)\n",
				depth, (void*)n, (int)n->artnum, (int)n->m_maVisible,
				(long)n->viewsize.left,(long)n->viewsize.top,(long)n->viewsize.Width(),(long)n->viewsize.Height(),
				n->m_maX,n->m_maY,n->m_maW,n->m_maH); }
		if (!n->m_maVisible) continue;          /* hidden tab page / collapsed node */
		ma_oob_render_node(n);
		ma_oob_paint_tree_rec(n->fchild, depth + 1);
	}
}
static void ma_oob_paint_tree(RDialog* n)
{
	static int dumped=0; if(!dumped && getenv("MA_TRACE_OOB")){dumped=1; ma_oob_dump(n,0,'t');}
	ma_oob_paint_tree_rec(n, 0);
}
/* S82: the CLICK counterpart of ma_oob_paint_tree_rec. Until now the map idle routed clicks to
   the two toolbars only, so every OOB dialog (Player Log, Squads, Bases, DIS, Overview, ...) was
   RENDER-ONLY: its tabs, rows and title-bar tick did nothing, and the only way to switch a Player
   Log tab was the MA_OOB_PLAYERLOG_TAB scaffold hook. This walks the SAME tree with the SAME
   MaXYOffset() offsets the paint pass uses — so the hit rects cannot drift from the drawn ones —
   and delivers through the existing ma_ole_toolbar_click.
   Children are tested BEFORE their parent because they paint on top of it. */
static int ma_oob_click_tree_rec(RDialog* n, int depth, int cx, int cy)
{
	if (!n || depth > 8) return 0;
	for (; n; n = n->sibling)
	{
		if (!n->m_maVisible) continue;
		if (ma_oob_click_tree_rec(n->fchild, depth + 1, cx, cy)) return 1;
		long off = n->MaXYOffset();
		if (ma_ole_toolbar_click((void*)n, (short)(off & 0xFFFF), (short)((off >> 16) & 0xFFFF), cx, cy))
		{
			if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[oobclick] node=%p depth=%d took (%d,%d)\n",(void*)n,depth,cx,cy);
			return 1;
		}
	}
	return 0;
}
static int ma_map_click_oob(int cx, int cy)
{
	if (getenv("MA_NO_OOB_CLICK")) return 0;
	CMainFrame* mf = (CMainFrame*)AfxGetMainWnd();
	if (!mf) return 0;
	/* S106: same two toolbars as the paint walk -- a dialog that is drawn but cannot be clicked
	   is only half hosted, and the MISSION RESULTS panel has four buttons (I.D. / Debrief /
	   Redo / Next Period) the player is meant to use. */
	CRToolBar* tbs[2]; tbs[0] = &mf->m_toolbar2; tbs[1] = &mf->m_toolbar5;
	for (int t = 0; t < 2; t++)
	for (int slot = 0; slot < DialogLinks::MAX_CHILD_DIALS; slot++)
	{
		RDialog* child = tbs[t]->LoggedChild(slot);
		if (!child || !child->m_maVisible) continue;
		if (ma_oob_click_tree_rec(child, 0, cx, cy)) return 1;
		/* Inside the dialog's own rect but not on a control: SWALLOW it. An open info dialog
		   sits over the operational map, and a click on its background must not fall through to
		   the map/toolbar underneath (which would pan, or fire a toolbar button, behind it). */
		if (cx >= child->m_maX && cx < child->m_maX + child->m_maW &&
		    cy >= child->m_maY && cy < child->m_maY + child->m_maH)
		{
			if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[oobclick] swallowed (%d,%d) inside dialog rect\n",cx,cy);
			return 1;
		}
	}
	return 0;
}
static int ma_map_paint_oob(void)
{
	CMainFrame* mf = (CMainFrame*)AfxGetMainWnd();
	if (!mf) return 0;
	int cnt = 0;
	void* seen[32]; int seen_n = 0;   /* S123: dialogs already painted this pass */
	/* S106 (PO-9): walk the DEBRIEF toolbar's children as well as the main toolbar's.
	   The map-screen dialogs (Player Log, Intelligence, Squads...) are logged against
	   `m_toolbar2` (CMainToolbar), and this walk only knew about that one. The post-mission
	   MISSION RESULTS panel is logged against `m_toolbar5` (CDebriefToolbar) by
	   CDebriefToolbar::OnClickedMissionresults, so it was created on every campaign flight and
	   then never painted -- the player exits with ALT+X, the map comes back with the date
	   advanced to "debrief", and nothing else appears. Traced: OpenMissionresults returns a live
	   child (0x...740) and no pixels ever follow it. Same shape as S82 (dialogs that existed but
	   were never composited); here the tree was fine and the ENUMERATION was too narrow. */
	CRToolBar* tbs[2]; tbs[0] = &mf->m_toolbar2; tbs[1] = &mf->m_toolbar5;
	for (int t = 0; t < 2; t++)
	{
		for (int slot = 0; slot < DialogLinks::MAX_CHILD_DIALS; slot++)
		{
			RDialog* child = tbs[t]->LoggedChild(slot);
			/* S123 (PO-17): paint each open dialog ONCE. The child-slot walk reaches the same
			   dialog through more than one slot -- measured: three different toolbar buttons
			   opened dialogs and every paint pass drew the SAME object (one pointer, one rect)
			   three times. Redundant painting of a translucent panel darkens it with each pass
			   and makes a single dialog read as several stacked on top of each other. */
			int already = 0;
			for (int q = 0; q < seen_n; ++q) if (seen[q] == (void*)child) { already = 1; break; }
			if (child && !already) {
				if (seen_n < 32) seen[seen_n++] = (void*)child;
				ma_oob_paint_tree(child); cnt++;
				/* S123 (PO-17: campaign dialogs pile up on top of each other). Print WHERE each
				   open dialog is placed, so overlap is a measurement rather than an impression. */
				/* Report each DISTINCT dialog once -- not the first N prints. A global count is
				   spent by whatever is on screen first (here, the dialog opened before the others
				   existed), so the interesting later ones never appear and read as absent. This
				   project has booked "filter, don't cap" four times; this is the fifth. */
				if (getenv("MA_TRACE_OOB")) {
					static void* told[32]; static int told_n = 0;
					int t2 = 0;
					for (int q = 0; q < told_n; ++q) if (told[q] == (void*)child) { t2 = 1; break; }
					if (!t2 && told_n < 32) {
						told[told_n++] = (void*)child;
						fprintf(stderr, "[oob]   dialog %p %s at (%d,%d) %dx%d\n",
						        (void*)child, typeid(*child).name(), (int)child->m_maX, (int)child->m_maY,
						        (int)child->m_maW, (int)child->m_maH);
					}
				}
			}
		}
	}
	if (cnt && getenv("MA_TRACE_OOB")) fprintf(stderr,"[oob] painted %d open dialog(s)\n", cnt);
	return cnt;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CMIGApp commands

int CMIGApp::Run() 
{
////	char* localeresult=setlocale( LC_ALL,"japanese");
	TRACE0("MessageLoop starting...\n");
	// TODO: Add your specialized code here and/or call the base class
///	Master_3d.Init(m_hInstance,m_pMainWnd->m_hWnd);
	for (int i=0;i<Master_3d.NUM_EVENTS;i++)
		Master_3d.htable[i]=CreateEvent(0,0,0,0);

	// Culled from CWinApp::Run() and CWinTread::Run()
	// The following bit is called once in CWinApp::Run()
	if (m_pMainWnd == NULL && AfxOleGetUserCtrl())
	{
		// Not launched /Embedding or /Automation, but has no main window!
		TRACE0("Warning: m_pMainWnd is NULL in CWinApp::Run - quitting application.\n");
		AfxPostQuitMessage(0);
	}


	// Add your specialized code here and/or call the base class
	//
	// The following is adapted from CWinThread::Run()
	// OnIdle()			is called when there are no more messages
	// PumpMessage()	is called when there is a windows message
	//						-returns FALSE after WM_QUIT message.
	//
	// I need to add new code for the message pump when in the 3d.
	// At present, I will just cheat.
	//

 	ASSERT_VALID(this);
	// for tracking the idle time state
	BOOL bIdle = TRUE;
	LONG lIdleCount = 0;

	// acquire and dispatch messages until a WM_QUIT message is received.
	for (;;)
	{
		int result;
		ULong t;;
		if (Inst3d::InThe3D()||!bIdle)
			result=MsgWaitForMultipleObjects(Mast3d::NUM_EVENTS,Master_3d.htable,FALSE,INFINITE,QS_ALLINPUT);//don't wait - ever!
		else
			result=MsgWaitForMultipleObjects(Mast3d::NUM_EVENTS,Master_3d.htable,FALSE,0,QS_ALLINPUT);//don't wait - ever!
		switch (result)
		{
		case WAIT_OBJECT_0+Mast3d::EVENT_KEYS:
//TEMPCODE DAW 01/10/99 			Inst3d::OnKeyInput();

			break;
		case WAIT_OBJECT_0+Mast3d::EVENT_MOUSE:
			break;

		case WAIT_OBJECT_0+Mast3d::EVENT_AGGREGATOR:
//DeadCode AMM 28Oct98 			if (_DPlay.Host)
//DeadCode AMM 28Oct98 			{
//DeadCode AMM 28Oct98 				if (_DPlay.Implemented)
//DeadCode AMM 28Oct98 				{
//DeadCode AMM 28Oct98 					t=1000000;
//DeadCode AMM 28Oct98 					TRACE0("want message lock\n");
//DeadCode AMM 28Oct98 					while (LockExchange(&Aggregator::inaggregator,1))
//DeadCode AMM 28Oct98 					{
//DeadCode AMM 28Oct98 						Sleep(0);
//DeadCode AMM 28Oct98 						if (!t--)
//DeadCode AMM 28Oct98 							_Error.SayAndQuit("run loop exceeds 1000000");
//DeadCode AMM 28Oct98 					};
//DeadCode AMM 28Oct98 					TRACE0("got lock for message\n");
//DeadCode AMM 28Oct98 					_Agg.AggregatorGetPackets(FALSE,FALSE);
//DeadCode AMM 28Oct98 					LockExchange(&Aggregator::inaggregator,0);
//DeadCode AMM 28Oct98 					TRACE0("released lock for message\n");
//DeadCode AMM 28Oct98 				}
//DeadCode AMM 28Oct98 			}
//DeadCode AMM 28Oct98 			else
//DeadCode AMM 28Oct98 				_Error.SayAndQuit("Player receiving aggregator interrupt!!!!");
//DeadCode AMM 28Oct98 
			break;

		case WAIT_OBJECT_0+Mast3d::NUM_EVENTS:
			while (::PeekMessage(&m_msgCur, NULL, NULL, NULL, PM_NOREMOVE))
			{
				bool fIn3d=Inst3d::InThe3D();
				if ((Rtestsh1::tempblockkeys || fIn3d) && m_msgCur.message>=WM_KEYFIRST &&  m_msgCur.message<=WM_KEYLAST)
				{
					while (::PeekMessage(&m_msgCur, NULL, WM_KEYFIRST, WM_KEYLAST,PM_REMOVE))
					{}
				}
				else
				if ((Rtestsh1::tempblockkeys || fIn3d) && wheelmessagenum && m_msgCur.message==wheelmessagenum)
				{
					while (::PeekMessage(&m_msgCur, NULL, wheelmessagenum, wheelmessagenum,PM_REMOVE))
					{}
				}
				else
				{
//DEADCODE RDH 12/07/99 					{	//messages received:						  //RDH 12/07/99
//DEADCODE RDH 12/07/99  						int msgval=short(m_msgCur.hwnd)*0x10000+short(m_msgCur.message);
//DEADCODE RDH 12/07/99  						static int col=0;
//DEADCODE RDH 12/07/99  						Mono_Text.PrintHexAt(col,0,msgval,8);
//DEADCODE RDH 12/07/99  						col+=10;if (col>80*25) col-=80*25;
//DEADCODE RDH 12/07/99  						Mono_Text.PrintHexAt(col,0,0,8);
//DEADCODE RDH 12/07/99  					}
					if (!PumpMessage())
					{
						TRACE0("MessageLoop exitting...\n");
						return ExitInstance();
					}
				}
			}
			if (IsIdleMessage(&m_msgCur))
			{
				bIdle = TRUE;
				lIdleCount = 0;
			}
			break;
		default:
			RDialog::m_moving=false;
#if defined(MA_LINUX)
			// Drain a pending flight-close (EXITKEY/prefs-exit posted a WM_COMMAND the compat
			// pump swallowed): run the game's own teardown + OnFlyingClosed on the MAIN thread,
			// then InThe3D() is false again and the 2D front-end resumes below.
			ma_process_flight_close();
			// Once the 3D flight world is launched (Inst3d created), the View3d draw thread
			// owns the screen: it renders to the DirectDraw back surface and presents via
			// Blt->ma_ddraw_present every frame. The 2D front-end render+present below must
			// stand down, or it overwrites the (slower) 3D frames with the menu canvas.
			bool ma_in3d = (Rtestsh1::THISTHIS && Rtestsh1::tmpinst != NULL);
			// Test hook: after N sustained 3D frames, request a graceful flight exit
			// (engine's own quit3d path) so the menu<->flight round-trip is scriptable.
			// S80: the frame counter is PER FLIGHT, not per process. It used to be a one-shot
			// static (`++n == N` is true exactly once in a run), so on the campaign loop's
			// SECOND mission the hook never fired and that flight flew forever. Reset it on
			// every 3D->front-end transition so each flight gets its own N frames.
			{
				static int  _aef = 0;
				static bool _was3d = false;
				if (ma_in3d) {
					const char* ae = getenv("BOB_AUTOEXIT");
					if (ae && ++_aef == atoi(ae)) {
						if (getenv("MA_TRACE_3D")) fprintf(stderr,"[3d] BOB_AUTOEXIT=%d reached -> ma_request_flight_exit()\n", atoi(ae));
						ma_request_flight_exit();
					}
				} else if (_was3d) {
					_aef = 0;   /* flight ended: re-arm for the next one */
				}
				_was3d = ma_in3d;
			}
			// Linux/GCC port: no WM_PAINT/WM_TIMER fire on Linux, so drive the active
			// front-end panel's render each idle — OnTimer advances the intro/animation
			// (and the Smacker stub's "done" -> OnLButtonUp transition), OnPaint composes the
			// background art + child controls into the GDI screen canvas; present it once.
			if (!ma_in3d && RDialog::m_pView) {
				RFullPanelDial* _fp = GetFullPanel(RDialog::m_pView);
				if (getenv("MA_TRACE_PAGE")) { static int _pg=0,_lp=-2; int cp=RDialog::m_pView->m_currentpage;
					if (cp!=_lp || (_pg++ % 60)==0) fprintf(stderr,"[page] currentpage=%d fp=%p branch=%s\n", cp,(void*)_fp, _fp?"panel":(cp==0?"map":"none")); _lp=cp; }
				/* On a map->panel transition (e.g. campaign frag), the map filled the whole
				   canvas; clear it once so a non-full-screen panel doesn't show the map through. */
				static bool _wasMap = false;
				static int _campStage = 0;   /* MA_CAMP_FLY: 1=frag launched (singlefrag up), 2=Fly driven */
				/* S80 (MA_CAMP_LOOP): the frag/Fly settle counters live out here, not inside their
				   own blocks, so the campaign loop can RE-ARM them for the next mission. They were
				   one-shot statics (`++n == N` never true again), which is why the campaign drive
				   could only ever fly the FIRST mission of a campaign. */
				static int _flyn  = 0;       /* idles since the singlefrag briefing came up */
				static int _fragn = 0;       /* idles since the map settled, before driving frag */
				static int _loopFlown  = 0;  /* campaign missions whose debrief we have ended */
				static int _loopSettle = 0;  /* idles since the post-mission debrief appeared */
				if (_fp) {
					if (_wasMap) { ma_gdi_clear_screen(); _wasMap = false; }
					/* M4 inc2 (MA_CAMP_FLY) stage 2: the campaign frag/briefing (singlefrag) is up.
					   After it settles, drive its Fly action (textlists col 1: Map=0/Fly=1/Prefs=2) ->
					   FragFly -> StartFlying -> the M1 3D-flight path -> OnFlyingClosed (CAMP) -> map. */
					if (getenv("MA_CAMP_FLY") && _campStage == 1) {
						if (++_flyn == 30) {
							if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] driving singlefrag Fly -> FragFly -> StartFlying\n");
							_campStage = 2;
							_fp->OnSelectRlistbox(0, 1);
						}
					}
					if (_fp->m_timerID) _fp->OnTimer(_fp->m_timerID);
					/* mouse hover: highlight the item under the cursor */
					{
						int _mx=0, _my=0, _mb=0; long _row=0, _col=0;
						ma_mouse_pos(&_mx, &_my, &_mb);
						ma_ole_mouse(&_fp->m_IDC_RLISTBOX, _fp, _mx, _my, 0, &_row, &_col);
					}
					_fp->OnPaint();
					/* composite ALL hosted OCX controls (menu listbox + destination-screen
					   statics/etc.) over the panel art at their template positions */
					ma_ole_draw_all((void*)1);
					/* click handled AFTER drawing: OnSelectRlistbox may destroy this panel
					   and launch the next screen (re-fetched next idle). */
					{
						int _cx=0, _cy=0; long _row=0, _col=0;
						if (ma_mouse_take_click(&_cx, &_cy)) {
							if (ma_ole_mouse(&_fp->m_IDC_RLISTBOX, _fp, _cx, _cy, 1, &_row, &_col))
								_fp->OnSelectRlistbox(_row, _col);
							else if (ma_ole_listbox_click(_cx, _cy))
								;   /* a child-dialog listbox (e.g. CLoad file list) handled it */
							else
								ma_ole_click(_cx, _cy);   /* not a listbox -> try a button */
						}
					}
				} else if (!getenv("MA_DISABLE_MAP") && RDialog::m_pView->m_currentpage == 0) {   /* campaign operational map: default-ON (Begin used to freeze a blank screen); MA_DISABLE_MAP turns it off */
						/* M4: campaign operational MAP active (LaunchMap set m_currentpage=0,
						   m_pfullpane=NULL). No WM_PAINT fires, so drive the map render each idle:
						   UpdateBitmaps StretchDIBits the scrolled/zoomed Korea MIDMAP tiles into the
						   screen canvas (+ DrawIcons for airfields/units). Navigation (pan/zoom/exit/
						   fly) is wired below from SDL input. Default-ON; MA_DISABLE_MAP turns it off. */
						static int _mapn=0;
						if (getenv("MA_TRACE_3D") && _mapn++<4) fprintf(stderr,"[map] render operational map (page=0)\n");
						_wasMap = true;
						if (getenv("MA_MAP_ZOOM")) RDialog::m_pView->m_zoom = (float)atof(getenv("MA_MAP_ZOOM"));
						/* Port: default the standard map view-filters ON the first map render so it
						   shows airfield/supply/bridge/waypoint/frontline icons. The filter toolbar
						   that normally toggles these is not hosted yet, so without this the map has
						   no icons. MA_NO_MAP_FILTERS inspects the raw loaded state. */
						if (!getenv("MA_NO_MAP_FILTERS")) { static int _f=0; if(!_f++){
							Save_Data.mapfilters|=FILT_RED_SUPPLY; Save_Data.mapfilters|=FILT_RED_BRIDGE;
							Save_Data.mapfilters|=FILT_MAINWP; Save_Data.mapfilters|=FILT_BLUE_AIRFIELDS;
							Save_Data.mapfilters|=FILT_FRONTLINE; } }
						/* ---- M4: in-map navigation (keyboard + mouse) ----
						   No window/scrollbar messages reach the view in the port, so drive the
						   map's scroll/zoom directly from SDL input each idle:
						     arrows / WASD or left-drag .. pan      mouse-wheel / +/- .. zoom
						     Esc .. exit to title (OnChangeToTitle)  F/Enter .. fly the mission */
						CMIGView* _v = RDialog::m_pView;
						int _cw=640, _ch=480; ma_gdi_canvas(&_cw, &_ch);
						bool _navExit = false;
						/* held-arrow / WASD pan */
						{ int held = ma_map_nav_held(); const int STEP = 28;
						  if (held & 1) _v->m_scrollpoint.x -= STEP;
						  if (held & 2) _v->m_scrollpoint.x += STEP;
						  if (held & 4) _v->m_scrollpoint.y -= STEP;
						  if (held & 8) _v->m_scrollpoint.y += STEP; }
						/* left-button drag pan */
						{ int mx=0,my=0,mb=0; ma_mouse_pos(&mx,&my,&mb);
						  static int _lx=0,_ly=0,_lb=0;
						  if (mb && _lb) { _v->m_scrollpoint.x += (_lx-mx); _v->m_scrollpoint.y += (_ly-my); }
						  _lx=mx; _ly=my; _lb=mb; }
						/* mouse-wheel zoom (about the screen centre) */
						{ int wz = ma_mouse_wheel();
						  while (wz>0){ ma_map_apply_zoom(_v,_v->m_zoom*1.25f,_cw/2,_ch/2); wz--; }
						  while (wz<0){ ma_map_apply_zoom(_v,_v->m_zoom*0.8f, _cw/2,_ch/2); wz++; } }
						/* one-shot key actions */
						{ int act;
						  while ((act = ma_map_nav_take()) != 0) {
							if (act==1)      ma_map_apply_zoom(_v,_v->m_zoom*1.5f,   _cw/2,_ch/2);
							else if (act==2) ma_map_apply_zoom(_v,_v->m_zoom*0.6667f,_cw/2,_ch/2);
							else if (act==3) { if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] Esc -> OnChangeToTitle\n");
							                   _wasMap=false; _navExit=true; _v->OnChangeToTitle(); break; }
							else if (act==4) { if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] Fly -> OnClickedFrag2\n");
							                   _campStage=1; MainToolBar().OnClickedFrag2(); }
						  } }
						/* clamp scroll to the map extents */
						if (!_navExit) {
						  if ((int)_v->m_size.cx > _cw && _v->m_scrollpoint.x > (int)_v->m_size.cx-_cw) _v->m_scrollpoint.x=(int)_v->m_size.cx-_cw;
						  if (_v->m_scrollpoint.x < 0) _v->m_scrollpoint.x=0;
						  if ((int)_v->m_size.cy > _ch && _v->m_scrollpoint.y > (int)_v->m_size.cy-_ch) _v->m_scrollpoint.y=(int)_v->m_size.cy-_ch;
						  if (_v->m_scrollpoint.y < 0) _v->m_scrollpoint.y=0;
						  RDialog::m_pView->UpdateBitmaps(RDialog::m_pView->GetDC());
#if defined(MA_LINUX)
						  /* Phase 1 CRToolBar hosting (BoB S88-92 recipe): parent-scoped draw of the
						     map toolbars' hosted RButtons over the map -- no global-draw stale bleed. */
						  /* S52: unit-conversion factors (mass.gm/alt.mediummm) are 0 until SetUnits; the OOB
						     dialogs divide by them in OnInitDialog (SIGFPE). Ensure they are set before a click. */
						  if (!Save_Data.mass.gm) Save_Data.SetUnits();
						  if (!getenv("MA_NO_MAP_TOOLBARS")) { CMainFrame* _mf=(CMainFrame*)AfxGetMainWnd(); if(_mf){  /* map chrome: main toolbar icon buttons (S48/S49). MA_NO_MAP_TOOLBARS disables. */
							  /* S109 (PO-11): the filter toolbar is TWO rows (extent 393x48) and was
							     drawn at y=26 with the main toolbar at y=52 -- a 22px overlap that
							     hid the red row behind the main row. It only became visible once the
							     buttons had art. Filters go to the top edge, main row directly under
							     them, which is the gold's stacking. */
							  { int _fw=0,_fh=0; ma_ole_dialog_extent(&_mf->m_toolbar1, &_fw, &_fh);
							    g_filt_ox = (_fw>0 && _cw>_fw+8) ? (_cw - _fw - 4) : 4; g_filt_oy = 4;
							    /* right-aligned so the map's DATE readout keeps the top-left corner it
							       has always had -- drawing the filter rows at x=4 covered it, which is
							       the same "a widget must not change the screen it draws on" rule S97
							       recorded for the system box. Gold has the date far left and the filter
							       rows to its right on the same band. */
							    ma_ole_draw_toolbar(&_mf->m_toolbar1, (void*)1, g_filt_ox, g_filt_oy); }
							  ma_ole_draw_toolbar(&_mf->m_toolbar2, (void*)1, 4, 52);
							  /* S109 (PO-11): the MISC toolbar (6 buttons: zoom in/out, save...) was
							     hosted but drawn NOWHERE -- the same enumeration gap S106 found with
							     the debrief toolbar's dialogs. Gold puts it on the main toolbar's
							     band, to its right, so right-align it there from its own extent
							     rather than a hardcoded x (S94's rule). Drawing it was pointless
							     before this sprint: without art it would have been six blank
							     rectangles. MA_NO_MISC_TOOLBAR reverts. */
							  if (!getenv("MA_NO_MISC_TOOLBAR")) {
								  int _mw=0,_mh=0;
								  if (ma_ole_dialog_extent(&_mf->m_toolbar3, &_mw, &_mh) && _mw>0) {
									  g_misc_ox = _cw - _mw - 4; if (g_misc_ox < 0) g_misc_ox = 0;
									  g_misc_oy = 52;
									  ma_ole_draw_toolbar(&_mf->m_toolbar3, (void*)1, g_misc_ox, g_misc_oy);
								  }
							  }
							  /* S108 (PO-11): the PO reports "many widgets are missing" on the
							     campaign map. Before drawing anything new, ask how many controls
							     each toolbar actually HOSTS -- a toolbar with zero hosted controls
							     is a different defect from one that is hosted and undrawn, and the
							     gold shows five distinct clusters (two filter rows, the main
							     toolbar, a right-hand group and the scale ruler down the left). */
							  if (getenv("MA_TRACE_TOOLBARS")) { static int _tb=0; if (!_tb++) {
								  struct { const char* n; CRToolBar* t; } _tbs[5] = {
									  {"filters(t1)",&_mf->m_toolbar1}, {"main(t2)",&_mf->m_toolbar2},
									  {"misc(t3)",(CRToolBar*)&_mf->m_toolbar3}, {"scale(t4)",(CRToolBar*)&_mf->m_toolbar4},
									  {"debrief(t5)",&_mf->m_toolbar5} };
								  for (int _i=0;_i<5;_i++) { int _w=0,_h=0;
									  int _found = ma_ole_dialog_extent(_tbs[_i].t, &_w, &_h);
									  fprintf(stderr,"[toolbars] %-12s hosted=%2d extent=%dx%d\n",
									          _tbs[_i].n, _found, _w, _h); } } }
							  /* S94 (PO-1): the SYSTEM BOX -- the upper-right widget cluster. It hosts
							     IDC_FILES (-> OnBye, the way OUT of the campaign), IDC_THUMBNAIL
							     (-> OnGoSmall) and IDC_ZOOMIN (-> OnGoNormal/zoom). CMainFrame creates
							     it and RDialog enables/disables it, but nothing ever DREW it, so the
							     campaign had no exit or resize widgets at all -- reported from play.
							     Placed at the canvas's top-right from the box's own control extent, not
							     a hardcoded width (same rule as the click recipes). */
							  /* S97 (PO-1): ON BY DEFAULT (MA_NO_SYSBOX reverts). S94 had to leave this
							     opt-in for two reasons, both now fixed. (1) The buttons had no art --
							     they do now. (2) Drawing the box left a different GDI font selected and
							     the map's date text, drawn from the same DC later in the frame,
							     inherited it: the date rendered in a plain sans instead of the game's
							     art face. A widget must not change the state of the screen it draws on,
							     so the DC's font is saved and restored around the draw. */
							  if (!getenv("MA_NO_SYSBOX") && _mf->m_wndSystemBox) {
								  int _bw=0,_bh=0;
								  int _found = ma_ole_dialog_extent(_mf->m_wndSystemBox, &_bw, &_bh);
								  if (getenv("MA_TRACE_SYSBOX")) { static int _n=0; if(_n++<3)
									  fprintf(stderr,"[sysbox] box=%p hosted=%d extent=%dx%d\n",
									          (void*)_mf->m_wndSystemBox, _found, _bw, _bh); }
								  if (getenv("MA_TRACE_SYSBOX"))
								  fprintf(stderr,"[sysbox] wnd=%p found=%d extent=%dx%d canvas=%dx%d\n",
								          (void*)_mf->m_wndSystemBox, _found, _bw, _bh, _cw, _ch);
							  if (_found && _bw>0) {
									  g_sysbox_ox = _cw - _bw - 4; g_sysbox_oy = 4;
									  if (g_sysbox_ox < 0) g_sysbox_ox = 0;
									  /* composite it here and ONLY here */
								  static void* _sbReg = 0;
								  if (_sbReg != _mf->m_wndSystemBox) {
									  _sbReg = _mf->m_wndSystemBox;
									  ma_ole_set_parent_scoped(_sbReg);
								  }
								  void* _sdc = ma_gdi_screen_dc();
									  void* _oldFont = ma_gdi_get_font(_sdc);
									  ma_ole_draw_toolbar(_mf->m_wndSystemBox, (void*)1, g_sysbox_ox, g_sysbox_oy);
									  ma_gdi_set_font(_sdc, _oldFont);
								  }

							  }
							  ma_map_paint_oob();   /* S53: render any open OOB-info dialog over the map */
							  /* S112 (PO-10): the DOCUMENTATION panel. The "?" button's route reaches
							     CWinApp::WinHelp (S98 fixed four broken links to get it there) and
							     that was still a no-op -- "clicking ? does nothing", as reported.
							     The panel lists the game's own help topics, read from MIG.HLP's
							     |TTLBTREE at runtime, and says plainly that the topic TEXT is not
							     decoded yet: the phrase-compression opcode table is still unsolved
							     (S99, and S112's 800-candidate search scored 2/39 against the title
							     oracle). Showing the index is honest; showing plausible nonsense
							     would be worse than showing nothing. A click anywhere closes it. */
							  if (ma_help_is_open()) {
								  void* _hdc = ma_gdi_screen_dc();
								  void* _hf  = ma_gdi_get_font(_hdc);
								  int _n = ma_help_topic_count();
								  int _px = 40, _py = 110, _pw = _cw - 80, _ph = _ch - 170;
								  if (_pw < 200) _pw = 200;
								  if (_ph < 120) _ph = 120;
								  /* the canvas takes B,G,R in the low bytes */
								  ma_gdi_fill_rect(_hdc, _px, _py, _px + _pw, _py + _ph, 0x00403020);
								  ma_gdi_fill_rect(_hdc, _px + 2, _py + 2, _px + _pw - 2, _py + 24, 0x00785840);
								  ma_gdi_set_text_color(_hdc, 0x00FFFFFF);
								  int _sel = ma_help_current();
								  { char _hdr[160];
								    if (_sel >= 0) snprintf(_hdr, sizeof(_hdr), "MiG Alley - %s   (click anywhere to close)", ma_help_topic(_sel));
								    else           snprintf(_hdr, sizeof(_hdr), "MiG Alley - Documentation: %d topics   (click anywhere to close)", _n);
								    ma_gdi_text_out(_hdc, _px + 10, _py + 6, _hdr, (int)strlen(_hdr)); }
								  int _y = _py + 32;
								  if (_sel >= 0) {
									  /* S114: the topic's REAL text, extracted from the game's own
									     MIG.RTF help source. Wrapped to the panel here rather than
									     at extraction time, so the data file stays the source's own
									     paragraphs. */
									  int _bl = ma_help_body_lines();
									  int _wrap = (_pw - 24) / 7;          /* ~7px per char in this face */
									  if (_wrap < 20) _wrap = 20;
									  for (int _i = 0; _i < _bl && _y < _py + _ph - 24; _i++) {
										  const char* _t = ma_help_body_line(_i);
										  int _len = (int)strlen(_t), _off = 0;
										  while (_off < _len && _y < _py + _ph - 24) {
											  int _take = _len - _off; if (_take > _wrap) _take = _wrap;
											  if (_off + _take < _len) {           /* break on a space */
												  int _b = _take;
												  while (_b > 8 && _t[_off + _b] != ' ') _b--;
												  if (_b > 8) _take = _b;
											  }
											  ma_gdi_text_out(_hdc, _px + 12, _y, _t + _off, _take);
											  _off += _take; while (_off < _len && _t[_off] == ' ') _off++;
											  _y += 13;
										  }
									  }
								  } else {
									  int _col = _px + 12, _cw2 = _pw / 2 - 16;
									  for (int _i = 0; _i < _n; _i++) {
										  const char* _t = ma_help_topic(_i);
										  if (!_t || !*_t) continue;
										  ma_gdi_text_out(_hdc, _col, _y, _t, (int)strlen(_t));
										  _y += 14;
										  if (_y > _py + _ph - 34) { _y = _py + 32; _col += _cw2; if (_col > _px + _pw - 60) break; }
									  }
								  }
								  ma_gdi_set_text_color(_hdc, 0x0070D0FF);
								  { const char* _f = _sel >= 0
										? "From the game's own help source (SRC/<LANG>/HELP/MIG.RTF)."
										: "No help topic is mapped to this screen's context id - showing the index.";
								    ma_gdi_text_out(_hdc, _px + 10, _py + _ph - 18, _f, (int)strlen(_f)); }
								  ma_gdi_set_font(_hdc, _hf);
							  }
							  /* S95 diagnostic (opt-in): ask the map itself where its clickable items are,
							     by running its own hit-test over a grid. Needed because icon positions
							     depend on campaign state and scroll, so no coordinate can be hardcoded
							     in a test. Prints once. */
							  if (getenv("MA_MAP_ITEM_SCAN") && RDialog::m_pView) {
								  static int _scanned=0, _scanf=0;
								  if (!_scanned && ++_scanf >= atoi(getenv("MA_MAP_ITEM_SCAN")) && ++_scanned) {
									  int _n=0; static int _seen[64]; int _ns=0; int _fx=-1,_fy=-1;
									  for (int _y=0; _y<_ch && _n<48; _y+=6)
										  for (int _x=0; _x<_cw && _n<48; _x+=6) {
											  int _id = RDialog::m_pView->m_mapdlg.FindMapItem(CPoint(_x,_y));
											  if (!_id) continue;
											  int _dup=0; for (int _k=0;_k<_ns;_k++) if (_seen[_k]==_id) _dup=1;
											  if (_dup) continue;
											  if (_ns<64) _seen[_ns++]=_id;
											  fprintf(stderr,"[mapitem] (%d,%d) id=%d(0x%x) band=%d\n",_x,_y,_id,_id,
											          Persons2::getbandfromUID((UniqueID)_id));
											  _n++;
											  if (_fx < 0 && _y >= 100) { _fx=_x; _fy=_y; }
										  }
									  fprintf(stderr,"[mapitem] scan done, %d hits over %dx%d\n",_n,_cw,_ch);
									  /* S95: click the first item found clear of the toolbars. This is what makes
									     PO-3 gateable -- icon coordinates move with campaign state, canvas size and
									     scroll (the first hand-picked coordinate went stale between two runs of
									     this very sprint), so the test asks the map where an icon IS and clicks
									     THAT, instead of hardcoding a point that is only right today. */
									  if (getenv("MA_MAP_CLICK_FIRST") && _fx >= 0) {
										  fprintf(stderr,"[mapitem] clicking first item at (%d,%d)\n",_fx,_fy);
										  RDialog::m_pView->m_mapdlg.MaDriveClick(CPoint(_fx,_fy));
									  }
								  }
							  }
							  /* S50 Phase-3: a click on a toolbar button fires its handler (OnClickedFrag2/...). */
							  { int _tcx=0,_tcy=0; if (ma_mouse_take_click(&_tcx,&_tcy)) {
								  /* S82: an OPEN OOB dialog is painted over the map, so it gets first
								     refusal on the click — otherwise its tabs/tick/rows are dead and
								     the click hits the toolbar or map behind it. */
								  /* S112 (PO-10): the documentation panel is modal-ish -- a click
								     anywhere dismisses it and goes no further, so it cannot pan the
								     map underneath (the same swallow rule as the OOB dialogs). */
								  if (ma_help_is_open()) { ma_help_open(0); }
								  else if (!ma_map_click_oob(_tcx, _tcy))
								  if (!(!getenv("MA_NO_SYSBOX") && _mf->m_wndSystemBox && g_sysbox_ox >= 0 &&
								        ma_ole_toolbar_click(_mf->m_wndSystemBox, g_sysbox_ox, g_sysbox_oy, _tcx, _tcy)))
								  if (!ma_ole_toolbar_click(&_mf->m_toolbar2, 4, 52, _tcx, _tcy))
								  /* S109: the filter rows moved to the top edge (see the draw), and the
								     misc toolbar is now drawn -- so both must take clicks at the SAME
								     offsets they were painted at. A widget that draws but cannot be
								     clicked is only half hosted (S106). */
								  if (g_filt_ox < 0 || !ma_ole_toolbar_click(&_mf->m_toolbar1, g_filt_ox, g_filt_oy, _tcx, _tcy))
								  if (getenv("MA_NO_MISC_TOOLBAR") || g_misc_ox < 0 ||
								      !ma_ole_toolbar_click(&_mf->m_toolbar3, g_misc_ox, g_misc_oy, _tcx, _tcy))
								  {
									  /* S95 (PO-3): nothing above wanted the click, so it belongs to the MAP
									     itself. The engine already has the whole chain -- CMapDlg::OnLButtonDown
									     -> FindMapItem() -> m_buttonid, then OnLButtonUp -> OnClickItem() ->
									     MainToolBar().OpenDossier(uid), the target dossier ("recon") dialog.
									     The port simply never delivered a mouse event to m_mapdlg, so clicking
									     an airfield did nothing (reported from play).
									     Drive the control's own handlers rather than re-implementing the
									     hit-test, exactly as the listbox/button paths do. Down+Up in the same
									     tick keeps m_bDragging FALSE, which also avoids CMapDlg::OnMouseMove --
									     it dereferences GetDC() unchecked (the S82 "the genuine handler may
									     contain an unported call" rule). MA_NO_MAP_ITEM_CLICK reverts. */
									  if (!getenv("MA_NO_MAP_ITEM_CLICK") && RDialog::m_pView)
									  {
										  RDialog::m_pView->m_mapdlg.MaDriveClick(CPoint(_tcx, _tcy));
										  if (getenv("MA_TRACE_CLICK"))
											  fprintf(stderr,"[mapclick] drove CMapDlg down/up at (%d,%d)\n", _tcx, _tcy);
									  }
								  } } } } }
#endif
						}
						/* M4 inc2 (gated MA_CAMP_FLY): auto-drive the campaign mission->fly chain. The map's
						   frag button (CMainToolbar::OnClickedFrag2) launches the singlefrag briefing
						   panel when a flyable mission exists -> its Fly button -> StartFlying (the M1
						   flight path) -> 3D -> OnFlyingClosed (CAMP branch) -> back to map. */
						if (!_navExit && getenv("MA_CAMP_FLY")) {
							if (++_fragn == 40) {
								if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] driving frag (OnClickedFrag2) -> singlefrag [campaign: %s]\n", ma_camp_state());
								_campStage = 1;
								MainToolBar().OnClickedFrag2();
							}
						}
						/* S80 G2 (gated MA_CAMP_LOOP=N): the FLYABLE MULTI-MISSION LOOP. After a flown
						   campaign mission, OnFlyingClosed's CAMP branch (FULLPANE.CPP:2674) sets
						   MMC.indebrief, calls NextMission, opens the mission-results child and returns
						   here to the map with the DEBRIEF toolbar up. The next link is that toolbar's
						   Next Period button (DBRFTLBR.CPP:226 -> MMC.EndDebrief -> the next period's
						   planning map); driving it and re-arming the frag/Fly counters flies mission
						   N+1. Fly N missions total, then stop driving and leave the campaign on the map. */
						if (!_navExit && getenv("MA_CAMP_LOOP") && ma_camp_indebrief()) {
							if (++_loopSettle == 20) {
								_loopSettle = 0;
								_loopFlown++;
								if (getenv("MA_TRACE_3D")) fprintf(stderr,"[loop] mission %d debriefed -> driving Next Period (EndDebrief) [campaign: %s]\n", _loopFlown, ma_camp_state());
								ma_camp_next_period();
								if (getenv("MA_TRACE_3D")) fprintf(stderr,"[loop] after Next Period [campaign: %s]\n", ma_camp_state());
								if (_loopFlown < atoi(getenv("MA_CAMP_LOOP"))) {
									_fragn = 0; _flyn = 0; _campStage = 0;   /* re-arm: frag + fly mission N+1 */
									if (getenv("MA_TRACE_3D")) fprintf(stderr,"[loop] re-armed frag/Fly drives for mission %d\n", _loopFlown+1);
								} else {
									if (getenv("MA_TRACE_3D"))
										fprintf(stderr,"[loop] MA_CAMP_LOOP target of %d mission(s) reached; drive stops\n", _loopFlown);
									/* arm the post-loop capture (MA_CAMP_LOOP_SHOT): let the map settle,
									   then dump the canvas -- evidence that the campaign is back on a
									   clean, renderable map after N flown missions. */
									if (getenv("MA_CAMP_LOOP_SHOT")) ma_loop_shot_countdown = 30;
								}
							}
						}
						/* S56 I4 (gated MA_OOB_PLAYERLOG): open the Player Log OOB dialog once the map
						   has settled -- headless capture path for the Wine-parity gold shot #15
						   (BOB_DUMP_FRAME then snapshots map + toolbar + Playerlog overlay). */
						if (!_navExit && getenv("MA_OOB_PLAYERLOG")) {
							static int _pln=0;
							if (++_pln == 40) {
								if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] driving OpenPlayerlog (MA_OOB_PLAYERLOG)\n");
								MainToolBar().OpenPlayerlog();
								/* S61 (gated MA_OOB_PLAYERLOG_TAB=N): select tab N
								   (0=Career, 1=Log of Missions, 2=Last Mission) in the SAME tick,
								   exactly as CMainToolbar::OnClickedMissionlog does for tab 2 —
								   GetDlgItem(IDJ_TABCTRL) on the HTabBox host, then SelectTab.
								   Lets a headless capture prove tab SWITCHING, not just that the
								   bar draws. Must be same-tick: once the dialog is up the map-idle
								   branch stops running, so a later counter never fires. */
								const char* _ptab = getenv("MA_OOB_PLAYERLOG_TAB");
								if (_ptab) {
									RDialog* _plc = MainToolBar().LoggedChild(CMainToolbar::PLAYERLOG);
									if (_plc && _plc->fchild && _plc->fchild->fchild) {
										CRTabs* _tab = (CRTabs*)_plc->fchild->fchild->GetDlgItem(IDJ_TABCTRL);
										if (_tab) {
											if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] SelectTab(%d)\n", atoi(_ptab));
											_tab->SelectTab(atoi(_ptab));
										} else if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] SelectTab: no tab control\n");
									} else if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] SelectTab: tree not built as expected\n");
								}
							}
						}
					/* S74 I1 (gated MA_OOB_OVERVIEW): open the Overview stats panel once the map
					   has settled -- CAC_view is the aircraft claims/kills table also shown at the
					   post-mission debrief, so this is a headless capture path for gold shot #12
					   (Claims table). NOTE: campaign-map Overview context, not the post-mission
					   debrief toolbar -- the CAC_view table content is the same, chrome differs. */
					if (!_navExit && getenv("MA_OOB_OVERVIEW")) {
						static int _dbn=0;
						if (++_dbn == 40) {
							if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] driving OnClickedOverview (MA_OOB_OVERVIEW)\n");
							MainToolBar().OnClickedOverview();
						}
					}
					/* ASan (S42, gated MA_CAMP_NEXTDAY): drive the day-advance / strategic-sim path
					   (OnClickedFrag2 forces frag2 no-flyable -> NextMission -> NextDay + NextPeriod). */
					if (!_navExit && getenv("MA_CAMP_NEXTDAY")) {
						static int _ndn=0;
						if (++_ndn == 40) {
							if (getenv("MA_TRACE_3D")) fprintf(stderr,"[map] driving NextDay (MA_CAMP_NEXTDAY)\n");
							MainToolBar().OnClickedFrag2();
						}
					}
					}
				}
			/* 3D flight bring-up: when the flight view (Rtestsh1) is active, drive its setup
			   status machine. The ctor sets S3D_STARTSETUP; OnPaint normally feeds S3D_DONESHEET
			   and the doc/view background feeds S3D_DONEBACK — but that background path is
			   deadcoded in our compat, so status never reaches 7 (==STARTSETUP|DONESHEET|DONEBACK)
			   and Launch3d is never triggered. Feed both bits here so setup completes -> Start3d
			   posts WM_GETSTRING -> OnGetString -> Launch3d (the world bring-up). [step 1 of the
			   3D render-loop wiring; the WM_GETSTRING delivery + Launch3d are the next links.] */
			if (Rtestsh1::THISTHIS
				&& Rtestsh1::setup3dstatus != Rtestsh1::S3D_GOING
				&& Rtestsh1::setup3dstatus != Rtestsh1::S3D_STOPPED) {
				Rtestsh1::Start3d(Rtestsh1::S3D_DONESHEET);
				Rtestsh1::Start3d(Rtestsh1::S3D_DONEBACK);
				if (getenv("MA_TRACE_3D")) { static int _n=0; if(_n++<8)
					fprintf(stderr,"[3d] flight-view active; setup3dstatus=%d (8=GOING: setup done, WM_GETSTRING posted)\n", Rtestsh1::setup3dstatus); }
			}
			/* setup complete (GOING) but the WM_GETSTRING self-post isn't delivered by our pump:
			   drive OnGetString -> Launch3d (world bring-up) directly, ONCE (tmpinst==NULL gate).
			   DEFAULT-ON now: the 3D DirectDraw init, software rasterizer, sim/draw threads, and
			   the flight->menu round-trip (ma_process_flight_close) are all wired and validated, so
			   clicking "Fly"/"Hot Shot" enters real 3D flight. Set MA_DISABLE_3D=1 to keep the flight
			   path non-fatal (2D only) for front-end-only debugging. (MA_ENABLE_3D still works too.) */
			if (!getenv("MA_DISABLE_3D")
				&& Rtestsh1::THISTHIS && Rtestsh1::setup3dstatus == Rtestsh1::S3D_GOING
				&& Rtestsh1::tmpinst == NULL) {
				if (getenv("MA_TRACE_3D")) fprintf(stderr,"[3d] driving Launch3d (OnGetString)...\n");
				Rtestsh1::THISTHIS->MaDriveLaunch();
				if (getenv("MA_TRACE_3D")) fprintf(stderr,"[3d] Launch3d returned; tmpinst=%p tmpview=%p\n", (void*)Rtestsh1::tmpinst, (void*)Rtestsh1::tmpview);
			}
			/* S58 (MA_SHOT, default-off): deterministic one-shot 2D parity capture, the BoB
			   BOB_SHOT recipe. After N front-end idles (panel AND map branches both funnel
			   through this present site) dump the GDI screen canvas straight to MA_SHOT_PATH
			   (default /tmp/mashot.ppm) and exit. Reads the software canvas, NOT the GL
			   framebuffer, so it works under SDL_VIDEODRIVER=dummy with no GL context --
			   the 2D parity evidence no longer depends on display health. */
			/* S80: the MA_CAMP_LOOP-armed variant of the same capture (see the countdown's
			   declaration) -- fires a fixed number of settled front-end idles AFTER the loop
			   drive finished, not at an absolute idle number. */
			if (!ma_in3d && ma_loop_shot_countdown > 0) {
				if (--ma_loop_shot_countdown == 0) {
					const char* sp = getenv("MA_SHOT_PATH"); if (!sp || !*sp) sp = "/tmp/mashot.ppm";
					int nz = ma_gdi_dump_to(sp);
					fprintf(stderr, "[shot] campaign loop complete -> %s (nonblack=%d) -- exiting\n", sp, nz);
					fflush(stderr);
					_exit(0);
				}
			}
			if (!ma_in3d && getenv("MA_SHOT")) {
				static long _shot_t = 0;
				if (++_shot_t >= atol(getenv("MA_SHOT"))) {
					const char* sp = getenv("MA_SHOT_PATH"); if (!sp || !*sp) sp = "/tmp/mashot.ppm";
					int nz = ma_gdi_dump_to(sp);
					fprintf(stderr, "[shot] front-end idle %ld -> %s (nonblack=%d) -- exiting\n",
						_shot_t, sp, nz); fflush(stderr);
					_exit(0);
				}
			}
			if (!ma_in3d)
				ma_gdi_present_screen();
#endif
//			TRACE0("0");
////			*(char*)0xb0A00+=1;
			if (!OnIdle(lIdleCount++))
				bIdle = FALSE; // assume "no idle" state
		}
	}

	ASSERT(FALSE);  // not reachable
}



