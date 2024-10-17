/**
 * @file   llexception.h
 * @author Nat Goodspeed
 * @date   2016-06-29
 * @brief  Types needed for generic exception handling
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLEXCEPTION_H)
#define LL_LLEXCEPTION_H

#include "always_return.h"
#include "stdtypes.h"
#include <stdexcept>
#include <utility>                  // std::forward
#include <boost/exception/exception.hpp>
#include <boost/throw_exception.hpp>
#include <boost/current_function.hpp>
#if LL_WINDOWS
#include <excpt.h>
#endif // LL_WINDOWS

// "Found someone who can comfort me
//  But there are always exceptions..."
//  - Empty Pages, Traffic, from John Barleycorn (1970)
//    https://www.youtube.com/watch?v=dRH0CGVK7ic

/**
 * LLException is intended as the common base class from which all
 * viewer-specific exceptions are derived. Rationale for why it's derived from
 * both std::exception and boost::exception is explained in
 * tests/llexception_test.cpp.
 *
 * boost::current_exception_diagnostic_information() is quite wonderful: if
 * all we need to do with an exception is log it, in most places we should
 * catch (...) and log boost::current_exception_diagnostic_information().
 * See CRASH_ON_UNHANDLED_EXCEPTION() and LOG_UNHANDLED_EXCEPTION() below.
 *
 * There may be circumstances in which it would be valuable to distinguish an
 * exception explicitly thrown by viewer code from an exception thrown by
 * (say) a third-party library. Catching (const LLException&) supports such
 * usage. However, most of the value of this base class is in the
 * diagnostic_information() available via Boost.Exception.
 */
struct LLException:
    public std::runtime_error,
    public boost::exception
{
    LLException(const std::string& what):
        std::runtime_error(what)
    {}
};

/**
 * The point of LLContinueError is to distinguish exceptions that need not
 * terminate the whole viewer session. In general, an uncaught exception will
 * be logged and will crash the viewer. However, though an uncaught exception
 * derived from LLContinueError will still be logged, the viewer will attempt
 * to continue processing.
 */
struct LLContinueError: public LLException
{
    LLContinueError(const std::string& what):
        LLException(what)
    {}
};

/**
 * Please use LLTHROW() to throw viewer exceptions whenever possible. This
 * enriches the exception's diagnostic_information() with the source file,
 * line and containing function of the LLTHROW() macro.
 */
#define LLTHROW(x)                                                      \
do {                                                                    \
    /* Capture the exception object 'x' by value. (Exceptions must */   \
    /* be copyable.) It might seem simpler to use                  */   \
    /* BOOST_THROW_EXCEPTION(annotate_exception_(x)) instead of    */   \
    /* three separate statements, but:                             */   \
    /* - We want to throw 'x' with its original type, not just a   */   \
    /*   reference to boost::exception.                            */   \
    /* - To return x's original type, annotate_exception_() would  */   \
    /*   have to be a template function.                           */   \
    /* - We want annotate_exception_() to be opaque.               */   \
    /* We also might consider embedding BOOST_THROW_EXCEPTION() in */   \
    /* our helper function, but we want the filename and line info */   \
    /* embedded by BOOST_THROW_EXCEPTION() to be the throw point   */   \
    /* rather than always indicating the same line in              */   \
    /* llexception.cpp.                                            */   \
    auto exc{x};                                                        \
    annotate_exception_(exc);                                           \
    BOOST_THROW_EXCEPTION(exc);                                         \
    /* Use the classic 'do { ... } while (0)' macro trick to wrap  */   \
    /* our multiple statements.                                    */   \
} while (0)
void annotate_exception_(boost::exception& exc);

/// Call this macro from a catch (...) clause
#define CRASH_ON_UNHANDLED_EXCEPTION(CONTEXT) \
     crash_on_unhandled_exception_(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, CONTEXT)
void crash_on_unhandled_exception_(const char*, int, const char*, const std::string&);

/// Call this from a catch (const LLContinueError&) clause, or from a catch
/// (...) clause in which you do NOT want the viewer to crash.
#define LOG_UNHANDLED_EXCEPTION(CONTEXT) \
     log_unhandled_exception_(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, CONTEXT)
