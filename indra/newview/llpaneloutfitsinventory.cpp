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
#include "llagentwearables.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llfloaterinventory.h"
#include "llfoldervieweventlistener.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "lllandmark.h"
#include "llsidepanelappearance.h"
#include "llsidetray.h"
#include "lltabcontainer.h"
#include "llviewerfoldertype.h"
#include "llviewerjointattachment.h"
#include "llvoavatarself.h"

// List Commands
#include "lldndbutton.h"
#include "llmenugl.h"
#include "llviewermenu.h"

static LLRegisterPanelClassWrapper<LLPanelOutfitsInventory> t_inventory("panel_outfits_inventory");

LLPanelOutfitsInventory::LLPanelOutfitsInventory() :
	mInventoryPanel(NULL),
	mParent(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
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
	
	initListCommandsHandlers();
	return TRUE;
}

void LLPanelOutfitsInventory::updateParent()
{
	if (mParent)
	{
		mParent->updateVerbs();
	}
}

void LLPanelOutfitsInventory::setParent(LLSidepanelAppearance* parent)
{
	mParent = parent;
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
	LLFolderViewEventListener* listenerp = getCorrectListenerForAction();
	if (listenerp)
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
	/*
	getRootFolder()->setSelectionByID(outfit_folder, TRUE);
	getRootFolder()->setNeedsAutoRename(TRUE);
	getRootFolder()->startRenamingSelectedItem();
	*/
}

void LLPanelOutfitsInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	updateListCommands();
	updateParent();
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

LLFolderViewEventListener *LLPanelOutfitsInventory::getCorrectListenerForAction()
{
	LLFolderViewItem* current_item = getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return NULL;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (getIsCorrectType(listenerp))
	{
		return listenerp;
	}
	return NULL;
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

//////////////////////////////////////////////////////////////////////////////////
// List Commands                                                                //

void LLPanelOutfitsInventory::initListCommandsHandlers()
{
	mListCommands = getChild<LLPanel>("bottom_panel");

	mListCommands->childSetAction("options_gear_btn", boost::bind(&LLPanelOutfitsInventory::onGearButtonClick, this));
	mListCommands->childSetAction("trash_btn", boost::bind(&LLPanelOutfitsInventory::onTrashButtonClick, this));
	mListCommands->childSetAction("add_btn", boost::bind(&LLPanelOutfitsInventory::onAddButtonClick, this));

	LLDragAndDropButton* trash_btn = mListCommands->getChild<LLDragAndDropButton>("trash_btn");
	trash_btn->setDragAndDropHandler(boost::bind(&LLPanelOutfitsInventory::handleDragAndDropToTrash, this
			,	_4 // BOOL drop
			,	_5 // EDragAndDropType cargo_type
			,	_7 // EAcceptance* accept
			));

	mCommitCallbackRegistrar.add("panel_outfits_inventory_gear_default.Custom.Action", boost::bind(&LLPanelOutfitsInventory::onCustomAction, this, _2));
	mEnableCallbackRegistrar.add("panel_outfits_inventory_gear_default.Enable", boost::bind(&LLPanelOutfitsInventory::isActionEnabled, this, _2));
	mMenuGearDefault = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("panel_outfits_inventory_gear_default.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLPanelOutfitsInventory::updateListCommands()
{
	bool trash_enabled = isActionEnabled("delete");

	mListCommands->childSetEnabled("trash_btn", trash_enabled);
}

void LLPanelOutfitsInventory::onGearButtonClick()
{
	showActionMenu(mMenuGearDefault,"options_gear_btn");
}

void LLPanelOutfitsInventory::onAddButtonClick()
{
	onNew();
}

void LLPanelOutfitsInventory::showActionMenu(LLMenuGL* menu, std::string spawning_view_name)
{
	if (menu)
	{
		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLView* spawning_view = getChild<LLView> (spawning_view_name);
		S32 menu_x, menu_y;
		//show menu in co-ordinates of panel
		spawning_view->localPointToOtherView(0, spawning_view->getRect().getHeight(), &menu_x, &menu_y, this);
		menu_y += menu->getRect().getHeight();
		LLMenuGL::showPopup(this, menu, menu_x, menu_y);
	}
}

void LLPanelOutfitsInventory::onTrashButtonClick()
{
	onClipboardAction("delete");
}

void LLPanelOutfitsInventory::onClipboardAction(const LLSD& userdata)
{
	std::string command_name = userdata.asString();
	getActivePanel()->getRootFolder()->doToSelected(getActivePanel()->getModel(),command_name);
}

void LLPanelOutfitsInventory::onCustomAction(const LLSD& userdata)
{
	if (!isActionEnabled(userdata))
		return;

	const std::string command_name = userdata.asString();
	if (command_name == "new")
	{
		onNew();
	}
	if (command_name == "edit")
	{
		onEdit();
	}
	if (command_name == "wear")
	{
		onWear();
	}
	if (command_name == "delete")
	{
		onClipboardAction("delete");
	}
}

BOOL LLPanelOutfitsInventory::isActionEnabled(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "delete")
	{
		BOOL can_delete = FALSE;
		LLFolderView *folder = getActivePanel()->getRootFolder();
		if (folder)
		{
			can_delete = TRUE;
			std::set<LLUUID> selection_set;
			folder->getSelectionList(selection_set);
			for (std::set<LLUUID>::iterator iter = selection_set.begin();
				 iter != selection_set.end();
				 ++iter)
			{
				const LLUUID &item_id = (*iter);
				LLFolderViewItem *item = folder->getItemByID(item_id);
				can_delete &= item->getListener()->isItemRemovable();
			}
			return can_delete;
		}
		return FALSE;
	}
	if (command_name == "edit" || 
		command_name == "wear")
	{
		return (getCorrectListenerForAction() != NULL);
	}
	return TRUE;
}

bool LLPanelOutfitsInventory::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept)
{
	*accept = ACCEPT_NO;

	const bool is_enabled = isActionEnabled("delete");
	if (is_enabled) *accept = ACCEPT_YES_MULTI;

	if (is_enabled && drop)
	{
		onClipboardAction("delete");
	}
	return true;
}

// List Commands                                                              //
////////////////////////////////////////////////////////////////////////////////

