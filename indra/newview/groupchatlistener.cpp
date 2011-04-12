/**
 * @file   groupchatlistener.cpp
 * @author Nat Goodspeed
 * @date   2011-04-11
 * @brief  Implementation for groupchatlistener.
 * 
 * $LicenseInfo:firstyear=2011&license=internal$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "groupchatlistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llgroupactions.h"

GroupChatListener::GroupChatListener():
    LLEventAPI("GroupChat",
               "API to enter, leave, send and intercept group chat messages")
{
    add("startIM",
        "Enter a group chat in group with UUID [\"id\"]\n"
        "Assumes the logged-in agent is already a member of this group.",
        &LLGroupActions::startIM,
        LLSDArray("id"));
    add("endIM",
        "Leave a group chat in group with UUID [\"id\"]\n"
        "Assumes a prior successful startIM request.",
        &LLGroupActions::endIM,
        LLSDArray("id"));
}
