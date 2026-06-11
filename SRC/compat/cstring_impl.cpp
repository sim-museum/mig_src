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
void CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
	va_list argCopy;
	va_copy(argCopy, argList);
	int nLen = vsnprintf(NULL, 0, lpszFormat, argCopy);
	va_end(argCopy);
	if (nLen < 0) nLen = 0;
	LPTSTR p = GetBuffer(nLen + 1);
	vsnprintf(p, nLen + 1, lpszFormat, argList);
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
	if (n > 0) { *this = tmp; return TRUE; }
	Empty();
	return FALSE;
}
