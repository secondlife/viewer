/**
 * @file llpreprocessor.h
 * @brief This file should be included in all Linden Lab files and
 * should only contain special preprocessor directives
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LLPREPROCESSOR_H
#define LLPREPROCESSOR_H

// Figure out endianness of platform
#ifdef LL_LINUX
#define __ENABLE_WSTRING
#include <endian.h>
#endif  //  LL_LINUX

#if (defined(LL_WINDOWS) || (defined(LL_LINUX) && (__BYTE_ORDER == __LITTLE_ENDIAN)) || (defined(LL_DARWIN) && defined(__LITTLE_ENDIAN__)))
#define LL_LITTLE_ENDIAN 1
#else
#define LL_BIG_ENDIAN 1
#endif


// Per-compiler switches

#ifdef __GNUC__
#define LL_FORCE_INLINE inline __attribute__((always_inline))
#else
#define LL_FORCE_INLINE __forceinline
#endif

// Mark-up expressions with branch prediction hints.  Do NOT use
// this with reckless abandon - it's an obfuscating micro-optimization
// outside of inner loops or other places where you are OVERWHELMINGLY
// sure which way an expression almost-always evaluates.
#if __GNUC__ >= 3
# define LL_LIKELY(EXPR) __builtin_expect (!!(EXPR), true)
# define LL_UNLIKELY(EXPR) __builtin_expect (!!(EXPR), false)
#else
# define LL_LIKELY(EXPR) (EXPR)
# define LL_UNLIKELY(EXPR) (EXPR)
#endif


// Figure out differences between compilers
#if defined(__GNUC__)
    #define GCC_VERSION (__GNUC__ * 10000 \
                        + __GNUC_MINOR__ * 100 \
                        + __GNUC_PATCHLEVEL__)
    #ifndef LL_GNUC
        #define LL_GNUC 1
    #endif
#elif defined(__MSVC_VER__) || defined(_MSC_VER)
    #ifndef LL_MSVC
        #define LL_MSVC 1
    #endif
#endif

// Deal with minor differences on Unixy OSes.
#if LL_DARWIN || LL_LINUX
    // Different name, same functionality.
    #define stricmp strcasecmp
    #define strnicmp strncasecmp

    // Not sure why this is different, but...
    #ifndef MAX_PATH
        #define MAX_PATH PATH_MAX
    #endif  //  not MAX_PATH

#endif

// Although thread_local is now a standard storage class, we can't just
// #define LL_THREAD_LOCAL as thread_local because the *usage* is different.
// We'll have to take the time to change LL_THREAD_LOCAL declarations by hand.
#if LL_WINDOWS
# define LL_THREAD_LOCAL __declspec(thread)
#else
# define LL_THREAD_LOCAL __thread
#endif


#if defined(LL_WINDOWS)
#define BOOST_REGEX_NO_LIB 1
#define CURL_STATICLIB 1
#ifndef XML_STATIC
#define XML_STATIC
#endif
#endif  //  LL_WINDOWS


// Deal with VC++ problems
#if LL_MSVC
// level 4 warnings that we need to disable:
#pragma warning (disable : 4251) // member needs to have dll-interface to be used by clients of class
#pragma warning (disable : 4275) // non dll-interface class used as base for dll-interface class
#endif  //  LL_MSVC

#if LL_WINDOWS
#define LL_DLLEXPORT __declspec(dllexport)
#define LL_DLLIMPORT __declspec(dllimport)
#elif LL_LINUX
#define LL_DLLEXPORT __attribute__ ((visibility("default")))
#define LL_DLLIMPORT
#else
#define LL_DLLEXPORT
#define LL_DLLIMPORT
#endif // LL_WINDOWS

#if __clang__ || ! defined(LL_WINDOWS)
// Only on Windows, and only with the Microsoft compiler (vs. clang) is
// wchar_t potentially not a distinct type.
#define LL_WCHAR_T_NATIVE 1
#else  // LL_WINDOWS
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
// _WCHAR_T_DEFINED is defined if wchar_t is provided at all.
// Specifically, it has value 1 if wchar_t is an intrinsic type, else empty.
// _NATIVE_WCHAR_T_DEFINED has value 1 if wchar_t is intrinsic, else undefined.
// For years we have compiled with /Zc:wchar_t-, meaning that wchar_t is a
// typedef for unsigned short (in stddef.h). Lore has it that one of our
// proprietary binary-only libraries has traditionally been built that way and
// therefore EVERYTHING ELSE requires it. Therefore, in a typical Linden
// Windows build, _WCHAR_T_DEFINED is defined but empty, while
// _NATIVE_WCHAR_T_DEFINED is undefined.
# if defined(_NATIVE_WCHAR_T_DEFINED)
#  define LL_WCHAR_T_NATIVE 1
# endif // _NATIVE_WCHAR_T_DEFINED
#endif // LL_WINDOWS

#if LL_COMMON_LINK_SHARED
// CMake automagically defines llcommon_EXPORTS only when building llcommon
// sources, and only when llcommon is a shared library (i.e. when
// LL_COMMON_LINK_SHARED). We must still test LL_COMMON_LINK_SHARED because
// otherwise we can't distinguish between (non-llcommon source) and (llcommon
// not shared).
# if defined(llcommon_EXPORTS)
#   define LL_COMMON_API LL_DLLEXPORT
# else //llcommon_EXPORTS
#   define LL_COMMON_API LL_DLLIMPORT
# endif //llcommon_EXPORTS
#else // LL_COMMON_LINK_SHARED
# define LL_COMMON_API
#endif // LL_COMMON_LINK_SHARED

// With C++11, decltype() is standard. We no longer need a platform-dependent
// macro to get the type of an expression.
#define LL_TYPEOF(expr) decltype(expr)

#define LL_TO_STRING_HELPER(x) #x
#define LL_TO_STRING(x) LL_TO_STRING_HELPER(x)
#define LL_TO_WSTRING_HELPER(x) L#x
#define LL_TO_WSTRING(x) LL_TO_WSTRING_HELPER(x)
#define LL_FILE_LINENO_MSG(msg) __FILE__ "(" LL_TO_STRING(__LINE__) ") : " msg
#define LL_GLUE_IMPL(x, y) x##y
#define LL_GLUE_TOKENS(x, y) LL_GLUE_IMPL(x, y)

#if LL_WINDOWS
#define LL_COMPILE_TIME_MESSAGE(msg) __pragma(message(LL_FILE_LINENO_MSG(msg)))
#else
// no way to get gcc 4.2 to print a user-defined diagnostic message only when a macro is used
#define LL_COMPILE_TIME_MESSAGE(msg)
#endif

// __FUNCTION__ works on all the platforms we care about, but...
#if LL_WINDOWS
#define LL_PRETTY_FUNCTION __FUNCSIG__
#else
#define LL_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

#endif  //  not LL_LINDEN_PREPROCESSOR_H
