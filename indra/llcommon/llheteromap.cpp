/**
 * @file   llheteromap.cpp
 * @author Nat Goodspeed
 * @date   2016-10-12
 * @brief  Implementation for llheteromap.
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llheteromap.h"
// STL headers
// std headers
// external library headers
// other Linden headers

LLHeteroMap::~LLHeteroMap()
{
    // For each entry in our map, we must call its deleter, which is the only
    // record we have of its original type.
    for (TypeMap::value_type& pair : mMap)
    {
        // pair.second is the std::pair; pair.second.first is the void*;
        // pair.second.second points to the deleter function
        (pair.second.second)(pair.second.first);
        pair.second.first = NULL;
    }
}
