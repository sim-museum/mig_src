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

//------------------------------------------------------------------------------
//Filename       keystub.cpp
//System         
//Author         Andy McRea
//Date           Tue 4 Jun 1996
//Description    Keyboard testing module stub
//------------------------------------------------------------------------------

#include	<dos.h>
//#include	"stdafx.h"
#include	"DosDefs.h"

#include	"Files.g"
#include	"fileman.h"

#include	"hardpasm.h"										//ARM 04Jun96
#pragma message ("Keystub")

#define	SPECIAL_KEYMAPS	1
#include	"keytest.h"
KeyMapping	*Debug3dMapTable=ThisKeyMapping;


keytests	Key_Tests;

//������������������������������������������������������������������������������
//Procedure		keytests
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	CONSTRUCTOR
//	//constructor for keyboard tests
	//creates a number of DOS memory buffers & initialises them
	//copies DOS ISR to one of the buffers
	//locks DEX ISR 
	//installs both ISRs		
//
//Inputs		None!
//
//Returns		None!
//
//------------------------------------------------------------------------------
extern	void	DLWRITE(void*,ULong);
#ifdef __WATCOMC__
#pragma	aux		DLWRITE			=	\
		"mov	ds:[esi],eax"		\
		parm	[esi] [eax]			
#else
#ifdef __MSVC__
inline	void	DLWRITE(void* num1,ULong num2)
{
#if defined(__GNUC__)
	*(ULong*)num1 = num2;	// Linux/GCC port: the asm just writes eax to ds:[esi]
#else
    __asm
    {
		mov esi,num1;
		mov eax,num2;
		mov ds:[esi],eax;
    }
#endif
}

#endif
#endif

	keytests::keytests()
{	
int	i;
	//create a number of DOS memory buffers & initialises them
	//UI queue
	hitqueueui.flat=new UIKeyQueue;
	//3d mapping
	reftable3d.flat=new KeyMap3d;
	//Clear out
	ClearKeyTables();

	//Buffer for DOS mode ISR

	//copy DOS ISR to the buffer

	//lock DEX ISR - borrowed this code...

	//INSTALL BOTH ISRs
	//Note old vectors

	//Set new vectors

//TEST CODE

//	RegUIConv((FileNum) 0);
//	InsertUIKeyThru=InsertUIKey;
	KeyFake3dThru=&keytests::KeyFake3d;/*Linux port: qualified member-fn ptr*/
}	

//������������������������������������������������������������������������������
//Procedure		keytests (destructor)
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
	keytests::~keytests()
{	//destructor for keyboard tests
	//disinstalls both ISRs
	//unlocks DEX ISR
	//frees lumps of memory

	//disinstall both ISRs

	//unlock DEX ISR

	//frees lumps of memory

	delete	(UIKeyQueue*)	hitqueueui.flat;
	delete	(KeyMap3d*)		reftable3d.flat;
	
}

//������������������������������������������������������������������������������
//Procedure		ClearKeyTables
//				(private)
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	Clears out data structures for keyboard- discards unused presses
//
//Inputs		none
//
//Returns		none
//
//------------------------------------------------------------------------------
void	keytests::ClearKeyTables()
{

int	i,j;
	//First, disable both keyboard systems
UIKeyQueue*
	keyqueue=(UIKeyQueue*)	hitqueueui.flat;
	keyqueue->active=FALSE;
KeyMap3d*
	keymap=(KeyMap3d*) reftable3d.flat;
	keymap->active=FALSE;

	//now zero everything in queue

	keyqueue->currshifts=0;
	keyqueue->addref=0;	//you may add when they are equal.
						//You can not add if they will become equal
	keyqueue->takeref=0;	//you may not take when they are equal
	keyqueue->queuesize=MAXqueuesize;
	for (i=16;i--;)
		keyqueue->keyholds[i]=0;

	//now zero everything in 3d system

	keymap->currshifts=0;
	for (i=16;i--;)
		keymap->keyholds[i]=0;
	for	(i=KeyMap3d::MAXKEYS;i--;)
		for (j=8;j--;)
			keymap->mappings[i][j]=0;
	for (i=(TOTALKEYBITFLAGS+7)/4;i--;)							//RJS 29Jan98
		keymap->bitflags[i]=0;
}


