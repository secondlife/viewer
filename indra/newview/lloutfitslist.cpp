/**
 * @file lloutfitslist.cpp
 * @brief List of agent's outfits for My Appearance side panel.
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

#include "lloutfitslist.h"

// llcommon
#include "llcommonutils.h"

// llcommon
#include "llcommonutils.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llwearableitemslist.h"

static LLRegisterPanelClassWrapper<LLOutfitsList> t_outfits_list("outfits_list");

LLOutfitsList::LLOutfitsList()
	:	LLPanel()
	,	mAccordion(NULL)
	,	mListCommands(NULL)
	,	mSelectedList(NULL)
{
	mCategoriesObserver = new LLInventoryCategoriesObserver();
	gInventory.addObserver(mCategoriesObserver);

	gInventory.addObserver(this);
}

LLOutfitsList::~LLOutfitsList()
{
	if (gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
		delete mCategoriesObserver;
	}

	if (gInventory.containsObserver(this))
	{
		gInventory.removeObserver(this);
	}
}

BOOL LLOutfitsList::postBuild()
{
	mAccordion = getChild<LLAccordionCtrl>("outfits_accordion");

	return TRUE;
}

//virtual
void LLOutfitsList::changed(U32 mask)
{
	if (!gInventory.isInventoryUsable())
		return;

	const LLUUID outfits = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
	LLViewerInventoryCategory* category = gInventory.getCategory(outfits);
	if (!category)
		return;

	// Start observing changes in "My Outfits" category.
	mCategoriesObserver->addCategory(outfits,
			boost::bind(&LLOutfitsList::refreshList, this, outfits));

	// Fetch "My Outfits" contents and refresh the list to display
	// initially fetched items. If not all items are fetched now
	// the observer will refresh the list as soon as the new items
	// arrive.
	category->fetch();
	refreshList(outfits);

	// This observer is used to start the initial outfits fetch
	// when inventory becomes usable. It is no longer needed because
	// "My Outfits" category is now observed by
	// LLInventoryCategoriesObserver.
	gInventory.removeObserver(this);
}

void LLOutfitsList::refreshList(const LLUUID& category_id)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;

	// Collect all sub-categories of a given category.
	LLIsType is_category(LLAssetType::AT_CATEGORY);
	gInventory.collectDescendentsIf(
		category_id,
		cat_array,
		item_array,
		LLInventoryModel::EXCLUDE_TRASH,
		is_category);

	uuid_vec_t vadded;
	uuid_vec_t vremoved;

	// Create added and removed items vectors.
	computeDifference(cat_array, vadded, vremoved);

	// Handle added tabs.
	for (uuid_vec_t::const_iterator iter = vadded.begin();
		 iter != vadded.end();
		 ++iter)
	{
		const LLUUID cat_id = (*iter);
		LLViewerInventoryCategory *cat = gInventory.getCategory(cat_id);
		if (!cat) continue;

		std::string name = cat->getName();

		static LLXMLNodePtr accordionXmlNode = getAccordionTabXMLNode();
		LLAccordionCtrlTab* tab = LLUICtrlFactory::defaultBuilder<LLAccordionCtrlTab>(accordionXmlNode, NULL, NULL);

		tab->setName(name);
		tab->setTitle(name);

		// *TODO: LLUICtrlFactory::defaultBuilder does not use "display_children" from xml. Should be investigated.
		tab->setDisplayChildren(false);
		mAccordion->addCollapsibleCtrl(tab);

		// Start observing the new outfit category.
		LLWearableItemsList* list  = tab->getChild<LLWearableItemsList>("wearable_items_list");
		if (!mCategoriesObserver->addCategory(cat_id, boost::bind(&LLWearableItemsList::updateList, list, cat_id)))
		{
			// Remove accordion tab if category could not be added to observer.
			mAccordion->removeCollapsibleCtrl(tab);
			continue;
		}

		// Map the new tab with outfit category UUID.
		mOutfitsMap.insert(LLOutfitsList::outfits_map_value_t(cat_id, tab));

		// Setting tab focus callback to monitor currently selected outfit.
		tab->setFocusReceivedCallback(boost::bind(&LLOutfitsList::changeOutfitSelection, this, list, cat_id));

		// Setting list commit callback to monitor currently selected wearable item.
		list->setCommitCallback(boost::bind(&LLOutfitsList::onSelectionChange, this, _1));

		// Fetch the new outfit contents.
		cat->fetch();

		// Refresh the list of outfit items after fetch().
		// Further list updates will be triggered by the category observer.
		list->updateList(cat_id);
	}

	// Handle removed tabs.
	for (uuid_vec_t::const_iterator iter=vremoved.begin(); iter != vremoved.end(); iter++)
	{
		outfits_map_t::iterator outfits_iter = mOutfitsMap.find((*iter));
		if (outfits_iter != mOutfitsMap.end())
		{
			// An outfit is removed from the list. Do the following:
			// 1. Remove outfit accordion tab from accordion.
			mAccordion->removeCollapsibleCtrl(outfits_iter->second);

			const LLUUID& outfit_id = outfits_iter->first;

			// 2. Remove outfit category from observer to stop monitoring its changes.
			mCategoriesObserver->removeCategory(outfit_id);

			// 3. Reset selection if selected outfit is being removed.
			if (mSelectedOutfitUUID == outfit_id)
			{
				changeOutfitSelection(NULL, LLUUID());
			}

			// 4. Remove category UUID to accordion tab mapping.
			mOutfitsMap.erase(outfits_iter);
		}
	}

	// Get changed items from inventory model and update outfit tabs
	// which might have been renamed.
	const LLInventoryModel::changed_items_t& changed_items = gInventory.getChangedIDs();
	for (LLInventoryModel::changed_items_t::const_iterator items_iter = changed_items.begin();
		 items_iter != changed_items.end();
		 ++items_iter)
	{
		updateOutfitTab(*items_iter);
	}

	mAccordion->arrange();
}

void LLOutfitsList::onSelectionChange(LLUICtrl* ctrl)
{
	LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(ctrl);
	if (!list) return;

	LLViewerInventoryItem *item = gInventory.getItem(list->getSelectedUUID());
	if (!item) return;

	changeOutfitSelection(list, item->getParentUUID());
}

void LLOutfitsList::performAction(std::string action)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(mSelectedOutfitUUID);
	if (!cat) return;

	if ("replaceoutfit" == action)
	{
		LLAppearanceMgr::instance().wearInventoryCategory( cat, FALSE, FALSE );
	}
	else if ("addtooutfit" == action)
	{
		LLAppearanceMgr::instance().wearInventoryCategory( cat, FALSE, TRUE );
	}
}

void LLOutfitsList::setFilterSubString(const std::string& string)
{
	mFilterSubString = string;

	for (outfits_map_t::iterator
			 iter = mOutfitsMap.begin(),
			 iter_end = mOutfitsMap.end();
		 iter != iter_end; ++iter)
	{
		LLAccordionCtrlTab* tab = iter->second;
		if (tab)
		{
			LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*> (tab->getAccordionView());
			if (list)
			{
				list->setFilterSubString(mFilterSubString);
			}

			if(!mFilterSubString.empty())
			{
				//store accordion tab state when filter is not empty
				tab->notifyChildren(LLSD().with("action","store_state"));
				tab->setDisplayChildren(true);
			}
			else
			{
				//restore accordion state after all those accodrion tab manipulations
				tab->notifyChildren(LLSD().with("action","restore_state"));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Private methods
//////////////////////////////////////////////////////////////////////////
LLXMLNodePtr LLOutfitsList::getAccordionTabXMLNode()
{
	LLXMLNodePtr xmlNode = NULL;
	bool success = LLUICtrlFactory::getLayeredXMLNode("outfit_accordion_tab.xml", xmlNode);
	if (!success)
	{
		llwarns << "Failed to read xml of Outfit's Accordion Tab from outfit_accordion_tab.xml" << llendl;
		return NULL;
	}

	return xmlNode;
}

void LLOutfitsList::computeDifference(
	const LLInventoryModel::cat_array_t& vcats, 
	uuid_vec_t& vadded, 
	uuid_vec_t& vremoved)
{
	uuid_vec_t vnew;
	// Creating a vector of newly collected sub-categories UUIDs.
	for (LLInventoryModel::cat_array_t::const_iterator iter = vcats.begin();
		iter != vcats.end();
		iter++)
	{
		vnew.push_back((*iter)->getUUID());
	}

	uuid_vec_t vcur;
	// Creating a vector of currently displayed sub-categories UUIDs.
	for (outfits_map_t::const_iterator iter = mOutfitsMap.begin();
		iter != mOutfitsMap.end();
		iter++)
	{
		vcur.push_back((*iter).first);
	}

	LLCommonUtils::computeDifference(vnew, vcur, vadded, vremoved);
}

void LLOutfitsList::updateOutfitTab(const LLUUID& category_id)
{
	outfits_map_t::iterator outfits_iter = mOutfitsMap.find(category_id);
	if (outfits_iter != mOutfitsMap.end())
	{
		LLViewerInventoryCategory *cat = gInventory.getCategory(category_id);
		if (!cat) return;

		std::string name = cat->getName();

		// Update tab name with the new category name.
		LLAccordionCtrlTab* tab = outfits_iter->second;
		if (tab)
		{
			tab->setName(name);
			tab->setTitle(name);
		}
	}
}

void LLOutfitsList::changeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id)
{
	// Reset selection in previously selected tab
	// if a new one is selected.
	if (list && mSelectedList && mSelectedList != list)
	{
		mSelectedList->resetSelection();
	}

	mSelectedList = list;
	mSelectedOutfitUUID = category_id;
}

// EOF
