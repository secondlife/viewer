/**
 * @file llpanellandmarks.cpp
 * @brief Landmarks tab for Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llpanellandmarks.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "lllandmark.h"

#include "llfloaterworldmap.h"
#include "llfloaterinventory.h"
#include "llfoldervieweventlistener.h"
#include "lllandmarklist.h"
#include "llsidetray.h"
#include "lltabcontainer.h"
#include "llworldmap.h"

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLLandmarksPanel> t_landmarks("panel_landmarks");

LLLandmarksPanel::LLLandmarksPanel()
	:	LLPanelPlacesTab(),
		mInventoryPanel(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_landmarks.xml");
}

LLLandmarksPanel::~LLLandmarksPanel()
{
	delete mSavedFolderState;
}

BOOL LLLandmarksPanel::postBuild()
{
	if (!gInventory.isInventoryUsable())
		return FALSE;

	mInventoryPanel = getChild<LLInventoryPanel>("landmarks_list");
	mInventoryPanel->setFilterTypes(0x1 << LLInventoryType::IT_LANDMARK);
	mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryPanel->openDefaultFolderForType(LLAssetType::AT_LANDMARK);
	mInventoryPanel->setSelectCallback(boost::bind(&LLLandmarksPanel::onSelectionChange, this, _1, _2));

	LLFolderView* root_folder = mInventoryPanel->getRootFolder();
	root_folder->setReshapeCallback(boost::bind(&LLLandmarksPanel::onSelectionChange, this, _1, _2));

	mActionBtn = getChild<LLButton>("selector");
	root_folder->addChild(mActionBtn);
	mActionBtn->setEnabled(TRUE);
	childSetAction("selector", boost::bind(&LLLandmarksPanel::onSelectorButtonClicked, this), this);

	return TRUE;
}

// virtual
void LLLandmarksPanel::onSearchEdit(const std::string& string)
{
	if (string == "")
	{
		mInventoryPanel->setFilterSubString(LLStringUtil::null);

		// re-open folders that were initially open
		mSavedFolderState->setApply(TRUE);
		mInventoryPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		mInventoryPanel->getRootFolder()->applyFunctorRecursively(opener);
		mInventoryPanel->getRootFolder()->scrollToShowSelection();
	}

	gInventory.startBackgroundFetch();

	if (mInventoryPanel->getFilterSubString().empty() && string.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}

	// save current folder open state if no filter currently applied
	if (mInventoryPanel->getRootFolder()->getFilterSubString().empty())
	{
		mSavedFolderState->setApply(FALSE);
		mInventoryPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}

	// set new filter string
	mInventoryPanel->setFilterSubString(string);
}

// virtual
void LLLandmarksPanel::onShowOnMap()
{
	LLFolderViewItem* current_item = mInventoryPanel->getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (listenerp->getInventoryType() != LLInventoryType::IT_LANDMARK)
		return;

	LLInventoryItem* inventory_item = gInventory.getItem(listenerp->getUUID());
	if (!inventory_item)
		return;

	LLLandmark* landmark = gLandmarkList.getAsset(inventory_item->getAssetUUID());
	if (!landmark)
		return;

	LLVector3d landmark_global_pos;
	if (!landmark->getGlobalPos(landmark_global_pos))
		return;
	
	LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
	if (!landmark_global_pos.isExactlyZero() && worldmap_instance)
	{
		worldmap_instance->trackLocation(landmark_global_pos);
		LLFloaterReg::showInstance("world_map", "center");
	}
}

// virtual
void LLLandmarksPanel::onTeleport()
{
	LLFolderViewItem* current_item = mInventoryPanel->getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		listenerp->openItem();
	}
}

/*
// virtual
void LLLandmarksPanel::onCopySLURL()
{
	LLFolderViewItem* current_item = mInventoryPanel->getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (listenerp->getInventoryType() != LLInventoryType::IT_LANDMARK)
		return;

	LLInventoryItem* inventory_item = gInventory.getItem(listenerp->getUUID());
	if (!inventory_item)
		return;

	LLLandmark* landmark = gLandmarkList.getAsset(inventory_item->getAssetUUID());
	if (!landmark)
		return;

	LLVector3d landmark_global_pos;
	if (!landmark->getGlobalPos(landmark_global_pos))
		return;

	U64 new_region_handle = to_region_handle(landmark_global_pos);

	LLWorldMap::url_callback_t cb = boost::bind(
			&LLPanelPlacesTab::onRegionResponse, this,
			landmark_global_pos, _1, _2, _3, _4);

	LLWorldMap::getInstance()->sendHandleRegionRequest(new_region_handle, cb, std::string("unused"), false);
}
*/

// virtual
void LLLandmarksPanel::updateVerbs()
{
	if (!isTabVisible()) 
		return;

	BOOL enabled = FALSE;

	LLFolderViewItem* current_item = mInventoryPanel->getRootFolder()->getCurSelectedItem();
	if (current_item)
	{
		LLFolderViewEventListener* listenerp = current_item->getListener();
		if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
		{
			enabled = TRUE;
		}
	}

	mTeleportBtn->setEnabled(enabled);
	mShowOnMapBtn->setEnabled(enabled);
}

void LLLandmarksPanel::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	LLFolderViewItem* current_item = mInventoryPanel->getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		S32 bottom = 0;
		LLFolderViewItem* folder = current_item->getParentFolder();

		while ( folder->getParentFolder() != NULL )
		{
			bottom += folder->getRect().mBottom;
			folder = folder->getParentFolder();
		}

		LLRect rect = current_item->getRect();
		LLRect btn_rect(
			rect.mRight - mActionBtn->getRect().getWidth(),
			bottom + rect.mTop,
			rect.mRight,
			bottom + rect.mBottom);

		mActionBtn->setRect(btn_rect);

		if (!mActionBtn->getVisible())
			mActionBtn->setVisible(TRUE);
	}
	else
	{
		if (mActionBtn->getVisible())
			mActionBtn->setVisible(FALSE);
	}

	updateVerbs();
}

void LLLandmarksPanel::onSelectorButtonClicked()
{
	LLFolderViewItem* cur_item = mInventoryPanel->getRootFolder()->getCurSelectedItem();

	LLFolderViewEventListener* listenerp = cur_item->getListener();
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		LLSD key;
		key["type"] = "landmark";
		key["id"] = listenerp->getUUID();

		LLSideTray::getInstance()->showPanel("panel_places", key);
	}
}

void LLLandmarksPanel::setSelectedItem(const LLUUID& obj_id)
{
	mInventoryPanel->setSelection(obj_id, FALSE);
}
