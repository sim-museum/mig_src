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

#include "stdafx.h"

// this global function searches resource.h and returns the resource
// number given a string representing a resource ID
// for example:
// Given "ID_OK" will search resource.h and find the number
// associated with this ID eg 1000

// this allows resource IDs to be placed as parameters
// eg for strings and bitmaps in controls
// returns -1 if could not find the ID

#if defined(MA_LINUX)
/* PO-48 / 2026-09-15. These two functions are the game's DESIGN-TIME resource resolver: they open
   `\mig\src\mfc\resource.h` and `\mig\src\mfc\mig.rc` -- absolute paths on a 1999 developer's
   machine -- and parse them to turn a caption like "IDS_AREYOUSURE" into the string it names. No
   shipped game has those files, so on Linux every control whose caption is a resource-ID NAME
   resolved to -1 and painted the literal fallback **"Invalid ID!"**. Measured on the current build:
   the QUIT GAME confirmation's body (`#1050@RMdlDlg`) and both bodies of the D.I.S. Notes dialog
   (`#1005/#1006@CDis_Note`) read "Invalid ID!" to the player.

   The port has the two halves already, in the wrong order: `bob_load_string` (the PE resource
   loader) turns an ID NUMBER into its string, and RESOURCE.H holds NAME -> number. So bake the
   name table and keep the loader for the rest.

   Regenerate ma_resource_ids.h after editing RESOURCE.H:
     python3 - <<'EOF'
     import re
     src=open('SRC/MFC/RESOURCE.H','rb').read().decode('latin-1'); seen={}
     for l in src.splitlines():
         m=re.match(r'\s*#define\s+([A-Za-z_]\w*)\s+(-?\d+)\s*$', l)
         if m: seen.setdefault(m.group(1), int(m.group(2)))
     items=sorted(seen.items())
     ...  # emit MA_RES_IDS[] exactly as the current file does
     EOF */
#include "../compat/ma_resource_ids.h"

static int ma_res_lookup(const char* name)
{
    if (!name || !*name) return -1;
    int lo = 0, hi = MA_RES_IDS_N - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = strcmp(name, MA_RES_IDS[mid].name);
        if (c == 0) return (int)MA_RES_IDS[mid].id;
        if (c < 0) hi = mid - 1; else lo = mid + 1;
    }
    return -1;
}

int GetResourceNumberFromID(const char* text)
{
    int id = ma_res_lookup(text);
    if (getenv("MA_TRACE_RESID"))
        fprintf(stderr, "[resid] lookup \"%s\" -> %d\n", text ? text : "(null)", id);
    return id;
}

