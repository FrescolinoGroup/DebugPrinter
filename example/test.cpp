/*
 * 
 * Demonstration for DebugPrinter.hpp
 * Compile with -O0 to prevent inlining of functions.
 * Compoile with -rdynamic to get function names.
 * 
 */


//~ #define DEBUGPRINTER_OFF
//~ #define DEBUGPRINTER_NO_EXECINFO
//~ #define DEBUGPRINTER_NO_CXXABI
//~ #define DEBUGPRINTER_NO_SEGVSTACK
#include "../DebugPrinter.hpp"
using namespace fsc;

#include <stdexcept>

template <typename T>
class Foo { public:
    Foo() {
        dout_FUNC
    }
};

template <typename T>
class Bar : public Foo<double> { public:
    Bar() {
        dout_FUNC
    }
    template <typename RET, typename ARG, int CNT>
    RET foo(ARG x) const {
        dout_HERE
        dout_FUNC
        dout.set_precision(CNT);
        dout, x, std::endl;
        return RET();
    }

};

void f3() { dout.stack(); }  // inlined if -O2 or -O3
void f2() { f3(); }
void f1() { f2(); }

//~ void segfault_function() { raise(SIGSEGV); }
void segfault_function() { *(int*)0 = 0; }

#include <fstream>

int main() {

    dout = std::cerr;
    dout.set_color("31");

    //~ std::ofstream fs("debug.log");
    //~ dout = fs;
    //~ dout.set_color();

    Bar<char&> b;

    dout_HERE

    b.foo<std::string, double, 42>(1.23);

    dout_HERE

    dout(b);

    dout_HERE

    dout.type(42);
    dout.type(b);
    dout.type(&Bar<double>::foo<int, std::string, 84>);
    dout.type(f1);

    dout_HERE

    f1();

    dout_HERE

    segfault_function();

    return 0;

}
