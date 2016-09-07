/*
 * Demonstration of DebugPrinter advanced functionality.
 * Recommended: wide terminal window.
 * Compile with -O0 to prevent inlining of functions.
 * Compile with -rdynamic to get function names.
 */

//~ #define DEBUGPRINTER_OFF            // optional to turn off the DebugPrinter
//~ #define DEBUGPRINTER_NO_EXECINFO    // optional to turn off dout_FUNC/STACK
//~ #define DEBUGPRINTER_NO_CXXABI      // optional to turn off name demangling
//~ #define DEBUGPRINTER_NO_SIGNALS     // optional to turn off crash report
#include <fsc/DebugPrinter.hpp>

#include <fstream>                      // for file I/O

using fsc::dout;                        // saves some typing

int main() {

    dout << "Normal printing." << std::endl;
    dout, "And ", 4, " more words.", std::endl;  // comma syntax
    (dout << '0', 0) << "0", std::endl;          // cave: remember precedence of
                                                 //       operators: << before ,

    dout_HERE

    bool write_file = true;
    if(write_file) {
        std::ofstream fs("debug.log");
        dout = std::move(fs);           // pass responsibility for file handling
        dout.set_color();               // don't write color escape code to file
    }

    dout, "Writing to file from any C++ scope.", std::endl;
    dout_HERE                           // this goes into the file

    dout = std::cerr;                   // set to cerr (default: std::cout)
    dout.set_color("1;34");             // default red output maybe too... red

    dout_HERE

    dout("highlighted text");           // operator() variations
    dout("label", "text");              // they take anything with ostream op<<
    dout("label", "text", " separator ");

    dout.set_precision(13);             // increase precision
    dout, 0., std::endl;

    dout_HERE

    dout.stack(100, false, 0);          // show whole libc stack

    return 0;

}
