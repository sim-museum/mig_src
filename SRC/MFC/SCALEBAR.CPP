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

// ScaleBar.cpp : implementation file
//

#include "stdafx.h"
#include "mig.h"
#include "ScaleBar.h"
#include "migview.h"
#include "mainfrm.h"
#include "savegame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#ifndef THIS_FILE_DEFINED
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// CScaleBar dialog


CScaleBar::CScaleBar(CWnd* pParent /*=NULL*/)
	: CRToolBar(pParent)
{
	//{{AFX_DATA_INIT(CScaleBar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_RButtonDown=FALSE;
}


void CScaleBar::DoDataExchange(CDataExchange* pDX)
{
	CRToolBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScaleBar)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScaleBar, CRToolBar)
	//{{AFX_MSG_MAP(CScaleBar)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScaleBar message handlers

BOOL CScaleBar::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CScaleBar::OnSize(UINT nType, int cx, int cy) 
{
	CRToolBar::OnSize(nType, cx, cy);
	Invalidate();
}

void CScaleBar::OnPaint()
{
	if (m_init) return;
	PAINTSTRUCT paintstruct;
	CDC* pDC;
	pDC=this->BeginPaint(&paintstruct);
	CRect rect;
	this->GetClientRect(rect);
#if defined(MA_LINUX)
	MaDrawScale(pDC, rect);
#else
	if (m_bHorzAlign) rect.left+=25;
	else rect.top+=25;
//	m_pView->m_pDialogBar = this;
	int counter;
	char string[10];
	#define zoom m_pView->m_zoom
	// a nice black background
	pDC->FillRect(rect, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));
	float grad_10 = zoom * Save_Data.dist.longcm * 10 /65536;
	float grad_1 = grad_10/10;
	float grad_tenth = grad_10/ 100;
	if (m_align==3)
	{
		pDC->SetTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(30,2));
		pDC->LineTo(CPoint(rect.right-5,2)); // line at top of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=30;x<rect.right-5;x+=grad_10)
		{
			pDC->MoveTo(CPoint(x,2));
			pDC->LineTo(CPoint(x,15));
			// and the corresponding number
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || zoom>=1.5)
				{
//DEADCODE DAW 07/05/99 					if (counter==0)
//DEADCODE DAW 07/05/99 						sprintf(string,"0 %s",);
//DEADCODE DAW 07/05/99 					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(x,15,string);
					if (counter==0)
						pDC->TextOut(x,30,LoadResString(Save_Data.dist.longabbr));
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (zoom>=4)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,2));
					pDC->LineTo(CPoint(x,12));
				}
				if (counter%10==0 || (counter%5==0 && zoom>=8))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(x,15,string);
					}
				}
				counter++;
			}
		}
		// now 1/10 mile seperators
		if (m_pView->m_zoom>=32)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,2));
					pDC->LineTo(CPoint(x,10));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(x,15,string);
				}
				counter++;
			}
		}
	}
	else if (m_align==1)
	{
		pDC->SetTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(30,m_width-3));
		pDC->LineTo(CPoint(rect.right-5,m_width-3)); // line at bottom of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=30;x<rect.right-5;x+=grad_10)
		{
			pDC->MoveTo(CPoint(x,m_width-2));
			pDC->LineTo(CPoint(x,m_width-15));
			// and the corresponding number
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || zoom>=1.5)
				{
//DEADCODE DAW 07/05/99 					if (counter==0)
//DEADCODE DAW 07/05/99 						sprintf(string,"0\n%s",);
//DEADCODE DAW 07/05/99 					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(x,0,string);
					if (counter==0)
						pDC->TextOut(x,15,LoadResString(Save_Data.dist.longabbr));
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (zoom>=4)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,m_width-2));
					pDC->LineTo(CPoint(x,m_width-12));
				}
				if (counter%10==0 || (counter%5==0 && zoom>=8))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(x,0,string);
					}
				}
				counter++;
			}
		}
		// now 1/10 mile seperators
		if (m_pView->m_zoom>=32)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,m_width-2));
					pDC->LineTo(CPoint(x,m_width-10));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(x,0,string);
				}
				counter++;
			}
		}
	}
	else if (m_align==2)
	{
		CFont* pfont;
		pfont = pDC->GetCurrentFont();
		LOGFONT logfont;
		pfont->GetLogFont(&logfont);
		int offset=logfont.lfHeight/2;
		pDC->SetTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(2,offset+25));
		pDC->LineTo(CPoint(2,rect.bottom-offset)); // line at top of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=offset+25;x<rect.bottom-offset;x+=grad_10)
		{
			pDC->MoveTo(CPoint(2,x));
			pDC->LineTo(CPoint(15,x));
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || zoom>=1.5)
				{
					if (counter==0)
						sprintf(string,"0 %s",(LPCTSTR)LoadResString(Save_Data.dist.longabbr));
					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(15,x-offset,string);
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (zoom>=4)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(2,x));
					pDC->LineTo(CPoint(12,x));
				}
				if (counter%10==0 || (counter%5==0 && zoom>=8))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(15,x-offset,string);
					}
				}
				counter++;
			}
		}

		// now 1/10 mile seperators
		if (m_pView->m_zoom>=32)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(2,x));
					pDC->LineTo(CPoint(10,x));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(15,x-offset,string);
				}
				counter++;
			}
		}
	}
	else if (m_align==4)
	{
		CFont* pfont;
		pfont = pDC->GetCurrentFont();
		LOGFONT logfont;
		pfont->GetLogFont(&logfont);
		int offset=logfont.lfHeight/2;
		pDC->SetTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(m_width-2,offset+25));
		pDC->LineTo(CPoint(m_width-2,rect.bottom-offset)); // line at top of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=offset+25;x<rect.bottom-offset;x+=grad_10)
		{
			pDC->MoveTo(CPoint(m_width-2,x));
			pDC->LineTo(CPoint(m_width-15,x));
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || grad_10>10)
				{
					if (counter==0)
						sprintf(string,"0 %s",(LPCTSTR)LoadResString(Save_Data.dist.longabbr));
					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(m_width-15,x-offset,string);
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (grad_10>=50)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(m_width-2,x));
					pDC->LineTo(CPoint(m_width-12,x));
				}
				if (counter%10==0 || (counter%5==0 && grad_10>=100))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(m_width-15,x-offset,string);
					}
				}
				counter++;
			}
		}

		// now 1/10 mile seperators
		if (grad_10>=500)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(m_width-2,x));
					pDC->LineTo(CPoint(m_width-10,x));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(m_width-15,x-offset,string);
				}
				counter++;
			}
		}
	}
