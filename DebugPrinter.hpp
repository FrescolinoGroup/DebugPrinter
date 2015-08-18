/** ****************************************************************************
 * 
 * \file       DebugPrinter.hpp
 * \brief      DebugPrinter header-only lib.
 * >           Creates a global static object named `dout` and defines the
 * >           macros: `dout_HERE`  `dout_FUNC`
 * \version    2015.0812
 * \author     Donjan Rodic, C. Frescolino
 * \date       2011-2015
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
 * #### Compilation:
 * 
 * Link with `-rdynamic` in order to properly get `stack()` frame names and
 * useful `dout_FUNC` output.
 * 
 * _Note_: compiler optimisations may inline functions (shorter stack).
 * 
 * Pass `DEBUGPRINTER_OFF` to turn off all functionality provided here. You can
 * leave the debug statements in your code, since all methods and macros become
 * inline and trivial, and thus should be optimised away by your compiler.
 * 
 * Pass `DEBUGPRINTER_NO_EXECINFO` flag on Windows (disables `stack()` and
 * `dout_FUNC`).
 * 
 * Pass `DEBUGPRINTER_NO_CXXABI` if you don't have a cxxabi demangle call in
 * your libc distribution. The stack and type methods will then print raw stack
 * frame names and a _c++filt_-ready output.
 * 
 * 
 * \class fsc::DebugPrinter
 * \brief Class for global static `dout` object
 * 
 * #### Usage:
 * For details and more examples, see the DebugPrinter member function
 * documentation.
 * 
 *     using namespace DebugPrinter;
 * 
 *     dout << "foo" << std::endl;
 *     dout, var , 5, " bar ", 6 << " foobar " << 7, 8;
 * 
 *     dout(object);                // highlights the object
 *     dout_HERE                    // shortcut for  dout(__func__, __LINE__);
 *     dout_stack(2);               // print top two stack frames
 *     dout_FUNC                    // print current function signature
 *     dout.type(object)            // RTTI for given object
 * 
 *     dout = std::cout             // set output stream
 *     dout.set_precision(13)       // set decimal display precision
 *     dout.set_color("31")        // set terminal highlighting color
 */


/**
 * DebugPrinter member function documentation
 * 
 * \fn inline void fsc::DebugPrinter::type(T obj)
 * \brief Prints the RTTI for given object
 * \details Example usage:
 * 
 *     dout.type(object)
 * 
 * \fn void fsc::DebugPrinter::stack() const
 * \brief Prints a stack trace
 * \param backtrace_size  print at most this many frames
 * \param compact         only print function names
 * \param begin           starting offset
 * \details Example usage:
 * 
 *     dout.stack();                // print a full stack trace
 *     dout.stack(count);           // print at most (int)count frames
 *     dout.stack(count, true);     // print in compact format
 *     dout_FUNC                    // shortcut for  dout.stack(1, true);
 * 
 * \fn inline void fsc::DebugPrinter::operator=(std::ostream & os)
 * \brief Assignment operator for changing streams
 * \param os  output stream to use
 * \details Default == `std::cout`. Change to any `std::ostream` with:
 * 
 *     dout = std::cerr;
 * 
 * \fn inline void fsc::DebugPrinter::set_precision(int prec)
 * \brief Number of displayed decimal digits
 * \param prec  desired precision
 * \details Default == 5. Example usage:
 * 
 *     dout.set_precision(12);
 * 
 * \fn inline void fsc::DebugPrinter::set_color(std::string str)
 * \brief Highlighting color
 * \param str  color code
 * \details
 * Assumes a `bash` compatible terminal and sets the `operator()` highlighting
 * color, for example cyan ( == "36" == default):
 * 
 *     dout.set_color("36");
 * 
 * \fn inline void fsc::DebugPrinter::set_color()
 * \brief Remvoe highlighting color
 * \details No color highlighting (e.g. when writing to a file) Usage example:
 * 
 *     dout.set_color();
 * 
 * For bash color codes check
 * http://www.tldp.org/HOWTO/Bash-Prompt-HOWTO/x329.html
 * 
 * \def dout_HERE
 * \brief shortcut
 * 
 * \def dout_FUNC
 * \brief shortcut
 * 
 ******************************************************************************/
 
 
#pragma once

/** \brief include guard */
#ifndef DEBUGPRINTER_HEADER
#define DEBUGPRINTER_HEADER

#include <iostream>

#ifndef DEBUGPRINTER_OFF

