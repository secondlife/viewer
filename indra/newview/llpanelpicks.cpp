/** 
 * @file llpanelpicks.cpp
 * @brief LLPanelPicks and related class implementations
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llavatarconstants.h"
#include "lltexturectrl.h"
#include "llviewergenericmessage.h"	// send_generic_message
#include "llworldmap.h"
#include "llfloaterworldmap.h"
#include "llpanelmeprofile.h"
#include "llfloaterreg.h"
#include "llpanelpicks.h"
#include "llavatarpropertiesprocessor.h"
#include "llpanelpick.h"

#define XML_BTN_NEW "new_btn"
#define XML_BTN_DELETE "trash_btn"
#define XML_BTN_INFO "info_btn"


//-----------------------------------------------------------------------------
// LLPanelPicks
//-----------------------------------------------------------------------------
LLPanelPicks::LLPanelPicks(const LLUUID& avatar_id /* = LLUUID::null */)
:LLPanelProfileTab(avatar_id), mMeProfilePanel(NULL)
{
	updateData();
}

LLPanelPicks::LLPanelPicks(const Params& params)
:LLPanelProfileTab(params), mMeProfilePanel(NULL)
{

}

LLPanelPicks::~LLPanelPicks()
{
	if(!getAvatarId().isNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
	}
}

void* LLPanelPicks::create(void* data /* = NULL */)
{
	LLSD* id = NULL;
	if(data)
	{
		id = static_cast<LLSD*>(data);
		return new LLPanelPicks(LLUUID(id->asUUID()));
	}
	return new LLPanelPicks();
}

void LLPanelPicks::updateData()
{
	LLAvatarPropertiesProcessor::getInstance()->sendDataRequest(getAvatarId(),APT_PICKS);
}

void LLPanelPicks::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_PICKS == type)
	{
		LLAvatarPicks* avatar_picks = static_cast<LLAvatarPicks*>(data);
		if(avatar_picks && getAvatarId() == avatar_picks->target_id)
		{
			std::string name, second_name;
			gCacheName->getName(getAvatarId(),name,second_name);
			childSetTextArg("pick_title", "[NAME]",name);

			LLView* picks_list = getChild<LLView>("back_panel",TRUE,FALSE);
			if(!picks_list) return;
			clear();

			//*TODO move it somewhere else?
			picks_list->setEnabled(FALSE);
			childSetEnabled(XML_BTN_NEW, false);
			childSetEnabled(XML_BTN_DELETE, false);
			childSetEnabled(XML_BTN_INFO, false);
						
			S32 height = avatar_picks->picks_list.size() * 85;
			LLRect rc = picks_list->getRect();
			rc.setLeftTopAndSize(rc.mLeft,rc.mTop,rc.getWidth(),height);
			picks_list->setRect(rc);
			picks_list->reshape(rc.getWidth(),rc.getHeight());

			LLAvatarPicks::picks_list_t::const_iterator it = avatar_picks->picks_list.begin();
			for(; avatar_picks->picks_list.end() != it; ++it)
			{
				LLUUID pick_id = it->first;
				std::string pick_name = it->second;

				LLPickItem* picture = LLPickItem::create();
				picks_list->addChild(picture);

				picture->setPictureName(pick_name);
				picture->setPictureId(pick_id);
				picture->setCreatorId(getAvatarId());

				S32 last_bottom = picks_list->getRect().getHeight();
				if(mPickItemList.size() > 0)
				{
					last_bottom = mPickItemList[mPickItemList.size()-1]->getRect().mBottom;
					last_bottom -= 5;
				}
				LLRect rc = picture->getRect();
				rc.mBottom = last_bottom - rc.getHeight();
				rc.mTop = last_bottom;
				picture->reshape(rc.getWidth(),rc.getHeight());
				picture->setRect(rc);

				
				LLAvatarPropertiesProcessor::instance().addObserver(mAvatarId, picture);
				picture->update();
				mPickItemList.push_back(picture);
			}
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);

			updateButtons();
			picks_list->setEnabled(TRUE);

		}
	}
}

void LLPanelPicks::clear()
{
	LLView* scroll = getChild<LLView>("back_panel",TRUE,FALSE);
	if(scroll)
	{
		picture_list_t::const_iterator it = mPickItemList.begin();
		for(; mPickItemList.end() != it; ++it)
		{
			scroll->removeChild(*it);
			delete *it;
		}
	}
	mPickItemList.clear();
}

