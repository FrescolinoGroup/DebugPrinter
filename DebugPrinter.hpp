/** ****************************************************************************
 * 
 * \file       DebugPrinter.hpp
 * \brief      DebugPrinter header-only lib.
 * >           Creates a static object named `dout` and defines debugging macros.
 * \version    2015.0827
 * \author   
 * Year      | Name
 * --------: | :------------
 * 2011-2015 | Donjan Rodic
 *      2015 | Mario Könz
 *      2015 | C. Frescolino
 * \copyright  Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at\n
 *     http://www.apache.org/licenses/LICENSE-2.0\n
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * \section dummy &nbsp;
 * \subsection Compilation Compilation
 * 
 * Link with `-rdynamic` in order to properly get `stack()` frame names and
 * useful `dout_FUNC` output.
 * 
 * _Note: compiler optimisations may inline functions (shorter stack)._
 * 
 * Pass `DEBUGPRINTER_OFF` to turn off all functionality provided here. You can
 * leave the debug statements in your code, since all methods and macros become
 * inline and trivial, and thus will be optimised away by your compiler at
 * sufficient optimisation flags.
 * 
 * Pass `DEBUGPRINTER_NO_EXECINFO` flag on Windows (disables `stack()`,
 * `dout_STACK` and `dout_FUNC`).
 * 
 * Pass `DEBUGPRINTER_NO_CXXABI` if you don't have a _cxxabi_ demangle call 
 * (translates raw type symbols, i.e. `typeid(std::string).name()` output `Ss`
 * to `std::string`) in your libc distribution. The stack and type methods will
 * then print raw stack frame names and a _c++filt_-ready output.
 * 
 * Pass `DEBUGPRINTER_NO_SIGNALS` to turn off automatic stack tracing when 
 * certain fatal signals occur. Passing this flag is recommended on non-Linux
 * systems.
 * 
 ******************************************************************************/


// ToDo: dout_TYPE checks for ref status: dout_TYPE(int&) vs dout_type(int&&)
// ToDo: sprinkle noexcept
#ifndef DEBUGPRINTER_HEADER
#define DEBUGPRINTER_HEADER

#include <iostream>

#ifndef DEBUGPRINTER_OFF

#include <iomanip>
#include <fstream>
#include <typeinfo>
#include <cstdlib>
#include <signal.h>
#include <algorithm>
#include <stdexcept>
#include <memory>

#ifndef DEBUGPRINTER_NO_EXECINFO
#include <execinfo.h>
#endif // DEBUGPRINTER_NO_EXECINFO

#ifndef DEBUGPRINTER_NO_CXXABI
#include <cxxabi.h>
#endif // DEBUGPRINTER_NO_CXXABI

#ifndef DEBUGPRINTER_NO_SIGNALS
#include <map>
#endif // DEBUGPRINTER_NO_SIGNALS

#endif // DEBUGPRINTER_OFF

/** \brief General fsc namespace */
namespace fsc {

#ifndef DEBUGPRINTER_OFF

/** \brief Class for global static `dout` object
 * 
 *  \section dummy &nbsp;
 *  \subsection Usage Usage
 *  For details, see the documentation for fsc::DebugPrinter member functions
 *  and DebugPrinter.hpp macros.
 *  ~~~{.cpp}
 *      // basic usage:
 * 
 *      dout_HERE                      // print current file and line
 *      dout_FUNC                      // print current function signature
 *      dout_STACK                     // print stack trace
 *      dout_TYPE(std::map<int,int>)   // print given type (e.g. in templates)
 *      dout_TYPE(var)                 // print demangled RTTI of given variable
 *      dout_VAR(var)                  // print highlighted 'name = value'
 * 
 * 
 *      // advanced usage:
 * 
 *      using fsc::dout;
 * 
 *      dout << "foo" << std::endl;
 *      dout, var , 5, " bar ", 6 << " foobar " << 7, 8, std::endl;
 * 
 *      dout(object);                  // highlight object
 *      dout(object, label, " at ");   // highlight label, object and separator
 *      dout.stack(4, false, 2);       // print 4 stack frames, omitting the first
 * 
 *      dout = std::cout               // set output stream
 *      dout.set_precision(13)         // set decimal display precision
 *      dout.set_color("31")           // set terminal highlighting color
 *  ~~~
 *  In case the program terminates with `SIGSEGV`, `SIGSYS`, `SIGABRT` or
 *  `SIGFPE`, you will automatically get a stack trace from the raise location.
 *  To turn off this behaviour, check the \link Compilation \endlink section.
 */
class DebugPrinter {