BOOL ConvertResourceID(const char* text, char* output)
{
    if (!output) return FALSE;
    output[0] = 0;
    int id = ma_res_lookup(text);
    if (id < 0) {
        if (getenv("MA_TRACE_RESID"))
            fprintf(stderr, "[resid] convert \"%s\" -> NO SUCH NAME\n", text ? text : "(null)");
        return FALSE;
    }
    char buf[512];
    int n = AfxLoadString((UINT)id, buf, (UINT)sizeof(buf));
    if (getenv("MA_TRACE_RESID"))
        fprintf(stderr, "[resid] \"%s\" -> id %d -> %d chars \"%s\"\n", text, id, n, n > 0 ? buf : "");
    if (n <= 0) return FALSE;
    /* the caller's buffer is char[100] at both call sites (RSTATICC/RBUTTONC) */
    strncpy(output, buf, 99); output[99] = 0;
    return TRUE;
}
#else
int GetResourceNumberFromID(const char* text)
{
	// ok first open resource.h which we'll assume is in
	// \mig\src\mfc\resource.h
	FILE* fp;
	fp=fopen("\\mig\\src\\mfc\\resource.h","rb");
	if (fp==NULL) 
	{
		MessageBox(AfxGetMainWnd()->m_hWnd,"Cannot find \\mig\\src\\mfc\\resource.h","ERROR",MB_OK);
		return -1;
	}
	// now parse it
	char c;
	int textlen=8+strlen(text);
	char buffer[255];
	char searchfor[255];
	strcpy(searchfor,"#define ");
	strcat(searchfor,text);
	// search for enter key since this must always precede #define
	// searching for the entire string is much slower...
	while (true)
	{
		while(true)
		{
			if(fread(&c,1,1,fp)==NULL) {fclose(fp);return -1;} // failed to find it
			if (c==13) break;
		}
		// now check if #define text is after it
		if(fread(&c,1,1,fp)==NULL) {fclose(fp);return -1;}
		if(fread(buffer,1,textlen+1,fp)<textlen+1) {fclose(fp);return -1;}
		if (buffer[textlen]==' ') // prevents triggering on partial successes...
		{
			buffer[textlen]=NULL; // NULL terminate the buffer
			if (strcmp(buffer,searchfor)==0)
				break;
		}
		fseek(fp,-(textlen+1),SEEK_CUR); // failed so backup and carry on...
	}
	// ok we have a match...
	// so find the number
	// first skip over spaces...
	while(true)
	{
		if(fread(&c,1,1,fp)==NULL) {fclose(fp);return -1;} // so near yet so far...
		if (c!=' ') break;
	}
	// extract number
	int x=0;
	while(true)
	{
		if (c==13)
		{
			buffer[x]=NULL;
			break;
		}
		buffer[x]=c;
		if(fread(&c,1,1,fp)==NULL) {fclose(fp);return -1;} // so near yet so far...
		x++;
	}
	fclose(fp);
	return atoi(buffer); // success!!!!
}

// this global function returns what a resource ID represents
// given a string representing a resource ID
// for example:
// Given "IDS_JOYSTICK1" will search mig.rc and return
// "Centre the joystick and press the fire button"
// Ideally all strings will be represented by resource IDs
// so that translaters can simply edit the string table
// to translate the entire game.
// this probably wont work for bitmaps since we will use filenums
// instead...(possibly?)

BOOL ConvertResourceID(const char* text, char* output)
{
	// ok first open mig.rc which we'll assume is in
	// \mig\src\mfc\mig.rc
	FILE* fp;
	fp=fopen("\\mig\\src\\mfc\\mig.rc","rb");
	if (fp==NULL) 
	{
		MessageBox(AfxGetMainWnd()->m_hWnd,"Cannot find \\mig\\src\\mfc\\mig.rc","ERROR",MB_OK);
		return FALSE;
	}
	// now parse it
	char c;
	int textlen=strlen(text);
// using a different approach this time
// first search for first letter of string...
	while (true)
	{
		while(true)
		{
			if(fread(&c,1,1,fp)==NULL) {fclose(fp);return FALSE;} // failed to find it
			if (c==text[0]) break;
		}
		output[0]=c;
		// now check if the rest of the string is after it
		if(fread((output+1),1,textlen,fp)<textlen-1) {fclose(fp);return FALSE;}
		if (output[textlen]==' ') // prevents triggering on partial successes...
		{
			output[textlen]=NULL; // NULL terminate the output
			if (strcmp(output,text)==0)
				break;
		}
		fseek(fp,-textlen,SEEK_CUR); // failed so backup and carry on...
	}
	// ok we have a match...
	// so find the string
	// first skip over spaces...
	while(true)
	{
		if(fread(&c,1,1,fp)==NULL) {fclose(fp);return FALSE;} // so near yet so far...
		if (c!=' ') break;
	}
	// extract string ignoring any quotes...
	int x=0;
	while(true)
	{
		if (c==13)
		{
			output[x]=NULL;
			break;
		}
		if (c!='"')
		{
			output[x]=c;
			x++;
		}
		if(fread(&c,1,1,fp)==NULL) {fclose(fp);return FALSE;} // so near yet so far...
	}
	fclose(fp);
	return TRUE; // Success!
}
#endif  /* MA_LINUX */



