//	pDC->DrawEdge(&rect,EDGE_ETCHED,BF_RECT); // border
#endif
	this->EndPaint(&paintstruct);
}

#if defined(MA_LINUX)
/* The body above, verbatim, callable with any DC + rect (see SCALEBAR.H). */
void CScaleBar::MaDrawScale(CDC* pDC, CRect rect)
{
	if (m_bHorzAlign) rect.left+=25;
	else rect.top+=25;
//	m_pView->m_pDialogBar = this;
	int counter;
	char string[10];
	#define zoom m_pView->m_zoom
#ifdef MA_LINUX
	/* MAP-RULER S3 (2026-09-14): the left ruler draws ticks and no numbers -- the 570 px that remain
	   between campaign_map and its reference. The labels are gated by
	   (counter%20==0 || (counter%10==0 && zoom>0.5) || grad_10>10) and placed from the current font's
	   height, so print those terms rather than guess which one fails.
	   NOTE: S2's version of this probe was written, built and run and was NOT in the tree afterwards.
	   The cause is now known and it was mine: the insert text carried a non-latin-1 character, the
	   write truncated the file to ZERO bytes and then threw, and the silence that followed was read
	   as a finding. Plain ASCII only in these inserts, and grep the marker back out before running.
	   MA_TRACE_SCALEBAR=1. */
	if (getenv("MA_TRACE_SCALEBAR"))
	{
		static int onceE = 0;
		if (!onceE)
		{
			onceE = 1;
			fprintf(stderr, "[scalebar] MaDrawScale align=%d width=%d horz=%d rect=(%ld,%ld)-(%ld,%ld)"
			                " zoom=%.3f longcm=%.3f grad10=%.2f\n",
			        (int)m_align, (int)m_width, (int)m_bHorzAlign,
			        (long)rect.left, (long)rect.top, (long)rect.right, (long)rect.bottom,
			        (double)m_pView->m_zoom, (double)Save_Data.dist.longcm,
			        (double)(m_pView->m_zoom * Save_Data.dist.longcm * 10 / 65536));
			fflush(stderr);
		}
	}
#endif
	// a nice black background
	pDC->FillRect(rect, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));
	float grad_10 = zoom * Save_Data.dist.longcm * 10 /65536;
	float grad_1 = grad_10/10;
	float grad_tenth = grad_10/ 100;
	if (m_align==3)
	{
		pDC->SetTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(30,2));
		pDC->LineTo(CPoint(rect.right-5,2)); // line at top of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=30;x<rect.right-5;x+=grad_10)
		{
			pDC->MoveTo(CPoint(x,2));
			pDC->LineTo(CPoint(x,15));
			// and the corresponding number
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || zoom>=1.5)
				{
//DEADCODE DAW 07/05/99 					if (counter==0)
//DEADCODE DAW 07/05/99 						sprintf(string,"0 %s",);
//DEADCODE DAW 07/05/99 					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(x,15,string);
					if (counter==0)
						pDC->TextOut(x,30,LoadResString(Save_Data.dist.longabbr));
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (zoom>=4)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,2));
					pDC->LineTo(CPoint(x,12));
				}
				if (counter%10==0 || (counter%5==0 && zoom>=8))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(x,15,string);
					}
				}
				counter++;
			}
		}
		// now 1/10 mile seperators
		if (m_pView->m_zoom>=32)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,2));
					pDC->LineTo(CPoint(x,10));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(x,15,string);
				}
				counter++;
			}
		}
	}
	else if (m_align==1)
	{
		pDC->SetTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(30,m_width-3));
		pDC->LineTo(CPoint(rect.right-5,m_width-3)); // line at bottom of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=30;x<rect.right-5;x+=grad_10)
		{
			pDC->MoveTo(CPoint(x,m_width-2));
			pDC->LineTo(CPoint(x,m_width-15));
			// and the corresponding number
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || zoom>=1.5)
				{
//DEADCODE DAW 07/05/99 					if (counter==0)
//DEADCODE DAW 07/05/99 						sprintf(string,"0\n%s",);
//DEADCODE DAW 07/05/99 					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(x,0,string);
					if (counter==0)
						pDC->TextOut(x,15,LoadResString(Save_Data.dist.longabbr));
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (zoom>=4)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,m_width-2));
					pDC->LineTo(CPoint(x,m_width-12));
				}
				if (counter%10==0 || (counter%5==0 && zoom>=8))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(x,0,string);
					}
				}
				counter++;
			}
		}
		// now 1/10 mile seperators
		if (m_pView->m_zoom>=32)
		{
			counter=0;
			for (float x=30;x<rect.right-5;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(x,m_width-2));
					pDC->LineTo(CPoint(x,m_width-10));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(x,0,string);
				}
				counter++;
			}
		}
	}
	else if (m_align==2)
	{
		CFont* pfont;
		pfont = pDC->GetCurrentFont();
		LOGFONT logfont;
		pfont->GetLogFont(&logfont);
		int offset=logfont.lfHeight/2;
		pDC->SetTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(2,offset+25));
		pDC->LineTo(CPoint(2,rect.bottom-offset)); // line at top of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=offset+25;x<rect.bottom-offset;x+=grad_10)
		{
			pDC->MoveTo(CPoint(2,x));
			pDC->LineTo(CPoint(15,x));
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || zoom>=1.5)
				{
					if (counter==0)
						sprintf(string,"0 %s",(LPCTSTR)LoadResString(Save_Data.dist.longabbr));
					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(15,x-offset,string);
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (zoom>=4)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(2,x));
					pDC->LineTo(CPoint(12,x));
				}
				if (counter%10==0 || (counter%5==0 && zoom>=8))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(15,x-offset,string);
					}
				}
				counter++;
			}
		}

		// now 1/10 mile seperators
		if (m_pView->m_zoom>=32)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(2,x));
					pDC->LineTo(CPoint(10,x));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(15,x-offset,string);
				}
				counter++;
			}
		}
	}
	else if (m_align==4)
	{
		CFont* pfont;
		pfont = pDC->GetCurrentFont();
		LOGFONT logfont;
		pfont->GetLogFont(&logfont);
		int offset=logfont.lfHeight/2;
		pDC->SetTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
		pDC->MoveTo(CPoint(m_width-2,offset+25));
		pDC->LineTo(CPoint(m_width-2,rect.bottom-offset)); // line at top of scale
		// first draw in 10 mile seperators these are always present
		counter=0;
		for (float x=offset+25;x<rect.bottom-offset;x+=grad_10)
		{
			pDC->MoveTo(CPoint(m_width-2,x));
			pDC->LineTo(CPoint(m_width-15,x));
			if (counter%5==0)
			{
				if (counter%20==0 || (counter%10==0 && zoom>0.5) || grad_10>10)
				{
					if (counter==0)
						sprintf(string,"0 %s",(LPCTSTR)LoadResString(Save_Data.dist.longabbr));
					else
						sprintf(string,"%d",counter*10);
					pDC->TextOut(m_width-15,x-offset,string);
				}
			}
			counter++;
		}
		// now 1 mile seperators
		if (grad_10>=50)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_1)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(m_width-2,x));
					pDC->LineTo(CPoint(m_width-12,x));
				}
				if (counter%10==0 || (counter%5==0 && grad_10>=100))
				{
					if (counter%50>0)
					{
						sprintf(string,"%d",counter);
						pDC->TextOut(m_width-15,x-offset,string);
					}
				}
				counter++;
			}
		}

		// now 1/10 mile seperators
		if (grad_10>=500)
		{
			counter=0;
			for (float x=offset+25;x<rect.bottom-offset;x+=grad_tenth)
			{
				if (counter%10>0)
				{
					pDC->MoveTo(CPoint(m_width-2,x));
					pDC->LineTo(CPoint(m_width-10,x));
				}
				else if (counter%50>0)
				{
				sprintf(string,"%d",counter/10);
				pDC->TextOut(m_width-15,x-offset,string);
				}
				counter++;
			}
		}
	}
