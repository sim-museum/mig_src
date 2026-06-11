// Linux port shim: pre-standard <fstream.h> -> modern <fstream>.
// See iostream.h shim: selective `using` (not `using namespace std`) so BoB's
// global `typedef char* string` (dosdefs.h) is not made ambiguous by std::string.
#ifndef BOB_COMPAT_FSTREAM_H
#define BOB_COMPAT_FSTREAM_H
/* native ABI for std::ifstream/ofstream/fstream despite -fpack-struct=1 (see iostream.h). */
#pragma pack(push,8)
#include <fstream>
#pragma pack(pop)
#include "iostream.h"
using std::filebuf;
using std::ifstream;
using std::ofstream;
using std::fstream;
#endif
