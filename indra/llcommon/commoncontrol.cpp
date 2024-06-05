/**
 * @file   commoncontrol.cpp
 * @author Nat Goodspeed
 * @date   2022-06-08
 * @brief  Implementation for commoncontrol.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "commoncontrol.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llevents.h"
#include "llsdutil.h"

LLSD LL::CommonControl::access(const LLSD& params)
{
    // We can't actually introduce a link-time dependency on llxml, or on any
    // global LLControlGroup (*koff* gSavedSettings *koff*) but we can issue a
    // runtime query. If we're running as part of a viewer with
    // LLViewerControlListener, we can use that to interact with any
    // instantiated LLControGroup.
    LLSD response;
    {
        LLEventStream reply("reply");
        LLTempBoundListener connection = reply.listen("listener",
                     [&response] (const LLSD& event)
                     {
                         response = event;
                         return false;
                     });
        LLSD rparams{ params };
        rparams["reply"] = reply.getName();
        LLEventPumps::instance().obtain("LLViewerControl").post(rparams);
    }
    // LLViewerControlListener responds immediately. If it's listening at all,
    // it will already have set response.
    if (! response.isDefined())
    {
        LLTHROW(NoListener("No LLViewerControl listener instantiated"));
    }
    LLSD error{ response["error"] };
    if (error.isDefined())
    {
        LLTHROW(ParamError(error));
    }
    response.erase("error");
    response.erase("reqid");
    return response;
}

/// set control group.key to defined default value
LLSD LL::CommonControl::set_default(const std::string& group, const std::string& key)
{
    return access(llsd::map("op", "set",
                            "group", group, "key", key))["value"];
}

/// set control group.key to specified value
LLSD LL::CommonControl::set(const std::string& group, const std::string& key, const LLSD& value)
{
    return access(llsd::map("op", "set",
                            "group", group, "key", key, "value", value))["value"];
}

/// toggle boolean control group.key
LLSD LL::CommonControl::toggle(const std::string& group, const std::string& key)
{
    return access(llsd::map("op", "toggle",
                            "group", group, "key", key))["value"];
}

/// get the definition for control group.key, (! isDefined()) if bad
/// ["name"], ["type"], ["value"], ["comment"]
LLSD LL::CommonControl::get_def(const std::string& group, const std::string& key)
{
    return access(llsd::map("op", "get",
                            "group", group, "key", key));
}

/// get the value of control group.key
LLSD LL::CommonControl::get(const std::string& group, const std::string& key)
{
    return access(llsd::map("op", "get",
                            "group", group, "key", key))["value"];
}

/// get defined groups
std::vector<std::string> LL::CommonControl::get_groups()
{
    auto groups{ access(llsd::map("op", "groups"))["groups"] };
    return { groups.beginArray(), groups.endArray() };
}

/// get definitions for all variables in group
LLSD LL::CommonControl::get_vars(const std::string& group)
{
    return access(llsd::map("op", "vars", "group", group))["vars"];
}
