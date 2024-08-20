/**
 * @file   llexception.cpp
 * @author Nat Goodspeed
 * @date   2016-08-12
 * @brief  Implementation for llexception.
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
#include <iomanip>
#include <sstream>
#include <typeinfo>
// external library headers
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/error_info.hpp>
// On Mac, got:
// #error "Boost.Stacktrace requires `_Unwind_Backtrace` function. Define
// `_GNU_SOURCE` macro or `BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED` if
// _Unwind_Backtrace is available without `_GNU_SOURCE`."
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED

#if LL_WINDOWS
// On Windows, header-only implementation causes macro collisions -- use
// prebuilt library
#define BOOST_STACKTRACE_LINK
#endif // LL_WINDOWS

#include <boost/stacktrace.hpp>
// other Linden headers
#include "llerror.h"
#include "llerrorcontrol.h"


// used to attach and extract stacktrace information to/from boost::exception,
// see https://www.boost.org/doc/libs/release/doc/html/stacktrace/getting_started.html#stacktrace.getting_started.exceptions_with_stacktrace
// apparently the struct passed as the first template param needs no definition?
typedef boost::error_info<struct errinfo_stacktrace_, boost::stacktrace::stacktrace>
        errinfo_stacktrace;

namespace {
// used by crash_on_unhandled_exception_() and log_unhandled_exception_()
void log_unhandled_exception_(LLError::ELevel level,
                              const char* file, int line, const char* pretty_function,
                              const std::string& context)
{
    // log same message but allow caller-specified severity level
    LL_VLOGS(level, "LLException") << LLError::abbreviateFile(file)
        << "(" << line << "): Unhandled exception caught in " << pretty_function;
    if (! context.empty())
    {
        LL_CONT << ": " << context;
    }
    LL_CONT << ":\n" << boost::current_exception_diagnostic_information() << LL_ENDL;
}
}

void crash_on_unhandled_exception_(const char* file, int line, const char* pretty_function,
                                   const std::string& context)
{
    // LL_ERRS() terminates and propagates message into crash dump.
    log_unhandled_exception_(LLError::LEVEL_ERROR, file, line, pretty_function, context);
}

void log_unhandled_exception_(const char* file, int line, const char* pretty_function,
                              const std::string& context)
{
    // Use LL_WARNS() because we seriously do not expect this to happen
    // routinely, but we DO expect to return from this function.
    log_unhandled_exception_(LLError::LEVEL_WARN, file, line, pretty_function, context);
}

void annotate_exception_(boost::exception& exc)
{
    // https://www.boost.org/doc/libs/release/libs/exception/doc/tutorial_transporting_data.html
    // "Adding of Arbitrary Data to Active Exception Objects"
    // Given a boost::exception&, we can add boost::error_info items to it
    // without knowing its leaf type.
    // The stacktrace constructor that lets us skip a level -- and why would
    // we always include annotate_exception_()? -- also requires a max depth.
    // For the nullary constructor, the stacktrace class declaration itself
    // passes static_cast<std::size_t>(-1), but that's kind of dubious.
    // Anyway, which of us is really going to examine more than 100 frames?
    exc << errinfo_stacktrace(boost::stacktrace::stacktrace(1, 100));
}

#if LL_WINDOWS

// For windows SEH exception handling we sometimes need a filter that will
// separate C++ exceptions from C SEH exceptions
static constexpr U32 STATUS_MSC_EXCEPTION = 0xE06D7363; // compiler specific
static constexpr U32 STATUS_STACK_FULL    = 0xC00000FD;

void LL::seh::fill_stacktrace(std::string& stacktrace, U32 code)
{
    // Sadly, despite its diagnostic importance, trying to capture a
    // stacktrace when the stack is already blown only terminates us faster.
    if (code == STATUS_STACK_FULL)
    {
        stacktrace = "(stack overflow, no traceback)";
    }
    else
    {
        stacktrace = to_string(boost::stacktrace::stacktrace());
    }
}

U32 LL::seh::common_filter(U32 code, struct _EXCEPTION_POINTERS*)
{
    if (code == STATUS_MSC_EXCEPTION)
    {
        // C++ exception, don't stop at this handler
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else
    {
        // This is a non-C++ exception, e.g. hardware check.
        // Pass control into the handler block.
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

void LL::seh::rethrow(U32 code, const std::string& stacktrace)
{
    std::ostringstream out;
    out << "Windows exception 0x" << std::hex << code;
    if (! stacktrace.empty())
    {
        out << '\n' << stacktrace;
    }
    LLTHROW(Windows_SEH_exception(out.str()));
}

#endif //LL_WINDOWS
