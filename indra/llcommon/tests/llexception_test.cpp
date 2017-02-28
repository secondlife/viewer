/**
 * @file   llexception_test.cpp
 * @author Nat Goodspeed
 * @date   2016-08-12
 * @brief  Tests for throwing exceptions.
 *
 * This isn't a regression test: it doesn't need to be run every build, which
 * is why the corresponding line in llcommon/CMakeLists.txt is commented out.
 * Rather it's a head-to-head test of what kind of exception information we
 * can collect from various combinations of exception base classes, type of
 * throw verb and sequences of catch clauses.
 *
 * This "test" makes no ensure() calls: its output goes to stdout for human
 * examination.
 *
 * As of 2016-08-12 with Boost 1.57, we come to the following conclusions.
 * These should probably be re-examined from time to time as we update Boost.
 *
 * - It is indisputably beneficial to use BOOST_THROW_EXCEPTION() rather than
 *   plain throw. The macro annotates the exception object with the filename,
 *   line number and function name from which the exception was thrown.
 *
 * - That being the case, deriving only from boost::exception isn't an option.
 *   Every exception object passed to BOOST_THROW_EXCEPTION() must be derived
 *   directly or indirectly from std::exception. The only question is whether
 *   to also derive from boost::exception. We decided to derive LLException
 *   from both, as it makes message output slightly cleaner, but this is a
 *   trivial reason: if a strong reason emerges to prefer single inheritance,
 *   dropping the boost::exception base class shouldn't be a problem.
 *
 * - (As you will have guessed, ridiculous things like a char* or int or a
 *   class derived from neither boost::exception nor std::exception can only
 *   be caught by that specific type or (...), and
 *   boost::current_exception_diagnostic_information() simply throws up its
 *   hands and confesses utter ignorance. Stay away from such nonsense.)
 *
 * - But if you derive from std::exception, to nat's surprise,
 *   boost::current_exception_diagnostic_information() gives as much
 *   information about exceptions in a catch (...) clause as you can get from
 *   a specific catch (const std::exception&) clause, notably the concrete
 *   exception class and the what() string. So instead of a sequence like
 *
 *   try { ... }
 *   catch (const boost::exception& e) { ... boost-flavored logging ... }
 *   catch (const std::exception& e)   { ... std::exception logging ... }
 *   catch (...)                       { ... generic logging ... }
 *
 *   we should be able to get away with only a catch (...) clause that logs
 *   boost::current_exception_diagnostic_information().
 *
 * - Going further: boost::current_exception_diagnostic_information() provides
 *   just as much information even within a std::set_terminate() handler. So
 *   it might not even be strictly necessary to include a catch (...) clause
 *   since the viewer does use std::set_terminate().
 *
 * - (We might consider adding a catch (int) clause because Kakadu internally
 *   throws ints, and who knows if one of those might leak out. If it does,
 *   boost::current_exception_diagnostic_information() can do nothing with it.
 *   A catch (int) clause could at least log the value and rethrow.)
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llexception.h"
// STL headers
// std headers
#include <typeinfo>
// external library headers
#include <boost/throw_exception.hpp>
// other Linden headers
#include "../test/lltut.h"

// helper for display output
// usage: std::cout << center(some string value, fill char, width) << std::endl;
// (assumes it's the only thing on that particular line)
struct center
{
    center(const std::string& label, char fill, std::size_t width):
        mLabel(label),
        mFill(fill),
        mWidth(width)
    {}

    // Use friend declaration not because we need to grant access, but because
    // it lets us declare a free operator like a member function.
    friend std::ostream& operator<<(std::ostream& out, const center& ctr)
    {
        std::size_t padded = ctr.mLabel.length() + 2;
        std::size_t left  = (ctr.mWidth - padded) / 2;
        std::size_t right = ctr.mWidth - left - padded;
        return out << std::string(left, ctr.mFill) << ' ' << ctr.mLabel << ' '
                   << std::string(right, ctr.mFill);
    }

    std::string mLabel;
    char mFill;
    std::size_t mWidth;
};

/*****************************************************************************
*   Four kinds of exceptions: derived from boost::exception, from
*   std::exception, from both, from neither
*****************************************************************************/
// Interestingly, we can't use this variant with BOOST_THROW_EXCEPTION()
// (which we want) -- we reach a failure topped by this comment:
//  //All boost exceptions are required to derive from std::exception,
//  //to ensure compatibility with BOOST_NO_EXCEPTIONS.
struct FromBoost: public boost::exception
{
    FromBoost(const std::string& what): mWhat(what) {}
    ~FromBoost() throw() {}
    std::string what() const { return mWhat; }
    std::string mWhat;
};

struct FromStd: public std::runtime_error
{
    FromStd(const std::string& what): std::runtime_error(what) {}
};

struct FromBoth: public boost::exception, public std::runtime_error
{
    FromBoth(const std::string& what): std::runtime_error(what) {}
};

// Same deal with FromNeither: can't use with BOOST_THROW_EXCEPTION().
struct FromNeither
{
    FromNeither(const std::string& what): mWhat(what) {}
    std::string what() const { return mWhat; }
    std::string mWhat;
};

/*****************************************************************************
*   Two kinds of throws: plain throw and BOOST_THROW_EXCEPTION()
*****************************************************************************/
template <typename EXC>
void plain_throw(const std::string& what)
{
    throw EXC(what);
}