BOOL LLPanelPicks::postBuild(void)
{
	childSetAction(XML_BTN_INFO, onClickInfo, this);
	childSetAction(XML_BTN_NEW, onClickNew, this);
	childSetAction(XML_BTN_DELETE, onClickDelete, this);

	childSetAction("teleport_btn", onClickTeleport, this);
	childSetAction("show_on_map_btn", onClickMap, this);
	return TRUE;
}

void LLPanelPicks::onActivate(const LLUUID& id)
{
	BOOL self = (gAgent.getID() == id);

	// only agent can edit her picks 
	childSetEnabled("edit_panel", self);
	childSetVisible("edit_panel", self);

	// and see a special title - set as invisible by default in xml file
	if (self)
	{
		childSetVisible("pick_title", !self);
		childSetVisible("pick_title_agent", self);
	}

	LLPanelProfileTab::onActivate(id);
}


//static
void LLPanelPicks::onClickInfo(void *data)
{
	LLPanelPicks* self = (LLPanelPicks*) data;
	if (self)
	{
		LLPanelPick* panel_pick_info = new LLPanelPick();
		
		//*TODO redo, use the selected pick from List View, but not the first (last) one
		LLView* scroll = self->getChild<LLView>("back_panel", TRUE, FALSE);
		LLPickItem* pick = static_cast<LLPickItem*>(scroll->getFirstChild());
		if (!pick) return;

		panel_pick_info->init(pick->getCreatorId(), pick->getPickId());

		//*HACK redo toggling of panels (should work on both "profiles")
		if (self->mMeProfilePanel)
		{
			panel_pick_info->setPanelMeProfile(self->mMeProfilePanel);
			//self->mMeProfilePanel->addChildInBack(panel_pick_info);
			self->mMeProfilePanel->togglePanel(panel_pick_info);
		}
	}
}

//static
void LLPanelPicks::onClickNew(void *data)
{
	LLPanelPicks* self = (LLPanelPicks*) data;
	if(self && self->mMeProfilePanel)
	{
		if (self->mPickItemList.size() >= MAX_AVATAR_PICKS)
		{
			//*TODO show warning message
			return;
		}
				
		//in edit mode
		LLPanelPick* panel_edit_pick = new LLPanelPick(TRUE);
		panel_edit_pick->createNewPick();

		//*HACK redo toggling of panels
		panel_edit_pick->setPanelMeProfile(self->mMeProfilePanel);
		self->mMeProfilePanel->togglePanel(panel_edit_pick);
	}
}

//static
void LLPanelPicks::onClickDelete(void *data)
{
	LLPanelPicks* self = (LLPanelPicks*) data;
	if(self && self->mMeProfilePanel)
	{
	//*TODO redo, use the selected pick from List View, but not the first (last) one
	LLView* scroll = self->getChild<LLView>("back_panel", TRUE, FALSE);
	LLPickItem* first_pick = static_cast<LLPickItem*>(scroll->getFirstChild());
	if (!first_pick) return;

	LLSD args; 
	args["PICK"] = first_pick->getPickName(); 
	LLNotifications::instance().add("DeleteAvatarPick", args, LLSD(), boost::bind(&LLPanelPicks::callbackDelete, self, _1, _2)); 
	}
}

bool LLPanelPicks::callbackDelete(const LLSD& notification, const LLSD& response) 
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	//*TODO redo, use the selected pick from List View, but not the first (last) one
	LLView* scroll = getChild<LLView>("back_panel",TRUE,FALSE);
	LLPickItem* first_pick = static_cast<LLPickItem*>(scroll->getFirstChild());
	if (!first_pick) return false;

	if (0 == option)
	{
		LLAvatarPropertiesProcessor::instance().sendPickDelete(first_pick->getPickId());

		scroll->removeChild(first_pick);
		mPickItemList.pop_back();
		first_pick = NULL;
	}
	updateButtons();
	return false;
}

void LLPanelPicks::setPanelMeProfile(LLPanelMeProfile* meProfilePanel)
{
	mMeProfilePanel = meProfilePanel;
}

//static
void LLPanelPicks::teleport(const LLVector3d& position)
{
	if (!position.isExactlyZero())
	{
		gAgent.teleportViaLocation(position);
		LLFloaterWorldMap::getInstance()->trackLocation(position);
	}
}

