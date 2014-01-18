/**
 * @file   llappviewerlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-06-23
 * @brief  Implementation for llappviewerlistener.
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
