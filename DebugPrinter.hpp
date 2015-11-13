/** ****************************************************************************
 * 
 * \file       DebugPrinter.hpp
 * \brief      DebugPrinter header-only lib.
 * >           Creates a static object named `dout` and defines debugging macros.
 * \version    2015.1110
 * \author
 * Year      | Name
 * :-------: | -------------
 * 2011-2015 | Donjan Rodic
 *      2015 | Mario Könz
 *      2015 | C. Frescolino
 * \copyright  Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at\n
 *     http://www.apache.org/licenses/LICENSE-2.0 \n
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
 * Pass `DEBUGPRINTER_OFF` to turn off all functionality provided here. The 
 * debug statements can be left in the code, since all methods and macros 
 * become inline and trivial, and thus will be optimised away by the 
 * compiler at sufficient optimisation flags. This flag will be automatically
 * assumed if the standard `NDEBUG` is passed.
 * 
 * Pass `DEBUGPRINTER_NO_EXECINFO` flag on Windows (makes `stack()`,
 * `dout_STACK` and `dout_FUNC` trivial).
 * 
 * Pass `DEBUGPRINTER_NO_CXXABI` if you don't have a _cxxabi_ demangle call 
 * (translates raw type symbols, i.e. `typeid(std::string).name()` output `Ss`
 * to `std::string`) in your libc distribution. The stack and type methods will
 * then print raw stack frame names and a _c++filt_-ready output.
 * 
 * Pass `DEBUGPRINTER_NO_SIGNALS` to turn off automatic stack tracing when 
 * certain fatal signals occur. Passing this flag is recommended on
 * non-Unix-like systems.
 * 
 ******************************************************************************/

// ToDo: constexpr DebugPrinter for compile-time debugging
//       when setting a "constexpr string" as output

#ifndef DEBUGPRINTER_HEADER
#define DEBUGPRINTER_HEADER

#include <iostream>

#ifdef NDEBUG
#define DEBUGPRINTER_OFF
#endif

#ifndef DEBUGPRINTER_OFF

#include <iomanip>
#include <fstream>
#include <sstream>
#include <typeinfo>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <type_traits>

#ifndef DEBUGPRINTER_NO_EXECINFO
#include <execinfo.h>
#endif // DEBUGPRINTER_NO_EXECINFO

#ifndef DEBUGPRINTER_NO_CXXABI
#include <cxxabi.h>
#endif // DEBUGPRINTER_NO_CXXABI

#ifndef DEBUGPRINTER_NO_SIGNALS
#include <signal.h>
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
 *      #include <DebugPrinter.hpp>
 *      // ...
 * 
 *      // basic usage:
 * 
 *      dout_HERE                      // print current file and line
 *      dout_FUNC                      // print full current function signature
 *      dout_STACK                     // print stack trace
 *      dout_TYPE(std::map<T,U>)       // print given type
 *      dout_TYPE_OF(var)              // print RTTI of variable
 *      dout_VAL(var)                  // print highlighted 'name = value'
 *      dout_PAUSE()                   // wait for user input (enter key)
 *      dout_PAUSE(x < 10)             // conditionally wait for user input
 * 
 * 
 *      // advanced usage (non-exhaustive, check member details):
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
 *      dout.set_color("1;34")         // set terminal highlighting color
 *  ~~~
 *  In case the program terminates with `SIGSEGV`, `SIGSYS`, `SIGABRT` or
 *  `SIGFPE`, you will automatically get a stack trace from the raise location.
 *  To turn off this behaviour, check the \link Compilation \endlink section.
 */
class DebugPrinter {

  public:

/*******************************************************************************
 * DebugPrinter ctor and friends
 */
 
  /// \brief Constructor for dout and user specified DebugPrinter objects.
  DebugPrinter() {

    operator=(std::cout);
    set_precision(5);
    set_color("0;31");

    #ifndef DEBUGPRINTER_NO_SIGNALS
    struct sigaction act;
    act.sa_handler = signal_handler;
    for(auto const & sig: sig_names()) {
        sigaction(sig.first, &act, NULL);
    }
    #endif // DEBUGPRINTER_NO_SIGNALS

  }

