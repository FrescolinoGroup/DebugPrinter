/*
 * Demonstration of DebugPrinter type and signature printing.
 * Recommended: wide terminal window.
 * Compile with -O0 to prevent inlining of functions.
 * Compile with -rdynamic to get function names.
 */

//~ #define DEBUGPRINTER_OFF            // optional to turn off the DebugPrinter
//~ #define DEBUGPRINTER_NO_EXECINFO    // optional to turn off dout_FUNC
//~ #define DEBUGPRINTER_NO_CXXABI      // optional to turn off name demangling
#include <fsc/DebugPrinter.hpp>

#include <vector>

template <typename T, typename U>
class Foo { public:
    Foo() {
        dout_HERE                       // print position, file and function
        dout_FUNC                       // print current function signature
    }
    template<typename R>
    R method(T, U) {
        dout_HERE
        dout_TYPE(R);                   // print full type
        dout_FUNC
        return R();
    }
};


int main() {

    dout_HERE                           // print position, file and function

    using t1 = std::vector<int>;
    dout_TYPE(t1);                      // print full type
    dout_TYPE(volatile const int&)
    dout_TYPE(std::string)

    dout_HERE

    t1 my_var;
    dout_TYPE_OF(42);                   // print runtime type and valueness
    dout_TYPE_OF(my_var[0]);
    dout_TYPE_OF(std::move(my_var[0]));

    dout_HERE

    my_var.push_back(42);
    dout_VAL(my_var[0]);                // print highlighted var=val pair
    dout_VAL(my_var);                   // std::vector has no ostream operator<<

    dout_HERE

    using t2 = Foo<int, char*>;
    using t3 = size_t;
    t2 my_var2;                         // calls constructor of Foo
    my_var2.method<t3>(0, nullptr);     // calls method of Foo

    dout_HERE

    dout_TYPE_OF(&t2::method<t3>);

    return 0;

}
