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
#include "llmenugl.h"
#include "llviewermenu.h"

#include "llpanelpicks.h"
#include "llavatarpropertiesprocessor.h"
#include "llpanelavatar.h"
#include "llpanelpick.h"

static const std::string XML_BTN_NEW = "new_btn";
static const std::string XML_BTN_DELETE = "trash_btn";
static const std::string XML_BTN_INFO = "info_btn";
static const std::string XML_BTN_TELEPORT = "teleport_btn";
static const std::string XML_BTN_SHOW_ON_MAP = "show_on_map_btn";

static const std::string XML_PICKS_LIST = "back_panel";

#define PICK_ITEMS_BETWEEN 5


//-----------------------------------------------------------------------------
// LLPanelPicks
//-----------------------------------------------------------------------------
LLPanelPicks::LLPanelPicks(const LLUUID& avatar_id /* = LLUUID::null */)
:	LLPanelProfileTab(avatar_id),
	mPopupMenu(NULL),
	mSelectedPickItem(NULL)
{
	updateData();
}

LLPanelPicks::~LLPanelPicks()
{
	if(getAvatarId().notNull())
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

			LLView* picks_list = getPicksList();
			if(!picks_list) return;
			
			// to restore selection of the same item later
			LLUUID pick_id_selected(LLUUID::null);
			if (mSelectedPickItem) pick_id_selected = mSelectedPickItem->getPickId();

			clear();

			//*TODO move it somewhere else?
			picks_list->setEnabled(FALSE);
			childSetEnabled(XML_BTN_NEW, false);
			childSetEnabled(XML_BTN_DELETE, false);
			childSetEnabled(XML_BTN_INFO, false);
			childSetEnabled(XML_BTN_TELEPORT,!avatar_picks->picks_list.empty());
			childSetEnabled(XML_BTN_SHOW_ON_MAP,!avatar_picks->picks_list.empty());
						
			LLAvatarPicks::picks_list_t::const_iterator it = avatar_picks->picks_list.begin();
			for(; avatar_picks->picks_list.end() != it; ++it)
			{
				LLUUID pick_id = it->first;
				std::string pick_name = it->second;

				LLPickItem* picture = LLPickItem::create();
				picks_list->addChild(picture);

				picture->setPickName(pick_name);
				picture->setPickId(pick_id);
				picture->setCreatorId(getAvatarId());

				LLAvatarPropertiesProcessor::instance().addObserver(mAvatarId, picture);
				picture->update();
				mPickItemList.push_back(picture);
				if (pick_id_selected != LLUUID::null && 
					pick_id == pick_id_selected) setSelectedPickItem(picture);
			}

			reshapePicksList();
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);

			updateButtons();
			if (!mSelectedPickItem && mPickItemList.size()) setSelectedPickItem(mPickItemList.back());
			picks_list->setEnabled(TRUE);

		}
	}
}

void LLPanelPicks::clear()
{
	LLView* scroll = getPicksList();
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
	mSelectedPickItem = NULL;
}


LLPickItem* LLPanelPicks::getSelectedPickItem()
{
	return mSelectedPickItem;
}


void LLPanelPicks::removePickItem( LLPickItem* pick_item )
{
	LLView* scroll = getPicksList();
	scroll->removeChild(pick_item);
	mPickItemList.remove(pick_item);
	if (mPickItemList.size() == 0)
	{
		mSelectedPickItem = NULL;
	}
	else 
	{
		setSelectedPickItem(mPickItemList.back());
	}

	reshapePicksList();
}

void LLPanelPicks::reshapePicksList()
{
	if (!mPickItemList.size()) return;
	LLView* pickList = getPicksList();

	S32 last_bottom = pickList->getRect().getHeight();
	child_list_const_iter_t child_it, child_first_it = pickList->getChildList()->begin();
	for ( child_it = child_first_it; child_it != pickList->getChildList()->end(); ++child_it)
	{
		LLView* const childp = *child_it;
		if(child_it != child_first_it)
		{
			last_bottom -= childp->getRect().getHeight();
			last_bottom -= PICK_ITEMS_BETWEEN;
		}
		reshapePickItem(childp, last_bottom);
	}

	S32 height = pickList->getChildCount() * ((*child_first_it)->getRect().getHeight() + PICK_ITEMS_BETWEEN);
	LLRect rc = pickList->getRect();
	rc.setLeftTopAndSize(rc.mLeft, rc.mTop, rc.getWidth(), height);
	pickList->reshape(rc.getWidth(), rc.getHeight());
	pickList->setRect(rc);
}

