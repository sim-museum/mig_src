// iostream.h -- pre-standard header shim: expose std iostream names globally.
// (Avoids `using namespace std` to not clash with the game's `typedef char* string`.)
#ifndef MA_PORT_IOSTREAM_H
#define MA_PORT_IOSTREAM_H
#include <iostream>
using std::cout; using std::cin; using std::cerr; using std::clog;
using std::endl; using std::ends; using std::flush; using std::dec; using std::hex; using std::oct;
using std::ios; using std::istream; using std::ostream; using std::iostream; using std::streambuf;
#endif
