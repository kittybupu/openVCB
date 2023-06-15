#pragma once
#ifndef FKlrDU3mpl8r0RC2FHcdSczyFbq49sWk
#define FKlrDU3mpl8r0RC2FHcdSczyFbq49sWk
/****************************************************************************************/

#if !defined _GNU_SOURCE && defined __gnu_linux__
# define _GNU_SOURCE //NOLINT
#endif

#ifdef _MSC_VER
# define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
# define _CRT_SECURE_NO_WARNINGS 1
# define _USE_DECLSPECS_FOR_SAL  1
#endif

#if defined _WIN32 || defined _WIN64
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN 1
# endif
# ifndef NOMINMAX
#  define NOMINMAX 1
# endif
#endif

#ifdef _MSC_VER
# include <sal.h>
#else
# define _Notnull_
# define _In_
# define _In_z_
# define _In_opt_
# define _In_opt_z_
# define _In_z_bytecount_(x)
# define _Out_
# define _Out_writes_(x)
# define _Out_z_cap_(x)
# define _Outptr_
# define _Outptr_result_z_
# define _Printf_format_string_
# define _Post_z_
# define _Noreturn_
# define _Analysis_noreturn_
#endif

#ifdef __RESHARPER__
# define _CRT_INTERNAL_NONSTDC_NAMES 1
#endif

//#define GLM_FORCE_MESSAGES 1
//#define GLM_FORCE_INLINE 1
#define GLM_HAS_IF_CONSTEXPR 1
#define GLM_HAS_CONSTEXPR 1
#define GLM_FORCE_XYZW_ONLY 1
//#define GLM_FORCE_AVX2 1
//#define GLM_FORCE_INTRINSICS 1

/*--------------------------------------------------------------------------------------*/

// ReSharper disable CppUnusedIncludeDirective
#ifdef __cplusplus

# include <algorithm>
# include <array>
# include <atomic>
# include <bit>
# include <chrono>
# include <concepts>
# include <exception>
# include <filesystem>
# include <forward_list>
# include <fstream>
# include <iostream>
# include <istream>
# include <iterator>
# include <limits>
# include <limits>
# include <list>
# include <map>
# include <memory>
# include <mutex>
# include <numeric>
# include <ostream>
# include <queue>
# include <random>
# include <ranges>
# include <set>
# include <sstream>
# include <stack>
# include <stdexcept>
# include <string>
# include <string_view>
# include <thread>
# include <tuple>
# include <unordered_map>
# include <unordered_set>
# include <utility>
# include <variant>
# include <vector>

# include <cassert>
# include <cerrno>
# include <cinttypes>
# include <climits>
# include <csignal>
# include <cstdarg>
# include <cstdint>
# include <cstdio>
# include <cstdlib>
# include <cstring>
# include <cwchar>

# include <glm/glm.hpp>

#if 0
# define FMT_HEADER_ONLY 1
# define FMT_USE_USER_DEFINED_LITERALS 0
# define FMT_USE_NONTYPE_TEMPLATE_ARGS 1
# define FMT_USE_FULL_CACHE_DRAGONBOX  1
# define FMT_INLINE   __forceinline
# define FMT_NOINLINE __declspec(noinline)
# include <fmt/format.h>
# include <fmt/format-inl.h>
# include <fmt/args.h>
# include <fmt/color.h>
# include <fmt/compile.h>
# include <fmt/os.h>
# include <fmt/ostream.h>
# include <fmt/ranges.h>
# include <fmt/std.h>
# include <fmt/xchar.h>
#endif

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
# include <wchar.h>

#endif // defined __cplusplus
// ReSharper restore CppUnusedIncludeDirective

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
# if defined _MSC_VER && defined _MSVC_LANG && _MSVC_LANG != __cplusplus
#  error "Invalid MSVC settings: add compiler option \"Zc:__cplusplus\""
# endif
# define CXX_LANG_VER __cplusplus
#endif

#if (defined _HAS_CXX17)                                  || \
    (defined __cplusplus      && __cplusplus >= 201703L)  || \
    (defined CXX_LANG_VER     && CXX_LANG_VER >= 201703L) || \
    (defined __STDC_VERSION__ && __STDC_VERSION__ > 201710L)
# define UNUSED            [[maybe_unused]]
# define NODISCARD         [[nodiscard]]
# define FALLTHROUGH       [[fallthrough]]
# define DEPRECATED        [[deprecated]]
# define DEPRECATED_MSG(x) [[deprecated(x)]]
#  ifdef _MSC_VER
#   define NORETURN        [[noreturn]]
#  else
#   define NORETURN        [[noreturn]]
#  endif
#elif defined __GNUC__ || defined __clang__ || defined __INTEL_COMPILER || defined __INTEL_LLVM_COMPILER
# define UNUSED            __attribute__((__unused__))
# define NODISCARD         __attribute__((__warn_unused_result__))
# define FALLTHROUGH       __attribute__((__fallthrough__))
# define NORETURN          __attribute__((__noreturn__))
# define DEPRECATED        __attribute__((__deprecated__))
# define DEPRECATED_MSG(x) __attribute__((__deprecated__(x)))
#elif defined _MSC_VER || defined __INTELLISENSE__ || defined __RESHARPER__
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
# define NOINLINE __attribute__((__noinline__))
#elif defined _MSC_VER
# define NOINLINE __declspec(noinline)
#else
# define NOINLINE
# define __forceinline __inline
#endif