  public:

/*******************************************************************************
 * Ctor and friends
 */

  DebugPrinter() : outstream(&std::cout), prec_(5) {
    set_color("31");
    #ifndef DEBUGPRINTER_NO_SIGNALS
    struct sigaction act;
    act.sa_handler = signal_handler;
    for(auto const & sig: sig_names()) {
        sigaction(sig.first, &act, NULL);
    }
    #endif // DEBUGPRINTER_NO_SIGNALS
  }
  template <typename T>
  friend DebugPrinter & operator<<(DebugPrinter &, const T&);
  friend inline DebugPrinter & operator<<(DebugPrinter &,
                                          std::ostream& (*pf)(std::ostream&));

/*******************************************************************************
 * Setters
 */

  /** \brief Assignment operator for changing streams
   *  \param os  output stream to use
   *  \details Default == `std::cout`. Change to any `std::ostream` with:
   *  ~~~{.cpp}
   *      dout = std::cerr;
   *  ~~~
   */
  inline void operator=(std::ostream & os) { outstream = &os; }

  /** \brief Assignment operator for moving streams
   *  \param os  output stream to take over
   *  \details This is the
   */
  template <typename T>
  inline auto operator=(T && os)
    ->  typename std::enable_if<!std::is_move_assignable<T>::value
                             && std::is_rvalue_reference<decltype(os)>::value, void>::type {
      std::cerr << "\033[0;36m" << ">>> " << __func__ << ": " << __LINE__ << "\033[0m" << std::endl;
    int dummy;
    std::string name = demangle(typeid(T).name(), dummy);
    // ToDo: throw bad life choice exception
    throw std::runtime_error("DebugPrinter error: object of type " + name + " is not move-assignable.");
  }
  /** \brief Assignment operator for moving streams
   *  \param os  output stream to take over
   *  \details Use to pass ownership if the stream object would leave scope.
   *  ~~~{.cpp}
   *      if(file_write == true) {
   *          std::ofstream fs("debug.log");
   *          dout = std::move(fs);
   *          dout.set_color();
   *      }
   *      dout << "Writing to debug.log";
   *  ~~~
   *  Note: trying to move `std::cout` (or other static standard streams)
   *  is considered a bad life choice.
   */
  template <typename T>
  inline auto operator=(T && os)
    ->  typename std::enable_if<std::is_move_assignable<T>::value
                             && std::is_rvalue_reference<decltype(os)>::value, void>::type {
      std::cerr << "\033[0;36m" << ">>> " << __func__ << ": " << __LINE__ << "\033[0m" << std::endl;
    outstream_ptr = std::shared_ptr<std::ostream>(new T(std::move(os)));
    outstream = outstream_ptr.get();
  }

  /** \brief Number of displayed decimal digits
   *  \param prec  desired precision
   *  \details Default == 5. Example usage:
   *  ~~~{.cpp}
   *      dout.set_precision(12);
   *  ~~~
   */
  inline void set_precision(std::streamsize prec) { prec_ = prec; }

  /** \brief Highlighting color
   *  \param str  color code
   *  \details
   *  Assumes a `bash` compatible terminal and sets the `operator()` highlighting
   *  color (also used for `dout_HERE` and `dout_VAR`), for example cyan ( ==
   *  "36" == default):
   *  ~~~{.cpp}
   *      dout.set_color("36");
   *  ~~~
   *  For bash color codes check
   *  http://www.tldp.org/HOWTO/Bash-Prompt-HOWTO/x329.html
   */
  inline void set_color(std::string str) {  // no chaining <- returns void
    if(!is_number(str))
      throw std::runtime_error("DebugPrinter error: invalid set_color() argument");
    hcol_ = "\033[0;" + str + "m";
    hcol_r_ = "\033[0m";
  }
  /** \brief Remove highlighting color
   *  \details No color highlighting (e.g. when writing to a file) Usage example:
   *  ~~~{.cpp}
   *      dout.set_color();
   *  ~~~
   */
  inline void set_color() {  // no chaining <- returns void
    hcol_ = "";
    hcol_r_ = "";
  }

/*******************************************************************************
 * Parentheses operators
 */

