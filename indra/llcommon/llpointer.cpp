/**
 * @file   llpointer.cpp
 * @author Nat Goodspeed
 * @date   2024-09-26
 * @brief  Implementation for llpointer.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llpointer.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llerror.h"

void LLPointerBase::wild_dtor(std::string_view msg)
{
//  LL_WARNS() << msg << LL_ENDL;
    llassert_msg(false, msg);
}
