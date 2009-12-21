/**
 * @file   llcommanddispatcherlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  Implementation for llcommanddispatcherlistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llcommanddispatcherlistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llcommandhandler.h"

LLCommandDispatcherListener::LLCommandDispatcherListener(/* LLCommandDispatcher* instance */):
    LLEventAPI("LLCommandDispatcher", "Access to LLCommandHandler commands") /* ,
    mDispatcher(instance) */
{
    add("dispatch",
        "Execute a command registered as an LLCommandHandler,\n"
        "passing any required parameters:\n"
        "[\"cmd\"] string command name\n"
        "[\"params\"] array of parameters, as if from components of URL path\n"
        "[\"query\"] map of parameters, as if from ?key1=val&key2=val\n"
        "[\"trusted\"] boolean indicating trusted browser [default true]",
        &LLCommandDispatcherListener::dispatch);
    add("enumerate",
        "Post to [\"reply\"] a map of registered LLCommandHandler instances, containing\n"
        "name key and (e.g.) untrusted flag",
        &LLCommandDispatcherListener::enumerate,
        LLSD().with("reply", LLSD()));
}

void LLCommandDispatcherListener::dispatch(const LLSD& params) const
{
    // For most purposes, we expect callers to want to be trusted.
    bool trusted_browser = true;
    if (params.has("trusted"))
    {
        // But for testing, allow a caller to specify untrusted.
        trusted_browser = params["trusted"].asBoolean();
    }
    LLCommandDispatcher::dispatch(params["cmd"], params["params"], params["query"], NULL,
                                  trusted_browser);
}

void LLCommandDispatcherListener::enumerate(const LLSD& params) const
{
    LLReqID reqID(params);
    LLSD response(LLCommandDispatcher::enumerate());
    reqID.stamp(response);
    LLEventPumps::instance().obtain(params["reply"]).post(response);
}
