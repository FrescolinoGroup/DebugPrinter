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

#include <stdexcept>

template <typename T>
class Foo { public:
    Foo() {
        dout_FUNC
    }
    typedef T type;
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

#include <fstream>
#include <map>

void f3() { dout_STACK }  // inlined if -O2 or -O3
void f2() { f3(); }
void f1() { f2(); }

//~ void segfault_function() { raise(SIGSEGV); }
//~ void segfault_function() { *(int*)0 = 0; }
void segfault_function() { }

struct asdf {
    void operator()(int a, int b) {
        std::cout << a << " " << b << std::endl;
    }
};

int main() {


    dout = std::cerr;
    dout.set_color("31");

    // ToDo: find better use case for dout_VAR
    //~ asdf e;
    //~ e(2,2);
    //~ dout_VAR(e);


    // ToDo: shared_ptr of ofstream
        std::ofstream fs("debug.log");
    dout = std::move(fs);
    dout = std::move(std::cout);

    dout << std::is_move_assignable<decltype(std::cout)>() << std::endl;
    dout << std::is_move_assignable<decltype(fs)>() << std::endl;
    dout << std::is_trivially_move_assignable<decltype(fs)>() << std::endl;
    if(false) {  // for file output switch to true
        //~ dout = std::move(fs);
        //~ dout = fs;
        //~ dout.set_color();
    }
    Bar<char&> b;

    dout_HERE

    b.foo<std::string, double, 42>(1.23);

    dout_HERE

    dout(b);
    dout("label", b, "  ");
    dout("label", "foo", "\t");

    dout_HERE

    int x = 5;
    dout_VAR(x);
    dout_VAR(b);
//~ 
    //~ dout_TYPE(42);
    //~ dout_TYPE(std::map<int, int>);
    //~ dout_TYPE(b);
    //~ dout_TYPE((&Bar<double>::foo<int, std::string, 84>));
    //~ dout_TYPE(f1);

    dout_HERE

    f1();

    dout_HERE

    segfault_function();

    return 0;

}
