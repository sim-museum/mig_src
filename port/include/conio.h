// conio.h -- Linux port stub (no DOS console).
#ifndef MA_PORT_CONIO_H
#define MA_PORT_CONIO_H
static inline int kbhit(void){ return 0; }
static inline int getch(void){ return 0; }
static inline int getche(void){ return 0; }
#endif
