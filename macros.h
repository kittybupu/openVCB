#pragma once
#ifndef HbZbx0UQ7ZbHdC6ycP7sanVKiv2tTMYY
#define HbZbx0UQ7ZbHdC6ycP7sanVKiv2tTMYY
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

#define OVCB_CAT2(_1, _2) _1 ## _2

#define OVCB_PASTE_0()
#define OVCB_PASTE_1(_1) _1
#define OVCB_PASTE_2(_1, _2) OVCB_CAT2(_1, _2)
#define OVCB_PASTE_3(_1, _2, _3) OVCB_PASTE_2(OVCB_PASTE_2(_1, _2), _3)
#define OVCB_PASTE_4(_1, _2, _3, _4) OVCB_PASTE_2(OVCB_PASTE_3(_1, _2, _3), _4)
#define OVCB_PASTE_5(_1, _2, _3, _4, _5) OVCB_PASTE_2(OVCB_PASTE_4(_1, _2, _3, _4), _5)
#define OVCB_PASTE_6(_1, _2, _3, _4, _5, _6) OVCB_PASTE_2(OVCB_PASTE_5(_1, _2, _3, _4, _5), _6)
#define OVCB_PASTE_7(_1, _2, _3, _4, _5, _6, _7) OVCB_PASTE_2(OVCB_PASTE_6(_1, _2, _3, _4, _5, _6), _7)
#define OVCB_PASTE_8(_1, _2, _3, _4, _5, _6, _7, _8) OVCB_PASTE_2(OVCB_PASTE_7(_1, _2, _3, _4, _5, _6, _7), _8)
#define OVCB_PASTE_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) OVCB_PASTE_2(OVCB_PASTE_8(_1, _2, _3, _4, _5, _6, _7, _8), _9)
#define OVCB_PASTE_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10) OVCB_PASTE_2(OVCB_PASTE_9(_1, _2, _3, _4, _5, _6, _7, _8, _9), _10)
#define OVCB_PASTE_11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) OVCB_PASTE_2(OVCB_PASTE_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10), _11)
#define OVCB_PASTE_12(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) OVCB_PASTE_2(OVCB_PASTE_11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11), _12)
#define OVCB_PASTE_13(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) OVCB_PASTE_2(OVCB_PASTE_12(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12), _13)
#define OVCB_PASTE_14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) OVCB_PASTE_2(OVCB_PASTE_13(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13), _14)
#define OVCB_PASTE_15(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) OVCB_PASTE_2(OVCB_PASTE_14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14), _15)

#define OVCB00_NUM_ARGS_b(_0a, _0b, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define OVCB00_NUM_ARGS_a(...) OVCB00_NUM_ARGS_b(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 0)
#define OVCB00_NUM_ARGS(...)   OVCB00_NUM_ARGS_a(__VA_ARGS__)
#define OVCB00_PASTE(...)      OVCB_PASTE_2(OVCB_PASTE_, OVCB00_NUM_ARGS(__VA_ARGS__))

#define OVCB_PASTE(...) OVCB00_PASTE(__VA_ARGS__)(__VA_ARGS__)

#define OVCB00_PASTE_TEST_2(a, b, c, d, ...) OVCB_PASTE(a, b, c, d, ##__VA_ARGS__)        // NOLINT(clang-diagnostic-gnu-zero-variadic-macro-arguments)
#define OVCB00_PASTE_TEST_1(a, b, c, ...)    OVCB00_PASTE_TEST_2(a, b, c, ##__VA_ARGS__)  // NOLINT(clang-diagnostic-gnu-zero-variadic-macro-arguments)
#if OVCB_PASTE(0, x, A, F, 5, 1, 0, U, L, L) != 0xAF510ULL || OVCB00_PASTE_TEST_1(0, x00, 9, B, 7, F, A, 3, F, F, 0, 0, U, L, L) != 0x009B7FA3FF00ULL
# error "Your C pre-processor is broken or non-conformant. Cannot continue."
#endif
#undef OVCB00_PASTE_TEST_1

/*--------------------------------------------------------------------------------------*/

#if !defined __attribute__ &&                                                       \
    ((!defined __GNUC__ && !defined __clang__ && !defined __INTEL_LLVM_COMPILER) || \
     defined __INTELLISENSE__ || defined __RESHARPER__)
# define __attribute__(x)
#endif
#ifndef __attr_access
# define __attr_access(x)  // NOLINT(bugprone-reserved-identifier, clang-diagnostic-reserved-macro-identifier, clang-diagnostic-unused-macros)
#endif
#if !defined __declspec && !defined _MSC_VER && !defined _WIN32
# define __declspec(...)
#endif
#ifndef __has_include
# define __has_include(x)
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