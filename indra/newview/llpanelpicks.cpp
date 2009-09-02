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
#include "llregistry.h"

#include "llpanelpicks.h"
#include "llavatarpropertiesprocessor.h"
#include "llpanelavatar.h"
#include "llpanelprofile.h"
#include "llpanelpick.h"
#include "llscrollcontainer.h"
#include "lllistctrl.h"

static const std::string XML_BTN_NEW = "new_btn";
static const std::string XML_BTN_DELETE = "trash_btn";
static const std::string XML_BTN_INFO = "info_btn";
static const std::string XML_BTN_TELEPORT = "teleport_btn";
static const std::string XML_BTN_SHOW_ON_MAP = "show_on_map_btn";

static LLRegisterPanelClassWrapper<LLPanelPicks> t_panel_picks("panel_picks");

//-----------------------------------------------------------------------------
// LLPanelPicks
//-----------------------------------------------------------------------------
LLPanelPicks::LLPanelPicks()
:	LLPanelProfileTab(),
	mPopupMenu(NULL),
	mProfilePanel(NULL),
	mPickPanel(NULL),
	mPicksList(NULL)
{
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

			// to restore selection of the same item later
			LLUUID pick_id_selected(LLUUID::null);
			if (getSelectedPickItem()) pick_id_selected = getSelectedPickItem()->getPickId();

			mPicksList->clear();

			//*TODO move it somewhere else?
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
				picture->childSetAction("info_chevron", boost::bind(&LLPanelPicks::onClickInfo, this));
				picture->setPickName(pick_name);
				picture->setPickId(pick_id);
				picture->setCreatorId(getAvatarId());

				LLAvatarPropertiesProcessor::instance().addObserver(getAvatarId(), picture);
				picture->update();
				mPicksList->addItem(picture);
				if (pick_id_selected != LLUUID::null && 
					pick_id == pick_id_selected) mPicksList->toggleSelection(picture);

				picture->setDoubleClickCallback(boost::bind(&LLPanelPicks::onDoubleClickItem, this, _1));
				picture->setRightMouseDownCallback(boost::bind(&LLPanelPicks::onRightMouseDownItem, this, _1, _2, _3, _4));
			}

			LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
			updateButtons();
		}
	}
}

LLPickItem* LLPanelPicks::getSelectedPickItem()
{
	std::list<LLPanel*> selected_items = mPicksList->getSelectedItems();

	if (selected_items.empty()) return NULL;
	return dynamic_cast<LLPickItem*>(selected_items.front());
}

BOOL LLPanelPicks::postBuild()
{
	mPicksList = getChild<LLListCtrl>("picks_list");

	childSetAction(XML_BTN_DELETE, boost::bind(&LLPanelPicks::onClickDelete, this));

	childSetAction("teleport_btn", boost::bind(&LLPanelPicks::onClickTeleport, this));
	childSetAction("show_on_map_btn", boost::bind(&LLPanelPicks::onClickMap, this));

	childSetAction("info_btn", boost::bind(&LLPanelPicks::onClickInfo, this));
	childSetAction("new_btn", boost::bind(&LLPanelPicks::onClickNew, this));
	
	CommitCallbackRegistry::ScopedRegistrar registar;
	registar.add("Pick.Info", boost::bind(&LLPanelPicks::onClickInfo, this));
	registar.add("Pick.Edit", boost::bind(&LLPanelPicks::onClickMenuEdit, this)); 
	registar.add("Pick.Teleport", boost::bind(&LLPanelPicks::onClickTeleport, this));
	registar.add("Pick.Map", boost::bind(&LLPanelPicks::onClickMap, this));
	registar.add("Pick.Delete", boost::bind(&LLPanelPicks::onClickDelete, this));
	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_picks.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	
	return TRUE;
}

void LLPanelPicks::onOpen(const LLSD& key)
{
	const LLUUID id(key.asUUID());
	BOOL self = (gAgent.getID() == id);

	// only agent can edit her picks 
	childSetEnabled("edit_panel", self);
	childSetVisible("edit_panel", self);

	// Disable buttons when viewing profile for first time
	if(getAvatarId() != id)
	{
		clear();

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

	LLPanelProfileTab::onOpen(key);
}

//static
void LLPanelPicks::onClickDelete()
{
	LLPickItem* pick_item = getSelectedPickItem();
	if (!pick_item) return;

	LLSD args; 
	args["PICK"] = pick_item->getPickName(); 
	LLNotifications::instance().add("DeleteAvatarPick", args, LLSD(), boost::bind(&LLPanelPicks::callbackDelete, this, _1, _2)); 
}

bool LLPanelPicks::callbackDelete(const LLSD& notification, const LLSD& response) 
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLPickItem* pick_item = getSelectedPickItem();

	if (0 == option)
	{
		LLAvatarPropertiesProcessor::instance().sendPickDelete(pick_item->getPickId());
		mPicksList->removeItem(pick_item);
	}
	updateButtons();
	return false;
}

