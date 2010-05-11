/**
 * @file llpaneloutfitedit.cpp
 * @brief Displays outfit edit information in Side Tray.
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

#include "llpaneloutfitedit.h"

// *TODO: reorder includes to match the coding standard
#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llcofwearables.h"
#include "llfilteredwearablelist.h"
#include "llinventory.h"
#include "llinventoryitemslist.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llviewermenu.h"
#include "llviewerwindow.h"
#include "llviewerinventory.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfloaterinventory.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llpaneloutfitsinventory.h"
#include "lluiconstants.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llsdutil.h"
#include "llsidepanelappearance.h"
#include "lltoggleablemenu.h"
#include "llwearablelist.h"

static LLRegisterPanelClassWrapper<LLPanelOutfitEdit> t_outfit_edit("panel_outfit_edit");

const U64 WEARABLE_MASK = (1LL << LLInventoryType::IT_WEARABLE);
const U64 ATTACHMENT_MASK = (1LL << LLInventoryType::IT_ATTACHMENT) | (1LL << LLInventoryType::IT_OBJECT);
const U64 ALL_ITEMS_MASK = WEARABLE_MASK | ATTACHMENT_MASK;

static const std::string SAVE_BTN("save_btn");
static const std::string REVERT_BTN("revert_btn");

class LLCOFObserver : public LLInventoryObserver
{
public:
	LLCOFObserver(LLPanelOutfitEdit *panel) : mPanel(panel), 
		mCOFLastVersion(LLViewerInventoryCategory::VERSION_UNKNOWN)
	{
		gInventory.addObserver(this);
	}

	virtual ~LLCOFObserver()
	{
		if (gInventory.containsObserver(this))
		{
			gInventory.removeObserver(this);
		}
	}
	
	virtual void changed(U32 mask)
	{
		if (!gInventory.isInventoryUsable()) return;
	
		LLUUID cof = LLAppearanceMgr::getInstance()->getCOF();
		if (cof.isNull()) return;

		LLViewerInventoryCategory* cat = gInventory.getCategory(cof);
		if (!cat) return;

		S32 cof_version = cat->getVersion();

		if (cof_version == mCOFLastVersion) return;

		mCOFLastVersion = cof_version;

		mPanel->update();
	}

protected:
	LLPanelOutfitEdit *mPanel;

	//last version number of a COF category
	S32 mCOFLastVersion;
};



LLPanelOutfitEdit::LLPanelOutfitEdit()
:	LLPanel(), 
	mSearchFilter(NULL),
	mCOFWearables(NULL),
	mInventoryItemsPanel(NULL),
	mCOFObserver(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
	
	mCOFObserver = new LLCOFObserver(this);
	
	mLookItemTypes.reserve(NUM_LOOK_ITEM_TYPES);
	for (U32 i = 0; i < NUM_LOOK_ITEM_TYPES; i++)
	{
		mLookItemTypes.push_back(LLLookItemType());
	}
	

}

LLPanelOutfitEdit::~LLPanelOutfitEdit()
{
	delete mSavedFolderState;

	delete mCOFObserver;
}

BOOL LLPanelOutfitEdit::postBuild()
{
	// gInventory.isInventoryUsable() no longer needs to be tested per Richard's fix for race conditions between inventory and panels

	mLookItemTypes[LIT_ALL] = LLLookItemType(getString("Filter.All"), ALL_ITEMS_MASK);
	mLookItemTypes[LIT_WEARABLE] = LLLookItemType(getString("Filter.Clothes/Body"), WEARABLE_MASK);
	mLookItemTypes[LIT_ATTACHMENT] = LLLookItemType(getString("Filter.Objects"), ATTACHMENT_MASK);

	mCurrentOutfitName = getChild<LLTextBox>("curr_outfit_name"); 

	childSetCommitCallback("filter_button", boost::bind(&LLPanelOutfitEdit::showWearablesFilter, this), NULL);
	childSetCommitCallback("list_view_btn", boost::bind(&LLPanelOutfitEdit::showFilteredWearablesPanel, this), NULL);

	mCOFWearables = getChild<LLCOFWearables>("cof_wearables_list");
	mCOFWearables->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onOutfitItemSelectionChange, this));

	mCOFWearables->getCOFCallbacks().mEditWearable = boost::bind(&LLPanelOutfitEdit::onEditWearableClicked, this);
	mCOFWearables->getCOFCallbacks().mDeleteWearable = boost::bind(&LLPanelOutfitEdit::onRemoveFromOutfitClicked, this);
	mCOFWearables->getCOFCallbacks().mMoveWearableCloser = boost::bind(&LLPanelOutfitEdit::moveWearable, this, true);
	mCOFWearables->getCOFCallbacks().mMoveWearableFurther = boost::bind(&LLPanelOutfitEdit::moveWearable, this, false);

	mCOFWearables->childSetAction("add_btn", boost::bind(&LLPanelOutfitEdit::toggleAddWearablesPanel, this));


	mInventoryItemsPanel = getChild<LLInventoryPanel>("inventory_items");
	mInventoryItemsPanel->setFilterTypes(ALL_ITEMS_MASK);
	mInventoryItemsPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryItemsPanel->setSelectCallback(boost::bind(&LLPanelOutfitEdit::onInventorySelectionChange, this, _1, _2));
	mInventoryItemsPanel->getRootFolder()->setReshapeCallback(boost::bind(&LLPanelOutfitEdit::onInventorySelectionChange, this, _1, _2));
	
	LLComboBox* type_filter = getChild<LLComboBox>("filter_wearables_combobox");
	type_filter->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onTypeFilterChanged, this, _1));
	type_filter->removeall();
	for (U32 i = 0; i < mLookItemTypes.size(); ++i)
	{
		type_filter->add(mLookItemTypes[i].displayName);
	}
	type_filter->setCurrentByIndex(LIT_ALL);
	
	mSearchFilter = getChild<LLFilterEditor>("look_item_filter");
	mSearchFilter->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onSearchEdit, this, _2));
	
	/* Removing add to look inline button (not part of mvp for viewer 2)
	LLButton::Params add_params;
	add_params.name("add_to_look");
	add_params.click_callback.function(boost::bind(&LLPanelOutfitEdit::onAddToLookClicked, this));
	add_params.label("+");
	
	mAddToLookBtn = LLUICtrlFactory::create<LLButton>(add_params);
	mAddToLookBtn->setEnabled(FALSE);
	mAddToLookBtn->setVisible(FALSE); */
	
	mEditWearableBtn = getChild<LLButton>("edit_wearable_btn");
	mEditWearableBtn->setEnabled(FALSE);
	mEditWearableBtn->setVisible(FALSE);
	mEditWearableBtn->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onEditWearableClicked, this));

	childSetAction(REVERT_BTN, boost::bind(&LLAppearanceMgr::wearBaseOutfit, LLAppearanceMgr::getInstance()));

	childSetAction(SAVE_BTN, boost::bind(&LLPanelOutfitEdit::saveOutfit, this, false));
	childSetAction("save_flyout_btn", boost::bind(&LLPanelOutfitEdit::showSaveMenu, this));

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar save_registar;
	save_registar.add("Outfit.Save.Action", boost::bind(&LLPanelOutfitEdit::saveOutfit, this, false));
	save_registar.add("Outfit.SaveAsNew.Action", boost::bind(&LLPanelOutfitEdit::saveOutfit, this, true));
	mSaveMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_save_outfit.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	mWearableListManager = new LLFilteredWearableListManager(
		getChild<LLInventoryItemsList>("filtered_wearables_list"), ALL_ITEMS_MASK);

	return TRUE;
}

