/**
 * @file llpaneldirevents.cpp
 * @brief Events panel in the legacy Search directory.
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

#include "llpaneldirevents.h"

#include <sstream>

// linden library includes
#include "message.h"
#include "llqueryflags.h"

// viewer project includes
#include "llagent.h"
#include "llviewercontrol.h"
#include "llnotificationsutil.h"
#include "llpaneldirbrowser.h"
#include "llresmgr.h"
#include "lluiconstants.h"
#include "llappviewer.h"

static LLPanelInjector<LLPanelDirEvents> t_panel_dir_events("panel_dir_events");

constexpr S32 DAY_TO_SEC = 24 * 60 * 60;

LLPanelDirEvents::LLPanelDirEvents()
    : LLPanelDirBrowser(),
    mDay(0)
{
    // more results per page for this
    mResultsPerPage = RESULTS_PER_PAGE_EVENTS;
}

bool LLPanelDirEvents::postBuild()
{
    LLPanelDirBrowser::postBuild();

    childSetCommitCallback("date_mode", onDateModeCallback, this);

    childSetAction("back_btn", onBackBtn, this);
    childSetAction("forward_btn", onForwardBtn, this);

    childSetCommitCallback("mature", onCommitMature, this);

    childSetAction("Search", LLPanelDirBrowser::onClickSearchCore, this);
    setDefaultBtn("Search");

    onDateModeCallback(NULL, this);

    mCurrentSortColumn = "time";

    setDay(0); // for today

    return true;
}

LLPanelDirEvents::~LLPanelDirEvents()
{
}

void LLPanelDirEvents::setDay(S32 day)
{
    mDay = day;

    // Get time UTC
    time_t utc_time = time_corrected();

    // Correct for offset
    utc_time += day * DAY_TO_SEC;

    // There's only one internal tm buffer.
    struct tm* internal_time;

    // Convert to Pacific, based on server's opinion of whether
    // it's daylight savings time there.
    internal_time = utc_to_pacific_time(utc_time, is_daylight_savings());

    std::string buffer = llformat("%d/%d",
            1 + internal_time->tm_mon,		// Jan = 0
            internal_time->tm_mday);	// 2001 = 101
    childSetValue("date_text", buffer);
}

// virtual
void LLPanelDirEvents::performQuery()
{
    // event_id 0 will perform no delete action.
    performQueryOrDelete(0);
}

void LLPanelDirEvents::performQueryOrDelete(U32 event_id)
{
    S32 relative_day = mDay;
    // Update the date field to show the date IN THE SERVER'S
    // TIME ZONE, as that is what will be displayed in each event

    // Get time UTC
    time_t utc_time = time_corrected();

    // Correct for offset
    utc_time += relative_day * DAY_TO_SEC;

    // There's only one internal tm buffer.
    struct tm* internal_time;

    // Convert to Pacific, based on server's opinion of whether
    // it's daylight savings time there.
    internal_time = utc_to_pacific_time(utc_time, is_daylight_savings());

    std::string buffer = llformat("%d/%d",
            1 + internal_time->tm_mon,		// Jan = 0
            internal_time->tm_mday);	// 2001 = 101
    childSetValue("date_text", buffer);

    // Record the relative day so back and forward buttons
    // offset from this day.
    mDay = relative_day;

    static LLUICachedControl<bool> incpg("ShowPGEvents", true);
    static LLUICachedControl<bool> incmature("ShowMatureEvents", false);
    static LLUICachedControl<bool> incadult("ShowAdultEvents", false);

    U32 scope = DFQ_DATE_EVENTS;
    if (incpg) scope |= DFQ_INC_PG;
    if (incmature && gAgent.canAccessMature()) scope |= DFQ_INC_MATURE;
    if (incadult && gAgent.canAccessAdult()) scope |= DFQ_INC_ADULT;

    if ( !( scope & (DFQ_INC_PG | DFQ_INC_MATURE | DFQ_INC_ADULT )))
    {
        LLNotificationsUtil::add("NoContentToSearch");
        return;
    }

    setupNewSearch();

    std::ostringstream params;

    // Date mode for the search
    if ("current" == childGetValue("date_mode").asString())
    {
        params << "u|";
    }
    else
    {
        params << mDay << "|";
    }

    // Categories are stored in the database in table indra.event_category
    // XML must match.
    U32 cat_id = childGetValue("category_combo").asInteger();

    params << cat_id << "|";
    params << childGetValue("event_search_text").asString();

    // send the message
    if (0 == event_id)
    {
        sendDirFindQuery(gMessageSystem, mSearchID, params.str(), scope, mSearchStart);
    }
    else
    {
        // This delete will also perform a query.
        LLMessageSystem* msg = gMessageSystem;

        msg->newMessageFast(_PREHASH_EventGodDelete);

        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

        msg->nextBlockFast(_PREHASH_EventData);
        msg->addU32Fast(_PREHASH_EventID, event_id);

        msg->nextBlockFast(_PREHASH_QueryData);
        msg->addUUIDFast(_PREHASH_QueryID, mSearchID);
        msg->addStringFast(_PREHASH_QueryText, params.str());
        msg->addU32Fast(_PREHASH_QueryFlags, scope);
        msg->addS32Fast(_PREHASH_QueryStart, mSearchStart);
        gAgent.sendReliableMessage();
    }
}

// static
void LLPanelDirEvents::onDateModeCallback(LLUICtrl* ctrl, void *data)
{
    LLPanelDirEvents* self = (LLPanelDirEvents*)data;
    if (self->childGetValue("date_mode").asString() == "date")
    {
        self->childEnable("forward_btn");
        self->childEnable("back_btn");
    }
    else
    {
        self->childDisable("forward_btn");
        self->childDisable("back_btn");
    }
}

// static
void LLPanelDirEvents::onBackBtn(void* data)
{
    LLPanelDirEvents* self = (LLPanelDirEvents*)data;
    self->resetSearchStart();
    self->setDay(self->mDay - 1);
    self->performQuery();
}


// static
void LLPanelDirEvents::onForwardBtn(void* data)
{
    LLPanelDirEvents* self = (LLPanelDirEvents*)data;
    self->resetSearchStart();
    self->setDay(self->mDay + 1);
    self->performQuery();
}


// static
void LLPanelDirEvents::onCommitMature(LLUICtrl* ctrl, void* data)
{
    // just perform another search
    onClickSearchCore(data);
}
