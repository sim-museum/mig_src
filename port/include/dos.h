// dos.h -- Linux port stub. The game's <dos.h> includers don't use DOS port/
// interrupt APIs (vestigial includes); provide a few harmless decls just in case.
#ifndef MA_PORT_DOS_H
#define MA_PORT_DOS_H
#include <unistd.h>
#define MK_FP(seg,off) ((void*)(((unsigned long)(seg)<<4)+(unsigned long)(off)))
#define FP_SEG(p) ((unsigned short)(((unsigned long)(p))>>4))
#define FP_OFF(p) ((unsigned short)(((unsigned long)(p))&0xF))
static inline void delay(unsigned ms){ usleep((useconds_t)ms*1000); }
static inline void _enable(void){}
static inline void _disable(void){}
#endif