//	pDC->DrawEdge(&rect,EDGE_ETCHED,BF_RECT); // border
}
#endif

void CScaleBar::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CRToolBar::OnRButtonDown(nFlags, point);
}

void CScaleBar::OnRButtonUp(UINT nFlags, CPoint point) 
{
	CRToolBar::OnRButtonUp(nFlags, point);
}

void CScaleBar::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_RButtonDown)
	{
		if (m_bHorzAlign)
			m_pView->m_zoom=((point.x-25)*m_oldzoom)/(m_oldPoint.x-25);
		else
			m_pView->m_zoom=((point.y-25)*m_oldzoom)/(m_oldPoint.y-25);
		if (m_pView->m_zoom<ZOOMMIN) 
			m_pView->m_zoom=ZOOMMIN;
		else 
			if (m_pView->m_zoom>ZOOMMAX) 
				m_pView->m_zoom=ZOOMMAX;
			else
			{
//				m_oldPoint=point;
//				m_oldzoom=m_pView->m_zoom;
			}
//		TRACE1("Redrawing! %f!",m_pView->m_zoom);
		RedrawWindow();
//	CRect rect;														  //JIM 08/05/99
//	m_pView->GetClientRect(rect);
//	m_pView->Zoom(CPoint(rect.right/2,rect.bottom/2),false);
//		TRACE1("Finished! %f\n",m_pView->m_zoom);

	}
	CRToolBar::OnMouseMove(nFlags, point);
}

void CScaleBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if ((m_bHorzAlign && point.x<25) ||
		(!m_bHorzAlign && point.y<25))
	{
		CRToolBar::OnLButtonDown(nFlags, point);
	}
	else
	{
		m_RButtonDown=TRUE;
		this->SetCapture();
		m_oldPoint=point;
		m_oldzoom=m_pView->m_zoom;
	}
}

void CScaleBar::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_RButtonDown=FALSE;
	ReleaseCapture(); 	
	CRect rect;
	m_pView->GetClientRect(rect);
	m_pView->Zoom(CPoint(rect.right/2,rect.bottom/2));

	CRToolBar::OnLButtonUp(nFlags, point);
}

#if defined(MA_LINUX)
/* ---------------------------------------------------------------------------------------
   S135 (PO-22): paint the distance ruler.

   The bar has always been constructed and initialised -- CMainFrame does
   `m_toolbar4.Create(CScaleBar::IDD,view); m_toolbar4.Init(this,200,400,48,AFX_IDW_DOCKBAR_LEFT,4)`
   -- which is what gives it m_align=4 (labels right-aligned against the map) and m_width=48.
   What the port lacks is the dock manager that would size it and send it WM_PAINT, so OnPaint
   was never called and the map had no ruler. The PO asked for it directly ("we also need the
   map ruler"), and the gold capture shows it: a black strip down the left edge, a white rule at
   its inner edge, ticks every 10 units and a label every 50, headed "0 Nm".

   Everything numeric here comes from the game's own OnPaint -- grad = zoom * dist.longcm * 10 /
   65536 -- so the ruler cannot disagree with the map it annotates. This function only supplies
   the geometry the dock manager would have, and restores the DC state it borrows (S97: a widget
   must not change the state of the screen it draws on; that rule was written after the map's
   date readout inherited a font from the system box).
   --------------------------------------------------------------------------------------- */