  /** \brief Print highlighted label and object
   *  \param label  should have a std::ostream & operator<< overload
   *  \param obj    should have a std::ostream & operator<< overload
   *  \param sc     delimiter between label and object
   *  \details If `label` or `obj` don't have the required operator << overloads,
   *  the function will print an error instead. Example usage:
   *  ~~~{.cpp}
   *      dout(label, object);
   *      dout(label, object, "\t");
   *  ~~~
   */
  template <typename T, typename U>
  inline void operator()(const T& label, const U& obj,
                         const std::string sc = ": ") const {
    print_stream_impl<m_and<has_stream<T>, has_stream<U> >::value>(label, obj, sc);
  }
  /** \brief Print highlighted object
   *  \param obj    should have a std::ostream & operator<< overload
   *  \details Equivalent to 
   *  ~~~{.cpp}
   *      dout(">>>", obj, " ");
   *  ~~~
   *  Example usage:
   *  ~~~{.cpp}
   *      dout(object);
   *  ~~~
   */
  template <typename T>
  inline void operator()(const T& obj) const { operator()(">>>", obj, " "); }

/*******************************************************************************
 * RTTI (stack)
 */

  #ifndef DEBUGPRINTER_NO_EXECINFO
  /** \brief Print a stack trace
   *  \param backtrace_size  print at most this many frames
   *  \param compact         only print function names
   *  \param begin           starting offset
   *  \details
   *  Print one line per stack frame, consisting of the binary object name, the
   *  demangled function name, and the offset within the function and within the
   *  binary.
   *  Example usage:
   *  ~~~{.cpp}
   *      dout.stack();                // print a full stack trace
   *      dout.stack(count);           // print at most (int)count frames
   *      dout.stack(count, true);     // print in compact format
   *      dout.stack(count, true, 2);  // and slice off the "first" frame
   *      dout_FUNC                    // shortcut for  dout.stack(1, true);
   *  ~~~
   */
  void stack(
      const int backtrace_size = max_backtrace,
      const bool compact = false,
      const int begin = 1 /* should stay 1 except for special needs */
    ) const {

    std::ostream & out = *outstream;
    int end_ = -1;               // ignore last

    void * stack[max_backtrace];
    int r = backtrace(stack, backtrace_size+begin-end_);
    char ** symbols = backtrace_symbols(stack, r);
    if(!symbols) return;

    int end = r + end_;
    if(compact == false)
      out << "DebugPrinter obtained " << end-begin << " stack frames:"
          << std::endl;

    #ifndef DEBUGPRINTER_NO_CXXABI

    for(int i = begin; i < end; ++i) {
      std::string line = std::string(symbols[i]);
      std::string prog = prog_part(line);
      std::string mangled = mangled_part(line);
      std::string offset = offset_part(line);
      std::string mainoffset = address_part(line);
      int status;
      std::string demangled = demangle(mangled, status);
      switch (status) {
        case -1:
          out << " Error: Could not allocate memory!" << std::endl;
          break;
        case -3:
          out << " Error: Invalid argument to demangle()" << std::endl;
          break;
        case -2:  // invalid name under the C++ ABI mangling rules
          demangled = mangled;
          // fallthrough
        default:
          if(compact == false)
            out << "  " << prog << ":  " << demangled << "  "
                << offset << "  [+" << mainoffset << "]"<< std::endl;
          else
            out << demangled << std::endl;
      }
    }
    if(compact == false) out << std::endl;

    #else // DEBUGPRINTER_NO_CXXABI

    for(int i = begin; i < end; ++i) {
      if(compact == false)
        out << "  " << symbols[i] << std::endl;
      else
        out << mangled_part(std::string(symbols[i])) << std::endl;
    }

    if(compact == false) out << std::endl;
    out << "echo '' && c++filt";
    for(int i = begin; i < end; ++i)
      out << " " << mangled_part(std::string(symbols[i]));
    out << " && echo ''" << std::endl;
    if(compact == false) out << std::endl;

    #endif // DEBUGPRINTER_NO_CXXABI

    free(symbols);
  }

  #else // DEBUGPRINTER_NO_EXECINFO

  void stack(...) const {
    *outstream << "DebugPrinter::stack() not available" << std::endl;
  }

  #endif // DEBUGPRINTER_NO_EXECINFO

/*******************************************************************************
 * "Private" parts
 */
 
  /// \cond DEBUGPRINTER_DONOTDOCME_HAVESOMEDECENCY_PLEASE
  struct detail {

    inline static void type_name(const DebugPrinter &dout, const std::string & name) {
      int dummy;
      *(dout.outstream) << dout.demangle(name, dummy) << std::endl;
    }

  };
  /// \endcond

/*******************************************************************************
 * Private parts
 */

  private:

  std::ostream * outstream;                      // output stream
  std::shared_ptr<std::ostream> outstream_ptr;   // output stream
  std::streamsize prec_;                         // precision
  std::string hcol_;                             // highlighting color
  std::string hcol_r_;                           // neutral color

  static const unsigned int max_backtrace = 50;
  static const unsigned int max_demangled = 4096;

