/* FreeFalcon/BoB Linux port - CString implementation.
 *
 * SRC/H/cstring.h is MFC's CString header (ref-counted CStringData: m_pchData
 * points just past a {nRefs,nDataLength,nAllocLength} header). On Windows the
 * implementation came from mfc42.lib; here we provide a compatible one
 * (the classic strcore.cpp logic) for the non-inline methods bob actually uses. */

#define BOB_LINUX 1
#include "dosdefs.h"
#include "cstring.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cstdint>      /* uintptr_t/intmax_t/ptrdiff_t (FormatV %s walker) */
#include <string>       /* std::string (FormatV maps parse + output buffer) */
#include <pthread.h>    /* maps-cache lock */
#include <fcntl.h>      /* open() /proc/self/maps */
#include <unistd.h>     /* read()/close() */

/* ---- nil/empty string ----------------------------------------------------
   rgInitData lays out a CStringData{nRefs=-1, nDataLength=0, nAllocLength=0}
   followed by a '\0'. afxPchNil points at that '\0' (i.e. data()), so
   GetData() on the empty string yields the {-1,0,0} header (nRefs=-1 => never
   freed/shared). CString's sole member is m_pchData, so we can hand back an
   "empty CString" by reinterpreting the afxPchNil pointer variable as one. */
static long rgInitData[(sizeof(CStringData)/sizeof(long)) + 1] = { -1, 0, 0, 0 };
static char* afxPchNil = (char*)(((unsigned char*)&rgInitData) + sizeof(CStringData));

const CString& AFXAPI AfxGetEmptyString()
{
	return *reinterpret_cast<const CString*>(&afxPchNil);
}

/* ---- helpers (SafeStrlen is inline in the header) ------------------------- */
void CString::AllocBuffer(int nLen)
{
	if (nLen == 0)
		Init();
	else
	{
		CStringData* pData = (CStringData*)malloc(sizeof(CStringData) + (nLen + 1) * sizeof(TCHAR));
		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		pData->nAllocLength = nLen;
		m_pchData = pData->data();
	}
}

void CString::Release()
{
	if (GetData()->nRefs >= 0)
	{
		if (--GetData()->nRefs <= 0)
			free(GetData());
		Init();
	}
}

void PASCAL CString::Release(CStringData* pData)
{
	if (pData->nRefs >= 0)
		if (--pData->nRefs <= 0)
			free(pData);
}

void CString::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = "";
}

void CString::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringData* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength + 1) * sizeof(TCHAR));
	}
}

void CString::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
}

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
{
	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
		dest.Init();
	else
	{
		dest.AllocBuffer(nNewLen);
		memcpy(dest.m_pchData, m_pchData + nCopyIndex, nCopyLen * sizeof(TCHAR));
	}
}

void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen * sizeof(TCHAR));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '\0';
}

void CString::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data)
{
	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len * sizeof(TCHAR));
		memcpy(m_pchData + nSrc1Len, lpszSrc2Data, nSrc2Len * sizeof(TCHAR));
	}
}

void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	if (nSrcLen == 0)
		return;
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		CStringData* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		CString::Release(pOldData);
	}
	else
	{
		memcpy(m_pchData + GetData()->nDataLength, lpszSrcData, nSrcLen * sizeof(TCHAR));
		GetData()->nDataLength += nSrcLen;
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

/* ---- constructors / destructor ------------------------------------------- */
CString::CString()
{
	Init();
}

CString::CString(const CString& stringSrc)
{
	if (stringSrc.GetData()->nRefs >= 0)
	{
		m_pchData = stringSrc.m_pchData;
		GetData()->nRefs++;
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}

CString::CString(TCHAR ch, int nLength)
{
	Init();
	if (nLength >= 1)
	{
		AllocBuffer(nLength);
		memset(m_pchData, ch, nLength);
	}
}

CString::CString(LPCSTR lpsz)
{
	Init();
	int nLen = SafeStrlen(lpsz);
	if (nLen != 0)
	{
		AllocBuffer(nLen);
		memcpy(m_pchData, lpsz, nLen * sizeof(TCHAR));
	}
}

CString::CString(LPCTSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength * sizeof(TCHAR));
	}
}

CString::~CString()
{
	if (GetData() != (CStringData*)&rgInitData)
		if (--GetData()->nRefs <= 0)
			free(GetData());
}

/* ---- assignment / concat -------------------------------------------------- */
const CString& CString::operator=(const CString& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != (CStringData*)&rgInitData) ||
			stringSrc.GetData()->nRefs < 0)
		{
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			Release();
			m_pchData = stringSrc.m_pchData;
			GetData()->nRefs++;
		}
	}
	return *this;
}

