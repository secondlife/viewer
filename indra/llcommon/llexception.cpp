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
#include <typeinfo>
// external library headers
#include <boost/exception/diagnostic_information.hpp>
// other Linden headers
#include "llerror.h"
#include "llerrorcontrol.h"

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
