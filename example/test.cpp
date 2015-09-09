/*
 * Demonstration for DebugPrinter.hpp
 * Compile with -O0 to prevent inlining of functions.
 * Compile with -rdynamic to get function names.
 */
//~ g++     _Z2ttILi1EcENSt9enable_ifIXntsr  St12is_referenceIDtfp_EE 5valueEvE4typeET0_
//~ clang++ _Z2ttILi1EcENSt9enable_ifIXntsr3std12is_referenceIDtfp_EEE5valueEvE4typeET0_

//~ #define DEBUGPRINTER_OFF
//~ #define DEBUGPRINTER_NO_EXECINFO
//~ #define DEBUGPRINTER_NO_CXXABI
//~ #define DEBUGPRINTER_NO_SIGNALS
#include "../DebugPrinter.hpp"
using fsc::dout;

#include <stdexcept>
#include <fstream>
#include <map>
#include <utility>
#include <type_traits>

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


void f3() { dout_STACK }  // inlined if -O2 or -O3
void f2() { f3(); }
void f1() { f2(); }

//~ void segfault_function() { raise(SIGSEGV); }
//~ void segfault_function() { *(int*)0 = 0; }
void segfault_function() { }




/*******************************************************************************
 * constexpr bools
 */
template <typename T> constexpr bool isConst(T&) { return false; }
template <typename T> constexpr bool isConst(T const&) { return true; }



/*******************************************************************************
 * distinguish qualifiers
 * 1) sfinae activation
 * 2) with is_same with a struct for each qualifier and then squash them
 */
template <int T, typename U>
auto tt(U t) -> typename std::enable_if<!std::is_reference<decltype(t)>::value, void>::type {
    (void)t;
    dout_FUNC
    dout_TYPE(T)
    dout_TYPE(U)
    std::cout << isConst(t) << std::endl;
    dout_HERE
    std::cout << std::is_const<decltype(t)>() << std::endl;
    std::cout << std::is_reference<decltype(t)>() << std::endl;
    std::cout << std::is_lvalue_reference<decltype(t)>() << std::endl;
    std::cout << std::is_rvalue_reference<decltype(t)>() << std::endl;
    //~ std::cout << __PRETTY_FUNCTION__ << std::endl;
}
//~ template <int T, typename U>
//~ auto tt(U & t) -> typename std::enable_if<std::is_reference<decltype(t)>::value, void>::type {
    //~ (void)t;
    //~ dout_FUNC
    //~ dout_TYPE(T)
    //~ dout_TYPE(U)
    //~ std::cout << isConst(t) << std::endl;
    //~ dout_HERE
    //~ std::cout << std::is_const<decltype(t)>() << std::endl;
    //~ std::cout << std::is_reference<decltype(t)>() << std::endl;
    //~ std::cout << std::is_lvalue_reference<decltype(t)>() << std::endl;
    //~ std::cout << std::is_rvalue_reference<decltype(t)>() << std::endl;
//~ }
//~ template <typename T, typename U>
//~ void tt(U t) {
    //~ (void)t;
    //~ dout_FUNC
    //~ dout_TYPE(T)
    //~ dout_TYPE(U)
//~ }

/*******************************************************************************
 * type_info grabbing
 */
//~ template <typename *(std::type_info*)(T), typename U>
//~ void tt(U t) {
    //~ (void)t;
    //~ dout_FUNC
    //~ dout_TYPE(T)
    //~ dout_TYPE(U)
//~ }
//~ template <std::type_info* T, typename U>
//~ void tt(U t) {
    //~ (void)t;
    //~ dout_FUNC
    //~ dout_TYPE(T)
    //~ dout_TYPE(U)
//~ }
// typeid(int)      *(const std::type_info*)(& _ZTIi)’
// typeid(c int)    *(const std::type_info*)(& _ZTIi)’
// typeid(c char)   *(const std::type_info*)(& _ZTIc)’


//~ #define V(x) tt<x>(x)
//~ #define V(x) tt<typeid(x)>(x)
//~ #define V(x) tt<__typeof__(x)>(x)


/*******************************************************************************
 * variadic templates, std=c++14
 * split vt<typename T, typename U> and vt<typename T, ...> where ... = concrete types
 */
//~ template <typename... Ts>
//~ void vari() {}
template <int Ts>
void vari() {}


/*******************************************************************************
 * void*
 */

//~ template <typename std::nullptr V>
//~ void maybe(V* p) { (void)p; }
//~ template <void* V>
//~ void maybe(char* p) { (void)p; }


int main() {

char aa = 1; (void)aa;

//~ maybe<static_cast<void*>(&aa)>(&aa);

std::cout << static_cast<void*>(&aa) << std::endl;

//~ vari<typeid(4)>();

//~ tt<volatile int&&>(int());
//~ tt<aa>(int());
//~ V(aa);
//~ V('a');
//~ V(int);

//~ std::cout << __typeof__(aa) << std::endl;

decltype(std::declval<int>()) asdf=5;(void)asdf;
//~ std::cout << typeid(decltype(aa)).name() << std::endl;
//~ std::cout << typeid(decltype((aa))).name() << std::endl;
//~ std::cout << typeid(decltype((int))).name() << std::endl;

dout_HERE
dout.type<decltype(5)>(5);
dout.type(aa);

return 0;

    dout = std::cerr;
    dout.set_color("1;34");

    bool write_file = false;  // pick:  true  false
    if(write_file) {
        std::ofstream fs("debug.log");
        dout = std::move(fs);
        dout.set_color();
    }

    Bar<char&> b;

    dout_HERE

    b.foo<std::string, double, 42>(1.23);

    dout_HERE

    //~ dout(b);
    //~ dout("label", b, "  ");
    dout("label", "foo", "\t");

    dout_HERE

    int x = 5;
    dout_VAR(x);
    //~ dout_VAR(b);

    dout_TYPE(42);
    dout_TYPE(std::map<int, int>);
    //~ dout_TYPE(b);
    dout_TYPE((&Bar<double>::foo<int, std::string, 84>));
    dout_TYPE(f1);

    dout_HERE

    f1();

    dout_HERE

    segfault_function();

    return 0;

}