bool LLPanelPicks::callbackTeleport( const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (0 == option)
	{
		onClickTeleport();
	}
	return false;
}

//static
void LLPanelPicks::onClickTeleport()
{
	LLPickItem* pick_item = getSelectedPickItem();
	if (!pick_item) return;
	LLPanelPick::teleport(pick_item->getPosGlobal());
}

//static
void LLPanelPicks::onClickMap()
{
	LLPickItem* pick_item = getSelectedPickItem();
	if (!pick_item) return;
	LLPanelPick::showOnMap(pick_item->getPosGlobal());
}


void LLPanelPicks::onRightMouseDownItem(LLUICtrl* item, S32 x, S32 y, MASK mask)
{
	if (mPopupMenu)
	{
		mPopupMenu->buildDrawLabels();
		mPopupMenu->updateParent(LLMenuGL::sMenuContainer);
		((LLContextMenu*)mPopupMenu)->show(x, y);
		LLMenuGL::showPopup(item, mPopupMenu, x, y);
	}
}

void LLPanelPicks::onDoubleClickItem(LLUICtrl* item)
{
	LLPickItem* pick_item = dynamic_cast<LLPickItem*>(item);
	if (!pick_item) return;
	LLSD args; 
	args["PICK"] = pick_item->getPickName(); 
	LLNotifications::instance().add("TeleportToPick", args, LLSD(), boost::bind(&LLPanelPicks::callbackTeleport, this, _1, _2)); 
}

void LLPanelPicks::updateButtons()
{
	int picks_num = mPicksList->size();
	childSetEnabled(XML_BTN_INFO, picks_num > 0);

	if (getAvatarId() == gAgentID)
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

void LLPanelPicks::setProfilePanel(LLPanelProfile* profile_panel)
{
	mProfilePanel = profile_panel;
}


void LLPanelPicks::buildPickPanel()
{
	if (mPickPanel == NULL)
	{
		mPickPanel = new LLPanelPick();
		mPickPanel->setExitCallback(boost::bind(&LLPanelPicks::onClickBack, this));
	}
}

void LLPanelPicks::onClickNew()
{
	buildPickPanel();
	mPickPanel->setEditMode(TRUE);
	mPickPanel->createNewPick();
	getProfilePanel()->togglePanel(mPickPanel);
}

void LLPanelPicks::onClickInfo()
{
	LLPickItem* pick = getSelectedPickItem();
	if (!pick) return;

	buildPickPanel();
	mPickPanel->reset();
	mPickPanel->init(pick->getCreatorId(), pick->getPickId());
	getProfilePanel()->togglePanel(mPickPanel);
}

void LLPanelPicks::onClickBack()
{
	getProfilePanel()->togglePanel(mPickPanel);
}

void LLPanelPicks::onClickMenuEdit()
{
	//*TODO, refactor - most of that is similar to onClickInfo
	LLPickItem* pick = getSelectedPickItem();
	if (!pick) return;

	buildPickPanel();
	mPickPanel->reset();
	mPickPanel->init(pick->getCreatorId(), pick->getPickId());
	mPickPanel->setEditMode(TRUE);
	getProfilePanel()->togglePanel(mPickPanel);
}

inline LLPanelProfile* LLPanelPicks::getProfilePanel()
{
	llassert_always(NULL != mProfilePanel);
	return mProfilePanel;
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

	LLTextureCtrl* picture = getChild<LLTextureCtrl>("picture");
	picture->setImageAssetID(pick_data->snapshot_id);
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
	LLAvatarPropertiesProcessor::instance().removeObserver(mCreatorID, this);
}

void LLPanelPicks::onClose()
{
	// Toggle off Pick Info panel if it is visible.
	if(mPickPanel && mPickPanel->getVisible())
	{
		getProfilePanel()->togglePanel(mPickPanel);
	}
}
