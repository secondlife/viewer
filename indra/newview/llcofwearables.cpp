/** 
 * @file llcofwearables.cpp
 * @brief LLCOFWearables displayes wearables from the current outfit split into three lists (attachments, clothing and body parts)
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llcofwearables.h"

#include "llagentdata.h"
#include "llappearancemgr.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llwearableitemslist.h"

static LLRegisterPanelClassWrapper<LLCOFAccordionListAdaptor> t_cof_accodion_list_adaptor("accordion_list_adaptor");

static LLRegisterPanelClassWrapper<LLCOFWearables> t_cof_wearables("cof_wearables");

const LLSD REARRANGE = LLSD().with("rearrange", LLSD());


LLCOFWearables::LLCOFWearables() : LLPanel(),
	mAttachments(NULL),
	mClothing(NULL),
	mBodyParts(NULL),
	mLastSelectedList(NULL)
{
};


// virtual
BOOL LLCOFWearables::postBuild()
{
	mAttachments = getChild<LLFlatListView>("list_attachments");
	mClothing = getChild<LLFlatListView>("list_clothing");
	mBodyParts = getChild<LLFlatListView>("list_body_parts");


	//selection across different list/tabs is not supported
	mAttachments->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mAttachments));
	mClothing->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mClothing));
	mBodyParts->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mBodyParts));
	
	mAttachments->setCommitOnSelectionChange(true);
	mClothing->setCommitOnSelectionChange(true);
	mBodyParts->setCommitOnSelectionChange(true);

	return LLPanel::postBuild();
}

void LLCOFWearables::onSelectionChange(LLFlatListView* selected_list)
{
	if (!selected_list) return;

	if (selected_list != mLastSelectedList)
	{
		if (selected_list != mAttachments) mAttachments->resetSelection(true);
		if (selected_list != mClothing) mClothing->resetSelection(true);
		if (selected_list != mBodyParts) mBodyParts->resetSelection(true);

		mLastSelectedList = selected_list;
	}

	onCommit();
}

void LLCOFWearables::refresh()
{
	typedef std::vector<LLSD> values_vector_t;
	typedef std::map<LLFlatListView*, values_vector_t> selection_map_t;

	selection_map_t preserve_selection;

	// Save current selection
	mAttachments->getSelectedValues(preserve_selection[mAttachments]);
	mClothing->getSelectedValues(preserve_selection[mClothing]);
	mBodyParts->getSelectedValues(preserve_selection[mBodyParts]);

	clear();

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t cof_items;

	gInventory.collectDescendents(LLAppearanceMgr::getInstance()->getCOF(), cats, cof_items, LLInventoryModel::EXCLUDE_TRASH);

	populateAttachmentsAndBodypartsLists(cof_items);


	LLAppearanceMgr::wearables_by_type_t clothing_by_type(LLWearableType::WT_COUNT);
	LLAppearanceMgr::getInstance()->divvyWearablesByType(cof_items, clothing_by_type);
	
	populateClothingList(clothing_by_type);

	// Restore previous selection
	for (selection_map_t::iterator
			 iter = preserve_selection.begin(),
			 iter_end = preserve_selection.end();
		 iter != iter_end; ++iter)
	{
		LLFlatListView* list = iter->first;
		const values_vector_t& values = iter->second;
		for (values_vector_t::const_iterator
				 value_it = values.begin(),
				 value_it_end = values.end();
			 value_it != value_it_end; ++value_it)
		{
			list->selectItemByValue(*value_it);
		}
	}
}


void LLCOFWearables::populateAttachmentsAndBodypartsLists(const LLInventoryModel::item_array_t& cof_items)
{
	for (U32 i = 0; i < cof_items.size(); ++i)
	{
		LLViewerInventoryItem* item = cof_items.get(i);
		if (!item) continue;

		const LLAssetType::EType item_type = item->getType();
		if (item_type == LLAssetType::AT_CLOTHING) continue;
		LLPanelInventoryListItemBase* item_panel = NULL;
		if (item_type == LLAssetType::AT_OBJECT)
		{
				item_panel = LLPanelInventoryListItemBase::create(item);
			mAttachments->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}
		else if (item_type == LLAssetType::AT_BODYPART)
		{
			item_panel = buildBodypartListItem(item);
			if (!item_panel) continue;

			mBodyParts->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}
	}

	if (mAttachments->size())
	{
		mAttachments->sort(); //*TODO by Name
		mAttachments->notify(REARRANGE); //notifying the parent about the list's size change (cause items were added with rearrange=false)
	}

	if (mBodyParts->size())
	{
		mBodyParts->sort(); //*TODO by name
	}

	mBodyParts->notify(REARRANGE);
}

//create a clothing list item, update verbs and show/hide line separator
LLPanelClothingListItem* LLCOFWearables::buildClothingListItem(LLViewerInventoryItem* item, bool first, bool last)
{
	llassert(item);

	LLPanelClothingListItem* item_panel = LLPanelClothingListItem::create(item);
	if (!item_panel) return NULL;

	//updating verbs
	//we don't need to use permissions of a link but of an actual/linked item
	if (item->getLinkedItem()) item = item->getLinkedItem();

	bool allow_modify = item->getPermissions().allowModifyBy(gAgentID);
	
	item_panel->setShowLockButton(!allow_modify);
	item_panel->setShowEditButton(allow_modify);

	item_panel->setShowMoveUpButton(!first);
	item_panel->setShowMoveDownButton(!last);

	//setting callbacks
	//*TODO move that item panel's inner structure disclosing stuff into the panels
	item_panel->childSetAction("btn_delete", mCOFCallbacks.mDeleteWearable);
	item_panel->childSetAction("btn_move_up", mCOFCallbacks.mMoveWearableFurther);
	item_panel->childSetAction("btn_move_down", mCOFCallbacks.mMoveWearableCloser);
	item_panel->childSetAction("btn_edit", mCOFCallbacks.mEditWearable);
	
	//turning on gray separator line for the last item in the items group of the same wearable type
	item_panel->childSetVisible("wearable_type_separator_panel", last);

	return item_panel;
}

LLPanelBodyPartsListItem* LLCOFWearables::buildBodypartListItem(LLViewerInventoryItem* item)
{
	llassert(item);

	LLPanelBodyPartsListItem* item_panel = LLPanelBodyPartsListItem::create(item);
	if (!item_panel) return NULL;

	//updating verbs
	//we don't need to use permissions of a link but of an actual/linked item
	if (item->getLinkedItem()) item = item->getLinkedItem();

	bool allow_modify = item->getPermissions().allowModifyBy(gAgentID);
	item_panel->setShowLockButton(!allow_modify);
	item_panel->setShowEditButton(allow_modify);

	//setting callbacks
	//*TODO move that item panel's inner structure disclosing stuff into the panels
	item_panel->childSetAction("btn_delete", mCOFCallbacks.mDeleteWearable);
	item_panel->childSetAction("btn_edit", mCOFCallbacks.mEditWearable);

	return item_panel;
}

void LLCOFWearables::populateClothingList(LLAppearanceMgr::wearables_by_type_t& clothing_by_type)
{
	llassert(clothing_by_type.size() == LLWearableType::WT_COUNT);

	for (U32 type = LLWearableType::WT_SHIRT; type < LLWearableType::WT_COUNT; ++type)
	{
		U32 size = clothing_by_type[type].size();
		if (!size) continue;

		LLAppearanceMgr::sortItemsByActualDescription(clothing_by_type[type]);

		//clothing items are displayed in reverse order, from furthest ones to closest ones (relatively to the body)
		for (U32 i = size; i != 0; --i)
		{
			LLViewerInventoryItem* item = clothing_by_type[type][i-1];

			LLPanelClothingListItem* item_panel = buildClothingListItem(item, i == size, i == 1);
			if (!item_panel) continue;

			mClothing->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}
	}

	addClothingTypesDummies(clothing_by_type);

	mClothing->notify(REARRANGE);
}

//adding dummy items for missing wearable types
void LLCOFWearables::addClothingTypesDummies(const LLAppearanceMgr::wearables_by_type_t& clothing_by_type)
{
	llassert(clothing_by_type.size() == LLWearableType::WT_COUNT);
	
	for (U32 type = LLWearableType::WT_SHIRT; type < LLWearableType::WT_COUNT; type++)
	{
		U32 size = clothing_by_type[type].size();
		if (size) continue;

		LLWearableType::EType w_type = static_cast<LLWearableType::EType>(type);
		LLPanelInventoryListItemBase* item_panel = LLPanelDummyClothingListItem::create(w_type);
		if(!item_panel) continue;
		mClothing->addItem(item_panel, LLUUID::null, ADD_BOTTOM, false);
	}
}

LLUUID LLCOFWearables::getSelectedUUID()
{
	if (!mLastSelectedList) return LLUUID::null;
	
	return mLastSelectedList->getSelectedUUID();
}

void LLCOFWearables::clear()
{
	mAttachments->clear();
	mClothing->clear();
	mBodyParts->clear();
}

//EOF
