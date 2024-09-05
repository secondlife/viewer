/**
 * @file   lockstatic.cpp
 * @author Nat Goodspeed
 * @date   2024-05-23
 * @brief  Implementation for lockstatic.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lockstatic.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llerror.h"
#include "stringize.h"

void llthread::LockStaticBase::throwDead(const char* mangled)
{
    LLTHROW(Dead(stringize(LLError::Log::demangle(mangled), " called after cleanup()")));
}
