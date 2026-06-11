// process.h -- Linux port stub for Watcom _beginthread/spawn family.
#ifndef MA_PORT_PROCESS_H
#define MA_PORT_PROCESS_H
typedef unsigned long uintptr_t_thr;
static inline unsigned long _beginthread(void(*)(void*),unsigned,void*){ return 0; }
static inline void _endthread(void){}
#endif
