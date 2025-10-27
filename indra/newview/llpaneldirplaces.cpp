/**
 * @file llpaneldirplaces.cpp
 * @brief Places panel in the legacy Search directory.
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

#include "llpaneldirplaces.h"

#include "message.h"
#include "llparcel.h"
#include "llregionflags.h"
#include "llqueryflags.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterdirectory.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpaneldirbrowser.h"
#include "llsearcheditor.h"
#include "lltextbox.h"
#include "llviewercontrol.h"

static LLPanelInjector<LLPanelDirPlaces> t_panel_dir_people("panel_dir_places");

LLPanelDirPlaces::LLPanelDirPlaces()
    : LLPanelDirBrowser()
{
    mMinSearchChars = 3;
}

bool LLPanelDirPlaces::postBuild()
{
    LLPanelDirBrowser::postBuild();

    //getChild<LLLineEditor>("name")->setKeystrokeCallback(boost::bind(&LLPanelDirBrowser::onKeystrokeName, _1, _2), NULL);

    childSetAction("Search", &LLPanelDirBrowser::onClickSearchCore, this);
    //childDisable("Search");

    mCurrentSortColumn = "dwell";
    mCurrentSortAscending = false;

    return true;
}

LLPanelDirPlaces::~LLPanelDirPlaces()
{
}

// virtual
void LLPanelDirPlaces::performQuery()
{
    std::string place_name = childGetValue("name").asString();
    if (place_name.length() < mMinSearchChars)
    {
        return;
    }

    // "hi " is three chars but not a long-enough search
    std::string query_string = place_name;
    LLStringUtil::trim( query_string );
    bool query_was_filtered = (query_string != place_name);

    // possible we threw away all the short words in the query so check length
    if ( query_string.length() < mMinSearchChars )
    {
        LLNotificationsUtil::add("SeachFilteredOnShortWordsEmpty");
        return;
    };

    // if we filtered something out, display a popup
    if ( query_was_filtered )
    {
        LLSD args;
        args["FINALQUERY"] = query_string;
        LLNotificationsUtil::add("SeachFilteredOnShortWords", args);
    };

    std::string catstring = childGetValue("Category").asString();

    // Because LLParcel::C_ANY is -1, must do special check
    S32 category = 0;
    if (catstring == "any")
    {
        category = LLParcel::C_ANY;
    }
    else
    {
        category = LLParcel::getCategoryFromString(catstring);
    }

    U32 flags = 0x0;
    bool adult_enabled = gAgent.canAccessAdult();
    bool mature_enabled = gAgent.canAccessMature();

    static LLUICachedControl<bool> inc_pg("ShowPGSims", true);
    static LLUICachedControl<bool> inc_mature("ShowMatureSims", false);
    static LLUICachedControl<bool> inc_adult("ShowAdultSims", false);

    if (inc_pg)
    {
        flags |= DFQ_INC_PG;
    }

    if (inc_mature && mature_enabled)
    {
        flags |= DFQ_INC_MATURE;
    }

    if (inc_adult && adult_enabled)
    {
        flags |= DFQ_INC_ADULT;
    }

    if (0x0 == flags)
    {
        LLNotificationsUtil::add("NoContentToSearch");
        return; 
    }
	
    queryCore(query_string, category, flags);
}

void LLPanelDirPlaces::initialQuery()
{
    // All Linden locations in PG/Mature sims, any name.
    U32 flags = DFQ_INC_PG | DFQ_INC_MATURE;
    queryCore(LLStringUtil::null, LLParcel::C_LINDEN, flags);
}

void LLPanelDirPlaces::queryCore(const std::string& name, S32 category, U32 flags)
{
    setupNewSearch();

// JC: Sorting by dwell severely impacts the performance of the query.
// Instead of sorting on the dataserver, we sort locally once the results
// are received.
// IW: Re-enabled dwell sort based on new 3-character minimum description
// Hopefully we'll move to next-gen Find before this becomes a big problem

    flags |= DFQ_DWELL_SORT;

    LLMessageSystem* msg = gMessageSystem;

    msg->newMessage("DirPlacesQuery");
    msg->nextBlock("AgentData");
    msg->addUUID("AgentID", gAgent.getID());
    msg->addUUID("SessionID", gAgent.getSessionID());
    msg->nextBlock("QueryData");
    msg->addUUID("QueryID", getSearchID());
    msg->addString("QueryText", name);
    msg->addU32("QueryFlags", flags);
    msg->addS8("Category", (S8)category);
    // No longer support queries by region name, too many regions
    // for combobox, no easy way to do autocomplete. JC
    msg->addString("SimName", "");
    msg->addS32Fast(_PREHASH_QueryStart,mSearchStart);
    gAgent.sendReliableMessage();
}

