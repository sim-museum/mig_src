/* FreeFalcon Linux Port - new.h compatibility */
#ifndef FF_COMPAT_NEW_H
#define FF_COMPAT_NEW_H
#ifdef __cplusplus
#include <new>
#include <cstddef>		/* NULL */
typedef void (*_PNH)(void);
static inline _PNH _set_new_handler(_PNH h) { (void)h; return (_PNH)0; }
#endif
#endif
