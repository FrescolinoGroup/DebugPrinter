/*
 * Demonstration of DebugPrinter stack and crash info, and pausing.
 * Recommended: wide terminal window.
 * Compile with -O0 to prevent inlining of functions.
 * Compile with -rdynamic to get function names.
 */

//~ #define DEBUGPRINTER_OFF            // optional to turn off the DebugPrinter
//~ #define DEBUGPRINTER_NO_EXECINFO    // optional to turn off dout_FUNC/STACK
//~ #define DEBUGPRINTER_NO_CXXABI      // optional to turn off name demangling
//~ #define DEBUGPRINTER_NO_SIGNALS     // optional to turn off crash report
#include <fsc/DebugPrinter.hpp>

void func3() {                          // inlined if -O2 or -O3
    dout_STACK                          // print frame stack
}
void func2() { func3(); }
void func1() { func2(); }

void segfault_function() {
    dout_HERE
    *(volatile int*)0 = 0;
}

int main() {

    func1();

    dout_PAUSE()                        // take a break

    for(int i = 0; i < 10; ++i) {
        dout_VAL(i);
        dout_PAUSE(i<5 && i%2==0)       // pause on condition
    }

    dout_PAUSE("and now we crash")      // pause with message

    segfault_function();

    return 0;

}