//static
void LLPanelPicks::onClickTeleport(void* data)
{
	LLPanelPicks* self = (LLPanelPicks*)data;

	if (!self->mPickItemList.size()) return;

	//*TODO use the selected Pick instead of the last one in the list of Picks
	LLPickItem* last_pick = self->mPickItemList.back();
	if (!last_pick) return;

	teleport(last_pick->getPosGlobal());
}

//static
void LLPanelPicks::onClickMap(void* data)
{
	LLPanelPicks* self = (LLPanelPicks*)data;
	
	if (!self->mPickItemList.size()) return;
	
	//*TODO use the selected Pick instead of the last one in the list of Picks
	LLPickItem* last_pick = self->mPickItemList.back();
	if (!last_pick) return;

	showOnMap(last_pick->getPosGlobal());

}

//static
void LLPanelPicks::showOnMap(const LLVector3d& position)
{
	LLFloaterWorldMap::getInstance()->trackLocation(position);
	LLFloaterReg::showInstance("world_map", "center");
}

void LLPanelPicks::updateButtons()
{
	int picks_num = mPickItemList.size();
	childSetEnabled(XML_BTN_INFO, picks_num > 0);

	if (mAvatarId == gAgentID)
	{
		childSetEnabled(XML_BTN_NEW, picks_num < MAX_AVATAR_PICKS);
		childSetEnabled(XML_BTN_DELETE, picks_num > 0);
	}
}


//-----------------------------------------------------------------------------
// LLPanelPicks
//-----------------------------------------------------------------------------
LLPickItem::LLPickItem()
: LLPanel()
, mPicID(LLUUID::null)
, mCreatorID(LLUUID::null)
, mParcelID(LLUUID::null)
, mSnapshotID(LLUUID::null)
, mNeedData(true)
{
	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_pic_list_item.xml");
}

LLPickItem::~LLPickItem()
{
	if (!mCreatorID.isNull())
	{
		LLAvatarPropertiesProcessor::instance().removeObserver(mCreatorID, this);
	}

}

LLPickItem* LLPickItem::create()
{
	return new LLPickItem();
}

void LLPickItem::init(LLPickData* pick_data)
{
	setPictureDescription(pick_data->desc);
	setSnapshotId(pick_data->snapshot_id);
	mPosGlobal = pick_data->pos_global;
	mLocation = pick_data->location_text;

	LLTextureCtrl* picture = getChild<LLTextureCtrl>("picture", TRUE, FALSE);
	if (picture)
	{
		picture->setImageAssetID(pick_data->snapshot_id);
	}
}

void LLPickItem::setPicture()
{

}

void LLPickItem::setPictureName(const std::string& name)
{
	mPickName = name;
	childSetValue("picture_name",name);

}

const std::string& LLPickItem::getPickName()
{
	return mPickName;
}

const LLUUID& LLPickItem::getCreatorId()
{
	return mCreatorID;
}

const LLUUID& LLPickItem::getSnapshotId()
{
	return mSnapshotID;
}

void LLPickItem::setPictureDescription(const std::string& descr)
{
	childSetValue("picture_descr",descr);
}

void LLPickItem::setPictureId(const LLUUID& id)
{
	mPicID = id;
}

const LLUUID& LLPickItem::getPickId()
{
	return mPicID;
}

const LLVector3d& LLPickItem::getPosGlobal()
{
	return mPosGlobal;
}

const std::string& LLPickItem::getLocation()
{
	return mLocation;
}

const std::string LLPickItem::getDescription()
{
	return childGetValue("picture_descr").asString();
}

void LLPickItem::update()
{
	mNeedData = true;
	LLAvatarPropertiesProcessor::instance().sendDataRequest(mCreatorID, APT_PICK_INFO, &mPicID);
	mNeedData = false;
}

void LLPickItem::processProperties(void *data, EAvatarProcessorType type)
{
	if (APT_PICK_INFO != type) return;
	if (!data) return;

	LLPickData* pick_data = static_cast<LLPickData *>(data);
	if (!pick_data) return;
	if (mPicID != pick_data->pick_id) return;

	init(pick_data);
	LLAvatarPropertiesProcessor::instance().removeObserver(pick_data->agent_id, this);
}

