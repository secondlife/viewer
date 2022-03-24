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

#include <stdexcept>
#include <boost/exception/exception.hpp>
#include <boost/throw_exception.hpp>
#include <boost/current_function.hpp>

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


#if LL_WINDOWS

// SEH exception filtering for use in __try __except
// Separates C++ exceptions from C SEH exceptions
// Todo: might be good idea to do some kind of seh_to_msc_wrapper(function, ARGS&&);
U32 msc_exception_filter(U32 code, struct _EXCEPTION_POINTERS *exception_infop);

#endif //LL_WINDOWS

#endif /* ! defined(LL_LLEXCEPTION_H) */
