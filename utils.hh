#pragma once
#ifndef FKlrDU3mpl8r0RC2FHcdSczyFbq49sWk
#define FKlrDU3mpl8r0RC2FHcdSczyFbq49sWk
/****************************************************************************************/

#if !defined _GNU_SOURCE && defined __gnu_linux__
#  define _GNU_SOURCE //NOLINT
#endif

#ifdef _MSC_VER
#  define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#  define _CRT_SECURE_NO_WARNINGS 1
#  define _USE_DECLSPECS_FOR_SAL  1
#endif

#if defined _WIN32 || defined _WIN64
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX 1
#  endif
#endif

#ifndef _MSC_VER
#  define _Notnull_
#  define _In_
#  define _In_z_
#  define _In_opt_
#  define _In_opt_z_
#  define _In_z_bytecount_(x)
#  define _Out_
#  define _Out_writes_(x)
#  define _Out_z_cap_(x)
#  define _Outptr_
#  define _Outptr_result_z_
#  define _Printf_format_string_
#  define _Post_z_
#  define _Noreturn_
#  define _Analysis_noreturn_
#endif

#ifdef __RESHARPER__
#  define _CRT_INTERNAL_NONSTDC_NAMES 1
#endif

/*--------------------------------------------------------------------------------------*/

#ifdef __cplusplus

/*
 * Include all the things? No.
 */
#include <algorithm>
//#include <any>
//#include <array>
#include <atomic>
//#include <bit>
//#include <bitset>
#include <chrono>
//#include <codecvt>
#include <compare>
#include <concepts>
#include <condition_variable>
//#include <exception>
#include <filesystem>
#include <forward_list>
//#include <fstream>
//#include <functional>
//#include <future>
//#include <iomanip>
//#include <ios>
//#include <iosfwd>
#include <iostream>
//#include <istream>
#include <iterator>
#include <limits>
#include <list>
//#include <locale>
#include <map>
#include <memory>
#include <mutex>
//#include <new>
//#include <numbers>
//#include <numeric>
//#include <optional>
//#include <ostream>
#include <queue>
#include <random>
//#include <ratio>
//#include <regex>
//#include <scoped_allocator>
#include <set>
#include <span>
#include <sstream>
#include <stack>
//#include <stdexcept>
#include <string>
#include <string_view>
//#include <system_error>
#include <thread>
#include <tuple>
//#include <type_traits>
//#include <typeindex>
//#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
//#include <valarray>
#include <variant>
#include <vector>

#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

inline namespace MAIN_PACKAGE_NAMESPACE {
using namespace std::literals;
namespace util {
template <typename T1>
constexpr bool eq_any(T1 const &left, T1 const &right)
{
      return left == right;
}

template <typename T1, typename ...Types>
constexpr bool eq_any(T1 const &left, T1 const &right, Types const &...rest)
{
      return left == right || eq_any(left, rest...);
}
} // namespace util
} // namespace MAIN_PACKAGE_NAMESPACE

#else // not C++

# include <assert.h>
# include <ctype.h>
# include <errno.h>
# include <inttypes.h>
# include <limits.h>
# include <stdarg.h>
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

#endif // defined __cplusplus

/*--------------------------------------------------------------------------------------*/

#if !defined __attribute__ &&                                                       \
    ((!defined __GNUC__ && !defined __clang__ && !defined __INTEL_LLVM_COMPILER) || \
     defined __INTELLISENSE__ || defined __RESHARPER__)
# define __attribute__(x)
#endif
#ifndef __attr_access
# define __attr_access(x)
#endif
#if !defined __declspec && !defined _MSC_VER && !defined _WIN32
# define __declspec(...)
#endif
#ifndef __has_include
# define __has_include(x)
#endif
#if defined __GNUC__ && (defined __clang_major__ && __clang_major__ >= 14)
# define __attribute_error__(x) __attribute__((__error__(x)))
#else
# define __attribute_error__(x) DEPRECATED_MSG(x)
#endif

