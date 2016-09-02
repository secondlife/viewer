/**
 * @file   llcleanup.cpp
 * @author Nat Goodspeed
 * @date   2016-08-30
 * @brief  Implementation for llcleanup.
 * 
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llcleanup.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llerror.h"
#include "llerrorcontrol.h"

void log_subsystem_cleanup(const char* file, int line, const char* function,
                           const char* classname)
{
    LL_INFOS("Cleanup") << LLError::abbreviateFile(file) << "(" << line << "): "
                        << "calling " << classname << "::cleanupClass() in "
                        << function << LL_ENDL;
}
