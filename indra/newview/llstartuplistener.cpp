/**
 * @file   llstartuplistener.cpp
 * @author Nat Goodspeed
 * @date   2009-12-08
 * @brief  Implementation for llstartuplistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
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
#include "stringize.h"

LLStartupListener::LLStartupListener(/* LLStartUp* instance */):
    LLEventAPI("LLStartUp", "Access e.g. LLStartup::postStartupState()") /* ,
    mStartup(instance) */
{
    add("postStartupState", "Refresh \"StartupState\" listeners with current startup state",
        &LLStartupListener::postStartupState);
    add("getStateTable", "Reply with array of EStartupState string names",
        &LLStartupListener::getStateTable);
}

void LLStartupListener::postStartupState(const LLSD&) const
{
    LLStartUp::postStartupState();
}

void LLStartupListener::getStateTable(const LLSD& event) const
{
    Response response(LLSD(), event);

    // This relies on our knowledge that STATE_STARTED is the very last
    // EStartupState value. If that ever stops being true, we're going to lie
    // without realizing it. I can think of no reliable way to test whether
    // the enum has been extended *beyond* STATE_STARTED. We could, of course,
    // test whether stuff has been inserted before it, by testing its
    // numerical value against the constant value as of the last time we
    // looked; but that's pointless, as values inserted before STATE_STARTED
    // will continue to work fine. The bad case is if new symbols get added
    // *after* it.
    LLSD table;
    // note <= comparison: we want to *include* STATE_STARTED.
    for (LLSD::Integer istate{0}; istate <= LLSD::Integer(STATE_STARTED); ++istate)
    {
        table.append(LLStartUp::startupStateToString(EStartupState(istate)));
    }
    response["table"] = table;
}
