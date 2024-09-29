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
#include "workqueue.h"

LLAppViewerListener::LLAppViewerListener(const LLAppViewerGetter& getter):
    LLEventAPI("LLAppViewer",
               "LLAppViewer listener to (e.g.) request shutdown"),
    mAppViewerGetter(getter)
{
    // add() every method we want to be able to invoke via this event API.
    add("userQuit",
        "Ask to quit with user confirmation prompt",
        &LLAppViewerListener::userQuit);
    add("requestQuit",
        "Ask to quit nicely",
        &LLAppViewerListener::requestQuit);
    add("forceQuit",
        "Quit abruptly",
        &LLAppViewerListener::forceQuit);
}

void LLAppViewerListener::userQuit(const LLSD& event)
{
    LL_INFOS() << "Listener requested user quit" << LL_ENDL;
    // Trying to engage this from (e.g.) a Lua-hosting C++ coroutine runs
    // afoul of an assert in the logging machinery that LLMutex must be locked
    // only from the main coroutine.
    LL::WorkQueue::getInstance("mainloop")->post(
        [appviewer=mAppViewerGetter()]{ appviewer->userQuit(); });
}

void LLAppViewerListener::requestQuit(const LLSD& event)
{
    LL_INFOS() << "Listener requested quit" << LL_ENDL;
    LL::WorkQueue::getInstance("mainloop")->post(
        [appviewer=mAppViewerGetter()]{ appviewer->requestQuit(); });
}

void LLAppViewerListener::forceQuit(const LLSD& event)
{
    LL_INFOS() << "Listener requested force quit" << LL_ENDL;
    LL::WorkQueue::getInstance("mainloop")->post(
        [appviewer=mAppViewerGetter()]{ appviewer->forceQuit(); });
}