//������������������������������������������������������������������������������
//Procedure		Reg3dConv
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	Register a new conversion table
//
//Inputs		filenum:	table containing conversion
//
//Returns		none
//
//------------------------------------------------------------------------------
//void	keytests::Reg3dConv(FileNum	filenum)
//{
//	if (filenum)
//	{
//fileblock
//		tmp(filenum);
//This file is made up of word pairs: 
//1st word indexes the mappings array by 2 bytes - 0-128,0-7. 
//2nd word indexes the bitflags by bit index. Always even. 2 bits per mapping.
//
//		Reg3dConv((KeyMapping*) getdata(tmp),getsize(tmp)/4);
//	}
//	else
//	{
//		ClearKeyTables();
//		((KeyMap3d*) reftable3d.flat)->active=TRUE;
//	}
//} 

//������������������������������������������������������������������������������
//Procedure		Reg3dConv
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	maps 1 3d keypress manually
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
static __int64	lastkeychecksum=0;
void	keytests::Reg3dConv(KeyMapping* mapreq,int	i)
{
	ClearKeyTables();
KeyMap3d*
	keymap=(KeyMap3d*) reftable3d.flat;
	int	keychecksum=0;
	while (i--)
	{
		keychecksum=keychecksum*2+(keychecksum>>63)+mapreq->scancode+mapreq->shiftstate+mapreq->bitflag;
		if (mapreq->scancode<KeyMap3d::MAXKEYS && mapreq->shiftstate<8) keymap->mappings[mapreq->scancode][mapreq->shiftstate]=mapreq->bitflag;
		mapreq++;
		breakif(i==0 || mapreq->bitflag==0);
	}
	((KeyMap3d*) reftable3d.flat)->active=TRUE;
	ModeIs3D=TRUE;
	if (lastkeychecksum==0)
		lastkeychecksum=keychecksum;
	else
		if(lastkeychecksum!=keychecksum)
			_Error.EmitSysErr("Key table has changed between loads???");
}
void keytests::Reg3dConv(FileNum f)
{
	fileblock b=f;
	Reg3dConv((KeyMapping*)b.getdata(),b.getsize()/sizeof(KeyMapping));


}
//������������������������������������������������������������������������������
//Procedure		RegUIConv
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	Register a userinterface keyboard conversion
//				This will be a stack of converters, starting with the
//				main user-interface keys.
//
//Inputs		filenum: translation list
//
//Returns	
//
//------------------------------------------------------------------------------
void	keytests::RegUIConv(FileNum )	//filenum)
{
//	if (((UIKeyQueue*)	hitqueueui.flat)->active==FALSE)
//		ClearKeyTables();
//This file registers a foreground translation buffer rather than an interrupt feature
//So I will ignore for now, and just activate the interrupt service!
//	((UIKeyQueue*)	hitqueueui.flat)->active=TRUE;
//	ModeIs3D=FALSE;
}

//������������������������������������������������������������������������������
//Procedure		UnRegUIConv
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	Deregisters keyboard mapping for user interface
//				The user interface mappings are a stack
//
//Inputs		none
//
//Returns		none
//
//------------------------------------------------------------------------------
void	keytests::UnRegUIConv()
{

}
//������������������������������������������������������������������������������
//Procedure		GetUIKey
//Author		Jim Taylor
//Date			Mon 30 Oct 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
Bool	keytests::GetUIKey(UIKeyEvent& returnedevent)
{
//UIKeyQueue*
	//keyqueue=(UIKeyQueue*)	hitqueueui.flat;
//int	i;
	//{	_disable ();
		//i=keyqueue->addref - keyqueue->takeref;
//TempCode JIM 30Jan96 char*	B000=(char*) 0xB0000;
//TempCode PD 06Dec95 		B000[160*20]=(char) (keyqueue->addref+'a');
//TempCode PD 06Dec95 		B000[160*20+2]=(char) (keyqueue->takeref+'a');
	//}	_enable();
	//if	(i==0)	return(FALSE);
	//returnedevent=keyqueue->keyqueue[keyqueue->takeref];
	//{	_disable ();
		//keyqueue->takeref+=1;
		//if (keyqueue->takeref==keyqueue->queuesize)
			//keyqueue->takeref=0;
	//}	_enable();
	return	(TRUE);
}

