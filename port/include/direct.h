// direct.h -- Linux port shim for directory ops.
#ifndef MA_PORT_DIRECT_H
#define MA_PORT_DIRECT_H
#include <unistd.h>
#include <sys/stat.h>
static inline int _mkdir(const char*p){ return mkdir(p,0755); }
static inline int _chdir(const char*p){ return chdir(p); }
static inline char* _getcwd(char*b,int n){ return getcwd(b,n); }
#endif
