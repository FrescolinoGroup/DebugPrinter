/*
 * Demonstration for DebugPrinter.hpp
 * Compile with -O0 to prevent inlining of functions.
 * Compile with -rdynamic to get function names.
 */

//~ #define DEBUGPRINTER_OFF
//~ #define DEBUGPRINTER_NO_EXECINFO
//~ #define DEBUGPRINTER_NO_CXXABI
//~ #define DEBUGPRINTER_NO_SIGNALS
#include "../DebugPrinter.hpp"

using fsc::dout;

#include <fstream>
#include <map>

void f3(int x) {
    std::cout << 0 << std::endl;
    (void)x; dout_STACK
}  // inlined if -O2 or -O3
void f2() { int y = 3; f3(4); }
void f1() { f2(); }


int main() {

    f1();

    return 0;

}
