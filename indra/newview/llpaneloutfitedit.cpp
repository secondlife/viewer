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
#include "llinventory.h"
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

class LLInventoryLookObserver : public LLInventoryObserver
{
public:
	LLInventoryLookObserver(LLPanelOutfitEdit *panel) : mPanel(panel) {}
	virtual ~LLInventoryLookObserver() 
	{
		if (gInventory.containsObserver(this))
		{
			gInventory.removeObserver(this);
		}
	}
	
	virtual void changed(U32 mask)
	{
		if (mask & (LLInventoryObserver::ADD | LLInventoryObserver::REMOVE))
		{
			mPanel->updateLookInfo();
		}
	}
protected:
	LLPanelOutfitEdit *mPanel;
};

class LLLookFetchObserver : public LLInventoryFetchDescendentsObserver
{
public:
	LLLookFetchObserver(LLPanelOutfitEdit *panel) :
	mPanel(panel)
	{}
	LLLookFetchObserver() {}
	virtual void done()
	{
		mPanel->lookFetched();
		if(gInventory.containsObserver(this))
		{
			gInventory.removeObserver(this);
		}
	}
private:
	LLPanelOutfitEdit *mPanel;
};



LLPanelOutfitEdit::LLPanelOutfitEdit()
:	LLPanel(), mCurrentOutfitID(), mFetchLook(NULL), mSearchFilter(NULL),
mLookContents(NULL), mInventoryItemsPanel(NULL), mAddToOutfitBtn(NULL),
mRemoveFromOutfitBtn(NULL), mLookObserver(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
	
	mFetchLook = new LLLookFetchObserver(this);
	mLookObserver = new LLInventoryLookObserver(this);
	gInventory.addObserver(mLookObserver);
	
	mLookItemTypes.reserve(NUM_LOOK_ITEM_TYPES);
	for (U32 i = 0; i < NUM_LOOK_ITEM_TYPES; i++)
	{
		mLookItemTypes.push_back(LLLookItemType());
	}
	
	// TODO: make these strings translatable
	mLookItemTypes[LIT_ALL] = LLLookItemType("All Items", ALL_ITEMS_MASK);
	mLookItemTypes[LIT_WEARABLE] = LLLookItemType("Shape & Clothing", WEARABLE_MASK);
	mLookItemTypes[LIT_ATTACHMENT] = LLLookItemType("Attachments", ATTACHMENT_MASK);

}

LLPanelOutfitEdit::~LLPanelOutfitEdit()
{
	delete mSavedFolderState;
	if (gInventory.containsObserver(mFetchLook))
	{
		gInventory.removeObserver(mFetchLook);
	}
	delete mFetchLook;
	
	if (gInventory.containsObserver(mLookObserver))
	{
		gInventory.removeObserver(mLookObserver);
	}
	delete mLookObserver;
}

BOOL LLPanelOutfitEdit::postBuild()
{
	// gInventory.isInventoryUsable() no longer needs to be tested per Richard's fix for race conditions between inventory and panels
		
	mCurrentOutfitName = getChild<LLTextBox>("curr_outfit_name"); 

	childSetCommitCallback("add_btn", boost::bind(&LLPanelOutfitEdit::showAddWearablesPanel, this), NULL);

	mLookContents = getChild<LLScrollListCtrl>("look_items_list");
	mLookContents->sortByColumn("look_item_sort", TRUE);
	mLookContents->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onOutfitItemSelectionChange, this));

	mInventoryItemsPanel = getChild<LLInventoryPanel>("inventory_items");
	mInventoryItemsPanel->setFilterTypes(ALL_ITEMS_MASK);
	mInventoryItemsPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryItemsPanel->setSelectCallback(boost::bind(&LLPanelOutfitEdit::onInventorySelectionChange, this, _1, _2));
	mInventoryItemsPanel->getRootFolder()->setReshapeCallback(boost::bind(&LLPanelOutfitEdit::onInventorySelectionChange, this, _1, _2));
	
	LLComboBox* type_filter = getChild<LLComboBox>("inventory_filter");
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
	
	childSetAction("add_to_outfit_btn", boost::bind(&LLPanelOutfitEdit::onAddToOutfitClicked, this));
	childSetEnabled("add_to_outfit_btn", false);

	mUpBtn = getChild<LLButton>("up_btn");
	mUpBtn->setEnabled(TRUE);
	mUpBtn->setClickedCallback(boost::bind(&LLPanelOutfitEdit::onUpClicked, this));
	
	//*TODO rename mLookContents to mOutfitContents
	mLookContents = getChild<LLScrollListCtrl>("look_items_list");
	mLookContents->sortByColumn("look_item_sort", TRUE);
	mLookContents->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onOutfitItemSelectionChange, this));

	mRemoveFromOutfitBtn = getChild<LLButton>("remove_from_outfit_btn");
	mRemoveFromOutfitBtn->setEnabled(FALSE);
	mRemoveFromOutfitBtn->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onRemoveFromOutfitClicked, this));

	mEditWearableBtn = getChild<LLButton>("edit_wearable_btn");
	mEditWearableBtn->setEnabled(FALSE);
	mEditWearableBtn->setVisible(FALSE);
	mEditWearableBtn->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onEditWearableClicked, this));

	childSetAction("revert_btn", boost::bind(&LLAppearanceMgr::wearBaseOutfit, LLAppearanceMgr::getInstance()));

	childSetAction("save_btn", boost::bind(&LLPanelOutfitEdit::saveOutfit, this, false));
	childSetAction("save_as_btn", boost::bind(&LLPanelOutfitEdit::saveOutfit, this, true));
	childSetAction("save_flyout_btn", boost::bind(&LLPanelOutfitEdit::showSaveMenu, this));

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar save_registar;
	save_registar.add("Outfit.Save.Action", boost::bind(&LLPanelOutfitEdit::saveOutfit, this, false));
	save_registar.add("Outfit.SaveAsNew.Action", boost::bind(&LLPanelOutfitEdit::saveOutfit, this, true));
	mSaveMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_save_outfit.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	return TRUE;
}

