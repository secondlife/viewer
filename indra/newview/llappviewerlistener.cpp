/**
 * @file   llappviewerlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-06-23
 * @brief  Implementation for llappviewerlistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llappviewerlistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llappviewer.h"

LLAppViewerListener::LLAppViewerListener(const LLAppViewerGetter& getter):
    LLEventAPI("LLAppViewer",
               "LLAppViewer listener to (e.g.) request shutdown"),
    mAppViewerGetter(getter)
{
    // add() every method we want to be able to invoke via this event API.
    add("requestQuit",
        "Ask to quit nicely",
        &LLAppViewerListener::requestQuit);
    add("forceQuit",
        "Quit abruptly",
        &LLAppViewerListener::forceQuit);
}

void LLAppViewerListener::requestQuit(const LLSD& event)
{
    mAppViewerGetter()->requestQuit();
}

void LLAppViewerListener::forceQuit(const LLSD& event)
{
    mAppViewerGetter()->forceQuit();
}
