// Linux port shim: pre-standard <iostream.h> -> modern <iostream>.
// VC6's <iostream.h> exposed the stream names in the GLOBAL namespace but did
// NOT define std::string there. BoB has its own `typedef char* string`
// (dosdefs.h), so we must NOT `using namespace std` (that would make std::string
// ambiguous with BoB's). Instead bring in only the stream-related names.
#ifndef BOB_COMPAT_IOSTREAM_H
#define BOB_COMPAT_IOSTREAM_H
/* CRITICAL: the game builds with -fpack-struct=1 (MSVC /Zp1). That packing must
   NOT reach the std C++ stream/locale types -- they live in libstdc++ with the
   NATIVE layout, so a std::ifstream/ostream constructed by packed game code has a
   mismatched subobject layout and libstdc++ scribbles memory when it operates on
   it (e.g. BIStream save loads corrupted the stack). #pragma pack(8) overrides
   -fpack-struct here and restores the native ABI. */
#pragma pack(push,8)
#include <iostream>
#include <iomanip>
#pragma pack(pop)
using std::ios;
using std::ios_base;
using std::istream;
using std::ostream;
using std::iostream;
using std::streambuf;
using std::streampos;
using std::streamoff;
using std::streamsize;
using std::cin;
using std::cout;
using std::cerr;
using std::clog;
using std::endl;
using std::ends;
using std::flush;
using std::ws;
using std::dec;
using std::hex;
using std::oct;
using std::setw;
using std::setfill;
using std::setprecision;
using std::setbase;
using std::setiosflags;
using std::resetiosflags;
#endif
