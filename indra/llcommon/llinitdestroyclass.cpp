/**
 * @file   llinitdestroyclass.cpp
 * @author Nat Goodspeed
 * @date   2016-08-30
 * @brief  Implementation for llinitdestroyclass.
 * 
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llinitdestroyclass.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llerror.h"

void LLCallbackRegistry::fireCallbacks() const
{
    for (FuncList::const_iterator fi = mCallbacks.begin(), fe = mCallbacks.end();
         fi != fe; ++fi)
    {
        LL_INFOS("LLInitDestroyClass") << "calling " << fi->first << "()" << LL_ENDL;
        fi->second();
    }
}
