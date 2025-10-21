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
#include "llqueryflags.h"
#include "llviewercontrol.h"
#include "llsearcheditor.h"
#include "message.h"

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
    //childDisable("Search");
    setDefaultBtn( "Search" );

    return true;
}

LLPanelDirGroups::~LLPanelDirGroups()
{
    // Children all cleaned up by default view destructor.
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
    if ( !gSavedSettings.getBOOL("ShowMatureGroups")  || gAgent.isTeen() )
    {
        scope |= DFQ_FILTER_MATURE;
    }

    mCurrentSortColumn = "score";
    mCurrentSortAscending = FALSE;

    // send the message
    sendDirFindQuery(
        gMessageSystem,
        mSearchID,
        childGetValue("name").asString(),
        scope,
        mSearchStart);
}