void LLPanelOutfitEdit::moveWearable(bool closer_to_body)
{
	LLUUID item_id = mCOFWearables->getSelectedUUID();
	if (item_id.isNull()) return;
	
	LLViewerInventoryItem* wearable_to_move = gInventory.getItem(item_id);
	LLAppearanceMgr::getInstance()->moveWearable(wearable_to_move, closer_to_body);
}

void LLPanelOutfitEdit::toggleAddWearablesPanel()
{
	childSetVisible("add_wearables_panel", !childIsVisible("add_wearables_panel"));
}

void LLPanelOutfitEdit::showWearablesFilter()
{
	childSetVisible("filter_combobox_panel", childGetValue("filter_button"));
}

void LLPanelOutfitEdit::showFilteredWearablesPanel()
{
	childSetVisible("filtered_wearables_panel", !childIsVisible("filtered_wearables_panel"));
}

void LLPanelOutfitEdit::saveOutfit(bool as_new)
{
	if (!as_new && LLAppearanceMgr::getInstance()->updateBaseOutfit())
	{
		// we don't need to ask for an outfit name, and updateBaseOutfit() successfully saved.
		// If updateBaseOutfit fails, ask for an outfit name anyways
		return;
	}

	LLPanelOutfitsInventory* panel_outfits_inventory = LLPanelOutfitsInventory::findInstance();
	if (panel_outfits_inventory)
	{
		panel_outfits_inventory->onSave();
	}

	//*TODO how to get to know when base outfit is updated or new outfit is created?
}

