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
#include "llcallbacklist.h"
#include "llviewerwindow.h"

LLViewerWindowListener::LLViewerWindowListener(LLViewerWindow* llviewerwindow):
    LLEventAPI("LLViewerWindow",
               "LLViewerWindow listener to (e.g.) save a screenshot"),
    mViewerWindow(llviewerwindow)
{
    // add() every method we want to be able to invoke via this event API.
    add("saveSnapshot",
        "Save screenshot: [\"filename\"] (extension may be specified: bmp, jpeg, png)\n"
        "[\"width\"], [\"height\"], [\"showui\"], [\"showhud\"], [\"rebuild\"], [\"type\"]\n"
        "type: \"COLOR\", \"DEPTH\"\n",
        &LLViewerWindowListener::saveSnapshot,
        llsd::map("filename", LLSD::String(), "reply", LLSD()));
    add("requestReshape",
        "Resize the window: [\"w\"], [\"h\"]",
        &LLViewerWindowListener::requestReshape);
}

void LLViewerWindowListener::saveSnapshot(const LLSD& event) const
{
    typedef std::map<LLSD::String, LLSnapshotModel::ESnapshotLayerType> TypeMap;
    TypeMap types;
#define tp(name) types[#name] = LLSnapshotModel::SNAPSHOT_TYPE_##name
    tp(COLOR);
    tp(DEPTH);
#undef tp

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
    bool showhud = true;
    if (event.has("showhud"))
        showhud = event["showhud"].asBoolean();
    bool rebuild(event["rebuild"]); // defaults to false
    LLSnapshotModel::ESnapshotLayerType type(LLSnapshotModel::SNAPSHOT_TYPE_COLOR);
    if (event.has("type"))
    {
        TypeMap::const_iterator found = types.find(event["type"]);
        if (found == types.end())
        {
            sendReply(llsd::map("error", stringize("Unrecognized type ", std::quoted(event["type"].asString()), " [\"COLOR\"] or [\"DEPTH\"] is expected.")), event);
            return;
        }
        type = found->second;
    }

    std::string filename(event["filename"]);
    if (filename.empty())
    {
        sendReply(llsd::map("error", stringize("File path is empty.")), event);
        return;
    }

    LLSnapshotModel::ESnapshotFormat format(LLSnapshotModel::SNAPSHOT_FORMAT_BMP);
    std::string ext = gDirUtilp->getExtension(filename);
    if (ext.empty())
    {
        filename.append(".bmp");
    }
    else if (ext == "png")
    {
        format = LLSnapshotModel::SNAPSHOT_FORMAT_PNG;
    }
    else if (ext == "jpeg" || ext == "jpg")
    {
        format = LLSnapshotModel::SNAPSHOT_FORMAT_JPEG;
    }
    else if (ext != "bmp")
    {
        sendReply(llsd::map("error", stringize("Unrecognized format. [\"png\"], [\"jpeg\"] or [\"bmp\"] is expected.")), event);
        return;
    }
    // take snapshot on the main coro
    doOnIdleOneTime([this, event, filename, width, height, showui, showhud, rebuild, type, format]()
                    { sendReply(llsd::map("result", mViewerWindow->saveSnapshot(filename, width, height, showui, showhud, rebuild, type, format)), event); });

}

void LLViewerWindowListener::requestReshape(LLSD const & event_data) const
{
    if(event_data.has("w") && event_data.has("h"))
    {
        mViewerWindow->reshape(event_data["w"].asInteger(), event_data["h"].asInteger());
    }
}