  /// \brief Deleted copy constructor
  DebugPrinter(const DebugPrinter &) = delete;

  /// \brief Deleted move constructor
  DebugPrinter(DebugPrinter &&) = delete;

  template <typename T>
  friend inline DebugPrinter & operator<<(DebugPrinter &, const T&);

  friend inline DebugPrinter & operator<<(DebugPrinter &,
                                          std::ostream& (*pf)(std::ostream&));

/*******************************************************************************
 * DebugPrinter setters
 */

  /** \brief Assignment operator for changing streams
   *  \param os  output stream to use
   *  \details Default == `std::cout`. Change to any `std::ostream` with:
   *  ~~~{.cpp}
   *      dout = std::cerr;
   *  ~~~
   *  The DebugPrinter assumes that the object is managed elsewhere (to have it
   *  take ownership, check the assigment operator for moving streams).
   */
  inline void operator=(std::ostream & os) noexcept { outstream = &os; }

  /** \brief Assignment operator for moving streams
   *  \param os  output stream to take over
   *  \details Use to pass ownership if the stream object would leave scope.
   *  ~~~{.cpp}
   *      if(file_write == true) {
   *          std::ofstream fs("debug.log");
   *          dout = std::move(fs);
   *          dout.set_color();
   *      } // fs gets destroyed here
   *      dout << "This shows up in debug.log";
   *  ~~~
   *  Note: trying to move `std::cout` (or other static standard streams)
   *  is considered a bad life choice.
   */
  template <typename T>
  inline auto operator=(T && os)
    ->  std::enable_if_t<std::is_move_assignable<T>::value
                      && std::is_rvalue_reference<decltype(os)>::value> {
    outstream_mm = std::shared_ptr<std::ostream>(new T(std::move(os)));
    outstream = outstream_mm.get();
  }

  /** \brief Number of displayed decimal digits
   *  \param prec  desired precision
   *  \details Default == 5. Example usage:
   *  ~~~{.cpp}
   *      dout.set_precision(12);
   *  ~~~
   */
  inline void set_precision(const std::streamsize prec) noexcept { prec_ = prec; }

