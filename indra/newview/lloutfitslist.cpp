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

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lllistcontextmenu.h"
#include "lltransutil.h"
#include "llviewermenu.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llwearableitemslist.h"

static bool is_tab_header_clicked(LLAccordionCtrlTab* tab, S32 y);

//////////////////////////////////////////////////////////////////////////

class OutfitContextMenu : public LLListContextMenu
{
protected:
	/* virtual */ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		LLUUID selected_id = mUUIDs.front();

		registrar.add("Outfit.WearReplace",
			boost::bind(&LLAppearanceMgr::replaceCurrentOutfit, &LLAppearanceMgr::instance(), selected_id));
		registrar.add("Outfit.WearAdd",
			boost::bind(&LLAppearanceMgr::addCategoryToCurrentOutfit, &LLAppearanceMgr::instance(), selected_id));
		registrar.add("Outfit.TakeOff",
				boost::bind(&LLAppearanceMgr::takeOffOutfit, &LLAppearanceMgr::instance(), selected_id));
		// *TODO: implement this
		// registrar.add("Outfit.Rename", boost::bind());
		// registrar.add("Outfit.Delete", boost::bind());

		return createFromFile("menu_outfit_tab.xml");
	}
};

//////////////////////////////////////////////////////////////////////////

static LLRegisterPanelClassWrapper<LLOutfitsList> t_outfits_list("outfits_list");

LLOutfitsList::LLOutfitsList()
	:	LLPanel()
	,	mAccordion(NULL)
	,	mListCommands(NULL)
{
	mCategoriesObserver = new LLInventoryCategoriesObserver();
	gInventory.addObserver(mCategoriesObserver);

	gInventory.addObserver(this);

	mOutfitMenu = new OutfitContextMenu();
}