void LLPanelPicks::reshapePickItem(LLView* const pick_item, const S32 last_bottom)
{
	LLRect rc = pick_item->getRect();
	rc.mBottom = last_bottom - rc.getHeight();
	rc.mTop = last_bottom;
	pick_item->reshape(rc.getWidth(), rc.getHeight());
	pick_item->setRect(rc);
}

LLView* LLPanelPicks::getPicksList() const
{
	return getChild<LLView>(XML_PICKS_LIST, TRUE, FALSE);
}

BOOL LLPanelPicks::postBuild(void)
{
	childSetAction(XML_BTN_DELETE, onClickDelete, this);

	childSetAction("teleport_btn", onClickTeleport, this);
	childSetAction("show_on_map_btn", onClickMap, this);

	mCommitCallbackRegistrar.add("Pick.Teleport", boost::bind(onClickTeleport, this));
	mCommitCallbackRegistrar.add("Pick.Map", boost::bind(onClickMap, this));
	mCommitCallbackRegistrar.add("Pick.Delete", boost::bind(onClickDelete, this));

	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_picks.xml", gMenuHolder);

	return TRUE;
}

void LLPanelPicks::onActivate(const LLUUID& id)
{
	BOOL self = (gAgent.getID() == id);

	// only agent can edit her picks 
	childSetEnabled("edit_panel", self);
	childSetVisible("edit_panel", self);

	// Disable buttons when viewing profile for first time
	if(getAvatarId() != id)
	{
		childSetEnabled(XML_BTN_INFO,FALSE);
		childSetEnabled(XML_BTN_TELEPORT,FALSE);
		childSetEnabled(XML_BTN_SHOW_ON_MAP,FALSE);
	}

	// and see a special title - set as invisible by default in xml file
	if (self)
	{
		childSetVisible("pick_title", !self);
		childSetVisible("pick_title_agent", self);
	}

	LLPanelProfileTab::onActivate(id);
}

//static
void LLPanelPicks::onClickDelete(void *data)
{
	LLPanelPicks* self = (LLPanelPicks*) data;
	if (!self) return;

	LLPickItem* pick_item = self->getSelectedPickItem();
	if (!pick_item) return;

	LLSD args; 
	args["PICK"] = pick_item->getPickName(); 
	LLNotifications::instance().add("DeleteAvatarPick", args, LLSD(), boost::bind(&LLPanelPicks::callbackDelete, self, _1, _2)); 
}

bool LLPanelPicks::callbackDelete(const LLSD& notification, const LLSD& response) 
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLPickItem* pick_item = getSelectedPickItem();

	if (0 == option)
	{
		LLAvatarPropertiesProcessor::instance().sendPickDelete(pick_item->getPickId());
		removePickItem(pick_item);
	}
	updateButtons();
	return false;
}

//static
void LLPanelPicks::onClickTeleport(void* data)
{
	LLPanelPicks* self = (LLPanelPicks*)data;
	LLPickItem* pick_item = self->getSelectedPickItem();
	if (!pick_item) return;
	LLPanelPick::teleport(pick_item->getPosGlobal());
}

//static
void LLPanelPicks::onClickMap(void* data)
{
	LLPanelPicks* self = (LLPanelPicks*)data;
	LLPickItem* pick_item = self->getSelectedPickItem();
	if (!pick_item) return;
	LLPanelPick::showOnMap(pick_item->getPosGlobal());
}


BOOL LLPanelPicks::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (isMouseInPick(x, y))
	{
		if (mPopupMenu)
		{
			mPopupMenu->buildDrawLabels();
			mPopupMenu->updateParent(LLMenuGL::sMenuContainer);
			((LLContextMenu*)mPopupMenu)->show(x, y, FALSE);
			LLMenuGL::showPopup(this, mPopupMenu, x, y);
		}
		return TRUE;
	}
	return LLPanel::handleRightMouseDown(x, y, mask);
}