  /** \brief Highlighting color
   *  \param str  color code
   *  \details
   *  Assumes a `bash` compatible terminal and sets the `operator()`
   *  highlighting color (also used for `dout_HERE` and `dout_VAR`), for example
   *  red == "0;31" (default). Example usage:
   *  ~~~{.cpp}
   *      dout.set_color("1;34");
   *  ~~~
   *  For bash color codes check
   *  http://www.tldp.org/HOWTO/Bash-Prompt-HOWTO/x329.html
   */
  inline void set_color(const std::string str) {  // no chaining <- returns void
    if(is_number(str.substr(0,1)) && is_number(str.substr(2,2))) {
      hcol_ = "\033[" + str + "m";
      hcol_r_ = "\033[0m";
    } else
      throw std::runtime_error("DebugPrinter error: invalid set_color() argument");
  }
  /** \brief Remove highlighting color
   *  \details No color highlighting (e.g. when writing to a file). Example
   *  usage:
   *  ~~~{.cpp}
   *      dout.set_color();
   *  ~~~
   */
  inline void set_color() noexcept {  // no chaining <- returns void
    hcol_ = "";
    hcol_r_ = "";
  }

/*******************************************************************************
 * DebugPrinter parentheses operators
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
  inline void operator()(const T & label, U const & obj,
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
 * DebugPrinter info on type and RTTI (stack)
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

    using uint = unsigned int;

    uint end = begin + backtrace_size;   // don't pull more than necessary
    if(end > max_backtrace)
      end = max_backtrace;

    void * stack[max_backtrace];
    uint r = backtrace(stack, end);
    if(end == max_backtrace)             // prettiness hack: ignore binary line
      --r;
    end = r;                             // update to received end
    char ** symbols = backtrace_symbols(stack, end);

    if(compact == false)
      out << "DebugPrinter obtained " << end-begin << " stack frames:"
          << std::endl;
    if(!symbols) return;

    #ifndef DEBUGPRINTER_NO_CXXABI

    for(uint i = begin; i < end; ++i) {
      std::string line = std::string(symbols[i]);
      std::string prog = prog_part(line);
      std::string mangled = mangled_part(line);
      std::string offset = offset_part(line);
      std::string mainoffset = address_part(line);
      if(mangled == "")
        std::cerr << "DebugPrinter error: No dynamic symbol (you probably didn't compile with -rdynamic)"
                  << std::endl;
      int status;
      std::string demangled = demangle(mangled, status);
      switch (status) {
        case -1:
          out << "DebugPrinter error: Could not allocate memory!" << std::endl;
          break;
        case -3:
          out << "DebugPrinter error: Invalid argument to demangle()" << std::endl;
          break;
        case -2:  // invalid name under the C++ ABI mangling rules
          demangled = mangled;
          // fallthrough
        default:
          if(compact == false)
            out << "  " << prog << ":  " << demangled << "\t+"
                << offset << "\t[+" << mainoffset << "]"<< std::endl;
          else
            out << demangled << std::endl;
      }
    }
    if(compact == false) out << std::endl;

    #else // DEBUGPRINTER_NO_CXXABI

    for(uint i = begin; i < end; ++i) {
      if(compact == false)
        out << "  " << symbols[i] << std::endl;
      else
        out << mangled_part(std::string(symbols[i])) << std::endl;
    }

    if(compact == false) out << std::endl;
    out << "echo '' && c++filt";
    for(uint i = begin; i < end; ++i)
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
 * DebugPrinter "private" parts
 */
 
  /// \cond DEBUGPRINTER_DONOTDOCME_HAVESOMEDECENCY_PLEASE
  struct detail {

    DebugPrinter & super;

    // Simulate method specialisation through overloading
    template<typename T> struct fwdtype {};