template <typename EXC>
void boost_throw(const std::string& what)
{
    BOOST_THROW_EXCEPTION(EXC(what));
}

// Okay, for completeness, functions that throw non-class values. We wouldn't
// even deign to consider these if we hadn't found examples in our own source
// code! (Note that Kakadu's internal exception support is still based on
// throwing ints.)
void throw_char_ptr(const std::string& what)
{
    throw what.c_str(); // umm...
}

void throw_int(const std::string& what)
{
    throw int(what.length());
}

/*****************************************************************************
*   Three sequences of catch clauses:
*   boost::exception then ...,
*   std::exception then ...,
*   or just ...
*****************************************************************************/
void catch_boost_dotdotdot(void (*thrower)(const std::string&), const std::string& what)
{
    try
    {
        thrower(what);
    }
    catch (const boost::exception& e)
    {
        std::cout << "catch (const boost::exception& e)" << std::endl;
        std::cout << "e is " << typeid(e).name() << std::endl;
        std::cout << "boost::diagnostic_information(e):\n'"
                  << boost::diagnostic_information(e) << "'" << std::endl;
        // no way to report e.what()
    }
    catch (...)
    {
        std::cout << "catch (...)" << std::endl;
        std::cout << "boost::current_exception_diagnostic_information():\n'"
                  << boost::current_exception_diagnostic_information() << "'"
                  << std::endl;
    }
}

void catch_std_dotdotdot(void (*thrower)(const std::string&), const std::string& what)
{
    try
    {
        thrower(what);
    }
    catch (const std::exception& e)
    {
        std::cout << "catch (const std::exception& e)" << std::endl;
        std::cout << "e is " << typeid(e).name() << std::endl;
        std::cout << "boost::diagnostic_information(e):\n'"
                  << boost::diagnostic_information(e) << "'" << std::endl;
        std::cout << "e.what: '"
                  << e.what() << "'" << std::endl;
    }
    catch (...)
    {
        std::cout << "catch (...)" << std::endl;
        std::cout << "boost::current_exception_diagnostic_information():\n'"
                  << boost::current_exception_diagnostic_information() << "'"
                  << std::endl;
    }
}

void catch_dotdotdot(void (*thrower)(const std::string&), const std::string& what)
{
    try
    {
        thrower(what);
    }
    catch (...)
    {
        std::cout << "catch (...)" << std::endl;
        std::cout << "boost::current_exception_diagnostic_information():\n'"
                  << boost::current_exception_diagnostic_information() << "'"
                  << std::endl;
    }
}

/*****************************************************************************
*   Try a particular kind of throw against each of three catch sequences
*****************************************************************************/
void catch_several(void (*thrower)(const std::string&), const std::string& what)
{
    std::cout << std::string(20, '-') << "catch_boost_dotdotdot(" << what << ")" << std::endl;
    catch_boost_dotdotdot(thrower, "catch_boost_dotdotdot(" + what + ")");

    std::cout << std::string(20, '-') << "catch_std_dotdotdot(" << what << ")" << std::endl;
    catch_std_dotdotdot(thrower, "catch_std_dotdotdot(" + what + ")");

    std::cout << std::string(20, '-') << "catch_dotdotdot(" << what << ")" << std::endl;
    catch_dotdotdot(thrower, "catch_dotdotdot(" + what + ")");
}

/*****************************************************************************
*   For a particular kind of exception, try both kinds of throw against all
*   three catch sequences
*****************************************************************************/
template <typename EXC>
void catch_both_several(const std::string& what)
{
    std::cout << std::string(20, '*') << "plain_throw<" << what << ">" << std::endl;
    catch_several(plain_throw<EXC>, "plain_throw<" + what + ">");

    std::cout << std::string(20, '*') << "boost_throw<" << what << ">" << std::endl;
    catch_several(boost_throw<EXC>, "boost_throw<" + what + ">");
}

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llexception_data
    {
    };
    typedef test_group<llexception_data> llexception_group;
    typedef llexception_group::object object;
    llexception_group llexceptiongrp("llexception");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("throwing exceptions");

        // For each kind of exception, try both kinds of throw against all
        // three catch sequences
        std::size_t margin = 72;
        std::cout << center("FromStd", '=', margin) << std::endl;
        catch_both_several<FromStd>("FromStd");

        std::cout << center("FromBoth", '=', margin) << std::endl;
        catch_both_several<FromBoth>("FromBoth");

        std::cout << center("FromBoost", '=', margin) << std::endl;
        // can't throw with BOOST_THROW_EXCEPTION(), just use catch_several()
        catch_several(plain_throw<FromBoost>, "plain_throw<FromBoost>");

        std::cout << center("FromNeither", '=', margin) << std::endl;
        // can't throw this with BOOST_THROW_EXCEPTION() either
        catch_several(plain_throw<FromNeither>, "plain_throw<FromNeither>");

        std::cout << center("const char*", '=', margin) << std::endl;
        // We don't expect BOOST_THROW_EXCEPTION() to throw anything so daft
        // as a const char* or an int, so don't bother with
        // catch_both_several() -- just catch_several().
        catch_several(throw_char_ptr, "throw_char_ptr");

        std::cout << center("int", '=', margin) << std::endl;
        catch_several(throw_int, "throw_int");
    }
} // namespace tut
