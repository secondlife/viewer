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

/**
 * LLException is intended as the common base class from which all
 * viewer-specific exceptions are derived. It is itself a subclass of
 * boost::exception; use catch (const boost::exception& e) clause to log the
 * string from boost::diagnostic_information(e).
 *
 * Since it is also derived from std::exception, a generic catch (const
 * std::exception&) should also work, though what() is unlikely to be as
 * informative as boost::diagnostic_information().
 *
 * Please use BOOST_THROW_EXCEPTION()
 * http://www.boost.org/doc/libs/release/libs/exception/doc/BOOST_THROW_EXCEPTION.html
 * to throw viewer exceptions whenever possible. This enriches the exception's
 * diagnostic_information() with the source file, line and containing function
 * of the BOOST_THROW_EXCEPTION() macro.
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

#endif /* ! defined(LL_LLEXCEPTION_H) */