    // Specialisation for type printing, used through dout_TYPE and dout_TYPE_OF
    #define DEBUGPRINTER_TYPE_SPEC(mods)                                       \
    template <typename T>                                                      \
    inline void type(detail::fwdtype<T mods>,                                  \
                     const std::string valness = "",                           \
                     const std::string expr = "") const {                      \
      int dummy;                                                               \
      std::string info = "";                                                   \
      if(expr != "")                                                           \
        info = "  {" + valness + " " + expr + "}";                             \
      auto mod = super.mod_split(std::string(#mods));                          \
      super << mod.first << super.demangle(typeid(T).name(), dummy)            \
            << mod.second << info << std::endl;                                \
    }                                                                         //
    DEBUGPRINTER_TYPE_SPEC()
    DEBUGPRINTER_TYPE_SPEC(&)
    DEBUGPRINTER_TYPE_SPEC(&&)
    DEBUGPRINTER_TYPE_SPEC(const)
    DEBUGPRINTER_TYPE_SPEC(const &)
    DEBUGPRINTER_TYPE_SPEC(const &&)
    DEBUGPRINTER_TYPE_SPEC(volatile)
    DEBUGPRINTER_TYPE_SPEC(volatile &)
    DEBUGPRINTER_TYPE_SPEC(volatile &&)
    DEBUGPRINTER_TYPE_SPEC(const volatile)
    DEBUGPRINTER_TYPE_SPEC(const volatile &)
    DEBUGPRINTER_TYPE_SPEC(const volatile &&)
    #undef DEBUGPRINTER_TYPE_SPEC

    // Get valueness of provided variable
    template<typename T>
    const std::string valueness(T &&) const noexcept {
        return super.valueness_impl(fwdtype<T>());
    }

    // Was used to print types and vars through same iface, but loses cv + ref
    // We cannot implement a function like sizeof() and typeid() that takes
    // types and instances.
    // ToDo: or can we?
    //       try appending #def TYPE TYPE_IMPL(input x) and abuse comma split
    //       and typeid/sizeof for x
    inline void type_name(const std::string & name,
                          const std::string & traits = "") {
      int dummy;
      *(super.outstream) << traits << super.demangle(name, dummy) << std::endl;
    }

    inline void pause(std::string reason) const {
      if(reason != "") reason = " (" + reason + ")";
      std::cout << "DebugPrinter paused" << reason
                << ". Press ENTER to continue." << std::flush;
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    template <typename T>
    static const T & pausecheck(const T & t) {
      return t;
    }
    static bool pausecheck() { return true; }

  } const detail_{*this};
  /// \endcond

/*******************************************************************************
 * DebugPrinter private parts
 */

  private:

  std::ostream * outstream;                      // output stream
  std::shared_ptr<std::ostream> outstream_mm;    // managed output stream
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
  //~ static DebugPrinter & static_init() { static DebugPrinter d; return d; }
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
    std::unique_ptr<char, void(*)(void*)>  // needs to call free, not delete
      dmgl(abi::__cxa_demangle(str.c_str(), 0, 0, &status), std::free);
    return (status==0) ? dmgl.get() : str;  // calls string ctor for char*
  }

  #else // DEBUGPRINTER_NO_CXXABI

  inline std::string demangle(const std::string & str, int &) const noexcept {
    return str;
  }

  #endif // DEBUGPRINTER_NO_CXXABI

  // Fetch different parts from a stack trace line
  #ifdef __APPLE__
  inline std::string prog_part(const std::string str) const {
    std::stringstream ss(str);
    std::string res;
    ss >> res; ss >> res;
    return res;
  }
  inline std::string mangled_part(const std::string str) const {
    std::stringstream ss(str);
    std::string res;
    ss >> res; ss >> res; ss >> res; ss >> res;
    return res;
  }
  inline std::string offset_part(const std::string str) const {
    std::stringstream ss(str);
    std::string res;
    ss >> res; ss >> res; ss >> res; ss >> res; ss >> res; ss >> res;
    return res;
  }
  inline std::string address_part(const std::string str) const {
    std::stringstream ss(str);
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
    else return str.substr(pos+1, str.find(")", pos) - pos-1);
  }
  inline std::string address_part(const std::string str) const {
    std::string::size_type pos = str.find("[");
    if(pos == std::string::npos) return "";
    else return str.substr(pos+1, str.find("]", pos) - pos-1);
  }
  #endif

  // Used for set_color validation
  bool is_number(const std::string& s) const {
    return !s.empty() && std::find_if(s.begin(), s.end(),
                            [](char c) { return !std::isdigit(c); }) == s.end();
  }

  // Implementation of operator()
  template <bool B, typename U, typename V>
  std::enable_if_t<!B>
  print_stream_impl(const U& label, const V& obj, const std::string&) const {
    auto typ = [this](const auto & obj)  // careless (dummy) demangle wrapper
               { int dummy = 0; return demangle(typeid(obj).name(), dummy); };
    *outstream << "DebugPrinter error: object of type "
               << ( has_stream<U>::value ? typ(obj) : typ(label) ) << std::endl
               << "                    has no suitable " << typ(*outstream)
               << " operator<< overload." << std::endl;
  }
  template <bool B, typename U, typename V>
  std::enable_if_t<B>
  print_stream_impl(const U& label, const V& obj, const std::string& sc) const {
    *outstream << hcol_ << label << sc << obj
               << hcol_r_ << std::endl;
  }


  // ToDo: move to external component
  template <bool B, typename... T>
  struct m_and_impl {
    using type = std::integral_constant<bool , B>;
  };

  template <bool B, typename T, typename... U>
  struct m_and_impl<B, T, U...> {
    using type = typename m_and_impl<B && T::value, U...>::type;
  };
  template <typename... T>
  struct m_and : public m_and_impl<true, T...>::type {
  };

  template<typename T, typename S>
  struct has_stream_impl {
      template<typename U>
      static auto check(int) noexcept
        -> decltype( std::declval<U>() << std::declval<T>(), std::true_type() );
      template<typename>
      static auto check(...) noexcept -> std::false_type;
      using type = decltype(check<S>(0));
  };

  template<typename T, typename S = std::ostream>
  struct has_stream : public has_stream_impl<T, S&>::type {  // that ::type implicitly has ::value
  };

  // Used to split mods for type()/type_of()
  std::pair<std::string, std::string> mod_split(const std::string & s) const {
    auto pos = s.find('&');
    pos = pos==std::string::npos ? s.size() : pos;
    std::string first = s.substr(0, pos), second = s.substr(pos);

    if(first.size() > 0 && first.back()!= ' ') first += " ";
    if(second.size() > 0) second = " " + second;
    return std::make_pair(first, second);
  }

  // Used for valueness printing
  template <typename T>
  const std::string valueness_impl(detail::fwdtype<T>) const noexcept
    { return "r-value"; }
  template <typename T>
  const std::string valueness_impl(detail::fwdtype<T&>) const noexcept
    { return "l-value"; }

};

/*******************************************************************************
 * std::ostream overloads
 */

/// \brief operator<< overload for std::ostream
template <typename T>
DebugPrinter & operator<<(DebugPrinter & d, const T& output) {
  std::ostream & out = *d.outstream;
  std::streamsize savep = out.precision();
  std::ios_base::fmtflags savef =
      out.setf(std::ios_base::fixed, std::ios::floatfield);
  out << std::setprecision(static_cast<int>(d.prec_)) << std::fixed << output
      << std::setprecision(static_cast<int>(savep));
  out.setf(savef, std::ios::floatfield);
  out.flush();
  return d;
}

/// \brief operator<< overload for std::ostream manipulators
inline DebugPrinter & operator<<(DebugPrinter & d,
                                 std::ostream& (*pf)(std::ostream&)) {
  std::ostream & out = *d.outstream;
  out << pf;
  return d;
}

/// \brief operator, overload for std::ostream
template <typename T>
inline DebugPrinter & operator,(DebugPrinter & d, const T& output) {
  return operator<<(d, output);
}

/// \brief operator, overload for std::ostream manipulators
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
/// \brief Static global heap-allocated object
static DebugPrinter& dout = *new DebugPrinter;

/*******************************************************************************
 * Macros
 */

/** \brief Print current line in the form `filename:line (function)`
 *  \details Example usage:
 *  ~~~{.cpp}
 *      dout_HERE
 *  ~~~
 *  Shortcut for (pseudocode):
 *  ~~~{.cpp}
 *      fsc::dout(__FILE__ ,__LINE__ + __func__, ":");
 *  ~~~
 * \hideinitializer
 */
#define dout_HERE fsc::dout(__FILE__ ,         std::to_string(__LINE__)        \
                                      + " (" + std::string(__func__) + ")",":");

/** \brief Print current function signature
 *  \details Example usage:
 *  ~~~{.cpp}
 *      template<typename T> void f(T&&) {
 *         dout_FUNC
 *      }
 *  ~~~
 *  Shortcut for:
 *  ~~~{.cpp}
 *      fsc::dout.stack(1, true);
 *  ~~~
 *  Needs to be compiled with `-rdynamic` (you will get an error message
 *  otherwise)
 * \hideinitializer
 */
#define dout_FUNC fsc::dout.stack(1, true);

/** \brief Print highlighted 'name = value' of given variable or expression
 *  \param ...  the expression to print; must be streamable into the stream
 *              object assigned to the currently used fsc::DebugPrinter
 *  \details
 *  Example usage:
 *  ~~~{.cpp}
 *      int x = 5;
 *      dout_VAL(x)
 *      dout_VAL(1==2)
 *  ~~~
 *  This is a shortcut for preprocessed equivalent of:
 *  ~~~{.cpp}
 *      fsc::dout(#expression, expression, " = ");
 *  ~~~
 * \hideinitializer
 */
#define dout_VAL(...) fsc::dout(#__VA_ARGS__, __VA_ARGS__, " = ");

/** \brief Print demangled type information of given type.
 *  \param ...  can't be an incomplete type.
 *  \details This macro prints the instantiated demangled input type.\n
 *  Example usage:
 *  ~~~{.cpp}
 *      dout_TYPE(std::map<T,U>)   // in a template
 *  ~~~
 * \hideinitializer
 */
#define dout_TYPE(...) fsc::dout.detail_.type(                                 \
  fsc::DebugPrinter::detail::fwdtype<__VA_ARGS__>()                            \
);                                                                            //

