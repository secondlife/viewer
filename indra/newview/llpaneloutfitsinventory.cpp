/**
 * @file llpaneloutfitsinventory.cpp
 * @brief Outfits inventory panel
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

#include "llpaneloutfitsinventory.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lllandmark.h"

#include "llfloaterworldmap.h"
#include "llfloaterinventory.h"
#include "llfoldervieweventlistener.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llsidetray.h"
#include "lltabcontainer.h"
#include "llagentwearables.h"
#include "llviewerjointattachment.h"
#include "llviewerfoldertype.h"
#include "llvoavatarself.h"

static LLRegisterPanelClassWrapper<LLPanelOutfitsInventory> t_inventory("panel_outfits_inventory");

LLPanelOutfitsInventory::LLPanelOutfitsInventory() :
	mInventoryPanel(NULL),
	mActionBtn(NULL),
	mWearBtn(NULL),
	mEditBtn(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);

	// LLUICtrlFactory::getInstance()->buildPanel(this, "panel_outfits_inventory.xml");
}

LLPanelOutfitsInventory::~LLPanelOutfitsInventory()
{
	delete mSavedFolderState;
}

// virtual
BOOL LLPanelOutfitsInventory::postBuild()
{
	mInventoryPanel = getChild<LLInventoryPanel>("outfits_list");
	mInventoryPanel->setFilterTypes(1LL << LLFolderType::FT_OUTFIT, TRUE);
	mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryPanel->openDefaultFolderForType(LLFolderType::FT_MY_OUTFITS);
	mInventoryPanel->setSelectCallback(boost::bind(&LLPanelOutfitsInventory::onSelectionChange, this, _1, _2));
	
	LLFolderView* root_folder = getRootFolder();
	root_folder->setReshapeCallback(boost::bind(&LLPanelOutfitsInventory::onSelectionChange, this, _1, _2));
	
	mWearBtn = getChild<LLButton>("wear_btn");
	mEditBtn = getChild<LLButton>("edit_btn");
	mActionBtn = getChild<LLButton>("action_btn");
	
	return TRUE;
}

// virtual
void LLPanelOutfitsInventory::onSearchEdit(const std::string& string)
{
	if (string == "")
	{
		mInventoryPanel->setFilterSubString(LLStringUtil::null);

		// re-open folders that were initially open
		mSavedFolderState->setApply(TRUE);
		getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		getRootFolder()->applyFunctorRecursively(opener);
		getRootFolder()->scrollToShowSelection();
	}

	gInventory.startBackgroundFetch();

	if (mInventoryPanel->getFilterSubString().empty() && string.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}

	// save current folder open state if no filter currently applied
	if (getRootFolder()->getFilterSubString().empty())
	{
		mSavedFolderState->setApply(FALSE);
		getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}

	// set new filter string
	mInventoryPanel->setFilterSubString(string);
}

void LLPanelOutfitsInventory::onWear()
{
	LLFolderViewItem* current_item = getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (getIsCorrectType(listenerp))
	{
		listenerp->performAction(NULL, NULL,"replaceoutfit");
	}
}

void LLPanelOutfitsInventory::onEdit()
{
}

void LLPanelOutfitsInventory::onNew()
{
	const std::string& outfit_name = LLViewerFolderType::lookupNewCategoryName(LLFolderType::FT_OUTFIT);
	LLUUID outfit_folder = gAgentWearables.makeNewOutfitLinks(outfit_name);
	getRootFolder()->setSelectionByID(outfit_folder, TRUE);
	getRootFolder()->setNeedsAutoRename(TRUE);
}

void LLPanelOutfitsInventory::updateVerbs()
{
	BOOL enabled = FALSE;

	LLFolderViewItem* current_item = getRootFolder()->getCurSelectedItem();
	if (current_item)
	{
		LLFolderViewEventListener* listenerp = current_item->getListener();
		if (getIsCorrectType(listenerp))
		{
			enabled = TRUE;
		}
	}

	if (mWearBtn)
	{
		mWearBtn->setEnabled(enabled);
	}
	// mEditBtn->setEnabled(enabled);
}

void LLPanelOutfitsInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	LLFolderViewItem* current_item = getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return;
	
	/*
	  LLFolderViewEventListener* listenerp = current_item->getListener();
	  if (getIsCorrectType(listenerp))
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
	*/

	updateVerbs();
}

void LLPanelOutfitsInventory::onSelectorButtonClicked()
{
	/*
	  LLFolderViewItem* cur_item = getRootFolder()->getCurSelectedItem();

	  LLFolderViewEventListener* listenerp = cur_item->getListener();
	  if (getIsCorrectType(listenerp))
	  {
	  LLSD key;
	  key["type"] = "look";
	  key["id"] = listenerp->getUUID();

	  LLSideTray::getInstance()->showPanel("sidepanel_appearance", key);
	  } 
	*/
}

bool LLPanelOutfitsInventory::getIsCorrectType(const LLFolderViewEventListener *listenerp) const
{
	if (listenerp->getInventoryType() == LLInventoryType::IT_CATEGORY)
	{
		LLViewerInventoryCategory *cat = gInventory.getCategory(listenerp->getUUID());
		if (cat && cat->getPreferredType() == LLFolderType::FT_OUTFIT)
		{
			return true;
		}
	}
	return false;
}

LLFolderView *LLPanelOutfitsInventory::getRootFolder()
{
	return mInventoryPanel->getRootFolder();
}
