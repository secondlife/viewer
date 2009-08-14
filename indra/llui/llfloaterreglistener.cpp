/**
 * @file   llfloaterreglistener.cpp
 * @author Nat Goodspeed
 * @date   2009-08-12
 * @brief  Implementation for llfloaterreglistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
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

LLFloaterRegListener::LLFloaterRegListener(const std::string& pumpName):
    LLDispatchListener(pumpName, "op")
{
    add("getBuildMap",  &LLFloaterRegListener::getBuildMap,  LLSD().insert("reply", LLSD()));
    LLSD requiredName;
    requiredName["name"] = LLSD();
    add("showInstance", &LLFloaterRegListener::showInstance, requiredName);
    add("hideInstance", &LLFloaterRegListener::hideInstance, requiredName);
}

void LLFloaterRegListener::getBuildMap(const LLSD& event) const
{
    // Honor the "reqid" convention by echoing event["reqid"] in our reply packet.
    LLReqID reqID(event);
    LLSD reply(reqID.makeResponse());
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
    LLEventPumps::instance().obtain(event["reply"]).post(reply);
}

void LLFloaterRegListener::showInstance(const LLSD& event) const
{
    LLFloaterReg::showInstance(event["name"], event["key"], event["focus"]);
}

void LLFloaterRegListener::hideInstance(const LLSD& event) const
{
    LLFloaterReg::hideInstance(event["name"], event["key"]);
}