/** \brief Print demangled type information of given variable or expression.
 *  \param ...  can't have an incomplete type.
 *  \details This macro prints the demangled (as specified by the local cxxabi)
 *           RTTI of the input variable or resolved expression, including cvr
 *           qualifiers and valueness.\n
 *  Example usage:
 *  ~~~{.cpp}
 *      dout_TYPE_OF(var)                           // var is an object
 *      dout_TYPE_OF(std::cout << 1 << std::endl;)
 *  ~~~
 * \hideinitializer
 */
#define dout_TYPE_OF(...) fsc::dout.detail_.type(                              \
    fsc::DebugPrinter::detail::fwdtype<decltype(__VA_ARGS__)>()                \
  , fsc::dout.detail_.valueness(__VA_ARGS__)                                   \
  , #__VA_ARGS__                                                               \
);                                                                            //

/** \brief Print a stack trace.
 *  \details  Example usage:
 *  ~~~{.cpp}
 *      void f2() {
 *        dout_STACK
 *      }
 *      void f1() {
 *        f2();
 *      }
 *  ~~~
 *  Shortcut for
 *  ~~~{.cpp}
 *     fsc::dout.stack();
 *  ~~~
 *  Needs to be compiled with `-rdynamic` (you will get an error message
 *  otherwise). Keep in mind that compiler optimisations will inline functions.
 * \hideinitializer
 */