void CScaleBar::MaPaintAt(int x, int y, int w, int h)
{
	if (!m_pView || w <= 0 || h <= 0) return;
	void* hdc = ma_gdi_screen_dc();
	if (!hdc) return;

	m_maW = w; m_maH = h;      /* what GetClientRect reports inside OnPaint */
	m_width = w;
	m_length = h;

	int ox = 0, oy = 0;
	ma_gdi_set_viewport_org(hdc, x, y, &ox, &oy);
	void* oldFont  = ma_gdi_get_font(hdc);
	int   oldAlign = ma_gdi_get_text_align(hdc);

	/* The screen DC arrives carrying whatever font the map last drew its date readout with --
	   a large art face. On Windows this bar would have had the dialog font. Left as inherited,
	   the distance labels came out ~30px tall and were clipped by the 48px strip ("100" drew as
	   "00"). Select a small face for the ruler, which is also what makes the right-alignment
	   land inside the strip. */
	static void* s_font = 0;
	if (!s_font) s_font = ma_gdi_font_create(13, 400, 0, 0);
	if (s_font) ma_gdi_set_font(hdc, s_font);
	/* MAP-RULER S3 (2026-09-14): SET THE TEXT COLOUR. The ruler draws its ticks with a PEN
	   (MoveTo/LineTo) and its distance labels with TextOut, and this path set the font and the
	   alignment but never the text colour -- so the labels inherited whatever the map last used and
	   came out invisible on the bar's own black backdrop. Measured this sprint: align=4, width=48,
	   grad10=35.48, so the label gate (grad_10 > 10) is TRUE and TextOut IS being called about five
	   times down the bar; the numbers simply could not be seen. The reference capture has them.
	   MA_NO_RULER_TEXTCOL=1 reverts. */
	if (!getenv("MA_NO_RULER_TEXTCOL")) ma_gdi_set_text_color(hdc, 0x00FFFFFF);

	OnPaint();

	ma_gdi_set_text_align(hdc, oldAlign);
	ma_gdi_set_font(hdc, oldFont);
	ma_gdi_set_viewport_org(hdc, ox, oy, 0, 0);
}
#endif

