/**
 * @file llpaneldirgroups.cpp
 * @brief Groups panel in the legacy Search directory.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#include "llpaneldirgroups.h"

#include "llagent.h"
#include "llgroupactions.h"
#include "llqueryflags.h"
#include "llscrolllistctrl.h"
#include "llsearcheditor.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"

static LLPanelInjector<LLPanelDirGroups> t_panel_dir_groups("panel_dir_groups");

LLPanelDirGroups::LLPanelDirGroups()
    : LLPanelDirBrowser()
{
    mMinSearchChars = 3;
}


bool LLPanelDirGroups::postBuild()
{
    LLPanelDirBrowser::postBuild();

    //getChild<LLLineEditor>("name")->setKeystrokeCallback(boost::bind(&LLPanelDirBrowser::onKeystrokeName, _1, _2), NULL);

    childSetAction("Search", &LLPanelDirBrowser::onClickSearchCore, this);
    setDefaultBtn( "Search" );

    if (gAgent.isTeen())
    {
        childSetEnabled("incmature", false);
        gSavedSettings.setBOOL("ShowMatureGroups", false);
    }

    LLScrollListCtrl* results = getChild<LLScrollListCtrl>("results");
    results->setRightMouseDownCallback(boost::bind(&LLPanelDirGroups::onResultsRightClick, this, _1, _2, _3));

    LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
    registrar.add("People.Groups.Action", boost::bind(&LLPanelDirGroups::onContextMenuItemClick, this, _2));
    enable_registrar.add("People.Groups.Enable", boost::bind(&LLPanelDirGroups::onContextMenuItemEnable, this, _2));

    LLToggleableMenu* menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_people_groups.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    if (menu)
    {
        mPopupMenuHandle = menu->getHandle();
    }

    return true;
}

LLPanelDirGroups::~LLPanelDirGroups()
{
}

void LLPanelDirGroups::onResultsRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
    LLScrollListCtrl* results = getChild<LLScrollListCtrl>("results");
    LLScrollListItem* item = results->hitItem(x, y);
    LLToggleableMenu* menu = mPopupMenuHandle.get();
    if (item && menu)
    {
        results->selectItemAt(x, y, MASK_NONE);
        mSelectedGroupID = item->getUUID();
        menu->buildDrawLabels();
        menu->updateParent(LLMenuGL::sMenuContainer);
        LLMenuGL::showPopup(ctrl, menu, x, y);
    }
}

bool LLPanelDirGroups::onContextMenuItemClick(const LLSD& userdata)
{
    std::string action = userdata.asString();

    if (action == "view_info")
    {
        LLGroupActions::show(mSelectedGroupID);
    }
    else if (action == "chat")
    {
        LLGroupActions::startIM(mSelectedGroupID);
    }
    else if (action == "call")
    {
        LLGroupActions::startCall(mSelectedGroupID);
    }
    else if (action == "activate")
    {
        LLGroupActions::activate(mSelectedGroupID);
    }
    else if (action == "leave")
    {
        LLGroupActions::leave(mSelectedGroupID);
    }

    return true;
}

bool LLPanelDirGroups::onContextMenuItemEnable(const LLSD& userdata)
{
    // Only "View Info" is available for groups the agent is not a member of.
    if (userdata.asString() == "view_info")
    {
        return mSelectedGroupID.notNull();
    }

    return LLGroupActions::isInGroup(mSelectedGroupID);
}

// virtual
void LLPanelDirGroups::performQuery()
{
    if (childGetValue("name").asString().length() < mMinSearchChars)
    {
        return;
    }

    setupNewSearch();

    // groups
    U32 scope = DFQ_GROUPS;

    // Check group mature filter.
    if ( gSavedSettings.getBOOL("ShowMatureGroups") && !gAgent.isTeen() )
    {
        // Supposed behavior:
        // if nothing is set will search for <= mature
        // if DFQ_INC_PG is set, will look for <= PG
        // if DFQ_INC_MATURE is set, will look for == mature
        // if DFQ_INC_ADULT is set, will look for >= adult
        // Not compatible with legacy DFQ_FILTER_MATURE.
        // But there appears to be a server bug, so we only use
        // this to show all and use legacy setting for 'pg only'
        scope |= DFQ_INC_PG;
        scope |= DFQ_INC_MATURE;
        scope |= DFQ_INC_ADULT;
    }
    else
    {
        // DFQ_FILTER_MATURE is a legacy setting
        scope |= DFQ_FILTER_MATURE;
    }
    mCurrentSortColumn = "score";
    mCurrentSortAscending = false;

    // send the message
    sendDirFindQuery(
        gMessageSystem,
        mSearchID,
        childGetValue("name").asString(),
        scope,
        mSearchStart);
}