void LLPanelOutfitEdit::showAddWearablesPanel()
{
	childSetVisible("add_wearables_panel", childGetValue("add_btn"));
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

	if (LLAppearanceMgr::getInstance()->wearItemOnAvatar(listenerp->getUUID()))
	{
		updateLookInfo();
	}
}


void LLPanelOutfitEdit::onRemoveFromOutfitClicked(void)
{
	LLUUID id_to_remove = mLookContents->getSelectionInterface()->getCurrentID();
	
	LLAppearanceMgr::getInstance()->removeItemFromAvatar(id_to_remove);

	updateLookInfo();

	mRemoveFromOutfitBtn->setEnabled(FALSE);
}


void LLPanelOutfitEdit::onUpClicked(void)
{
	LLUUID inv_id = mLookContents->getSelectionInterface()->getCurrentID();
	if (inv_id.isNull())
	{
		//nothing selected, do nothing
		return;
	}

	LLViewerInventoryItem *link_item = gInventory.getItem(inv_id);
	if (!link_item)
	{
		llwarns << "could not find inventory item based on currently worn link." << llendl;
		return;
	}


	LLUUID asset_id = link_item->getAssetUUID();
	if (asset_id.isNull())
	{
		llwarns << "inventory link has null Asset ID. could not get object reference" << llendl;
	}

	static const std::string empty = "";
	LLWearableList::instance().getAsset(asset_id,
										empty,	// don't care about wearable name
										link_item->getActualType(),
										LLSidepanelAppearance::editWearable,
										(void*)getParentUICtrl());
}


void LLPanelOutfitEdit::onEditWearableClicked(void)
{
	LLUUID id_to_edit = mLookContents->getSelectionInterface()->getCurrentID();
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
		childSetEnabled("add_to_outfit_btn", true);
		break;
	default:
		childSetEnabled("add_to_outfit_btn", false);
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
	S32 left_offset = -4;
	S32 top_offset = -10;
	LLScrollListItem* item = mLookContents->getLastSelectedItem();
	if (!item)
		return;

	LLRect rect = item->getRect();
	LLRect btn_rect(
					left_offset + rect.mRight - 50,
					top_offset  + rect.mTop,
					left_offset + rect.mRight - 30,
					top_offset  + rect.mBottom);
	
	mEditWearableBtn->setRect(btn_rect);
	
	mEditWearableBtn->setEnabled(TRUE);
	if (!mEditWearableBtn->getVisible())
	{
		mEditWearableBtn->setVisible(TRUE);
	}


	const LLUUID& id_item_to_remove = item->getUUID();
	LLViewerInventoryItem* item_to_remove = gInventory.getItem(id_item_to_remove);
	if (!item_to_remove) return;

	switch (item_to_remove->getType())
	{
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_OBJECT:
		mRemoveFromOutfitBtn->setEnabled(TRUE);
		break;
	default:
		mRemoveFromOutfitBtn->setEnabled(FALSE);
		break;
	}
}

void LLPanelOutfitEdit::changed(U32 mask)
{
}

void LLPanelOutfitEdit::lookFetched(void)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;

	// collectDescendentsIf takes non-const reference:
	LLFindCOFValidItems is_cof_valid;
	gInventory.collectDescendentsIf(mCurrentOutfitID,
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									is_cof_valid);
	for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		const LLViewerInventoryItem *item = (*iter);
		
		LLSD row;
		row["id"] = item->getUUID();
		LLSD& columns = row["columns"];
		columns[0]["column"] = "look_item";
		columns[0]["type"] = "text";
		columns[0]["value"] = item->getName();
		columns[1]["column"] = "look_item_sort";
		columns[1]["type"] = "text"; // TODO: multi-wearable sort "type" should go here.
		columns[1]["value"] = "BAR"; // TODO: Multi-wearable sort index should go here
		
		mLookContents->addElement(row);
	}
}

void LLPanelOutfitEdit::updateLookInfo()
{	
	if (getVisible())
	{
		mLookContents->clearRows();

		mFetchLook->setFetchID(mCurrentOutfitID);
		mFetchLook->startFetch();
		if (mFetchLook->isFinished())
		{
			mFetchLook->done();
		}
		else
		{
			gInventory.addObserver(mFetchLook);
		}
	}
}

void LLPanelOutfitEdit::displayCurrentOutfit()
{
	if (!getVisible())
	{
		setVisible(TRUE);
	}

	mCurrentOutfitID = LLAppearanceMgr::getInstance()->getCOF();

	std::string current_outfit_name;
	if (LLAppearanceMgr::getInstance()->getBaseOutfitName(current_outfit_name))
	{
		mCurrentOutfitName->setText(current_outfit_name);
	}
	else
	{
		mCurrentOutfitName->setText(getString("No Outfit"));
	}

	updateLookInfo();
}


