/**
 * @file   llfloaterreglistener.cpp
 * @author Nat Goodspeed
 * @date   2009-08-12
 * @brief  Implementation for llfloaterreglistener.
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
#include "linden_common.h"
// associated header
#include "llfloaterreglistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llfloaterreg.h"
#include "llfloater.h"
#include "llbutton.h"

LLFloaterRegListener::LLFloaterRegListener():
    LLEventAPI("LLFloaterReg",
               "LLFloaterReg listener to (e.g.) show/hide LLFloater instances")
{
    add("getBuildMap",
        "Return on [\"reply\"] data about all registered LLFloaterReg floater names",
        &LLFloaterRegListener::getBuildMap,
        LLSD().with("reply", LLSD()));
    LLSD requiredName;
    requiredName["name"] = LLSD();
    add("showInstance",
        "Ask to display the floater specified in [\"name\"]",
        &LLFloaterRegListener::showInstance,
        requiredName);
    add("hideInstance",
        "Ask to hide the floater specified in [\"name\"]",
        &LLFloaterRegListener::hideInstance,
        requiredName);
    add("toggleInstance",
        "Ask to toggle the state of the floater specified in [\"name\"]",
        &LLFloaterRegListener::toggleInstance,
        requiredName);
    add("instanceVisible",
        "Return on [\"reply\"] an event whose [\"visible\"] indicates the visibility "
        "of the floater specified in [\"name\"]",
        &LLFloaterRegListener::instanceVisible,
        requiredName);
    LLSD requiredNameButton;
    requiredNameButton["name"] = LLSD();
    requiredNameButton["button"] = LLSD();
    add("clickButton",
        "Simulate clicking the named [\"button\"] in the visible floater named in [\"name\"]",
        &LLFloaterRegListener::clickButton,
        requiredNameButton);
}

void LLFloaterRegListener::getBuildMap(const LLSD& event) const
{
    LLSD reply;
    // Build an LLSD map that mirrors sBuildMap. Since we have no good way to
    // represent a C++ callable in LLSD, the only part of BuildData we can
    // store is the filename. For each LLSD map entry, it would be more
    // extensible to store a nested LLSD map containing a single key "file" --
    // but we don't bother, simply storing the string filename instead.
    for (LLFloaterReg::build_map_t::const_iterator mi(LLFloaterReg::sBuildMap.begin()),
                                                   mend(LLFloaterReg::sBuildMap.end());
         mi != mend; ++mi)
    {
        reply[mi->first] = mi->second.mFile;
    }
    // Send the reply to the LLEventPump named in event["reply"].
    sendReply(reply, event);
}

void LLFloaterRegListener::showInstance(const LLSD& event) const
{
    LLFloaterReg::showInstance(event["name"], event["key"], event["focus"]);
}

void LLFloaterRegListener::hideInstance(const LLSD& event) const
{
    LLFloaterReg::hideInstance(event["name"], event["key"]);
}

void LLFloaterRegListener::toggleInstance(const LLSD& event) const
{
    LLFloaterReg::toggleInstance(event["name"], event["key"]);
}

void LLFloaterRegListener::instanceVisible(const LLSD& event) const
{
    sendReply(LLSDMap("visible", LLFloaterReg::instanceVisible(event["name"], event["key"])),
              event);
}

void LLFloaterRegListener::clickButton(const LLSD& event) const
{
    // If the caller requests a reply, build the reply.
    LLReqID reqID(event);
    LLSD reply(reqID.makeResponse());

    LLFloater* floater = LLFloaterReg::findInstance(event["name"], event["key"]);
    if (! LLFloater::isShown(floater))
    {
        reply["type"]  = "LLFloater";
        reply["name"]  = event["name"];
        reply["key"]   = event["key"];
        reply["error"] = floater? "!isShown()" : "NULL";
    }
    else
    {
        // Here 'floater' points to an LLFloater instance with the specified
        // name and key which isShown().
        LLButton* button = floater->findChild<LLButton>(event["button"]);
        if (! LLButton::isAvailable(button))
        {
            reply["type"]  = "LLButton";
            reply["name"]  = event["button"];
            reply["error"] = button? "!isAvailable()" : "NULL";
        }
        else
        {
            // Here 'button' points to an isAvailable() LLButton child of
            // 'floater' with the specified button name. Pretend to click it.
            button->onCommit();
            // Leave reply["error"] isUndefined(): no error, i.e. success.
        }
    }

    // Send a reply only if caller asked for a reply.
    LLSD replyPump(event["reply"]);
    if (replyPump.isString())       // isUndefined() if absent
    {
        LLEventPumps::instance().obtain(replyPump).post(reply);
    }
}
