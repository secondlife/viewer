/**
 * @file   llstartuplistener.cpp
 * @author Nat Goodspeed
 * @date   2009-12-08
 * @brief  Implementation for llstartuplistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llstartuplistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llstartup.h"


LLStartupListener::LLStartupListener(/* LLStartUp* instance */):
    LLEventAPI("LLStartUp", "Access e.g. LLStartup::postStartupState()") /* ,
    mStartup(instance) */
{
    add("postStartupState", "Refresh \"StartupState\" listeners with current startup state",
        &LLStartupListener::postStartupState);
}

void LLStartupListener::postStartupState(const LLSD&) const
{
    LLStartUp::postStartupState();
}
