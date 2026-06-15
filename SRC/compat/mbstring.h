/* FreeFalcon Linux Port - mbstring.h compatibility */
#ifndef FF_COMPAT_MBSTRING_H
#define FF_COMPAT_MBSTRING_H
#ifdef FF_LINUX
#include <string.h>
#include <strings.h>
#define _mbscmp(a, b)  strcmp((const char *)(a), (const char *)(b))
#define _mbsicmp(a, b) strcasecmp((const char *)(a), (const char *)(b))
#define _mbscpy(a, b)  strcpy((char *)(a), (const char *)(b))
#define _mbslen(a)     strlen((const char *)(a))
#define _mbschr(a, c)  strchr((const char *)(a), (c))
#define _mbsrchr(a, c) strrchr((const char *)(a), (c))
#define _mbsstr(a, b)  strstr((const char *)(a), (const char *)(b))
/* single-byte locale: next char value / advance one char */
static inline unsigned int _mbsnextc(const unsigned char *s) { return (unsigned int)*s; }
static inline unsigned char *_mbsinc(const unsigned char *s) { return (unsigned char *)(s + 1); }
#endif
#endif
