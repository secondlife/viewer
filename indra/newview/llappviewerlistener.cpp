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

LLAppViewerListener::LLAppViewerListener(const std::string& pumpname,
                                         const LLAppViewerGetter& getter):
    LLDispatchListener(pumpname, "op"),
    mAppViewerGetter(getter)
{
    // add() every method we want to be able to invoke via this event API.
    add("requestQuit", &LLAppViewerListener::requestQuit);
    add("forceQuit", &LLAppViewerListener::forceQuit);
}

void LLAppViewerListener::requestQuit(const LLSD& event)
{
    mAppViewerGetter()->requestQuit();
}

void LLAppViewerListener::forceQuit(const LLSD& event)
{
    mAppViewerGetter()->forceQuit();
}