BOOL LLPanelPicks::handleMouseDown( S32 x, S32 y, MASK mask )
{
	if (isMouseInPick(x, y)) return TRUE;
	return LLPanel::handleMouseDown(x, y, mask);
}

void LLPanelPicks::updateButtons()
{
	int picks_num = mPickItemList.size();
	childSetEnabled(XML_BTN_INFO, picks_num > 0);

	if (mAvatarId == gAgentID)
	{
		childSetEnabled(XML_BTN_NEW, picks_num < MAX_AVATAR_PICKS);
		childSetEnabled(XML_BTN_DELETE, picks_num > 0);

		//*TODO move somewhere this calls
		// we'd better set them up earlier when a panel was being constructed
		mPopupMenu->setItemVisible("pick_delete", TRUE);
		mPopupMenu->setItemVisible("pick_edit", TRUE);
		mPopupMenu->setItemVisible("pick_separator", TRUE);
	}

	//*TODO update buttons like Show on Map, Teleport etc.

}

void LLPanelPicks::setSelectedPickItem( LLPickItem* item )
{
	if (!item) return;
	if (mSelectedPickItem == item) return;
	if (mSelectedPickItem && mSelectedPickItem->isBackgroundVisible())
	{
		mSelectedPickItem->setBackgroundVisible(FALSE);
	}
	item->setBackgroundVisible(TRUE);
	mSelectedPickItem = item;
}

BOOL LLPanelPicks::isMouseInPick( S32 x, S32 y )
{
	LLView* scroll = getChild<LLView>("profile_scroll");
	if (!scroll->parentPointInView(x, y)) return FALSE;

	S32 x_l = x;
	S32 y_l = y;
	
	picture_list_t::const_iterator it = mPickItemList.begin();
	for(; mPickItemList.end() != it; ++it)
	{
		localPointToOtherView(x, y, &x_l, &y_l, (*it));
		if ((*it)->pointInView(x_l, y_l))
		{
			setSelectedPickItem(*it);
			return TRUE;
		}
	}	
	return FALSE;
}


//-----------------------------------------------------------------------------
// LLPanelPicks
//-----------------------------------------------------------------------------
LLPickItem::LLPickItem()
: LLPanel()
, mPickID(LLUUID::null)
, mCreatorID(LLUUID::null)
, mParcelID(LLUUID::null)
, mSnapshotID(LLUUID::null)
, mNeedData(true)
{
	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_pick_list_item.xml");
}

LLPickItem::~LLPickItem()
{
	if (mCreatorID.notNull())
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
	setPickDesc(pick_data->desc);
	setSnapshotId(pick_data->snapshot_id);
	mPosGlobal = pick_data->pos_global;
	mLocation = pick_data->location_text;

	LLTextureCtrl* picture = getChild<LLTextureCtrl>("picture", TRUE, FALSE);
	if (picture)
	{
		picture->setImageAssetID(pick_data->snapshot_id);
	}
}

void LLPickItem::setPickName(const std::string& name)
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

void LLPickItem::setPickDesc(const std::string& descr)
{
	childSetValue("picture_descr",descr);
}

void LLPickItem::setPickId(const LLUUID& id)
{
	mPickID = id;
}

const LLUUID& LLPickItem::getPickId()
{
	return mPickID;
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
	LLAvatarPropertiesProcessor::instance().sendDataRequest(mCreatorID, APT_PICK_INFO, &mPickID);
	mNeedData = false;
}

void LLPickItem::processProperties(void *data, EAvatarProcessorType type)
{
	if (APT_PICK_INFO != type) return;
	if (!data) return;

	LLPickData* pick_data = static_cast<LLPickData *>(data);
	if (!pick_data) return;
	if (mPickID != pick_data->pick_id) return;

	init(pick_data);
	LLAvatarPropertiesProcessor::instance().removeObserver(pick_data->agent_id, this);
}

