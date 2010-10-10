/**
 * @file   llviewerwindowlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-06-30
 * @brief  Implementation for llviewerwindowlistener.
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
#include "llviewerwindowlistener.h"
// STL headers
#include <map>
// std headers
// external library headers
// other Linden headers
#include "llviewerwindow.h"

LLViewerWindowListener::LLViewerWindowListener(LLViewerWindow* llviewerwindow):
    LLEventAPI("LLViewerWindow",
               "LLViewerWindow listener to (e.g.) save a screenshot"),
    mViewerWindow(llviewerwindow)
{
    // add() every method we want to be able to invoke via this event API.
    LLSD saveSnapshotArgs;
    saveSnapshotArgs["filename"] = LLSD::String();
    saveSnapshotArgs["reply"] = LLSD::String();
    // The following are optional, so don't build them into required prototype.
//  saveSnapshotArgs["width"] = LLSD::Integer();
//  saveSnapshotArgs["height"] = LLSD::Integer();
//  saveSnapshotArgs["showui"] = LLSD::Boolean();
//  saveSnapshotArgs["rebuild"] = LLSD::Boolean();
//  saveSnapshotArgs["type"] = LLSD::String();
    add("saveSnapshot",
        "Save screenshot: [\"filename\"], [\"width\"], [\"height\"], [\"showui\"], [\"rebuild\"], [\"type\"]\n"
        "type: \"COLOR\", \"DEPTH\"\n"
        "Post on [\"reply\"] an event containing [\"ok\"]",
        &LLViewerWindowListener::saveSnapshot,
        saveSnapshotArgs);
    add("requestReshape",
        "Resize the window: [\"w\"], [\"h\"]",
        &LLViewerWindowListener::requestReshape);
}

void LLViewerWindowListener::saveSnapshot(const LLSD& event) const
{
    LLReqID reqid(event);
    typedef std::map<LLSD::String, LLViewerWindow::ESnapshotType> TypeMap;
    TypeMap types;
#define tp(name) types[#name] = LLViewerWindow::SNAPSHOT_TYPE_##name
    tp(COLOR);
    tp(DEPTH);
#undef  tp
    // Our add() call should ensure that the incoming LLSD does in fact
    // contain our required arguments. Deal with the optional ones.
    S32 width (mViewerWindow->getWindowWidthRaw());
    S32 height(mViewerWindow->getWindowHeightRaw());
    if (event.has("width"))
        width = event["width"].asInteger();
    if (event.has("height"))
        height = event["height"].asInteger();
    // showui defaults to true, requiring special treatment
    bool showui = true;
    if (event.has("showui"))
        showui = event["showui"].asBoolean();
    bool rebuild(event["rebuild"]); // defaults to false
    LLViewerWindow::ESnapshotType type(LLViewerWindow::SNAPSHOT_TYPE_COLOR);
    if (event.has("type"))
    {
        TypeMap::const_iterator found = types.find(event["type"]);
        if (found == types.end())
        {
            LL_ERRS("LLViewerWindowListener") << "LLViewerWindowListener::saveSnapshot(): "
                                              << "unrecognized type " << event["type"] << LL_ENDL;
	    return;
        }
        type = found->second;
    }
    bool ok = mViewerWindow->saveSnapshot(event["filename"], width, height, showui, rebuild, type);
    LLSD response(reqid.makeResponse());
    response["ok"] = ok;
    LLEventPumps::instance().obtain(event["reply"]).post(response);
}

void LLViewerWindowListener::requestReshape(LLSD const & event_data) const
{
	if(event_data.has("w") && event_data.has("h"))
	{
		mViewerWindow->reshape(event_data["w"].asInteger(), event_data["h"].asInteger());
	}
}