//������������������������������������������������������������������������������
//Procedure		mousepos
//Author		Jim Taylor
//Date			Tue 30 Jan 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
//ScreenXY&	keytests::mousepos()
//{
//UIKeyQueue*
//	keyqueue=(UIKeyQueue*)	hitqueueui.flat;
//	return(keyqueue->currmouse);
//}

//������������������������������������������������������������������������������
//Procedure		InsertUIKey
//Author		Jim Taylor
//Date			Tue 30 Jan 1996
//
//Description	insert a key press/release event into the key queue
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	keytests::InsertUIKey(UIKeyNoMouse& keyfake)
{
//UIKeyQueue*
//	keyqueue=(UIKeyQueue*)	hitqueueui.flat;
//	{	_disable();
//int		oldadd=keyqueue->addref;
//int		addref=oldadd+1;
//		if (addref==keyqueue->queuesize)
//			addref=0;
//		if (addref==keyqueue->takeref)
//			return;
//		keyqueue->keyqueue[oldadd].keycode=keyfake.keycode;
//		keyqueue->keyqueue[oldadd].keyhit=keyfake.keyhit;
//		if (keyfake.keyshifts)
//			keyqueue->keyqueue[oldadd].keyshifts=keyfake.keyshifts;
//		else
//			keyqueue->keyqueue[oldadd].keyshifts=keyqueue->currshifts;
//		keyqueue->keyqueue[oldadd].mousecoords=keyqueue->currmouse;
//		keyqueue->addref=addref;
//	}	_enable();
} 

//������������������������������������������������������������������������������
//Procedure		BTR
//Author		Jim Taylor
//Date			Fri 3 Nov 1995
//
//Description	bit test and reset assembler macro. 
//				It will be fun to get this working - not!
//
//Inputs		pointer to word memory
//				index to test
//
//Returns		false=0/true
//
//------------------------------------------------------------------------------
//DeadCode ARM 04Jun96 extern	Bool	BITRESET(void*,ULong);
//DeadCode ARM 04Jun96 #pragma	aux		BITRESET		=		\
//DeadCode ARM 04Jun96 		"btr	ds:[esi],ecx"	\
//DeadCode ARM 04Jun96 		"sbb	al,al"		\
//DeadCode ARM 04Jun96 		"neg	al"			\
//DeadCode ARM 04Jun96 		parm	[esi] [ecx]	\
//DeadCode ARM 04Jun96 		value	[al]			\

//������������������������������������������������������������������������������
//Procedure		BTS
//Author		Jim Taylor
//Date			Fri 3 Nov 1995
//
//Description	bit test and reset assembler macro. 
//				It will be fun to get this working - not!
//
//Inputs		pointer to word memory
//				index to test
//
//Returns		false=0/true
//
//------------------------------------------------------------------------------
//DeadCode ARM 04Jun96 extern	Bool	BITSET(void*,ULong);
//DeadCode ARM 04Jun96 #pragma	aux		BITSET		=		\
//DeadCode ARM 04Jun96 		"bts	ds:[esi],ecx"	\
//DeadCode ARM 04Jun96 		"sbb	al,al"		\
//DeadCode ARM 04Jun96 		"neg	al"			\
//DeadCode ARM 04Jun96 		parm	[esi] [ecx]	\
//DeadCode ARM 04Jun96 		value	[al]			\

//������������������������������������������������������������������������������
//Procedure		BTS
//Author		Jim Taylor
//Date			Fri 3 Nov 1995
//
//Description	bit test and reset assembler macro. 
//				It will be fun to get this working - not!
//Inputs		pointer to word memory
//				index to test
//
//Returns		false=0/true
//
//------------------------------------------------------------------------------
//DeadCode ARM 04Jun96 extern	Bool	BITTEST(void*,ULong);
//DeadCode ARM 04Jun96 #pragma	aux		BITTEST		=		\
//DeadCode ARM 04Jun96 		"bt		ds:[esi],ecx"	\
//DeadCode ARM 04Jun96 		"sbb	al,al"		\
//DeadCode ARM 04Jun96 		"neg	al"			\
//DeadCode ARM 04Jun96 		parm	[esi] [ecx]	\
//DeadCode ARM 04Jun96 		value	[al]			\