#include <iomanip>
#include <typeinfo>
#include <cstdlib>
#include <algorithm>

#ifndef DEBUGPRINTER_NO_EXECINFO
#include <execinfo.h>
#endif // DEBUGPRINTER_NO_EXECINFO

#ifndef DEBUGPRINTER_NO_CXXABI
#include <cxxabi.h>
#endif // DEBUGPRINTER_NO_CXXABI

#endif // DEBUGPRINTER_OFF

namespace fsc {

#ifndef DEBUGPRINTER_OFF

// DebugPrinter class, see documentation at the beginning of this file
class DebugPrinter {

  public:

/*******************************************************************************
 * Ctor and friends
 */

  DebugPrinter() : outstream(&std::cout), _prec(5), hcol("36") {}

  template <typename T>
  friend DebugPrinter & operator<<(DebugPrinter & d, const T& output);
  friend inline DebugPrinter & operator<<(DebugPrinter & d,
                                          std::ostream& (*pf)(std::ostream&));

/*******************************************************************************
 * Setters
 */

  inline void operator=(std::ostream & os) { outstream = &os; }
  inline void set_precision(int prec) { _prec = prec; }
  inline void set_color(std::string str) {  // no chaining <- returns void
    if(!is_number(str))
      throw std::runtime_error("DebugPrinter error: invalid set_color() argument");
    hcol = "\033[0;" + str + "m";
    hcol_r = "\033[0m";
  }
  inline void set_color() {  // no chaining <- returns void
    hcol = "";
    hcol_r = "";
  }

/*******************************************************************************
 * Parentheses operators
 */

  /** \brief Prints highlighted label and object
   *  \param label  should have a std::ostream & operator<< overload
   *  \param obj    should have a std::ostream & operator<< overload
   *  \param sc     delimiter between label and object
   *  \details Example usage:
   * 
   *      dout(label, object); */
  template <typename T, typename U>
  inline void operator()(const T& label, const U& obj,
                         const std::string sc = ": ") const {

    print_stream_impl<m_and<has_stream<T>, has_stream<U> >::value>(label, obj, sc);

  }
  /** \brief Prints highlighted object
   *  \param obj    should have a std::ostream & operator<< overload
   *  \details Equivalent to 
   * 
   *      dout(">>> ", obj, "");
   * 
   *  Example usage:
   * 
   *      dout(label); */
  template <typename T>
  inline void operator()(const T& obj) const { operator()(">>> ", obj, ""); }

/*******************************************************************************
 * RTTI (type and stack methods)
 */

  template <typename T>
  inline void type(T obj) const {
    int dummy;
    *outstream << demangle( typeid(obj).name() , dummy) << std::endl;
  }

  #ifndef DEBUGPRINTER_NO_EXECINFO