#if defined(MA_LINUX)
/* PO-11: paint the map's scale ruler onto the strategic-map framebuffer.
   Cross-ported from BoB (bob_map_paint_scalebar, SRC/MFC/SCALEBAR.CPP) -- same engine, same
   problem: the MFC docking frame's OnPaint never runs on Linux, so the ruler that draws itself
   is never asked to. Drive its real tick drawing with a screen DC whose viewport origin is the
   docked strip. MA_NO_SCALEBAR=1 disables. */
extern "C" void* ma_gdi_screen_dc(void);
extern "C" void  ma_gdi_fill_rect(void* hdc, int l, int t, int r, int b, unsigned colorref);
extern "C" void  ma_gdi_set_viewport_org(void* hdc, int x, int y, int* oldx, int* oldy);

extern "C" void ma_map_paint_scalebar(CMIGView* v, int sw, int sh)
{
	if (getenv("MA_NO_SCALEBAR")) return;
	/* DO NOT use v->m_pScaleBar here -- but NOT for the reason first recorded.
	   CORRECTION (2026-09-04): an earlier version of this comment claimed the member was "never
	   assigned anywhere in the tree". That was WRONG, and wrong for a reason this repo documents:
	   the default `grep` is ugrep and silently skips files it calls binary, so it missed
	   MAINFRM.CPP. `/bin/grep` finds the assignment immediately --
	       MAINFRM.CPP:408   view->m_pScaleBar = &m_toolbar4;   (m_toolbar4 is a real CScaleBar)
	   so the member IS assigned, from a legitimate object.
	   What is still UNEXPLAINED: at map-paint time the member reads 0x8c55197 -- not 4-byte
	   aligned, so not a valid object address -- across every run, and it stays that way after
	   initialising it to NULL in CMIGView's constructor. A class-layout mismatch between TUs was
	   checked and ruled out (MIGVIEW.H has no MA_LINUX-conditional members). Whether MAINFRM's
	   assignment runs before this paint, and what is writing that value, is an OPEN question.
	   Until it is answered, own a local instance: reading or writing through a pointer whose
	   provenance is not understood is the risk, and the measured difference is real -- through
	   that pointer the ruler drew 0/5/10/15/20, with an owned object it draws 50/100/150/200/250,
	   which matches the game's own Nm scale. */
	static CScaleBar s_bar;                 /* constructed once; ours, and initialised */
	CScaleBar* sb = &s_bar;
	static int once = 0;
	if (getenv("MA_TRACE_SCALEBAR") && !once) { once = 1;
		fprintf(stderr, "[scalebar] v=%p sb=%p (owned; view's m_pScaleBar=%p is UNINITIALISED and unused) sw=%d sh=%d\n",
		        (void*)v, (void*)sb, v ? (void*)v->m_pScaleBar : (void*)0, sw, sh);
		fflush(stderr); }
	if (!v) return;
	sb->m_init = FALSE;                     /* OnPaint's early-out flag; we drive MaDrawScale directly */
	sb->m_bHorzAlign = FALSE;               /* vertical ruler down the right edge */
	const int barW = 48;                       /* the strip the ruler draws within */
	int bx = sw - barW;
	void* hdc = ma_gdi_screen_dc();
	/* The ruler's drawing is CLIENT-relative (it starts at CPoint(30,2)), so it needs the DC's
	   viewport origin moved to the docked strip -- otherwise it paints over the top-left of the
	   map. ma_gdi adds (ox,oy) to every destination coord. Restore the origin afterwards: this
	   DC is the shared screen DC and everything painted later would inherit the offset. */
	int oldx = 0, oldy = 0;
	ma_gdi_fill_rect(hdc, bx, 0, bx + barW, sh, 0x000000);   /* the ruler's black backdrop */
	ma_gdi_set_viewport_org(hdc, bx, 0, &oldx, &oldy);
	CDC dc; dc.m_hDC = (HDC)hdc;
	CRect rect(0, 0, barW, sh);
	sb->m_align = 4;                           /* right-docked vertical ruler */
	sb->m_pView = v;
	sb->MaDrawScale(&dc, rect);
	ma_gdi_set_viewport_org(hdc, oldx, oldy, 0, 0);
}
#endif
