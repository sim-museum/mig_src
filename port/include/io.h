// io.h -- Linux port shim mapping Watcom low-level I/O to POSIX.
#ifndef MA_PORT_IO_H
#define MA_PORT_IO_H
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
static inline long filelength(int fd){ struct stat s; return fstat(fd,&s)==0 ? (long)s.st_size : -1L; }
#endif