#ifdef __cplusplus
# if defined _MSC_VER && defined _MSVC_LANG
#  define CXX_LANG_VER _MSVC_LANG
# endif
# ifndef CXX_LANG_VER
#  define CXX_LANG_VER __cplusplus
# endif
#endif

#if (defined __cplusplus      && __cplusplus >= 201703L)  || \
    (defined CXX_LANG_VER     && CXX_LANG_VER >= 201703L) || \
    (defined __STDC_VERSION__ && __STDC_VERSION__ > 201710L)
# define UNUSED            [[maybe_unused]]
# define NODISCARD         [[nodiscard]]
# define FALLTHROUGH       [[fallthrough]]
# define DEPRECATED        [[deprecated]]
# define DEPRECATED_MSG(x) [[deprecated(x)]]
#  ifdef _MSC_VER
#   define NORETURN        [[noreturn]] _Analysis_noreturn_ 
#  else
#   define NORETURN        [[noreturn]]
#  endif
#elif defined __GNUC__ || defined __clang__
# define UNUSED            __attribute__((__unused__))
# define NODISCARD         __attribute__((__warn_unused_result__))
# define FALLTHROUGH       __attribute__((__fallthrough__))
# define NORETURN          __attribute__((__noreturn__))
# define DEPRECATED        __attribute__((__deprecated__))
# define DEPRECATED_MSG(x) __attribute__((__deprecated__(x)))
#elif defined _MSC_VER
# define UNUSED            __pragma(warning(suppress : 4100 4101 4102))
# define NODISCARD         _Check_return_
# define FALLTHROUGH       __fallthrough
# define NORETURN          __declspec(noreturn)
# define DEPRECATED        __declspec(deprecated)
# define DEPRECATED_MSG(x) __declspec(deprecated(x))
#else
# define UNUSED
# define NODISCARD
# define NORETURN
# define FALLTHROUGH
# define DEPRECATED
# define DEPRECATED_MSG(x)
#endif

#ifndef NO_OBNOXIOUS_TWO_LETTER_CONVENIENCE_MACROS_PLEASE
# define ND NODISCARD
# define UU UNUSED
#endif

#if (defined __GNUC__ || defined __clang__) && !defined _MSC_VER
# define NOINLINE __attribute__((__noinline__))
# ifndef __always_inline
#  define __always_inline __attribute__((__always_inline__)) __inline
# endif
# if !defined __forceinline && !(defined _MSC_VER || defined __INTEL_LLVM_COMPILER)
#  define __forceinline __always_inline
# endif
#elif defined _MSC_VER
# define NOINLINE __declspec(noinline)
#else
# define NOINLINE
# define __forceinline __inline
#endif

#if !defined ATTRIBUTE_PRINTF
# if defined __clang__ || defined __INTEL_LLVM_COMPILER || !defined __GNUC__
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__printf__, __VA_ARGS__)))
# else
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__gnu_printf__, __VA_ARGS__)))
# endif
#endif

#define ATTRIBUTE_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#define PRINTF_FORMAT_STRING   _In_z_ _Printf_format_string_ char const *__restrict

#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#ifndef __WORDSIZE
# if SIZE_MAX == ULLONG_MAX
#  define __WORDSIZE 64
# elif SIZE_MAX == UINT_MAX
#  define __WORDSIZE 32
# elif SIZE_MAX == SHRT_MAX
#  error "This is the no 16-bit computers club, and we already have a PDP-11. You'll have to leave."
# else
#  error "I have no useful warning message to give here."
# endif
#endif

#if __WORDSIZE == 64 && (defined __amd64__ || defined __amd64 || defined __x86_64__ || \
                         defined __x86_64 || defined _M_AMD64 || defined _AMD64_)
# ifndef __x86_64__
#  define __x86_64__ 1
# endif
#elif __WORDSIZE == 32 &&                                                            \
    (defined __i386__ || defined __i486__ || defined __i586__ || defined __i686__ || \
     defined __i386 || defined __IA32__ || defined __I86__ || defined __386 ||       \
     defined _M_I86 || defined _M_IX86)
# ifndef __i386__
#  define __i386__ 1
# endif
#endif


/****************************************************************************************/
#endif // Common.hh
// vim: ft=cpp