/**
 * @file   llcleanup.h
 * @author Nat Goodspeed
 * @date   2015-05-20
 * @brief  Mechanism for cleaning up subsystem resources
 * 
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Copyright (c) 2015, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCLEANUP_H)
#define LL_LLCLEANUP_H

#include <boost/current_function.hpp>

// Instead of directly calling SomeClass::cleanupClass(), use
// SUBSYSTEM_CLEANUP(SomeClass);
// This logs the call as well as performing it. That gives us a baseline
// subsystem shutdown order against which to compare subsequent dynamic
// shutdown schemes.
#define SUBSYSTEM_CLEANUP(CLASSNAME)                                    \
    do {                                                                \
        log_subsystem_cleanup(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, #CLASSNAME); \
        CLASSNAME::cleanupClass();                                      \
    } while (0)
// Use ancient do { ... } while (0) macro trick to permit a block of
// statements with the same syntax as a single statement.

void log_subsystem_cleanup(const char* file, int line, const char* function,
                           const char* classname);

#endif /* ! defined(LL_LLCLEANUP_H) */
