/**
 * @file llfloatermediamonitor.cpp
 * @author Callum Prentice
 * @brief Debug floater containing a list of active media sources
 *        and data for each such as URL, priority, location etc.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lleventtimer.h"
#include "llformat.h"
#include "llscrolllistcolumn.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lluictrlfactory.h"
#include "llurlaction.h"
#include "lluuid.h"
#include "llviewermedia.h"

#include "llfloatermediamonitor.h"

namespace
{
    // "ui"/"prim"/"parcel" (LLViewerMedia::getEmbeddedBrowserDebugInfo()'s "kind") -> display label.
    std::string mediaKindToDisplayLabel(const std::string& kind)
    {
        if (kind == "ui")
        {
            return "UI";
        }
        if (kind == "parcel")
        {
            return "Parcel";
        }
        if (kind == "prim")
        {
            return "Prim";
        }
        return kind;
    }

    constexpr F32 REFRESH_PERIOD_SECONDS = 2.f;
}

LLFloaterMediaMonitor::LLFloaterMediaMonitor(const LLSD& key)
    :   LLFloater("floater_media_monitor")
    // :   LLFloater("floater_inventory_thumbnails_helper")
{
}

LLFloaterMediaMonitor::~LLFloaterMediaMonitor()
{
    delete mRefreshTimer;
}

void LLFloaterMediaMonitor::onOpen(const LLSD& key)
{
    updateDisplayList();

    if (!mRefreshTimer)
    {
        mRefreshTimer = LLEventTimer::run_every(REFRESH_PERIOD_SECONDS, boost::bind(&LLFloaterMediaMonitor::updateDisplayList, this));
    }
}

void LLFloaterMediaMonitor::onClose(bool app_quitting)
{
    delete mRefreshTimer;
    mRefreshTimer = nullptr;
}

bool LLFloaterMediaMonitor::postBuild()
{
    mActiveMediaList = getChild<LLScrollListCtrl>("active_media_list");

    // Refresh list of media manually
    getChild<LLUICtrl>("refresh_btn")->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::onRefreshBtn, this));
    
    // Close the floater
    getChild<LLUICtrl>("close_btn")->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::onCloseBtn, this));

    // Double-clicking a row opens its URL, or teleports to its Location, in the browser/viewer
    mActiveMediaList->setDoubleClickCallback(boost::bind(&LLFloaterMediaMonitor::onDoubleClickItem, this));

    mBaseTitle = getTitle();

    return true;
}

void LLFloaterMediaMonitor::updateDisplayList()
{
    mActiveMediaList->deleteAllItems();

    LLSD media_list = LLViewerMedia::getInstance()->getEmbeddedBrowserDebugInfo();

    for (LLSD::array_const_iterator it = media_list.beginArray(); it != media_list.endArray(); ++it)
    {
        const LLSD& media = *it;

        std::string slurl = media["slurl"].asString();

        LLSD row;
        row["columns"][0]["column"] = "slot";
        row["columns"][0]["value"] = media["slot"];
        row["columns"][1]["column"] = "priority";
        row["columns"][1]["value"] = media["priority_label"];
        row["columns"][2]["column"] = "media_type";
        row["columns"][2]["value"] = mediaKindToDisplayLabel(media["kind"].asString());
        row["columns"][3]["column"] = "url";
        row["columns"][3]["value"] = media["url"];
        row["columns"][4]["column"] = "location";
        row["columns"][4]["value"] = slurl.empty() ? "-" : slurl;

        mActiveMediaList->addElement(row);
    }

    S32 count = static_cast<S32>(media_list.size());
    S32 max_instances = LLViewerMedia::getInstance()->getMaxInstances();
    setTitle(llformat("%s - %d/%d instances", mBaseTitle.c_str(), count, max_instances));
}

void LLFloaterMediaMonitor::onDoubleClickItem()
{
    LLScrollListItem* item = mActiveMediaList->getFirstSelected();
    if (!item)
    {
        return;
    }

    S32 mouse_x = 0;
    S32 mouse_y = 0;
    LLUI::getInstance()->getMousePositionLocal(mActiveMediaList, &mouse_x, &mouse_y);
    S32 column_index = mActiveMediaList->getColumnIndexFromOffset(mouse_x);

    LLScrollListColumn* column = mActiveMediaList->getColumn(column_index);
    LLScrollListCell* cell = item->getColumn(column_index);
    if (!column || !cell)
    {
        return;
    }

    std::string cell_text = cell->getValue().asString();

    if (column->mName == "url")
    {
        // Always the desktop browser here, deliberately bypassing the internal/external
        // routing LLUrlAction::openURL() would otherwise apply (PreferredBrowserBehavior,
        // per-domain rules) -- for a QA tool, consistent one-URL-one-window behavior
        // matters more than following the same routing regular in-world link clicks get.
        LLUrlAction::openURLExternal(cell_text);
    }
    else if (column->mName == "location" && cell_text != "-")
    {
        LLUrlAction::teleportToLocation(cell_text);
    }
}

void LLFloaterMediaMonitor::onRefreshBtn()
{
    updateDisplayList();
}

void LLFloaterMediaMonitor::onCloseBtn()
{
    closeFloater();
}