  #ifndef DEBUGPRINTER_NO_SIGNALS
  using sig_type = int;
  static std::map<sig_type, std::string> sig_names() {
    std::map<sig_type, std::string> res;
    res[SIGABRT] = "SIGABRT";
    res[SIGFPE] = "SIGFPE";
    res[SIGSEGV] = "SIGSEGV";
    res[SIGSYS] = "SIGSYS";
    return res;
  }
  static DebugPrinter & static_init() { static DebugPrinter d; return d; }
  static void signal_handler(int signum) {
    static DebugPrinter d;
    d << "DebugPrinter handler caught signal "
      << sig_names()[signum] << " (" << signum << ")" << std::endl;
    //~ static_init().stack(max_backtrace, false, 3);
    d.stack(max_backtrace, false, 3);
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    sigaction(signum, &act, NULL);
  }
  #endif // DEBUGPRINTER_NO_SIGNALS

  #ifndef DEBUGPRINTER_NO_CXXABI

  inline std::string demangle(const std::string & str, int & status) const {
    char * demangled = abi::__cxa_demangle(str.c_str(), 0, 0, &status);
    std::string out = (status==0) ? std::string(demangled) : str;
    free(demangled);
    return out;
  }

  #else // DEBUGPRINTER_NO_CXXABI

  inline std::string demangle(const std::string & str, int &) const {
    return str;
  }

  #endif // DEBUGPRINTER_NO_CXXABI

  // type() not public, since it has a weaker interface (no types as arguments)
  // than dout_TYPE
  template <typename T>
  inline void type(T obj) const {
    detail::type_name(*this, std::string(typeid(obj).name()));
  }

  // Fetch different parts from a stack trace line
#ifdef __APPLE__
  inline std::string prog_part(const std::string str) const {
    istringstream ss(str);
    std::string res;
    ss >> res; ss >> res;
    return res;
  }
  inline std::string mangled_part(const std::string str) const {
    istringstream ss(str);
    std::string res;
    ss >> res; ss >> res; ss >> res; ss >> res;
    return res;
  }
  inline std::string offset_part(const std::string str) const {
    istringstream ss(str);
    std::string res;
    ss >> res; ss >> res; ss >> res; ss >> res; ss >> res;
    return res;
  }
  inline std::string address_part(const std::string str) const {
    istringstream ss(str);
    std::string res;
    ss >> res; ss >> res; ss >> res;
    return res;
  }
#else
  inline std::string prog_part(const std::string str) const {
    return str.substr(0, str.find("("));
  }
  inline std::string mangled_part(const std::string str) const {
    std::string::size_type pos = str.find("(") + 1;
    if(str.find("+", pos) == std::string::npos) return "";
    return str.substr(pos, str.find("+", pos) - pos);
  }
  inline std::string offset_part(const std::string str) const {
    std::string::size_type pos = str.find("+");
    if(pos == std::string::npos) return "";
    else return str.substr(pos, str.find(")", pos) - pos);
  }
  inline std::string address_part(const std::string str) const {
    std::string::size_type pos = str.find("[");
    if(pos == std::string::npos) return "";
    else return str.substr(pos+1, str.find("]", pos) - pos-1);
  }
#endif

  // Used for set_color validation
  bool is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), s.end(),
                            [](char c) { return !std::isdigit(c); }) == s.end();
  }

  // Implementation of operator()
  template <bool B, typename U, typename V>
  typename std::enable_if<!B, void>::type
    print_stream_impl(const U& label, const V& obj, const std::string&) const {
      int dummy;
      std::string stream = typeid(*outstream).name();
      *outstream << "DebugPrinter error: object of type ";
                    has_stream<U>::value ? type(obj) : type(label);
      *outstream << "                    has no suitable "
                 << demangle(stream, dummy) << " operator<< overload." << std::endl;
  }
  template <bool B, typename U, typename V>
  typename std::enable_if<B, void>::type
    print_stream_impl(const U& label, const V& obj, const std::string& sc) const {
      *outstream << hcol_ << label << sc << obj
                 << hcol_r_ << std::endl;
  }


  // ToDo: move to external component
  template <bool B, typename... T>
  struct m_and_impl {
    typedef std::integral_constant<bool , B> type;
  };
  template <bool B, typename T, typename... U>
  struct m_and_impl<B, T, U...> {
    typedef typename m_and_impl<B && T::value, U...>::type type;
  };
  template <typename... T>
  struct m_and : public m_and_impl<true, T...>::type {
  };

  template<typename T, typename S>
  struct has_stream_impl {
      template<typename U>
      static auto check(int)
        -> decltype( std::declval<U>() << std::declval<T>(), std::true_type() );
      template<typename>
      static auto check(...) -> std::false_type;
      typedef decltype(check<S>(0)) type;
  };

  template<typename T, typename S = std::ostream>
  struct has_stream : public has_stream_impl<T, S&>::type {  // that ::type implicitly has ::value
  };

};

