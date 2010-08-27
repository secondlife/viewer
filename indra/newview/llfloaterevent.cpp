/** 
 * @file llfloaterevent.cpp
 * @brief Display for events in the finder
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llfloaterevent.h"

#include "message.h"
#include "llnotificationsutil.h"
#include "llui.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llcachename.h"
#include "llcommandhandler.h"	// secondlife:///app/chat/ support
#include "lleventflags.h"
#include "lleventnotifier.h"
#include "llexpandabletextbox.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llinventorymodel.h"
#include "llsecondlifeurls.h"
#include "llslurl.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lluiconstants.h"
#include "llviewercontrol.h"
#include "llweb.h"
#include "llworldmap.h"
#include "llworldmapmessage.h"
#include "lluictrlfactory.h"
#include "lltrans.h"


class LLEventHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLEventHandler() : LLCommandHandler("event", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& params, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (params.size() < 1)
		{
			return false;
		}
		
		LLFloaterEvent* floater = LLFloaterReg::getTypedInstance<LLFloaterEvent>("event");
		if (floater)
		{
			floater->setEventID(params[0].asInteger());
			LLFloaterReg::showTypedInstance<LLFloaterEvent>("event");
			return true;
		}

		return false;
	}
};
LLEventHandler gEventHandler;

LLFloaterEvent::LLFloaterEvent(const LLSD& key)
	: LLFloater(key),

	  mEventID(0)
{
}


LLFloaterEvent::~LLFloaterEvent()
{
}


BOOL LLFloaterEvent::postBuild()
{
	mTBName = getChild<LLTextBox>("event_name");

	mTBCategory = getChild<LLTextBox>("event_category");
	
	mTBDate = getChild<LLTextBox>("event_date");

	mTBDuration = getChild<LLTextBox>("event_duration");

	mTBDesc = getChild<LLExpandableTextBox>("event_desc");

	mTBRunBy = getChild<LLTextBox>("event_runby");
	mTBLocation = getChild<LLTextBox>("event_location");
	mTBCover = getChild<LLTextBox>("event_cover");

	mTeleportBtn = getChild<LLButton>( "teleport_btn");
	mTeleportBtn->setClickedCallback(onClickTeleport, this);

	mMapBtn = getChild<LLButton>( "map_btn");
	mMapBtn->setClickedCallback(onClickMap, this);

	mNotifyBtn = getChild<LLButton>( "notify_btn");
	mNotifyBtn->setClickedCallback(onClickNotify, this);

	mCreateEventBtn = getChild<LLButton>( "create_event_btn");
	mCreateEventBtn->setClickedCallback(onClickCreateEvent, this);

	mGodDeleteEventBtn = getChild<LLButton>( "god_delete_event_btn");
	mGodDeleteEventBtn->setClickedCallback(boost::bind(&LLFloaterEvent::onClickDeleteEvent, this));

	return TRUE;
}

void LLFloaterEvent::setEventID(const U32 event_id)
{
	mEventID = event_id;
	// Should reset all of the panel state here
	resetInfo();

	if (event_id != 0)
	{
		sendEventInfoRequest();
	}
}

void LLFloaterEvent::onClickDeleteEvent()
{
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_EventGodDelete);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlockFast(_PREHASH_EventData);
	msg->addU32Fast(_PREHASH_EventID, mEventID);

	gAgent.sendReliableMessage();
}

void LLFloaterEvent::sendEventInfoRequest()
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

//static 
void LLFloaterEvent::processEventInfoReply(LLMessageSystem *msg, void **)
{
	// extract the agent id
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );

	LLFloaterEvent* floater = LLFloaterReg::getTypedInstance<LLFloaterEvent>("event");
	
	if(floater)
	{
		floater->mEventInfo.unpack(msg);
		floater->mTBName->setText(floater->mEventInfo.mName);
		floater->mTBCategory->setText(floater->mEventInfo.mCategoryStr);
		floater->mTBDate->setText(floater->mEventInfo.mTimeStr);
		floater->mTBDesc->setText(floater->mEventInfo.mDesc);
		floater->mTBRunBy->setText(LLSLURL("agent", floater->mEventInfo.mRunByID, "inspect").getSLURLString());

		floater->mTBDuration->setText(llformat("%d:%.2d", floater->mEventInfo.mDuration / 60, floater->mEventInfo.mDuration % 60));

		if (!floater->mEventInfo.mHasCover)
		{
			floater->mTBCover->setText(floater->getString("none"));
		}
		else
		{
			floater->mTBCover->setText(llformat("%d", floater->mEventInfo.mCover));
		}

		F32 global_x = (F32)floater->mEventInfo.mPosGlobal.mdV[VX];
		F32 global_y = (F32)floater->mEventInfo.mPosGlobal.mdV[VY];

		S32 region_x = llround(global_x) % REGION_WIDTH_UNITS;
		S32 region_y = llround(global_y) % REGION_WIDTH_UNITS;
		S32 region_z = llround((F32)floater->mEventInfo.mPosGlobal.mdV[VZ]);

		std::string desc = floater->mEventInfo.mSimName + llformat(" (%d, %d, %d)", region_x, region_y, region_z);
		floater->mTBLocation->setText(desc);

		floater->getChildView("rating_icon_m")->setVisible( FALSE);
		floater->getChildView("rating_icon_r")->setVisible( FALSE);
		floater->getChildView("rating_icon_pg")->setVisible( FALSE);
		floater->getChild<LLUICtrl>("rating_value")->setValue(floater->getString("unknown"));

		//for some reason there's not adult flags for now, so see if region is adult and then
		//set flags
		LLWorldMapMessage::url_callback_t cb = boost::bind(	&regionInfoCallback, floater->mEventInfo.mID, _1);
		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(floater->mEventInfo.mSimName, cb, std::string("unused"), false);

		if (floater->mEventInfo.mUnixTime < time_corrected())
		{
			floater->mNotifyBtn->setEnabled(FALSE);
		}
		else
		{
			floater->mNotifyBtn->setEnabled(TRUE);
		}

		if (gEventNotifier.hasNotification(floater->mEventInfo.mID))
		{
			floater->mNotifyBtn->setLabel(floater->getString("dont_notify"));
		}
		else
		{
			floater->mNotifyBtn->setLabel(floater->getString("notify"));
		}
	
		floater->mMapBtn->setEnabled(TRUE);
		floater->mTeleportBtn->setEnabled(TRUE);
	}
}

//static 
void LLFloaterEvent::regionInfoCallback(U32 event_id, U64 region_handle)
{
	LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromHandle(region_handle);
	LLFloaterEvent* floater = LLFloaterReg::getTypedInstance<LLFloaterEvent>("event");

	if (sim_info && floater && (event_id == floater->getEventID()))
	{
		// update the event with the maturity info
		if (sim_info->isAdult())
		{
			floater->getChildView("rating_icon_m")->setVisible( FALSE);
			floater->getChildView("rating_icon_r")->setVisible( TRUE);
			floater->getChildView("rating_icon_pg")->setVisible( FALSE);
			floater->getChild<LLUICtrl>("rating_value")->setValue(floater->getString("adult"));

		}
		else if (floater->mEventInfo.mEventFlags & EVENT_FLAG_MATURE)
		{
			floater->getChildView("rating_icon_m")->setVisible( TRUE);
			floater->getChildView("rating_icon_r")->setVisible( FALSE);
			floater->getChildView("rating_icon_pg")->setVisible( FALSE);
			floater->getChild<LLUICtrl>("rating_value")->setValue(floater->getString("moderate"));
		}
		else
		{
			floater->getChildView("rating_icon_m")->setVisible( FALSE);
			floater->getChildView("rating_icon_r")->setVisible( FALSE);
			floater->getChildView("rating_icon_pg")->setVisible( TRUE);
			floater->getChild<LLUICtrl>("rating_value")->setValue(floater->getString("general"));
		}
	}
}

void LLFloaterEvent::draw()
{
	mGodDeleteEventBtn->setVisible(gAgent.isGodlike());

	LLPanel::draw();
}

void LLFloaterEvent::resetInfo()
{
	mTBName->setText(LLStringUtil::null);
	mTBCategory->setText(LLStringUtil::null);
	mTBDate->setText(LLStringUtil::null);
	mTBDesc->setText(LLStringUtil::null);
	mTBDuration->setText(LLStringUtil::null);
	mTBCover->setText(LLStringUtil::null);
	mTBLocation->setText(LLStringUtil::null);
	mTBRunBy->setText(LLStringUtil::null);
	mNotifyBtn->setEnabled(FALSE);
	mMapBtn->setEnabled(FALSE);
	mTeleportBtn->setEnabled(FALSE);
}

// static
void LLFloaterEvent::onClickTeleport(void* data)
{
	LLFloaterEvent* self = (LLFloaterEvent*)data;
	LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
	if (!self->mEventInfo.mPosGlobal.isExactlyZero()&&worldmap_instance)
	{
		gAgent.teleportViaLocation(self->mEventInfo.mPosGlobal);
		worldmap_instance->trackLocation(self->mEventInfo.mPosGlobal);
	}
}


// static
void LLFloaterEvent::onClickMap(void* data)
{
	LLFloaterEvent* self = (LLFloaterEvent*)data;
	LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();

	if (!self->mEventInfo.mPosGlobal.isExactlyZero()&&worldmap_instance)
	{
		worldmap_instance->trackLocation(self->mEventInfo.mPosGlobal);
		LLFloaterReg::showInstance("world_map", "center");
	}
}


// static
void LLFloaterEvent::onClickCreateEvent(void* data)
{
	LLNotificationsUtil::add("PromptGoToEventsPage");//, LLSD(), LLSD(), callbackCreateEventWebPage); 
}


// static
void LLFloaterEvent::onClickNotify(void *data)
{
	LLFloaterEvent* self = (LLFloaterEvent*)data;

	if (!gEventNotifier.hasNotification(self->mEventID))
	{
		gEventNotifier.add(self->mEventInfo);
		self->mNotifyBtn->setLabel(self->getString("dont_notify"));
	}
	else
	{
		gEventNotifier.remove(self->mEventInfo.mID);
		self->mNotifyBtn->setLabel(self->getString("notify"));
	}
}