LLOutfitsList::~LLOutfitsList()
{
	delete mOutfitMenu;

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

		tab->setRightMouseDownCallback(boost::bind(&LLOutfitsList::onAccordionTabRightClick, this,
			_1, _2, _3, cat_id));

		tab->setDoubleClickCallback(boost::bind(&LLOutfitsList::onAccordionTabDoubleClick, this,
			_1, _2, _3, cat_id));

		// Setting tab focus callback to monitor currently selected outfit.
		tab->setFocusReceivedCallback(boost::bind(&LLOutfitsList::changeOutfitSelection, this, list, cat_id));

		// Setting list commit callback to monitor currently selected wearable item.
		list->setCommitCallback(boost::bind(&LLOutfitsList::onSelectionChange, this, _1));

		// Setting list refresh callback to apply filter on list change.
		list->setRefreshCompleteCallback(boost::bind(&LLOutfitsList::onFilteredWearableItemsListRefresh, this, _1));

		list->setRightMouseDownCallback(boost::bind(&LLOutfitsList::onWearableItemsListRightClick, this, _1, _2, _3));

		// Fetch the new outfit contents.
		cat->fetch();

		// Refresh the list of outfit items after fetch().
		// Further list updates will be triggered by the category observer.
		list->updateList(cat_id);

		// If filter is currently applied we store the initial tab state and
		// open it to show matched items if any.
		if (!mFilterSubString.empty())
		{
			tab->notifyChildren(LLSD().with("action","store_state"));
			tab->setDisplayChildren(true);

			// Setting mForceRefresh flag will make the list refresh its contents
			// even if it is not currently visible. This is required to apply the
			// filter to the newly added list.
			list->setForceRefresh(true);

			list->setFilterSubString(mFilterSubString);
		}
	}

	// Handle removed tabs.
	for (uuid_vec_t::const_iterator iter=vremoved.begin(); iter != vremoved.end(); iter++)
	{
		outfits_map_t::iterator outfits_iter = mOutfitsMap.find((*iter));
		if (outfits_iter != mOutfitsMap.end())
		{
			const LLUUID& outfit_id = outfits_iter->first;
			LLAccordionCtrlTab* tab = outfits_iter->second;

			// An outfit is removed from the list. Do the following:
			// 1. Remove outfit category from observer to stop monitoring its changes.
			mCategoriesObserver->removeCategory(outfit_id);

			// 2. Remove selected lists map entry.
			mSelectedListsMap.erase(outfit_id);

			// 3. Reset currently selected outfit id if it is being removed.
			if (outfit_id == mSelectedOutfitUUID)
			{
				mSelectedOutfitUUID = LLUUID();
			}

			// 4. Remove category UUID to accordion tab mapping.
			mOutfitsMap.erase(outfits_iter);

			// 5. Remove outfit tab from accordion.
			mAccordion->removeCollapsibleCtrl(tab);
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
	if (mSelectedOutfitUUID.isNull()) return;

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
	applyFilter(string);

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
	MASK mask = gKeyboard->currentMask(TRUE);

	// Reset selection in all previously selected tabs except for the current
	// if new selection is started.
	if (list && !(mask & MASK_CONTROL))
	{
		for (wearables_lists_map_t::iterator iter = mSelectedListsMap.begin();
				iter != mSelectedListsMap.end();
				++iter)
		{
			LLWearableItemsList* selected_list = (*iter).second;
			if (selected_list != list)
			{
				selected_list->resetSelection();
			}
		}

		// Clear current selection.
		mSelectedListsMap.clear();
	}

	mSelectedListsMap.insert(wearables_lists_map_value_t(category_id, list));
	mSelectedOutfitUUID = category_id;
}

void LLOutfitsList::onFilteredWearableItemsListRefresh(LLUICtrl* ctrl)
{
	if (!ctrl || mFilterSubString.empty())
		return;

	for (outfits_map_t::iterator
			 iter = mOutfitsMap.begin(),
			 iter_end = mOutfitsMap.end();
		 iter != iter_end; ++iter)
	{
		LLAccordionCtrlTab* tab = iter->second;
		if (!tab) continue;

		LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(tab->getAccordionView());
		if (list != ctrl) continue;

		std::string title = tab->getTitle();
		LLStringUtil::toUpper(title);

		std::string cur_filter = mFilterSubString;
		LLStringUtil::toUpper(cur_filter);

		if (std::string::npos == title.find(cur_filter))
		{
			// hide tab if its title doesn't pass filter
			// and it has no visible items
			tab->setVisible(list->size() != 0);

			// remove title highlighting because it might
			// have been previously highlighted by less restrictive filter
			tab->setTitle(tab->getTitle());
		}
		else
		{
			tab->setTitle(tab->getTitle(), cur_filter);
		}
	}
}

void LLOutfitsList::applyFilter(const std::string& new_filter_substring)
{
	for (outfits_map_t::iterator
			 iter = mOutfitsMap.begin(),
			 iter_end = mOutfitsMap.end();
		 iter != iter_end; ++iter)
	{
		LLAccordionCtrlTab* tab = iter->second;
		if (!tab) continue;

		bool more_restrictive = mFilterSubString.size() < new_filter_substring.size() && !new_filter_substring.substr(0, mFilterSubString.size()).compare(mFilterSubString);

		// Restore tab visibility in case of less restrictive filter
		// to compare it with updated string if it was previously hidden.
		if (!more_restrictive)
		{
			tab->setVisible(TRUE);
		}

		LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(tab->getAccordionView());
		if (list)
		{
			list->setFilterSubString(new_filter_substring);
		}

		if(mFilterSubString.empty() && !new_filter_substring.empty())
		{
			//store accordion tab state when filter is not empty
			tab->notifyChildren(LLSD().with("action","store_state"));
		}

		if (!new_filter_substring.empty())
		{
			std::string title = tab->getTitle();
			LLStringUtil::toUpper(title);

			std::string cur_filter = new_filter_substring;
			LLStringUtil::toUpper(cur_filter);

			if (std::string::npos == title.find(cur_filter))
			{
				// hide tab if its title doesn't pass filter
				// and it has no visible items
				tab->setVisible(list->size() != 0);

				// remove title highlighting because it might
				// have been previously highlighted by less restrictive filter
				tab->setTitle(tab->getTitle());
			}
			else
			{
				tab->setTitle(tab->getTitle(), cur_filter);
			}

			if (tab->getVisible())
			{
				// Open tab if it has passed the filter.
				tab->setDisplayChildren(true);
			}
			else
			{
				// Set force refresh flag to refresh not visible list
				// when some changes occur in it.
				list->setForceRefresh(true);
			}
		}
		else
		{
			// restore tab title when filter is empty
			tab->setTitle(tab->getTitle());

			//restore accordion state after all those accodrion tab manipulations
			tab->notifyChildren(LLSD().with("action","restore_state"));
		}
	}
}

void LLOutfitsList::onAccordionTabRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id)
{
	LLAccordionCtrlTab* tab = dynamic_cast<LLAccordionCtrlTab*>(ctrl);
	if(mOutfitMenu && is_tab_header_clicked(tab, y) && cat_id.notNull())
	{
		// Focus tab header to trigger tab selection change.
		LLUICtrl* header = tab->findChild<LLUICtrl>("dd_header");
		if (header)
		{
			header->setFocus(TRUE);
		}

		uuid_vec_t selected_uuids;
		selected_uuids.push_back(cat_id);
		mOutfitMenu->show(ctrl, selected_uuids, x, y);
	}
}

void LLOutfitsList::onAccordionTabDoubleClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id)
{
	LLAccordionCtrlTab* tab = dynamic_cast<LLAccordionCtrlTab*>(ctrl);
	if(is_tab_header_clicked(tab, y) && cat_id.notNull())
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
		if (!cat) return;

		LLAppearanceMgr::instance().wearInventoryCategory( cat, FALSE, FALSE );
	}
}

void LLOutfitsList::onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
	LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(ctrl);
	if (!list) return;

	uuid_vec_t selected_uuids;

	// Collect seleted items from all selected lists.
	for (wearables_lists_map_t::iterator iter = mSelectedListsMap.begin();
			iter != mSelectedListsMap.end();
			++iter)
	{
		uuid_vec_t uuids;
		(*iter).second->getSelectedUUIDs(uuids);

		S32 prev_size = selected_uuids.size();
		selected_uuids.resize(prev_size + uuids.size());
		std::copy(uuids.begin(), uuids.end(), selected_uuids.begin() + prev_size);
	}

	LLWearableItemsList::ContextMenu::instance().show(list, selected_uuids, x, y);
}

bool is_tab_header_clicked(LLAccordionCtrlTab* tab, S32 y)
{
	if(!tab || !tab->getHeaderVisible()) return false;

	S32 header_bottom = tab->getLocalRect().getHeight() - tab->getHeaderHeight();
	return y >= header_bottom;
}
// EOF