void LLPanelOutfitEdit::showSaveMenu()
{
	S32 x, y;
	LLUI::getMousePositionLocal(this, &x, &y);

	mSaveMenu->updateParent(LLMenuGL::sMenuContainer);
	LLMenuGL::showPopup(this, mSaveMenu, x, y);
}

void LLPanelOutfitEdit::onTypeFilterChanged(LLUICtrl* ctrl)
{
	LLComboBox* type_filter = dynamic_cast<LLComboBox*>(ctrl);
	llassert(type_filter);
	if (type_filter)
	{
		U32 curr_filter_type = type_filter->getCurrentIndex();
		mInventoryItemsPanel->setFilterTypes(mLookItemTypes[curr_filter_type].inventoryMask);
		mWearableListManager->setFilterMask(mLookItemTypes[curr_filter_type].inventoryMask);
	}
	
	mSavedFolderState->setApply(TRUE);
	mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	
	LLOpenFoldersWithSelection opener;
	mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(opener);
	mInventoryItemsPanel->getRootFolder()->scrollToShowSelection();
	
	LLInventoryModelBackgroundFetch::instance().start();
}

void LLPanelOutfitEdit::onSearchEdit(const std::string& string)
{
	if (mSearchString != string)
	{
		mSearchString = string;
		
		// Searches are case-insensitive
		LLStringUtil::toUpper(mSearchString);
		LLStringUtil::trimHead(mSearchString);
	}
	
	if (mSearchString == "")
	{
		mInventoryItemsPanel->setFilterSubString(LLStringUtil::null);
		
		// re-open folders that were initially open
		mSavedFolderState->setApply(TRUE);
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(opener);
		mInventoryItemsPanel->getRootFolder()->scrollToShowSelection();
	}
	
	LLInventoryModelBackgroundFetch::instance().start();
	
	if (mInventoryItemsPanel->getFilterSubString().empty() && mSearchString.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}
	
	// save current folder open state if no filter currently applied
	if (mInventoryItemsPanel->getRootFolder()->getFilterSubString().empty())
	{
		mSavedFolderState->setApply(FALSE);
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}
	
	// set new filter string
	mInventoryItemsPanel->setFilterSubString(mSearchString);
}

void LLPanelOutfitEdit::onAddToOutfitClicked(void)
{
	LLFolderViewItem* curr_item = mInventoryItemsPanel->getRootFolder()->getCurSelectedItem();
	if (!curr_item) return;

	LLFolderViewEventListener* listenerp  = curr_item->getListener();
	if (!listenerp) return;

	LLAppearanceMgr::getInstance()->wearItemOnAvatar(listenerp->getUUID());
}