/*******************************************************************************
 * std::ostream overloads
 */

/** \brief operator<< overload for std::ostream */
template <typename T>
DebugPrinter & operator<<(DebugPrinter & d, const T& output) {
  std::ostream & out = *d.outstream;
  std::streamsize savep = out.precision();
  std::ios_base::fmtflags savef =
                 out.setf(std::ios_base::fixed, std::ios::floatfield);
  out << std::setprecision((int)d.prec_) << std::fixed << output
            << std::setprecision((int)savep);
  out.setf(savef, std::ios::floatfield);
  out.flush();
  return d;
}

/** \brief operator<< overload for std::ostream manipulators */
inline DebugPrinter & operator<<(DebugPrinter & d,
                                 std::ostream& (*pf)(std::ostream&)) {
  std::ostream & out = *d.outstream;
  out << pf;
  return d;
}

/** \brief operator, overload for std::ostream */
template <typename T>
inline DebugPrinter & operator,(DebugPrinter & d, const T& output) {
  return operator<<(d, output);
}

/** \brief operator, overload for std::ostream manipulators */
inline DebugPrinter & operator,(DebugPrinter & d,
                                std::ostream& (*pf)(std::ostream&)) {
  return operator<<(d, pf);
}

/*******************************************************************************
 * Namespace Globals
 */

// Heap allocate => no destructor call at program exit (wiped by OS).
//               => no management of std::ostream& possible
//                  (stop BLC design like shared_ptr on DebugPrinter::outstream)
/** \brief Static global heap-allocated object.*/
static DebugPrinter& dout = *new DebugPrinter;

/** Macros ********************************************************************/

/** \brief Print current line
 *  \details Shortcut for
 *  ~~~{.cpp}
 *      fsc::dout(__FILE__ ,__LINE__);
 *  ~~~
 */
#define dout_HERE fsc::dout(__FILE__ ,__LINE__);

/** \brief Print current function signature
 *  \details Shortcut for
 *  ~~~{.cpp}
 *      fsc::dout.stack(1, true);
 *  ~~~
 */
#define dout_FUNC fsc::dout.stack(1, true);

/** \brief Print highlighted 'name = value' of given variable
 *  \param x  The variable
 *  \details Shortcut for preprocessed equivalent of
 *  ~~~{.cpp}
 *      fsc::dout(#var, var, " = ");
 *  ~~~
 */
#define dout_VAR(x) fsc::dout(#x, x, " = ");

/** \brief Print demangled type information of given variable or type.
 *  \details Example usage:
 *  ~~~{.cpp}
 *      dout_TYPE(std::map<int,int>)
 *      dout_TYPE(var)
 *  ~~~
 */
#define dout_TYPE(...) fsc::DebugPrinter::detail::type_name(fsc::dout, typeid( __VA_ARGS__ ).name());

/** \brief Print a stack trace.
 *  \details Shortcut for
 *  ~~~{.cpp}
 *     fsc::dout.stack();
 *  ~~~
 */
#define dout_STACK fsc::dout.stack();

/**
 * End DebugPrinter implementation
 ******************************************************************************/

#else // DEBUGPRINTER_OFF

class DebugPrinter {
  public:

  inline void operator=(std::ostream &) {}
  inline void operator=(std::ostream &&) {}
  inline void set_precision(int) {}
  inline void set_color(...) {}

  inline void operator()(...) const {}

  template <typename T>
  inline void type(T) const {}
  inline void stack(...) const {}
};

template <typename T>
inline DebugPrinter & operator<<(DebugPrinter & d, const T&) {return d;}
inline DebugPrinter & operator<<(DebugPrinter & d,
                                 std::ostream& (*)(std::ostream&)) {return d;}
template <typename T>
inline DebugPrinter & operator,(DebugPrinter & d, const T&) {return d;}
inline DebugPrinter & operator,(DebugPrinter & d,
                                std::ostream& (*)(std::ostream&)) {return d;}

static DebugPrinter dout;

#define dout_HERE ;
#define dout_FUNC ;
#define dout_VAR(...) ;
#define dout_TYPE(...) ;
#define dout_STACK ;


#endif // DEBUGPRINTER_OFF

} // namespace fsc

#endif // DEBUGPRINTER_HEADER