const CString& CString::operator=(LPCSTR lpsz)
{
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator+=(LPCTSTR lpsz)
{
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator+=(const CString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

const CString& CString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

/* ---- concatenation operators (friends) ------------------------------------ */
CString AFXAPI operator+(const CString& string1, const CString& string2)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
				 string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CString AFXAPI operator+(const CString& string, LPCTSTR lpsz)
{
	CString s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
				 CString::SafeStrlen(lpsz), lpsz);
	return s;
}

CString AFXAPI operator+(LPCTSTR lpsz, const CString& string)
{
	CString s;
	s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz,
				 string.GetData()->nDataLength, string.m_pchData);
	return s;
}

/* ---- search --------------------------------------------------------------- */
int CString::Find(TCHAR ch) const
{
	LPTSTR lpsz = (LPTSTR)strchr(m_pchData, ch);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::Find(LPCTSTR lpszSub) const
{
	LPTSTR lpsz = (LPTSTR)strstr(m_pchData, lpszSub);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::ReverseFind(TCHAR ch) const
{
	LPTSTR lpsz = (LPTSTR)strrchr(m_pchData, ch);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/* ---- substrings ----------------------------------------------------------- */
CString CString::Mid(int nFirst, int nCount) const
{
	if (nFirst < 0) nFirst = 0;
	if (nCount < 0) nCount = 0;
	int nLen = GetData()->nDataLength;
	if (nFirst + nCount > nLen) nCount = nLen - nFirst;
	if (nFirst > nLen) nCount = 0;
	CString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

CString CString::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CString CString::Left(int nCount) const
{
	if (nCount < 0) nCount = 0;
	if (nCount > GetData()->nDataLength) nCount = GetData()->nDataLength;
	CString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

CString CString::Right(int nCount) const
{
	if (nCount < 0) nCount = 0;
	int nLen = GetData()->nDataLength;
	if (nCount > nLen) nCount = nLen;
	CString dest;
	AllocCopy(dest, nCount, nLen - nCount, 0);
	return dest;
}

/* ---- trimming ------------------------------------------------------------- */
void CString::TrimRight()
{
	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '\0')
	{
		if (isspace((unsigned char)*lpsz))
		{ if (lpszLast == NULL) lpszLast = lpsz; }
		else lpszLast = NULL;
		lpsz++;
	}
	if (lpszLast != NULL)
	{
		*lpszLast = '\0';
		GetData()->nDataLength = (int)(lpszLast - m_pchData);
	}
}

void CString::TrimLeft()
{
	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;
	while (isspace((unsigned char)*lpsz)) lpsz++;
	if (lpsz != m_pchData)
	{
		int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

/* ---- buffer access -------------------------------------------------------- */
LPTSTR CString::GetBuffer(int nMinBufLength)
{
	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
		CStringData* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;
		if (nMinBufLength < nOldLen) nMinBufLength = nOldLen;
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nOldLen;
		CString::Release(pOldData);
	}
	return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();
	if (nNewLength == -1)
		nNewLength = (int)strlen(m_pchData);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

/* ---- formatting ----------------------------------------------------------- */
/* Linux/GCC porting fix for the MFC "CString in printf varargs" idiom (ported from the
 * sibling BoB Rowan-engine port). The game pervasively writes CSprintf("%s",aCString) /
 * str.Format("%s",aCString) WITHOUT a (LPCTSTR) cast. On MSVC a CString is passed to varargs
 * BY VALUE (it is one pointer member, so %s reads the char*). Under the Itanium/SysV C++ ABI
 * (Linux GCC) a class with a non-trivial copy ctor/dtor -- CString -- is passed to varargs
 * BY INVISIBLE REFERENCE (a pointer to the object), so a plain vsnprintf %s prints the
 * object's m_pchData *bytes* as text -> garbage. (RESSTRING/LoadResString/device .name etc.
 * all return CString; 23 CSprintf("%s",CString) sites in MiG, e.g. LSTMSNLG/FLT_TASK/SCONTROL.)
 *
 * Fix: pure-numeric formats keep the trusted libc vsnprintf path (zero change). Only when the
 * format contains %s do we walk the conversions and, for each %s, decide whether the argument is
 * a CString-by-reference (deref to its data pointer) or a genuine char* (use as-is) by validating
 * the CStringData{nRefs,nDataLength,nAllocLength} header that sits just before a real CString's
 * buffer, with /proc/self/maps-guarded reads so a stray char* can never fault. This only alters
 * %s-bearing formats, which are ALL currently broken, so it cannot regress a working format. */
namespace {
	struct MapRange { uintptr_t lo, hi; };
	static MapRange      g_maps[1024];
	static int           g_nmaps = 0;
	static pthread_mutex_t g_mapsLock = PTHREAD_MUTEX_INITIALIZER;

	static void reload_maps_locked() {
		g_nmaps = 0;
		int fd = open("/proc/self/maps", O_RDONLY);
		if (fd < 0) return;
		std::string acc; char buf[8192]; ssize_t n;
		while ((n = read(fd, buf, sizeof buf)) > 0) acc.append(buf, (size_t)n);
		close(fd);
		size_t pos = 0;
		while (pos < acc.size() && g_nmaps < 1024) {
			size_t eol = acc.find('\n', pos); if (eol == std::string::npos) eol = acc.size();
			unsigned long lo = 0, hi = 0; char perms[8] = {0};
			if (sscanf(acc.c_str() + pos, "%lx-%lx %4s", &lo, &hi, perms) == 3 && perms[0] == 'r')
				{ g_maps[g_nmaps].lo = lo; g_maps[g_nmaps].hi = hi; g_nmaps++; }
			pos = eol + 1;
		}
	}
	static bool in_maps(uintptr_t a, uintptr_t b) {
		for (int i = 0; i < g_nmaps; i++) if (a >= g_maps[i].lo && b <= g_maps[i].hi) return true;
		return false;
	}
	static bool addr_readable(const void* p, size_t len) {
		if (!p) return false;
		uintptr_t a = (uintptr_t)p, b = a + len;
		if (b < a) return false;
		pthread_mutex_lock(&g_mapsLock);
		if (g_nmaps == 0) reload_maps_locked();
		bool ok = in_maps(a, b);
		if (!ok) { reload_maps_locked(); ok = in_maps(a, b); }   /* heap may have grown -> refresh once */
		pthread_mutex_unlock(&g_mapsLock);
		return ok;
	}
	/* Decide a %s argument: CString-by-reference -> its data pointer; else a genuine char*. */
	static const char* resolve_str_arg(void* a) {
		if (!a) return (const char*)a;
		if (!addr_readable(a, sizeof(void*))) return (const char*)a;
		char* m = *(char**)a;                       /* would-be CString::m_pchData */
		if (m && addr_readable((const char*)m - sizeof(CStringData), sizeof(CStringData) + 1)) {
			CStringData* d = ((CStringData*)m) - 1;
			int dl = d->nDataLength, al = d->nAllocLength; long r = d->nRefs;
			if (dl >= 0 && al >= dl && al < (1 << 22) && (r > 0 || r == -1)
			    && addr_readable(m, (size_t)dl + 1) && m[dl] == '\0')
				return m;                           /* strong CStringData signature */
		}
		return (const char*)a;                      /* genuine char* */
	}
	static bool format_has_s(const char* f) {
		for (; *f; f++) if (f[0] == '%') { if (f[1] == '%') f++; else { for (const char* g=f+1; *g; g++)
			if (*g=='s'){return true;} else if (strchr("diouxXeEfFgGaAcpn",*g)) break; } }
		return false;
	}
}

void CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
	if (!lpszFormat) { Empty(); return; }
	if (!format_has_s(lpszFormat)) {            /* numeric/char only -> trusted libc path */
		va_list argCopy; va_copy(argCopy, argList);
		int nLen = vsnprintf(NULL, 0, lpszFormat, argCopy);
		va_end(argCopy);
		if (nLen < 0) nLen = 0;
		LPTSTR p = GetBuffer(nLen + 1);
		vsnprintf(p, nLen + 1, lpszFormat, argList);
		ReleaseBuffer(nLen);
		return;
	}
	/* %s present -> walk conversions, fixing CString-by-ref args. */
	std::string out; char spec[80], tmp[1024];
	for (const char* f = lpszFormat; *f; ) {
		if (*f != '%') { out.push_back(*f++); continue; }
		const char* start = f++;
		if (*f == '%') { out.push_back('%'); f++; continue; }
		while (*f && strchr("-+ #0", *f)) f++;                 /* flags */
		bool sw = false, sp = false;
		if (*f == '*') { sw = true; f++; } else while (isdigit((unsigned char)*f)) f++;   /* width */
		if (*f == '.') { f++; if (*f == '*') { sp = true; f++; } else while (isdigit((unsigned char)*f)) f++; }
		int len = 0;                                            /* 0 none,1 l,2 ll,5 L,6 z,7 j,8 t */
		if (*f=='l') { f++; if (*f=='l'){len=2;f++;} else len=1; }
		else if (*f=='h') { f++; if (*f=='h') f++; }            /* promotes to int */
		else if (*f=='L') { len=5; f++; }
		else if (*f=='z') { len=6; f++; }
		else if (*f=='j') { len=7; f++; }
		else if (*f=='t') { len=8; f++; }
		char conv = *f ? *f++ : 0;
		int sl = (int)(f - start); if (sl >= (int)sizeof(spec)) sl = sizeof(spec) - 1;
		memcpy(spec, start, sl); spec[sl] = 0;
		int wa = sw ? va_arg(argList, int) : 0;
		int pa = sp ? va_arg(argList, int) : 0;
		tmp[0] = 0;
		#define EMIT(VAL) do { \
			if (sw && sp)   snprintf(tmp,sizeof tmp, spec, wa, pa, (VAL)); \
			else if (sw)    snprintf(tmp,sizeof tmp, spec, wa, (VAL)); \
			else if (sp)    snprintf(tmp,sizeof tmp, spec, pa, (VAL)); \
			else            snprintf(tmp,sizeof tmp, spec, (VAL)); } while (0)
		bool emitted = true;
		switch (conv) {
			case 's': { const char* s = resolve_str_arg(va_arg(argList, void*)); if (!s) s = "(null)"; EMIT(s); break; }
			case 'd': case 'i':
				if (len==2) EMIT(va_arg(argList,long long)); else if (len==1) EMIT(va_arg(argList,long));
				else if (len==6) EMIT(va_arg(argList,size_t)); else if (len==7) EMIT(va_arg(argList,intmax_t));
				else if (len==8) EMIT(va_arg(argList,ptrdiff_t)); else EMIT(va_arg(argList,int)); break;
			case 'u': case 'o': case 'x': case 'X':
				if (len==2) EMIT(va_arg(argList,unsigned long long)); else if (len==1) EMIT(va_arg(argList,unsigned long));
				else if (len==6) EMIT(va_arg(argList,size_t)); else EMIT(va_arg(argList,unsigned int)); break;
			case 'c': EMIT(va_arg(argList,int)); break;
			case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
				if (len==5) EMIT(va_arg(argList,long double)); else EMIT(va_arg(argList,double)); break;
			case 'p': EMIT(va_arg(argList,void*)); break;
			default: out += spec; emitted = false; break;       /* trailing '%' or unknown -> literal, no consume */
		}
		#undef EMIT
		if (emitted) out += tmp;
	}
	int nLen = (int)out.size();
	LPTSTR p = GetBuffer(nLen + 1);
	memcpy(p, out.data(), nLen); p[nLen] = 0;
	ReleaseBuffer(nLen);
}

void AFX_CDECL CString::Format(LPCTSTR lpszFormat, ...)
{
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

/* ---- resource string load: RT_STRING from the loaded boblang.dll --------- */
extern "C" void* bob_GetResourceHandle(void);
extern "C" int   bob_load_string(void* h, unsigned id, char* buf, int maxlen);
BOOL CString::LoadString(UINT nID)
{
	char tmp[1024];
	int n = bob_load_string(bob_GetResourceHandle(), nID, tmp, (int)sizeof(tmp));
	if (getenv("MA_TRACE_STR")) { static int c=0; if(c++<40) fprintf(stderr,"[LoadString] id=%u n=%d \"%s\"\n", nID, n, n>0?tmp:""); }
	if (n > 0) { *this = tmp; return TRUE; }
	Empty();
	return FALSE;
}

// Linux/GCC port: BSTR is char* in our compat (see compat_types.h); hand back a strdup'd
// copy so OCX getters (CRStaticCtrl::GetString) that return AllocSysString() link + work.
BSTR CString::AllocSysString() const { const char* s = m_pchData ? m_pchData : ""; char* b = (char*)malloc(strlen(s)+1); if (b) strcpy(b, s); return (BSTR)b; }

// Linux/GCC port: used by OVERLAY/RDIALOG etc.
void CString::MakeUpper() { CopyBeforeWrite(); for (LPTSTR p=m_pchData; p && *p; ++p) *p=(TCHAR)toupper((unsigned char)*p); }
void CString::SetAt(int nIndex, TCHAR ch) { CopyBeforeWrite(); m_pchData[nIndex]=ch; }
