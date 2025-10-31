/**
 * @file llpaneleventinfo.cpp
 * @brief Info panel for events in the legacy Search
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

#include "llpaneleventinfo.h"

#include "llagent.h"
#include "llbutton.h"
#include "lleventflags.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "llworldmap.h"

static LLPanelInjector<LLPanelEventInfo> t_panel_event_info("panel_event_info");

LLPanelEventInfo::LLPanelEventInfo()
    : LLPanel()
{
}

LLPanelEventInfo::~LLPanelEventInfo()
{
    if (mEventInfoConnection.connected())
    {
        mEventInfoConnection.disconnect();
    }
}

bool LLPanelEventInfo::postBuild()
{
    mTBName = getChild<LLTextBox>("event_name");

    mTBCategory = getChild<LLTextBox>("event_category");
    mTBDate = getChild<LLTextBox>("event_date");
    mTBDuration = getChild<LLTextBox>("event_duration");
    mTBDesc = getChild<LLTextEditor>("event_desc");
    mTBDesc->setWordWrap(true);

    mTBRunBy = getChild<LLTextBox>("event_runby");
    mTBLocation = getChild<LLTextBox>("event_location");
    mTBCover = getChild<LLTextBox>("event_cover");

    mTeleportBtn = getChild<LLButton>( "teleport_btn");
    mTeleportBtn->setClickedCallback(boost::bind(&LLPanelEventInfo::onClickTeleport, this));

    mMapBtn = getChild<LLButton>( "map_btn");
    mMapBtn->setClickedCallback(boost::bind(&LLPanelEventInfo::onClickMap, this));

    mNotifyBtn = getChild<LLButton>( "notify_btn");
    mNotifyBtn->setClickedCallback(boost::bind(&LLPanelEventInfo::onClickNotify, this));

    mEventInfoConnection = gEventNotifier.setEventInfoCallback(boost::bind(&LLPanelEventInfo::processEventInfoReply, this, _1));

    return true;
}

void LLPanelEventInfo::setEventID(const U32 event_id)
{
    mEventID = event_id;

    if (event_id != 0)
    {
        sendEventInfoRequest();
    }
}

void LLPanelEventInfo::sendEventInfoRequest()
{
    LLMessageSystem *msg = gMessageSystem;

    msg->newMessageFast(_PREHASH_EventInfoRequest);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
    msg->nextBlockFast(_PREHASH_EventData);
    msg->addU32Fast(_PREHASH_EventID, mEventID);
    gAgent.sendReliableMessage();
}

bool LLPanelEventInfo::processEventInfoReply(LLEventInfo event)
{
    if (event.mID != getEventID())
        return false;

    mTBName->setText(event.mName);
    mTBName->setToolTip(event.mName);
    mTBCategory->setText(event.mCategoryStr);
    mTBDate->setText(event.mTimeStr);
    mTBDesc->setText(event.mDesc);
    mTBRunBy->setText(LLSLURL("agent", event.mRunByID, "inspect").getSLURLString());

    mTBDuration->setText(llformat("%d:%.2d", event.mDuration / 60, event.mDuration % 60));

    if (!event.mHasCover)
    {
        mTBCover->setText(getString("none"));
    }
    else
    {
        mTBCover->setText(llformat("%d", event.mCover));
    }

    mTBLocation->setText(LLSLURL(event.mSimName, event.mPosGlobal).getSLURLString());

    if (event.mEventFlags & EVENT_FLAG_MATURE)
    {
        childSetVisible("event_mature_yes", true);
        childSetVisible("event_mature_no", false);
    }
    else
    {
        childSetVisible("event_mature_yes", false);
        childSetVisible("event_mature_no", true);
    }

    if (event.mUnixTime < time_corrected())
    {
        mNotifyBtn->setEnabled(false);
    }
    else
    {
        mNotifyBtn->setEnabled(true);
    }

    if (gEventNotifier.hasNotification(event.mID))
    {
        mNotifyBtn->setLabel(getString("dont_notify"));
    }
    else
    {
        mNotifyBtn->setLabel(getString("notify"));
    }
    mEventInfo = event;
    return true;
}

void LLPanelEventInfo::onClickTeleport()
{
    LLFloaterWorldMap* world_map = LLFloaterWorldMap::getInstance();
    if (world_map)
    {
        world_map->trackLocation(mEventInfo.mPosGlobal);
        gAgent.teleportViaLocation(mEventInfo.mPosGlobal);
    }
}

void LLPanelEventInfo::onClickMap()
{
    LLFloaterWorldMap* world_map = LLFloaterWorldMap::getInstance();
    if (world_map)
    {
        world_map->trackLocation(mEventInfo.mPosGlobal);
        LLFloaterReg::showInstance("world_map", "center");
    }
}

void LLPanelEventInfo::onClickNotify()
{
    if (!gEventNotifier.hasNotification(mEventID))
    {
        gEventNotifier.add(mEventInfo.mID, mEventInfo.mUnixTime, mEventInfo.mTimeStr, mEventInfo.mName);
        mNotifyBtn->setLabel(getString("dont_notify"));
    }
    else
    {
        gEventNotifier.remove(mEventInfo.mID);
        mNotifyBtn->setLabel(getString("notify"));
    }
}