  void stack(
      const int backtrace_size = max_backtrace,
      const bool compact = false,
      const int begin = 1 /* should stay 1 except for special needs */
    ) const {

    std::ostream & out = *outstream;
    int _end = -1;               // ignore last

    void * stack[max_backtrace];
    int r = backtrace(stack, backtrace_size+begin-_end);
    char ** symbols = backtrace_symbols(stack, r);
    if(!symbols) return;

    int end = r + _end;
    if(compact == false)
      out << "DebugPrinter obtained " << end-begin << " stack frames:"
          << std::endl;

    #ifndef DEBUGPRINTER_NO_CXXABI

    for(int i = begin; i < end; ++i) {
      std::string prog = "  " + prog_part(std::string(symbols[i])) + ":  ";
      std::string mangled = mangled_part(std::string(symbols[i]));
      std::string offset = "  +" + offset_part(std::string(symbols[i]));
      if(compact == true) { prog = ""; offset = ""; }
      int status;
      std::string demangled = demangle(mangled, status);
      switch (status) {
        case -1:
          out << " Error: Could not allocate memory!" << std::endl;
          break;
        case -2:  // invalid name under the C++ ABI mangling rules
          out << prog << mangled << offset << std::endl;
          break;
        case -3:
          out << " Error: Invalid argument to demangle()" << std::endl;
          break;
        default:
          out << prog << demangled << offset << std::endl;
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

  // The dout_FUNC macro pollutes the namespace but is way more convenient
  //~ #ifndef DEBUGPRINTER_NO_CXXABI
  //~ void func() { stack(1, true, 2); }
  //~ #endif // DEBUGPRINTER_NO_CXXABI

  #else // DEBUGPRINTER_NO_EXECINFO

  void stack(
      const int backtrace_size = max_backtrace,
      const bool compact = false,
      const int begin = 1 /* should stay 1 except for special needs */
  ) const {
    *outstream << "DebugPrinter::stack() not available" << std::endl;
  }

  #endif // DEBUGPRINTER_NO_EXECINFO

/*******************************************************************************
 * Private parts
 */

  private:

  std::ostream * outstream;
  int _prec;
  std::string hcol;
  std::string hcol_r;

  static const unsigned int max_backtrace = 50;
  static const unsigned int max_demangled = 4096;


  #ifndef DEBUGPRINTER_NO_CXXABI

  inline std::string demangle(const std::string & str, int & status) const {
    char * demangled = abi::__cxa_demangle(str.c_str(), 0, 0, &status);
    std::string out = (status==0) ? std::string(demangled) : str;
    free(demangled);
    return out;
  }

  #else // DEBUGPRINTER_NO_CXXABI

  inline std::string demangle(const std::string & str, int & status) const {
    return str;
  }

  #endif // DEBUGPRINTER_NO_CXXABI


  inline std::string prog_part(const std::string str) const {
    return str.substr(0, str.find("("));
  }
  inline std::string mangled_part(const std::string str) const {
    std::string::size_type pos = str.find("(") + 1;
    return str.substr(pos, str.find("+", pos) - pos);
  }
  inline std::string offset_part(const std::string str) const {
    std::string::size_type pos = str.find("+") + 1;
    return str.substr(pos, str.find(")", pos) - pos);
  }
  bool is_number(const std::string& s) {
    std::string::const_iterator it = s.begin();
    return !s.empty() && std::find_if(s.begin(), s.end(),
                            [](char c) { return !std::isdigit(c); }) == s.end();
  }

  // Select with SFINAE and helper metatemplates
  template <bool B, typename U, typename V>
  typename std::enable_if<!B, void>::type
    print_stream_impl(const U&, const V& obj, const std::string&) const {
      int dummy;
      std::string stream = typeid(*outstream).name();
      *outstream << "DebugPrinter error: object of type "; type(obj);
      *outstream << "                    has no suitable "
                 << demangle(stream, dummy) << " operator<< overload." << std::endl;
  }
  template <bool B, typename U, typename V>
  typename std::enable_if<B, void>::type
    print_stream_impl(const U& label, const V& obj, const std::string& sc) const {
      *outstream << hcol << label << sc << obj
                 << hcol_r << std::endl;
  }


  // TODO: move to external detail
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
      static constexpr auto check(int)
        -> decltype( std::declval<U>() << std::declval<T>(), std::true_type() );

      template<typename>
      static constexpr auto check(...) -> std::false_type;
      typedef decltype(check<S>(0)) type;
  };

  template<typename T, typename S = std::ostream>
  struct has_stream : public has_stream_impl<T, S&>::type {  // that ::type has ::value
  };

};

/*******************************************************************************
 * std::ostream overloads
 */

/** \brief operator<< overload for std::ostream */
template <typename T>
DebugPrinter & operator<<(DebugPrinter & d, const T& output) {
  std::ostream & out = *d.outstream;
  size_t savep = (size_t)out.precision();
  std::ios_base::fmtflags savef =
                 out.setf(std::ios_base::fixed, std::ios::floatfield);
  out << std::setprecision(d._prec) << std::fixed << output
            << std::setprecision(savep);
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
 * Globals
 */


// Heap allocate => no destructor call at program exit (wiped by OS).
/** \brief Static global heap-allocated object.*/
static DebugPrinter& dout = *new DebugPrinter;

#define dout_HERE dout(__func__, __LINE__);
#define dout_FUNC dout.stack(1, true);


/**
 * End
 ******************************************************************************/

#else // DEBUGPRINTER_OFF

class DebugPrinter {
  public:

  inline void operator=(std::ostream &) {}
  inline void set_precision(int) {}
  inline void set_color(std::string) {}

  template <typename T>
  inline void operator()(const T&) const {}
  template <typename T, typename U>
  inline void operator()(const T&, const U&) const {}
  template <typename T, typename U>
  inline void operator()(const T&, const U&, const std::string) const {}

  template <typename T>
  inline void type(T) const {}
  inline void stack() const {}
  inline void stack(const int) const {}
  inline void stack(const int, const bool) const {}
  inline void stack(const int, const bool, const int) const {}
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


#endif // DEBUGPRINTER_OFF

} // namespace fsc

#endif // DEBUGPRINTER_HEADER