//������������������������������������������������������������������������������
//Procedure		BTC
//Author		Jim Taylor
//Date			Fri 3 Nov 1995
//
//Description	bit test and reset assembler macro. 
//				It will be fun to get this working - not!
//
//Inputs		pointer to word memory
//				index to test
//
//Returns		false=0/true - old value
//
//------------------------------------------------------------------------------
//DeadCode ARM 04Jun96 extern	Bool	BITCOMP(void*,ULong);
//DeadCode ARM 04Jun96 #pragma	aux		BITCOMP		=		\
//DeadCode ARM 04Jun96 		"btc	ds:[esi],ecx"	\
//DeadCode ARM 04Jun96 		"sbb	al,al"		\
//DeadCode ARM 04Jun96 		"neg	al"			\
//DeadCode ARM 04Jun96 		parm	[esi] [ecx]	\
//DeadCode ARM 04Jun96 		value	[al]			\
//DeadCode ARM 04Jun96 
//������������������������������������������������������������������������������
//Procedure		KeyHeld3d
//Author		Jim Taylor
//Date			Fri 3 Nov 1995
//
//Description	Check if a key is currently held down
//
//Inputs		Alias to test for
//
//Returns		TRUE if not been released
//
//------------------------------------------------------------------------------

bool	keytests::KeyHeld3d(KeyVal3D keyval)
{
KeyMap3d*
		keymap=(KeyMap3d*) reftable3d.flat;
#if defined(MA_LINUX)
	/* STICKHOLD-1 S1 (2026-09-15). GOLD3D-1 S9 could not make a HELD key move the aeroplane: four
	   runs holding ELEVATOR_FORWARD produced no descent, while S8's identical call once did. The
	   first hypothesis -- that the in-flight menu's KeyPress3d was eating it -- is WRONG, and the
	   code says so: there are TWO bits per action, `keyval` (held) and `keyval+1` (edge).
	   KeyPress3d consumes only the EDGE; KeyHeld3d falls back to the HELD bit, which nothing
	   clears until key-up. So a held key cannot be eaten that way.
	   The open question is whether the HELD bit is ever set at all for a synthetic key -- if the
	   compat delivers buffered events only, a pushed key is a PRESS and never a HOLD, which is
	   exactly the observed behaviour. Report both bits.
	   MA_TRACE_KEYHELD=1 logs the first 40 calls where either bit is set (so it cannot flood, and
	   it is unconditional for those, so it cannot be starved). */
	{
		static int on = -1;
		if (on < 0) on = getenv("MA_TRACE_KEYHELD") ? 1 : 0;
		if (on) {
			static int n = 0;
			bool heldbit = BITTEST(keymap->bitflags, keyval);
			bool edgebit = BITTEST(keymap->bitflags, keyval+1);
			if ((heldbit || edgebit) && n++ < 40) {
				fprintf(stderr, "[keyheld] KeyHeld3d(%d) held=%d edge=%d\n",
				        (int)keyval, (int)heldbit, (int)edgebit);
				fflush(stderr);
			}
		}
	}
#endif
		if (BITRESET((keymap->bitflags),keyval+1))
			return(TRUE);
		return	(BITTEST((keymap->bitflags),keyval));
}

//������������������������������������������������������������������������������
//Procedure		KeyPress3d
//Author		Jim Taylor
//Date			Fri 3 Nov 1995
//
//Description	Check if a key has been pressed since last test
//
//Inputs		Alias to test for
//
//Returns		TRUE	if just pressed
//
//------------------------------------------------------------------------------
#if defined(MA_LINUX)
/* S439 (EPIC M, 2026-09-13): MA_KEYSEQ -- drive real SIM keys on a wall clock.
   Every in-sim test has been blocked on this (S437/S438). MA_TYPESEQ goes through ma_ole_char()
   into front-end OLE edit boxes and cannot reach the 3-D, and synthetic X keys reach no window on
   this desktop (Wayland). But the game has always had its own faking entry point, KeyFake3d(),
   which sets exactly the bitflags the sim polls -- so nothing needs inventing, only scheduling.

   MA_KEYSEQ="<ms>,<keynum>[,<holdms>][;...]" -- keynum is the KeyName() number from KEYMAPS.H, NOT a
   scancode: ACCELKEY is 130 (KEYMAPS.H:240). KEYMAPS.H:93 defines the KeyVal3D as keynum*2 and that
   conversion is done here, so a recipe can quote the header's own numbers. holdms defaults to 400.
   Times run from the first 3-D key poll, i.e. the flight, not process start. MA_TRACE_KEYSEQ=1
   reports every press and release.

   Pumped from KeyPress3d because the sim polls it every frame; the schedule is wall-clock and
   index-based, so several calls per frame are harmless. One key is held at a time. */