#define dout_STACK fsc::dout.stack();

/** \brief Pause execution (optionally) and wait for user key press (ENTER).
 *  \param ...  \n
 *              A string literal can be specified as argument to act as label.\n
 *              A pause condition can be specified as argument. This must be a
 *              valid expression for an if-statement.\n
 *  \details Example usage:
 *  ~~~{.cpp}
 *     dout_PAUSE()
 *     dout_PAUSE("label")
 *     for(int i = 0; i < 10; ++i)
 *       dout_PAUSE(i >= 8)
 *  ~~~
 * \hideinitializer
 */
#define dout_PAUSE(...)                                                        \
  if(fsc::dout.detail_.pausecheck(__VA_ARGS__))                                \
    fsc::dout.detail_.pause(#__VA_ARGS__);                                    //

/**
 * End DebugPrinter implementation
 ******************************************************************************/

#else // DEBUGPRINTER_OFF

class DebugPrinter {
  public:
  inline void operator=(std::ostream &) noexcept {}
  inline void operator=(std::ostream &&) {}
  inline void set_precision(const int) noexcept {}
  inline void set_color(...) noexcept {}
  inline void operator()(...) const {}
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
#define dout_VAL(...) ;
#define dout_TYPE(...) ;
#define dout_TYPE_OF(...) ;
#define dout_STACK ;
#define dout_PAUSE(...) ;

#endif // DEBUGPRINTER_OFF

/// \cond DEBUGPRINTER_DONOTDOCME_HAVESOMEDECENCY_PLEASE
namespace detail {
  namespace dont_even_ask {

    inline void function() { (void)dout; }

  }
}
/// \endcond

} // namespace fsc

#endif // DEBUGPRINTER_HEADER