void log_unhandled_exception_(const char*, int, const char*, const std::string&);

/*****************************************************************************
*   Structured Exception Handling
*****************************************************************************/
// this is used in platform-generic code -- define outside #if LL_WINDOWS
struct Windows_SEH_exception: public LLException
{
    Windows_SEH_exception(const std::string& what): LLException(what) {}
};

namespace LL
{
namespace seh
{

#if LL_WINDOWS //-------------------------------------------------------------

void fill_stacktrace(std::string& stacktrace, U32 code);

// wrapper around caller's U32 filter(U32 code, struct _EXCEPTION_POINTERS*)
// filter function: capture a stacktrace, if possible, before forwarding the
// call to the caller's filter() function
template <typename FILTER>
U32 filter_(std::string& stacktrace, FILTER&& filter,
            U32 code, struct _EXCEPTION_POINTERS* exptrs)
{
    // By the time the handler gets control, the stack has been unwound,
    // so report the stack trace now at filter() time.
    fill_stacktrace(stacktrace, code);
    return std::forward<FILTER>(filter)(code, exptrs);
}

template <typename TRYCODE, typename FILTER, typename HANDLER>
auto catcher_inner(std::string& stacktrace,
                   TRYCODE&& trycode, FILTER&& filter, HANDLER&& handler)
{
    __try
    {
        return std::forward<TRYCODE>(trycode)();
    }
    __except (filter_(stacktrace,
                      std::forward<FILTER>(filter),
                      GetExceptionCode(), GetExceptionInformation()))
    {
        return always_return<decltype(trycode())>(
            std::forward<HANDLER>(handler), GetExceptionCode(), stacktrace);
    }
}

// triadic variant specifies try(), filter(U32, struct _EXCEPTION_POINTERS*),
// handler(U32, const std::string& stacktrace)
// stacktrace may or may not be available
template <typename TRYCODE, typename FILTER, typename HANDLER>
auto catcher(TRYCODE&& trycode, FILTER&& filter, HANDLER&& handler)
{
    // Construct and destroy this stacktrace string in the outer function
    // because we can't do either in the function with __try/__except.
    std::string stacktrace;
    return catcher_inner(stacktrace,
                         std::forward<TRYCODE>(trycode),
                         std::forward<FILTER>(filter),
                         std::forward<HANDLER>(handler));
}

// common_filter() handles the typical case in which we want our handler
// clause to handle only Structured Exceptions rather than explicitly-thrown
// C++ exceptions
U32 common_filter(U32 code, struct _EXCEPTION_POINTERS*);

// dyadic variant specifies try(), handler(U32, stacktrace), assumes common_filter()
template <typename TRYCODE, typename HANDLER>
auto catcher(TRYCODE&& trycode, HANDLER&& handler)
{
    return catcher(std::forward<TRYCODE>(trycode),
                   common_filter,
                   std::forward<HANDLER>(handler));
}

[[noreturn]] void rethrow(U32 code, const std::string& stacktrace);

// monadic variant specifies try(), assumes default filter and handler
template <typename TRYCODE>
auto catcher(TRYCODE&& trycode)
{
    return catcher(std::forward<TRYCODE>(trycode), rethrow);
}

#else  // not LL_WINDOWS -----------------------------------------------------

template <typename TRYCODE, typename FILTER, typename HANDLER>
auto catcher(TRYCODE&& trycode, FILTER&&, HANDLER&&)
{
    return std::forward<TRYCODE>(trycode)();
}

template <typename TRYCODE, typename HANDLER>
auto catcher(TRYCODE&& trycode, HANDLER&&)
{
    return std::forward<TRYCODE>(trycode)();
}

template <typename TRYCODE>
auto catcher(TRYCODE&& trycode)
{
    return std::forward<TRYCODE>(trycode)();
}

#endif // not LL_WINDOWS -----------------------------------------------------

} // namespace LL::seh
} // namespace LL

#endif /* ! defined(LL_LLEXCEPTION_H) */