static void ma_keyseq_pump(keytests* kt)
{
	static int inited = 0, trace = 0, idx = 0, heldKeynum = -1;
	static const char* seq = 0;
	static long t0 = 0, releaseAt = 0;

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	long now = (long)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);

	if (!inited) {
		inited = 1;
		seq = getenv("MA_KEYSEQ");
		trace = getenv("MA_TRACE_KEYSEQ") ? 1 : 0;
		t0 = now;
		if (seq && trace)
			fprintf(stderr, "[keyseq] armed: \"%s\" (ms from first 3-D key poll)\n", seq), fflush(stderr);
	}
	if (!seq) return;

	if (heldKeynum >= 0 && now >= releaseAt) {
		kt->KeyFake3d((KeyVal3D)(heldKeynum * 2), FALSE, FALSE);
		if (trace) fprintf(stderr, "[keyseq] release keynum=%d at %ldms\n", heldKeynum, now - t0), fflush(stderr);
		heldKeynum = -1;
	}
	if (heldKeynum >= 0) return;

	const char* p = seq;
	for (int i = 0; i < idx && p; i++) { p = strchr(p, ';'); if (p) p++; }
	if (!p || !*p) return;

	long at = atol(p);
	if ((now - t0) < at) return;

	const char* c1 = strchr(p, ',');
	if (!c1) { idx++; return; }
	int keynum = atoi(c1 + 1);
	long hold = 400;
	const char* semi = strchr(p, ';');
	const char* c2 = strchr(c1 + 1, ',');
	if (c2 && (!semi || c2 < semi)) hold = atol(c2 + 1);

	kt->KeyFake3d((KeyVal3D)(keynum * 2), TRUE, TRUE);
	heldKeynum = keynum;
	releaseAt = now + hold;
	idx++;
	if (trace)
		fprintf(stderr, "[keyseq] press keynum=%d (KeyVal3D=%d) at %ldms (due %ldms) hold %ldms\n",
		        keynum, keynum * 2, now - t0, at, hold), fflush(stderr);
}
#endif

bool	keytests::KeyPress3d(KeyVal3D keyval)
{
KeyMap3d*
		keymap=(KeyMap3d*) reftable3d.flat;
#if defined(MA_LINUX)
	ma_keyseq_pump(this);
	/* PO-13 (S107): name the CONSUMER. This function is a test-and-CLEAR, so the only way to find
	   out who eats an in-flight menu's option key is to watch the function itself. MA_TRACE_KEYEAT
	   =<action index> logs every call for that one action and whether it found the bit set --
	   filtered to one key, so it cannot flood, and unconditional for that key, so it cannot be
	   starved (the trap this port keeps re-learning). */
	{
		static int want = -2;
		if (want == -2) { const char* e = getenv("MA_TRACE_KEYEAT"); want = e ? atoi(e) : -1; }
		if (want >= 0 && (int)keyval == want) {
			static int n = 0;
			bool set = BITTEST(keymap->bitflags, keyval+1);
			if (set || n++ < 4)
				fprintf(stderr, "[keyeat] KeyPress3d(%d) bit=%d ret=%d\n", (int)keyval, (int)set, (int)set);
		}
	}
#endif
		return(BITRESET((keymap->bitflags),keyval+1));
}

//������������������������������������������������������������������������������
//Procedure		KeyFake3d
//Author		Jim Taylor
//Date			Fri 3 Nov 1995
//
//Description	Fakes a key press to the 3d
//
//Inputs		Alias to fake
//				set or clear bits
//
//Returns		TRUE	if just pressed
//
//------------------------------------------------------------------------------
void	keytests::KeyFake3d(	KeyVal3D keyval,	//combo to fake
								Bool	held,		//set/clear held flag
								Bool	hit)		//set/clear oneshot flag
{
KeyMap3d*
		keymap=(KeyMap3d*) reftable3d.flat;
		if (held)
			BITSET((keymap->bitflags),keyval);
		else
			BITRESET((keymap->bitflags),keyval);
		if (hit)
			BITSET((keymap->bitflags),keyval+1);
		else
			BITRESET((keymap->bitflags),keyval+1);

}
