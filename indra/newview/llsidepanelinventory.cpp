/**
 * @file LLSidepanelInventory.cpp
 * @brief Side Bar "Inventory" panel
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llsidepanelinventory.h"

#include "llagent.h"
#include "llbutton.h"
#include "llinventorybridge.h"
#include "llinventorypanel.h"
#include "llpanelmaininventory.h"
#include "llsidepanelobjectinfo.h"
#include "lltabcontainer.h"

static const S32 LANDMARK_FOLDERS_MENU_WIDTH = 250;
static const std::string AGENT_INFO_TYPE			= "agent";
static const std::string CREATE_LANDMARK_INFO_TYPE	= "create_landmark";
static const std::string LANDMARK_INFO_TYPE			= "landmark";
static const std::string REMOTE_PLACE_INFO_TYPE		= "remote_place";
static const std::string TELEPORT_HISTORY_INFO_TYPE	= "teleport_history";

// Helper functions
static void setAllChildrenVisible(LLView* view, BOOL visible);

static LLRegisterPanelClassWrapper<LLSidepanelInventory> t_inventory("sidepanel_inventory");

LLSidepanelInventory::LLSidepanelInventory()
	:	LLPanel(),
		mSidepanelObjectInfo(NULL)
{

	//LLUICtrlFactory::getInstance()->buildPanel(this, "panel_inventory.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLSidepanelInventory::~LLSidepanelInventory()
{
}

BOOL LLSidepanelInventory::postBuild()
{
	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onInfoButtonClicked, this));

	mShareBtn = getChild<LLButton>("share_btn");
	mShareBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShareButtonClicked, this));

	mShareBtn = getChild<LLButton>("share_btn");
	mShareBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShareButtonClicked, this));

	mWearBtn = getChild<LLButton>("wear_btn");
	mWearBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onWearButtonClicked, this));

	mPlayBtn = getChild<LLButton>("play_btn");
	mPlayBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onPlayButtonClicked, this));

	mTeleportBtn = getChild<LLButton>("teleport_btn");
	mTeleportBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onTeleportButtonClicked, this));

	mOverflowBtn = getChild<LLButton>("overflow_btn");
	mOverflowBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onOverflowButtonClicked, this));

	mTabContainer = getChild<LLTabContainer>("Inventory Tabs");
	mSidepanelObjectInfo = getChild<LLSidepanelObjectInfo>("sidepanel_object_info");

	mPanelMainInventory = getChild<LLPanelMainInventory>("panel_main_inventory");
	mPanelMainInventory->setSelectCallback(boost::bind(&LLSidepanelInventory::onSelectionChange, this, _1, _2));

	LLButton* back_btn = mSidepanelObjectInfo->getChild<LLButton>("back_btn");
	back_btn->setClickedCallback(boost::bind(&LLSidepanelInventory::onBackButtonClicked, this));

	return TRUE;
}

void LLSidepanelInventory::onOpen(const LLSD& key)
{
	if(key.size() == 0)
		return;

	mSidepanelObjectInfo->reset();

	if (key.has("id"))
	{
		mSidepanelObjectInfo->setItemID(key["id"].asUUID());
	}
	
	if (key.has("object"))
	{
		mSidepanelObjectInfo->setObjectID(key["object"].asUUID());
	}

	toggleObjectInfoPanel(TRUE);
}

void LLSidepanelInventory::onInfoButtonClicked()
{
	LLInventoryItem *item = getSelectedItem();
	if (item)
	{
		mSidepanelObjectInfo->reset();
		mSidepanelObjectInfo->setItemID(item->getUUID());
		toggleObjectInfoPanel(TRUE);
	}
}

void LLSidepanelInventory::onShareButtonClicked()
{
}

void LLSidepanelInventory::performActionOnSelection(const std::string &action)
{
	LLInventoryPanel *panel = mPanelMainInventory->getActivePanel();
	LLFolderViewItem* current_item = panel->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return;
	}
	current_item->getListener()->performAction(panel->getRootFolder(), panel->getModel(), action);
}

void LLSidepanelInventory::onWearButtonClicked()
{
	performActionOnSelection("wear");
	performActionOnSelection("attach");
}

void LLSidepanelInventory::onPlayButtonClicked()
{
	performActionOnSelection("activate");
}

void LLSidepanelInventory::onTeleportButtonClicked()
{
	performActionOnSelection("teleport");
}

void LLSidepanelInventory::onOverflowButtonClicked()
{
}

void LLSidepanelInventory::onBackButtonClicked()
{
	toggleObjectInfoPanel(FALSE);
	updateVerbs();
}

void LLSidepanelInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	updateVerbs();
}

void LLSidepanelInventory::toggleObjectInfoPanel(BOOL visible)
{
	mSidepanelObjectInfo->setVisible(visible);
	mTabContainer->setVisible(!visible);

	if (visible)
	{
		mSidepanelObjectInfo->reset();
		mSidepanelObjectInfo->setEditMode(FALSE);

		LLRect rect = getRect();
		LLRect new_rect = LLRect(rect.mLeft, rect.mTop, rect.mRight, mTabContainer->getRect().mBottom);
		mSidepanelObjectInfo->reshape(new_rect.getWidth(),new_rect.getHeight());
	}
}

void LLSidepanelInventory::updateVerbs()
{
	mInfoBtn->setEnabled(FALSE);
	mShareBtn->setEnabled(FALSE);

	mWearBtn->setVisible(FALSE);
	mWearBtn->setEnabled(FALSE);
	mPlayBtn->setVisible(FALSE);
	mPlayBtn->setEnabled(FALSE);
 	mTeleportBtn->setVisible(FALSE);
 	mTeleportBtn->setEnabled(FALSE);
	
	const LLInventoryItem *item = getSelectedItem();
	if (!item)
		return;

	mInfoBtn->setEnabled(TRUE);
	mShareBtn->setEnabled(TRUE);

	switch(item->getInventoryType())
	{
		case LLInventoryType::IT_WEARABLE:
		case LLInventoryType::IT_OBJECT:
		case LLInventoryType::IT_ATTACHMENT:
			mWearBtn->setVisible(TRUE);
			mWearBtn->setEnabled(TRUE);
			break;
		case LLInventoryType::IT_SOUND:
		case LLInventoryType::IT_GESTURE:
		case LLInventoryType::IT_ANIMATION:
			mPlayBtn->setVisible(TRUE);
			mPlayBtn->setEnabled(TRUE);
			break;
		case LLInventoryType::IT_LANDMARK:
			mTeleportBtn->setVisible(TRUE);
			mTeleportBtn->setEnabled(TRUE);
			break;
		default:
			break;
	}
}

LLInventoryItem *LLSidepanelInventory::getSelectedItem()
{
	LLFolderViewItem* current_item = mPanelMainInventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return NULL;
	}
	const LLUUID &item_id = current_item->getListener()->getUUID();
	LLInventoryItem *item = gInventory.getItem(item_id);
	return item;
}
