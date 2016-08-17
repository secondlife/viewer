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

void crash_on_unhandled_exception_(const char* file, int line, const char* pretty_function)
{
    // LL_ERRS() terminates, but also propagates message into crash dump.
    LL_ERRS() << file << "(" << line << "): Unhandled exception caught in " << pretty_function
              << ":\n" << boost::current_exception_diagnostic_information() << LL_ENDL;
}

void log_unhandled_exception_(const char* file, int line, const char* pretty_function,
                             const LLContinueError& e)
{
    // Use LL_WARNS() because we seriously do not expect this to happen
    // routinely, but we DO expect to return from this function. Deriving your
    // exception from LLContinueError implies that such an exception should
    // NOT be fatal to the viewer, only to its current task.
    LL_WARNS() << file << "(" << line << "): Unhandled " << typeid(e).name()
               << " exception caught in " << pretty_function
               << ":\n" << boost::current_exception_diagnostic_information() << LL_ENDL;
}