#if !defined ATTRIBUTE_PRINTF
# if defined __RESHARPER__ || defined __INTEL_COMPILER || defined __INTEL_LLVM_COMPILER
#  define ATTRIBUTE_PRINTF(fst, snd) [[gnu::format(printf, fst, snd)]]
# elif defined __clang__ || !defined __GNUC__
#  define ATTRIBUTE_PRINTF(fst, snd) [[__gnu__::__format__(printf, fst, snd)]]
# elif defined __GNUC__
#  define ATTRIBUTE_PRINTF(fst, snd) [[__gnu__::__format__(gnu_printf, fst, snd)]]
# else
#  define ATTRIBUTE_PRINTF(fst, snd)
# endif
#endif

#define ATTRIBUTE_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#define PRINTF_FORMAT_STRING   _In_z_ _Printf_format_string_ char const *const __restrict

#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#ifndef __WORDSIZE
# if SIZE_MAX == UINT64_MAX
#  define __WORDSIZE 64
# elif SIZE_MAX == UINT32_MAX
#  define __WORDSIZE 32
# elif SIZE_MAX == UINT16_MAX
#  error "This is the no 16-bit computers club, and we already have a PDP-11. You'll have to leave."
# else
#  error "I have no useful warning message to give here."
# endif
#endif
#if __WORDSIZE != 64
# error "We don't serve your kind here. 64-bit computers only."
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

typedef unsigned int  uint;
typedef unsigned char uchar;
typedef signed char   schar;

#if __WORDSIZE == 64
#  define SIZE_C(n)  UINT64_C(n)
#  define SSIZE_C(n) INT64_C(n)
typedef int64_t ssize_t;
#elif __WORDSIZE == 32
#  define SIZE_C(n)  UINT32_C(n)
#  define SSIZE_C(n) INT32_C(n)
typedef int32_t ssize_t;
#else
#  define SIZE_C(n)  UINT16_C(n)
#  define SSIZE_C(n) INT16_C(n)
typedef int16_t ssize_t;
#endif

#ifdef __cplusplus
# define DELETE_COPY_CTORS(t)                                                      \
      t(t const &)                = delete; /*NOLINT(bugprone-macro-parentheses)*/ \
      t &operator=(t const &)     = delete  /*NOLINT(bugprone-macro-parentheses)*/

# define DELETE_MOVE_CTORS(t)                                                      \
      t(t &&) noexcept            = delete; /*NOLINT(bugprone-macro-parentheses)*/ \
      t &operator=(t &&) noexcept = delete  /*NOLINT(bugprone-macro-parentheses)*/

# define DEFAULT_COPY_CTORS(t)                                                      \
      t(t const &)                = default; /*NOLINT(bugprone-macro-parentheses)*/ \
      t &operator=(t const &)     = default  /*NOLINT(bugprone-macro-parentheses)*/

# define DEFAULT_MOVE_CTORS(t)                                                      \
      t(t &&) noexcept            = default; /*NOLINT(bugprone-macro-parentheses)*/ \
      t &operator=(t &&) noexcept = default  /*NOLINT(bugprone-macro-parentheses)*/

# define DELETE_ALL_CTORS(t) \
      DELETE_COPY_CTORS(t);  \
      DELETE_MOVE_CTORS(t)

#define DEFAULT_ALL_CTORS(t) \
      DEFAULT_COPY_CTORS(t); \
      DEFAULT_MOVE_CTORS(t)
#endif

/*======================================================================================*/

