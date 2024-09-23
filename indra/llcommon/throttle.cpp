/**
 * @file   throttle.cpp
 * @author Nat Goodspeed
 * @date   2024-08-12
 * @brief  Implementation for ThrottleBase.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "throttle.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "lltimer.h"

bool ThrottleBase::too_fast()
{
    F64 now = LLTimer::getElapsedSeconds();
    if (now < mNext)
    {
        return true;
    }
    else
    {
        mNext = now + mInterval;
        return false;
    }
}
