/**
 * @file   llurldispatcherlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  Implementation for llurldispatcherlistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llurldispatcherlistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llslurl.h"
#include "llurldispatcher.h"

LLURLDispatcherListener::LLURLDispatcherListener(/* LLURLDispatcher* instance */):
    LLEventAPI("LLURLDispatcher", "Internal URL handling") /* ,
    mDispatcher(instance) */
{
    add("dispatch",
        "At startup time or on clicks in internal web browsers,\n"
        "teleport, open map, or run requested command.\n"
        "[\"url\"] string url to dispatch\n"
        "[\"trusted\"] boolean indicating trusted browser [default true]",
        &LLURLDispatcherListener::dispatch);
    add("dispatchRightClick", "Dispatch [\"url\"] as if from a right-click on a hot link.",
        &LLURLDispatcherListener::dispatchRightClick);
    add("dispatchFromTextEditor", "Dispatch [\"url\"] as if from an edit field.",
        &LLURLDispatcherListener::dispatchFromTextEditor);
}

void LLURLDispatcherListener::dispatch(const LLSD& params) const
{
    // For most purposes, we expect callers to want to be trusted.
    bool trusted_browser = true;
    if (params.has("trusted"))
    {
        // But for testing, allow a caller to specify untrusted.
        trusted_browser = params["trusted"].asBoolean();
    }
    LLURLDispatcher::dispatch(params["url"], NULL, trusted_browser);
}

void LLURLDispatcherListener::dispatchRightClick(const LLSD& params) const
{
    LLURLDispatcher::dispatchRightClick(params["url"]);
}

void LLURLDispatcherListener::dispatchFromTextEditor(const LLSD& params) const
{
    LLURLDispatcher::dispatchFromTextEditor(params["url"]);
}