#ifdef __cplusplus
namespace openVCB {
using namespace ::std::literals;
namespace util {

ATTRIBUTE_PRINTF(1, 2)
extern void logf(PRINTF_FORMAT_STRING format, ...);

extern void logs(_In_ char const *msg, size_t len) ATTRIBUTE_NONNULL(1);
extern void logs(_In_ char const *msg) ATTRIBUTE_NONNULL(1);
inline void logs(std::string const &msg)     { logs(msg.data(), msg.size()); }
inline void logs(std::string_view const msg) { logs(msg.data(), msg.size()); }

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

namespace impl {

template <typename>
inline constexpr bool always_false = false;

# if (defined __GNUC__ || defined __clang__ || defined __INTEL_COMPILER) || \
     (__has_builtin(__builtin_bswap16) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64))
NODISCARD inline uint16_t bswap_native_16(uint16_t const val) { return __builtin_bswap16(val); }
NODISCARD inline uint32_t bswap_native_32(uint32_t const val) { return __builtin_bswap32(val); }
NODISCARD inline uint64_t bswap_native_64(uint64_t const val) { return __builtin_bswap64(val); }
# elif defined _MSC_VER
NODISCARD inline uint16_t bswap_native_16(uint16_t const val) { return _byteswap_ushort(val); }
NODISCARD inline uint32_t bswap_native_32(uint32_t const val) { return _byteswap_ulong(val); }
NODISCARD inline uint64_t bswap_native_64(uint64_t const val) { return _byteswap_uint64(val); }
# elif __cplusplus > 202002L && false
NODISCARD inline uint16_t bswap_native_16(uint16_t const val) { return ::std::byteswap(val); }
NODISCARD inline uint32_t bswap_native_32(uint32_t const val) { return ::std::byteswap(val); }
NODISCARD inline uint64_t bswap_native_64(uint64_t const val) { return ::std::byteswap(val); }
# else
#  define NO_bswap_SUPPORT
# endif

NODISCARD constexpr uint16_t bswap_16(uint16_t const val) noexcept {
# ifndef NO_bswap_SUPPORT
      if (::std::is_constant_evaluated())
# endif
            return static_cast<unsigned short>((val << 8) | (val >> 8));
# ifndef NO_bswap_SUPPORT
      else
            return bswap_native_16(val);
# endif
}

NODISCARD constexpr uint32_t bswap_32(uint32_t const val) noexcept
{
# ifndef NO_bswap_SUPPORT
      if (::std::is_constant_evaluated())
# endif
            return ((val << 030) & UINT32_C(0xFF00'0000)) |
                   ((val << 010) & UINT32_C(0x00FF'0000)) |
                   ((val >> 010) & UINT32_C(0x0000'FF00)) |
                   ((val >> 030) & UINT32_C(0x0000'00FF));
# ifndef NO_bswap_SUPPORT
      else
            return bswap_native_32(val);
# endif
}

NODISCARD constexpr uint64_t bswap_64(uint64_t const val) noexcept {
# ifndef NO_bswap_SUPPORT
      if (::std::is_constant_evaluated())
# endif
            return ((val << 070) & UINT64_C(0xFF00'0000'0000'0000)) |
                   ((val << 050) & UINT64_C(0x00FF'0000'0000'0000)) |
                   ((val << 030) & UINT64_C(0x0000'FF00'0000'0000)) |
                   ((val << 010) & UINT64_C(0x0000'00FF'0000'0000)) |
                   ((val >> 010) & UINT64_C(0x0000'0000'FF00'0000)) |
                   ((val >> 030) & UINT64_C(0x0000'0000'00FF'0000)) |
                   ((val >> 050) & UINT64_C(0x0000'0000'0000'FF00)) |
                   ((val >> 070) & UINT64_C(0x0000'0000'0000'00FF));
# ifndef NO_bswap_SUPPORT
      else
            return bswap_native_64(val);
# endif
}

template <typename T>
concept Swappable = ::std::is_integral_v<T> && sizeof(T) <= 8;

#ifdef NO_bswap_SUPPORT
# undef NO_bswap_SUPPORT
#endif

} // namespace impl


template <impl::Swappable T>
NODISCARD constexpr T bswap(T const val) noexcept
{
      if constexpr (sizeof(T) == 1)
            return val;
      else if constexpr (sizeof(T) == 2)
            return static_cast<T>(impl::bswap_16(static_cast<uint16_t>(val)));
      else if constexpr (sizeof(T) == 4)
            return static_cast<T>(impl::bswap_32(static_cast<uint32_t>(val)));
      else if constexpr (sizeof(T) == 8)
            return static_cast<T>(impl::bswap_64(static_cast<uint64_t>(val)));

      /* This isn't reachable but it shuts up dumber linters. */
      static_assert(impl::always_false<T>, "Invalid integer size");
      return -1;
}

template <impl::Swappable T>
NODISCARD constexpr T hton(T const val) noexcept
{
      if constexpr (::std::endian::native == ::std::endian::little)
            return bswap(val);
      else
            return val;
}

template <impl::Swappable T>
NODISCARD constexpr T ntoh(T const val) noexcept
{
      if constexpr (::std::endian::native == ::std::endian::little)
            return bswap(val);
      else
            return val;
}

NODISCARD inline intmax_t xatoi(char const *const ptr, int const base = 0) noexcept(false)
{
      char *eptr;
      auto &errno_ref = errno;
      errno_ref       = 0;

      auto const ans = ::strtoimax(ptr, &eptr, base);

      if (ptr == eptr)
            throw ::std::invalid_argument("Invalid atoi argument");
      if (errno_ref == ERANGE)
            throw ::std::range_error("atoi argument out of range");

      return ans;
}

NODISCARD inline uintmax_t xatou(char const *const ptr, int const base = 0) noexcept(false)
{
      char *eptr;
      auto &errno_ref = errno;
      errno_ref       = 0;

      auto const ans = ::strtoumax(ptr, &eptr, base);

      if (ptr == eptr)
            throw ::std::invalid_argument("Invalid atou argument");
      if (errno_ref == ERANGE)
            throw ::std::range_error("atou argument out of range");

      return ans;
}


} // namespace util
} // namespace openVCB
#endif


/****************************************************************************************/
#endif // Common.hh
// vim: ft=cpp