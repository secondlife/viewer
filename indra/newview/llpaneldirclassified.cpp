/**
 * @file llpaneldirclassified.cpp
 * @brief Classified panel in the legacy Search directory.
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

#include "llpaneldirclassified.h"

#include "llclassifiedflags.h"

#include "llfontgl.h"
#include "message.h"
#include "llqueryflags.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcontrol.h"
#include "llcombobox.h"
#include "llclassifiedinfo.h"
#include "lluiconstants.h"
#include "llpaneldirbrowser.h"
#include "lltextbox.h"

#include "llcheckboxctrl.h"
#include "llfloaterdirectory.h"
#include "lllineeditor.h"
#include "llsearcheditor.h"
#include "llviewermenu.h"
#include "llnotificationsutil.h"

static LLPanelInjector<LLPanelDirClassified> t_panel_dir_classified("panel_dir_classified");

LLPanelDirClassified::LLPanelDirClassified()
:   LLPanelDirBrowser()
{
}

bool LLPanelDirClassified::postBuild()
{
    LLPanelDirBrowser::postBuild();

    childSetAction("Search", onClickSearchCore, this);
    setDefaultBtn("Search");
    return true;
}

LLPanelDirClassified::~LLPanelDirClassified()
{
}

void LLPanelDirClassified::performQuery()
{
    static LLUICachedControl<bool> inc_pg("ShowPGClassifieds", true);
    static LLUICachedControl<bool> inc_mature("ShowMatureClassifieds", false);
    static LLUICachedControl<bool> inc_adult("ShowAdultClassifieds", false);

    if (!(inc_pg || inc_mature || inc_adult))
    {
        LLNotificationsUtil::add("NoContentToSearch");
        return;
    }

    // This sets mSearchID and clears the list of results
    setupNewSearch();

    // send the message
    LLMessageSystem *msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_DirClassifiedQuery);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

    bool filter_auto_renew = false;
    U32 query_flags = pack_classified_flags_request(filter_auto_renew, inc_pg,
                                                                       inc_mature && gAgent.canAccessMature(),
                                                                       inc_adult && gAgent.canAccessAdult());
    U32 category = childGetValue("Category").asInteger();

    msg->nextBlockFast(_PREHASH_QueryData);
    msg->addUUIDFast(_PREHASH_QueryID, mSearchID );
    msg->addStringFast(_PREHASH_QueryText, childGetValue("name").asString());
    msg->addU32Fast(_PREHASH_QueryFlags, query_flags);
    msg->addU32Fast(_PREHASH_Category, category);
    msg->addS32Fast(_PREHASH_QueryStart,mSearchStart);

    gAgent.sendReliableMessage();
}
