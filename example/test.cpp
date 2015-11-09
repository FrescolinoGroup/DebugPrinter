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

//~ template <typename T, typename U, typename = std::enable_if_t<std::is_same<T<U>, Foo<U>>::value> >
//~ template <typename T, typename = std::enable_if_t<>::value>>
template <typename T, typename...>
std::ostream & operator<<(std::ostream & os, const Foo<T>) {
//~ std::ostream & operator<<(std::ostream & os, const Foo<int>) {
dout_FUNC
    os << "It's a Foo!" << std::endl;
    return os;
}
//~ template <typename T>
//~ std::ostream & operator<<<Bar<T>>(std::ostream & os, const Foo<T> &) = delete;

void f3() { dout_STACK }  // inlined if -O2 or -O3
void f2() { f3(); }
void f1() { f2(); }

//~ void segfault_function() { raise(SIGSEGV); }
//~ void segfault_function() { *(int*)0 = 0; }
void segfault_function() { }


int main() {

    dout = std::cerr;
    dout.set_color("1;34");

    bool write_file = false;  // pick:  true  false
    if(write_file) {
        std::ofstream fs("debug.log");
        dout = std::move(fs);
        dout.set_color();
    }

    dout_HERE

    int && a = 0;
    int & ar = a;
    dout_TYPE_OF(a)
    dout_TYPE_OF(std::move(ar))
    dout_TYPE_OF(4)
    dout_TYPE(volatile const int&)
    dout_TYPE(std::map<int, int>);

    dout_HERE

    dout_PAUSE()
    dout_PAUSE("checkpoint 1")
    dout_PAUSE(a == 0)
    dout_PAUSE(a > 0)

    for(int i = 0; i < 10; ++i) {
        std::cout << i << std::endl;
        dout_PAUSE(i>=8)
    }

    dout_HERE

    Bar<char&> b;

    dout(b);
    dout("label", "foo", "\t->\t");

std::cout << b << std::endl;
std::cout << *( reinterpret_cast<Foo<int>*>(&b) ) << std::endl;
//~ operator<<<Foo<int>,int>(std::cout, *( reinterpret_cast<Foo<int>*>(&b) ) );
Foo<int> f;
std::cout << f << std::endl;
//~ operator<< <int>(std::cout, f);
//~ asdf <int>(std::cout, f);
//std::ostream& operator<< <int>(std::ostream&, Foo<int>)
return 0;

    dout_HERE

    b.foo<std::string, const double &&, 42>(1.23);

    dout_HERE

    dout_TYPE_OF(b);
    dout_TYPE_OF(&Bar<double>::foo<int, std::string, 84>);

    dout_HERE

    dout_TYPE_OF(f1);
    f1();

    dout_HERE

    segfault_function();

    return 0;

}