void LLPanelOutfitEdit::onRemoveFromOutfitClicked(void)
{
	LLUUID id_to_remove = mCOFWearables->getSelectedUUID();
	
	LLAppearanceMgr::getInstance()->removeItemFromAvatar(id_to_remove);
}


void LLPanelOutfitEdit::onEditWearableClicked(void)
{
	LLUUID id_to_edit = mCOFWearables->getSelectedUUID();
	LLViewerInventoryItem * item_to_edit = gInventory.getItem(id_to_edit);

	if (item_to_edit)
	{
		// returns null if not a wearable (attachment, etc).
		LLWearable* wearable_to_edit = gAgentWearables.getWearableFromAssetID(item_to_edit->getAssetUUID());
		if(wearable_to_edit)
		{
			bool can_modify = false;
			bool is_complete = item_to_edit->isFinished();
			// if item_to_edit is a link, its properties are not appropriate, 
			// lets get original item with actual properties
			LLViewerInventoryItem* original_item = gInventory.getItem(wearable_to_edit->getItemID());
			if(original_item)
			{
				can_modify = original_item->getPermissions().allowModifyBy(gAgentID);
				is_complete = original_item->isFinished();
			}

			if (can_modify && is_complete)
			{											 
				LLSidepanelAppearance::editWearable(wearable_to_edit, getParent());
				if (mEditWearableBtn->getVisible())
				{
					mEditWearableBtn->setVisible(FALSE);
				}
			}
		}
	}
}

void LLPanelOutfitEdit::onInventorySelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	LLFolderViewItem* current_item = mInventoryItemsPanel->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return;
	}

	LLViewerInventoryItem* item = current_item->getInventoryItem();
	if (!item) return;

	switch (item->getType())
	{
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_OBJECT:
	default:
		break;
	}
	
	/* Removing add to look inline button (not part of mvp for viewer 2)
	LLRect btn_rect(current_item->getLocalRect().mRight - 50,
					current_item->getLocalRect().mTop,
					current_item->getLocalRect().mRight - 30,
					current_item->getLocalRect().mBottom);
	
	mAddToLookBtn->setRect(btn_rect);
	mAddToLookBtn->setEnabled(TRUE);
	if (!mAddToLookBtn->getVisible())
	{
		mAddToLookBtn->setVisible(TRUE);
	}
	
	current_item->addChild(mAddToLookBtn); */
}

void LLPanelOutfitEdit::onOutfitItemSelectionChange(void)
{	
	LLUUID item_id = mCOFWearables->getSelectedUUID();

	//*TODO show Edit Wearable Button

	LLViewerInventoryItem* item_to_remove = gInventory.getItem(item_id);
	if (!item_to_remove) return;

	switch (item_to_remove->getType())
	{
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_OBJECT:
	default:
		break;
	}
}

void LLPanelOutfitEdit::update()
{
	mCOFWearables->refresh();

	updateVerbs();
}

void LLPanelOutfitEdit::displayCurrentOutfit()
{
	if (!getVisible())
	{
		setVisible(TRUE);
	}

	std::string current_outfit_name;
	if (LLAppearanceMgr::getInstance()->getBaseOutfitName(current_outfit_name))
	{
		mCurrentOutfitName->setText(current_outfit_name);
	}
	else
	{
		mCurrentOutfitName->setText(getString("No Outfit"));
	}

	update();
}

//private
void LLPanelOutfitEdit::updateVerbs()
{
	//*TODO implement better handling of COF dirtiness
	LLAppearanceMgr::getInstance()->updateIsDirty();

	bool outfit_is_dirty = LLAppearanceMgr::getInstance()->isOutfitDirty();
	
	childSetEnabled(SAVE_BTN, outfit_is_dirty);
	childSetEnabled(REVERT_BTN, outfit_is_dirty);

	mSaveMenu->setItemEnabled("save_outfit", outfit_is_dirty);
}

// EOF
