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

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llwearableitemslist.h"

static LLRegisterPanelClassWrapper<LLOutfitsList> t_outfits_list("outfits_list");

LLOutfitsList::LLOutfitsList()
	:	LLPanel()
	,	mAccordion(NULL)
	,	mListCommands(NULL)
{}

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

	mCategoriesObserver = new LLInventoryCategoriesObserver();
	gInventory.addObserver(mCategoriesObserver);

	gInventory.addObserver(this);

	return TRUE;
}

//virtual
void LLOutfitsList::changed(U32 mask)
{
	if (!gInventory.isInventoryUsable())
		return;

	// Start observing changes in "My Outfits" category.
	const LLUUID outfits = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
	mCategoriesObserver->addCategory(outfits,
			boost::bind(&LLOutfitsList::refreshList, this, outfits));

	LLViewerInventoryCategory* category = gInventory.getCategory(outfits);
	if (!category)
		return;

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

	uuid_vec_t vnew;

	// Creating a vector of newly collected sub-categories UUIDs.
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
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

	// Sorting both vectors to compare.
	std::sort(vcur.begin(), vcur.end());
	std::sort(vnew.begin(), vnew.end());

	uuid_vec_t vadded;
	uuid_vec_t vremoved;

	uuid_vec_t::iterator it;
	size_t maxsize = llmax(vcur.size(), vnew.size());
	vadded.resize(maxsize);
	vremoved.resize(maxsize);

	// what to remove
	it = set_difference(vcur.begin(), vcur.end(), vnew.begin(), vnew.end(), vremoved.begin());
	vremoved.erase(it, vremoved.end());

	// what to add
	it = set_difference(vnew.begin(), vnew.end(), vcur.begin(), vcur.end(), vadded.begin());
	vadded.erase(it, vadded.end());

	// Handle added tabs.
	for (uuid_vec_t::const_iterator iter = vadded.begin();
		 iter != vadded.end();
		 iter++)
	{
		const LLUUID cat_id = (*iter);
		LLViewerInventoryCategory *cat = gInventory.getCategory(cat_id);
		if (!cat)
			continue;

		std::string name = cat->getName();

		static LLXMLNodePtr accordionXmlNode = getAccordionTabXMLNode();

		accordionXmlNode->setAttributeString("name", name);
		accordionXmlNode->setAttributeString("title", name);
		LLAccordionCtrlTab* tab = LLUICtrlFactory::defaultBuilder<LLAccordionCtrlTab>(accordionXmlNode, NULL, NULL);

		// *TODO: LLUICtrlFactory::defaultBuilder does not use "display_children" from xml. Should be investigated.
		tab->setDisplayChildren(false); 
		mAccordion->addCollapsibleCtrl(tab);

		// Map the new tab with outfit category UUID.
		mOutfitsMap.insert(LLOutfitsList::outfits_map_value_t(cat_id, tab));

		// Start observing the new outfit category.
		LLWearableItemsList* list  = tab->getChild<LLWearableItemsList>("wearable_items_list");
		mCategoriesObserver->addCategory(cat_id, boost::bind(&LLWearableItemsList::updateList, list, cat_id));

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

			// 2. Remove outfit category from observer to stop monitoring its changes.
			mCategoriesObserver->removeCategory(outfits_iter->first);

			// 3. Remove category UUID to accordion tab mapping.
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

void LLOutfitsList::updateOutfitTab(const LLUUID& category_id)
{
	outfits_map_t::iterator outfits_iter = mOutfitsMap.find(category_id);
	if (outfits_iter != mOutfitsMap.end())
	{
		LLViewerInventoryCategory *cat = gInventory.getCategory(category_id);
		if (!cat)
			return;

		std::string name = cat->getName();

		// Update tab name with the new category name.
		LLAccordionCtrlTab* tab = outfits_iter->second;
		if (tab)
		{
			tab->setName(name);
		}

		// Update tab title with the new category name using textbox
		// in accordion tab header.
		LLTextBox* tab_title = tab->findChild<LLTextBox>("dd_textbox");
		if (tab_title)
		{
			tab_title->setText(name);
		}
	}
}

void LLOutfitsList::setFilterSubString(const std::string& string)
{
	mFilterSubString = string;
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

// EOF
