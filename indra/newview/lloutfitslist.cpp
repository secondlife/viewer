/**
 * @file lloutfitslist.cpp
 * @brief List of agent's outfits for My Appearance side panel.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
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
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llinspecttexture.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "lloutfitobserver.h"
#include "lltoggleablemenu.h"
#include "lltransutil.h"
#include "llviewermenu.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llwearableitemslist.h"

static bool is_tab_header_clicked(LLAccordionCtrlTab* tab, S32 y);

static const LLOutfitTabNameComparator OUTFIT_TAB_NAME_COMPARATOR;

/*virtual*/
bool LLOutfitTabNameComparator::compare(const LLAccordionCtrlTab* tab1, const LLAccordionCtrlTab* tab2) const
{
    std::string name1 = tab1->getTitle();
    std::string name2 = tab2->getTitle();

    return (LLStringUtil::compareDict(name1, name2) < 0);
}

struct outfit_accordion_tab_params : public LLInitParam::Block<outfit_accordion_tab_params, LLOutfitAccordionCtrlTab::Params>
{
    Mandatory<LLWearableItemsList::Params> wearable_list;

    outfit_accordion_tab_params()
    :   wearable_list("wearable_items_list")
    {}
};

const outfit_accordion_tab_params& get_accordion_tab_params()
{
    static outfit_accordion_tab_params tab_params;
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;

        LLXMLNodePtr xmlNode;
        if (LLUICtrlFactory::getLayeredXMLNode("outfit_accordion_tab.xml", xmlNode))
        {
            LLXUIParser parser;
            parser.readXUI(xmlNode, tab_params, "outfit_accordion_tab.xml");
        }
        else
        {
            LL_WARNS() << "Failed to read xml of Outfit's Accordion Tab from outfit_accordion_tab.xml" << LL_ENDL;
        }
    }

    return tab_params;
}


static LLPanelInjector<LLOutfitsList> t_outfits_list("outfits_list");

LLOutfitsList::LLOutfitsList()
    :   LLOutfitListBase()
    ,   mAccordion(NULL)
    ,   mListCommands(NULL)
    ,   mItemSelected(false)
{
}

LLOutfitsList::~LLOutfitsList()
{
}

bool LLOutfitsList::postBuild()
{
    mAccordion = getChild<LLAccordionCtrl>("outfits_accordion");
    mAccordion->setComparator(&OUTFIT_TAB_NAME_COMPARATOR);

    return LLOutfitListBase::postBuild();
}

//virtual
void LLOutfitsList::onOpen(const LLSD& info)
{
    if (!mIsInitialized)
    {
        // Start observing changes in Current Outfit category.
        LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLOutfitsList::onCOFChanged, this));
    }

    LLOutfitListBase::onOpen(info);

    LLAccordionCtrlTab* selected_tab = mAccordion->getSelectedTab();
    if (!selected_tab) return;

    // Pass focus to the selected outfit tab.
    selected_tab->showAndFocusHeader();
}


void LLOutfitsList::updateAddedCategory(LLUUID cat_id)
{
    LLViewerInventoryCategory *cat = gInventory.getCategory(cat_id);
    if (!cat) return;

    std::string name = cat->getName();

    outfit_accordion_tab_params tab_params(get_accordion_tab_params());
    tab_params.cat_id = cat_id;
    LLOutfitAccordionCtrlTab *tab = LLUICtrlFactory::create<LLOutfitAccordionCtrlTab>(tab_params);
    if (!tab) return;
    LLWearableItemsList* wearable_list = LLUICtrlFactory::create<LLWearableItemsList>(tab_params.wearable_list);
    wearable_list->setShape(tab->getLocalRect());
    tab->addChild(wearable_list);

    tab->setName(name);
    tab->setTitle(name);

    // *TODO: LLUICtrlFactory::defaultBuilder does not use "display_children" from xml. Should be investigated.
    tab->setDisplayChildren(false);
    mAccordion->addCollapsibleCtrl(tab);

    // Start observing the new outfit category.
    LLWearableItemsList* list = tab->getChild<LLWearableItemsList>("wearable_items_list");
    if (!mCategoriesObserver->addCategory(cat_id, boost::bind(&LLWearableItemsList::updateList, list, cat_id)))
    {
        // Remove accordion tab if category could not be added to observer.
        mAccordion->removeCollapsibleCtrl(tab);

        // kill removed tab
        tab->die();
        return;
    }

    // Map the new tab with outfit category UUID.
    mOutfitsMap.insert(LLOutfitsList::outfits_map_value_t(cat_id, tab));

    tab->setRightMouseDownCallback(boost::bind(&LLOutfitListBase::outfitRightClickCallBack, this,
        _1, _2, _3, cat_id));

    // Setting tab focus callback to monitor currently selected outfit.
    tab->setFocusReceivedCallback(boost::bind(&LLOutfitListBase::ChangeOutfitSelection, this, list, cat_id));

    // Setting callback to reset items selection inside outfit on accordion collapsing and expanding (EXT-7875)
    tab->setDropDownStateChangedCallback(boost::bind(&LLOutfitsList::resetItemSelection, this, list, cat_id));

    // force showing list items that don't match current filter(EXT-7158)
    list->setForceShowingUnmatchedItems(true);

    // Setting list commit callback to monitor currently selected wearable item.
    list->setCommitCallback(boost::bind(&LLOutfitsList::onListSelectionChange, this, _1));

    // Setting list refresh callback to apply filter on list change.
    list->setRefreshCompleteCallback(boost::bind(&LLOutfitsList::onRefreshComplete, this, _1));

    list->setRightMouseDownCallback(boost::bind(&LLOutfitsList::onWearableItemsListRightClick, this, _1, _2, _3));

    // Fetch the new outfit contents.
    cat->fetch();

    // Refresh the list of outfit items after fetch().
    // Further list updates will be triggered by the category observer.
    list->updateList(cat_id);

    // If filter is currently applied we store the initial tab state.
    if (!getFilterSubString().empty())
    {
        tab->notifyChildren(LLSD().with("action", "store_state"));

        // Setting mForceRefresh flag will make the list refresh its contents
        // even if it is not currently visible. This is required to apply the
        // filter to the newly added list.
        list->setForceRefresh(true);

        list->setFilterSubString(getFilterSubString(), false);
    }
}

void LLOutfitsList::updateRemovedCategory(LLUUID cat_id)
{
    outfits_map_t::iterator outfits_iter = mOutfitsMap.find(cat_id);
    if (outfits_iter != mOutfitsMap.end())
    {
        const LLUUID& outfit_id = outfits_iter->first;
        LLAccordionCtrlTab* tab = outfits_iter->second;

        // An outfit is removed from the list. Do the following:
        // 1. Remove outfit category from observer to stop monitoring its changes.
        mCategoriesObserver->removeCategory(outfit_id);

        // 2. Remove the outfit from selection.
        deselectOutfit(outfit_id);

        // 3. Remove category UUID to accordion tab mapping.
        mOutfitsMap.erase(outfits_iter);

        // 4. Remove outfit tab from accordion.
        mAccordion->removeCollapsibleCtrl(tab);

        // kill removed tab
        if (tab != NULL)
        {
            tab->die();
        }
    }
}

//virtual
void LLOutfitsList::onHighlightBaseOutfit(LLUUID base_id, LLUUID prev_id)
{
    if (mOutfitsMap[prev_id])
    {
        mOutfitsMap[prev_id]->setTitleFontStyle("NORMAL");
        mOutfitsMap[prev_id]->setTitleColor(LLUIColorTable::instance().getColor("AccordionHeaderTextColor"));
    }
    if (mOutfitsMap[base_id])
    {
        mOutfitsMap[base_id]->setTitleFontStyle("BOLD");
        mOutfitsMap[base_id]->setTitleColor(LLUIColorTable::instance().getColor("SelectedOutfitTextColor"));
    }
}

void LLOutfitsList::onListSelectionChange(LLUICtrl* ctrl)
{
    LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(ctrl);
    if (!list) return;

    LLViewerInventoryItem *item = gInventory.getItem(list->getSelectedUUID());
    if (!item) return;

    ChangeOutfitSelection(list, item->getParentUUID());
}

void LLOutfitListBase::performAction(std::string action)
{
    if (mSelectedOutfitUUID.isNull()) return;

    LLViewerInventoryCategory* cat = gInventory.getCategory(mSelectedOutfitUUID);
    if (!cat) return;

    if ("replaceoutfit" == action)
    {
        LLAppearanceMgr::instance().wearInventoryCategory( cat, false, false );
    }
    else if ("addtooutfit" == action)
    {
        LLAppearanceMgr::instance().wearInventoryCategory( cat, false, true );
    }
    else if ("rename_outfit" == action)
    {
        LLAppearanceMgr::instance().renameOutfit(mSelectedOutfitUUID);
    }
}

void LLOutfitsList::onSetSelectedOutfitByUUID(const LLUUID& outfit_uuid)
{
    for (outfits_map_t::iterator iter = mOutfitsMap.begin();
            iter != mOutfitsMap.end();
            ++iter)
    {
        if (outfit_uuid == iter->first)
        {
            LLAccordionCtrlTab* tab = iter->second;
            if (!tab) continue;

            LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(tab->getAccordionView());
            if (!list) continue;

            tab->setFocus(true);
            ChangeOutfitSelection(list, outfit_uuid);

            tab->changeOpenClose(false);
        }
    }
}

// virtual
bool LLOutfitListBase::isActionEnabled(const LLSD& userdata)
{
    if (mSelectedOutfitUUID.isNull()) return false;

    const std::string command_name = userdata.asString();
    if (command_name == "delete")
    {
        return !hasItemSelected() && LLAppearanceMgr::instance().getCanRemoveOutfit(mSelectedOutfitUUID);
    }
    if (command_name == "rename")
    {
        return get_is_category_renameable(&gInventory, mSelectedOutfitUUID);
    }
    if (command_name == "save_outfit")
    {
        bool outfit_locked = LLAppearanceMgr::getInstance()->isOutfitLocked();
        bool outfit_dirty = LLAppearanceMgr::getInstance()->isOutfitDirty();
        // allow save only if outfit isn't locked and is dirty
        return !outfit_locked && outfit_dirty;
    }
    if (command_name == "wear")
    {
        if (gAgentWearables.isCOFChangeInProgress())
        {
            return false;
        }

        if (hasItemSelected())
        {
            return canWearSelected();
        }

        // outfit selected
        return LLAppearanceMgr::instance().getCanReplaceCOF(mSelectedOutfitUUID);
    }
    if (command_name == "take_off")
    {
        // Enable "Take Off" if any of selected items can be taken off
        // or the selected outfit contains items that can be taken off.
        return ( hasItemSelected() && canTakeOffSelected() )
                || ( !hasItemSelected() && LLAppearanceMgr::getCanRemoveFromCOF(mSelectedOutfitUUID) );
    }

    if (command_name == "wear_add")
    {
        // *TODO: do we ever get here?
        return LLAppearanceMgr::getCanAddToCOF(mSelectedOutfitUUID);
    }

    return false;
}

void LLOutfitsList::getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const
{
    // Collect selected items from all selected lists.
    for (wearables_lists_map_t::const_iterator iter = mSelectedListsMap.begin();
            iter != mSelectedListsMap.end();
            ++iter)
    {
        uuid_vec_t uuids;
        (*iter).second->getSelectedUUIDs(uuids);

        auto prev_size = selected_uuids.size();
        selected_uuids.resize(prev_size + uuids.size());
        std::copy(uuids.begin(), uuids.end(), selected_uuids.begin() + prev_size);
    }
}

void LLOutfitsList::onCollapseAllFolders()
{
    for (outfits_map_t::iterator iter = mOutfitsMap.begin();
            iter != mOutfitsMap.end();
            ++iter)
    {
        LLAccordionCtrlTab* tab = iter->second;
        if(tab && tab->isExpanded())
        {
            tab->changeOpenClose(true);
        }
    }
}

void LLOutfitsList::onExpandAllFolders()
{
    for (outfits_map_t::iterator iter = mOutfitsMap.begin();
            iter != mOutfitsMap.end();
            ++iter)
    {
        LLAccordionCtrlTab* tab = iter->second;
        if(tab && !tab->isExpanded())
        {
            tab->changeOpenClose(false);
        }
    }
}

bool LLOutfitsList::hasItemSelected()
{
    return mItemSelected;
}

//////////////////////////////////////////////////////////////////////////
// Private methods
//////////////////////////////////////////////////////////////////////////

void LLOutfitsList::updateChangedCategoryName(LLViewerInventoryCategory *cat, std::string name)
{
    outfits_map_t::iterator outfits_iter = mOutfitsMap.find(cat->getUUID());
    if (outfits_iter != mOutfitsMap.end())
    {
        // Update tab name with the new category name.
        LLAccordionCtrlTab* tab = outfits_iter->second;
        if (tab)
        {
            tab->setName(name);
            tab->setTitle(name);
        }
    }
}

void LLOutfitsList::resetItemSelection(LLWearableItemsList* list, const LLUUID& category_id)
{
    list->resetSelection();
    mItemSelected = false;
    signalSelectionOutfitUUID(category_id);
}

void LLOutfitsList::onChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id)
{
    MASK mask = gKeyboard->currentMask(true);

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

    mItemSelected = list && (list->getSelectedItem() != NULL);

    mSelectedListsMap.insert(wearables_lists_map_value_t(category_id, list));
}

void LLOutfitsList::deselectOutfit(const LLUUID& category_id)
{
    // Remove selected lists map entry.
    mSelectedListsMap.erase(category_id);

    LLOutfitListBase::deselectOutfit(category_id);
}

void LLOutfitsList::restoreOutfitSelection(LLAccordionCtrlTab* tab, const LLUUID& category_id)
{
    // Try restoring outfit selection after filtering.
    if (mAccordion->getSelectedTab() == tab)
    {
        signalSelectionOutfitUUID(category_id);
    }
}

void LLOutfitsList::onRefreshComplete(LLUICtrl* ctrl)
{
    if (!ctrl || getFilterSubString().empty())
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

        applyFilterToTab(iter->first, tab, getFilterSubString());
    }
}

// virtual
void LLOutfitsList::onFilterSubStringChanged(const std::string& new_string, const std::string& old_string)
{
    mAccordion->setFilterSubString(new_string);

    outfits_map_t::iterator iter = mOutfitsMap.begin(), iter_end = mOutfitsMap.end();
    while (iter != iter_end)
    {
        const LLUUID& category_id = iter->first;
        LLAccordionCtrlTab* tab = iter++->second;
        if (!tab) continue;

        LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(tab->getAccordionView());
        if (list)
        {
            list->setFilterSubString(new_string, tab->getDisplayChildren());
        }

        if (old_string.empty())
        {
            // Store accordion tab state when filter is not empty
            tab->notifyChildren(LLSD().with("action", "store_state"));
        }

        if (!new_string.empty())
        {
            applyFilterToTab(category_id, tab, new_string);
        }
        else
        {
            tab->setVisible(true);

            // Restore tab title when filter is empty
            tab->setTitle(tab->getTitle());

            // Restore accordion state after all those accodrion tab manipulations
            tab->notifyChildren(LLSD().with("action", "restore_state"));

            // Try restoring the tab selection.
            restoreOutfitSelection(tab, category_id);
        }
    }

    mAccordion->arrange();
}

void LLOutfitsList::applyFilterToTab(
    const LLUUID&       category_id,
    LLAccordionCtrlTab* tab,
    const std::string&  filter_substring)
{
    if (!tab) return;
    LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(tab->getAccordionView());
    if (!list) return;

    std::string title = tab->getTitle();
    LLStringUtil::toUpper(title);

    std::string cur_filter = filter_substring;
    LLStringUtil::toUpper(cur_filter);

    tab->setTitle(tab->getTitle(), cur_filter);

    if (std::string::npos == title.find(cur_filter))
    {
        // Hide tab if its title doesn't pass filter
        // and it has no matched items
        tab->setVisible(list->hasMatchedItems());

        // Remove title highlighting because it might
        // have been previously highlighted by less restrictive filter
        tab->setTitle(tab->getTitle());

        // Remove the tab from selection.
        deselectOutfit(category_id);
    }
    else
    {
        // Try restoring the tab selection.
        restoreOutfitSelection(tab, category_id);
    }
}

bool LLOutfitsList::canWearSelected()
{
    if (!isAgentAvatarValid())
    {
        return false;
    }

    uuid_vec_t selected_items;
    getSelectedItemsUUIDs(selected_items);
    S32 nonreplacable_objects = 0;

    for (uuid_vec_t::const_iterator it = selected_items.begin(); it != selected_items.end(); ++it)
    {
        const LLUUID& id = *it;

        // Check whether the item is worn.
        if (!get_can_item_be_worn(id))
        {
            return false;
        }

        const LLViewerInventoryItem* item = gInventory.getItem(id);
        if (!item)
        {
            return false;
        }

        if (item->getType() == LLAssetType::AT_OBJECT)
        {
            nonreplacable_objects++;
        }
    }

    // All selected items can be worn. But do we have enough space for them?
    return nonreplacable_objects == 0 || gAgentAvatarp->canAttachMoreObjects(nonreplacable_objects);
}

void LLOutfitsList::wearSelectedItems()
{
    uuid_vec_t selected_uuids;
    getSelectedItemsUUIDs(selected_uuids);

    if(selected_uuids.empty())
    {
        return;
    }

    wear_multiple(selected_uuids, false);
}

void LLOutfitsList::onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
    LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(ctrl);
    if (!list) return;

    uuid_vec_t selected_uuids;

    getSelectedItemsUUIDs(selected_uuids);

    LLWearableItemsList::ContextMenu::instance().show(list, selected_uuids, x, y);
}

void LLOutfitsList::onCOFChanged()
{
    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;

    // Collect current COF items
    gInventory.collectDescendents(
        LLAppearanceMgr::instance().getCOF(),
        cat_array,
        item_array,
        LLInventoryModel::EXCLUDE_TRASH);

    uuid_vec_t vnew;
    uuid_vec_t vadded;
    uuid_vec_t vremoved;

    // From gInventory we get the UUIDs of links that are currently in COF.
    // These links UUIDs are not the same UUIDs that we have in each wearable items list.
    // So we collect base items' UUIDs to find them or links that point to them in wearable
    // items lists and update their worn state there.
    LLInventoryModel::item_array_t::const_iterator array_iter = item_array.begin(), array_end = item_array.end();
    while (array_iter < array_end)
    {
        vnew.push_back((*(array_iter++))->getLinkedUUID());
    }

    // We need to update only items that were added or removed from COF.
    LLCommonUtils::computeDifference(vnew, mCOFLinkedItems, vadded, vremoved);

    // Store the ids of items currently linked from COF.
    mCOFLinkedItems = vnew;

    // Append removed ids to added ids because we should update all of them.
    vadded.reserve(vadded.size() + vremoved.size());
    vadded.insert(vadded.end(), vremoved.begin(), vremoved.end());
    vremoved.clear();

    outfits_map_t::iterator map_iter = mOutfitsMap.begin(), map_end = mOutfitsMap.end();
    while (map_iter != map_end)
    {
        LLAccordionCtrlTab* tab = (map_iter++)->second;
        if (!tab) continue;

        LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(tab->getAccordionView());
        if (!list) continue;

        // Every list updates the labels of changed items  or
        // the links that point to these items.
        list->updateChangedItems(vadded);
    }
}

void LLOutfitsList::getCurrentCategories(uuid_vec_t& vcur)
{
    // Creating a vector of currently displayed sub-categories UUIDs.
    for (outfits_map_t::const_iterator iter = mOutfitsMap.begin();
        iter != mOutfitsMap.end();
        iter++)
    {
        vcur.push_back((*iter).first);
    }
}


void LLOutfitsList::sortOutfits()
{
    mAccordion->sort();
}

void LLOutfitsList::onOutfitRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id)
{
    LLAccordionCtrlTab* tab = dynamic_cast<LLAccordionCtrlTab*>(ctrl);
    if (mOutfitMenu && is_tab_header_clicked(tab, y) && cat_id.notNull())
    {
        // Focus tab header to trigger tab selection change.
        LLUICtrl* header = tab->findChild<LLUICtrl>("dd_header");
        if (header)
        {
            header->setFocus(true);
        }

        uuid_vec_t selected_uuids;
        selected_uuids.push_back(cat_id);
        mOutfitMenu->show(ctrl, selected_uuids, x, y);
    }
}

LLOutfitListGearMenuBase* LLOutfitsList::createGearMenu()
{
    return new LLOutfitListGearMenu(this);
}


bool is_tab_header_clicked(LLAccordionCtrlTab* tab, S32 y)
{
    if(!tab || !tab->getHeaderVisible()) return false;

    S32 header_bottom = tab->getLocalRect().getHeight() - tab->getHeaderHeight();
    return y >= header_bottom;
}

LLOutfitListBase::LLOutfitListBase()
    :   LLPanelAppearanceTab()
    ,   mIsInitialized(false)
{
    mCategoriesObserver = new LLInventoryCategoriesObserver();
    mOutfitMenu = new LLOutfitContextMenu(this);
    //mGearMenu = createGearMenu();
}

LLOutfitListBase::~LLOutfitListBase()
{
    delete mOutfitMenu;
    delete mGearMenu;

    if (gInventory.containsObserver(mCategoriesObserver))
    {
        gInventory.removeObserver(mCategoriesObserver);
    }
    delete mCategoriesObserver;
}

void LLOutfitListBase::onOpen(const LLSD& info)
{
    if (!mIsInitialized)
    {
        // *TODO: I'm not sure is this check necessary but it never match while developing.
        if (!gInventory.isInventoryUsable())
            return;

        const LLUUID outfits = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

        // *TODO: I'm not sure is this check necessary but it never match while developing.
        LLViewerInventoryCategory* category = gInventory.getCategory(outfits);
        if (!category)
            return;

        gInventory.addObserver(mCategoriesObserver);

        // Start observing changes in "My Outfits" category.
        mCategoriesObserver->addCategory(outfits,
            boost::bind(&LLOutfitListBase::observerCallback, this, outfits));

        //const LLUUID cof = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
        // Start observing changes in Current Outfit category.
        //mCategoriesObserver->addCategory(cof, boost::bind(&LLOutfitsList::onCOFChanged, this));

        LLOutfitObserver::instance().addBOFChangedCallback(boost::bind(&LLOutfitListBase::highlightBaseOutfit, this));
        LLOutfitObserver::instance().addBOFReplacedCallback(boost::bind(&LLOutfitListBase::highlightBaseOutfit, this));

        // Fetch "My Outfits" contents and refresh the list to display
        // initially fetched items. If not all items are fetched now
        // the observer will refresh the list as soon as the new items
        // arrive.
        category->fetch();
        refreshList(outfits);

        mIsInitialized = true;
    }
}

void LLOutfitListBase::observerCallback(const LLUUID& category_id)
{
    const LLInventoryModel::changed_items_t& changed_items = gInventory.getChangedIDs();
    mChangedItems.insert(changed_items.begin(), changed_items.end());
    refreshList(category_id);
}

void LLOutfitListBase::refreshList(const LLUUID& category_id)
{
    bool wasNull = mRefreshListState.CategoryUUID.isNull();
    mRefreshListState.CategoryUUID.setNull();

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

    // Memorize item names for each UUID
    std::map<LLUUID, std::string> names;
    for (const LLPointer<LLViewerInventoryCategory>& cat : cat_array)
    {
        names.emplace(std::make_pair(cat->getUUID(), cat->getName()));
    }

    // Fill added and removed items vectors.
    mRefreshListState.Added.clear();
    mRefreshListState.Removed.clear();
    computeDifference(cat_array, mRefreshListState.Added, mRefreshListState.Removed);
    // Sort added items vector by item name.
    std::sort(mRefreshListState.Added.begin(), mRefreshListState.Added.end(),
        [names](const LLUUID& a, const LLUUID& b)
        {
            return LLStringUtil::compareDict(names.at(a), names.at(b)) < 0;
        });
    // Initialize iterators for added and removed items vectors.
    mRefreshListState.AddedIterator = mRefreshListState.Added.begin();
    mRefreshListState.RemovedIterator = mRefreshListState.Removed.begin();

    LL_INFOS() << "added: " << mRefreshListState.Added.size() <<
        ", removed: " << mRefreshListState.Removed.size() <<
        ", changed: " << gInventory.getChangedIDs().size() <<
        LL_ENDL;

    mRefreshListState.CategoryUUID = category_id;
    if (wasNull)
    {
        gIdleCallbacks.addFunction(onIdle, this);
    }
}

// static
void LLOutfitListBase::onIdle(void* userdata)
{
    LLOutfitListBase* self = (LLOutfitListBase*)userdata;

    self->onIdleRefreshList();
}

void LLOutfitListBase::onIdleRefreshList()
{
    if (mRefreshListState.CategoryUUID.isNull())
        return;

    const F64 MAX_TIME = 0.05f;
    F64 curent_time = LLTimer::getTotalSeconds();
    const F64 end_time = curent_time + MAX_TIME;

    // Handle added tabs.
    while (mRefreshListState.AddedIterator < mRefreshListState.Added.end())
    {
        const LLUUID cat_id = (*mRefreshListState.AddedIterator++);
        updateAddedCategory(cat_id);

        curent_time = LLTimer::getTotalSeconds();
        if (curent_time >= end_time)
            return;
    }
    mRefreshListState.Added.clear();
    mRefreshListState.AddedIterator = mRefreshListState.Added.end();

    // Handle removed tabs.
    while (mRefreshListState.RemovedIterator < mRefreshListState.Removed.end())
    {
        const LLUUID cat_id = (*mRefreshListState.RemovedIterator++);
        updateRemovedCategory(cat_id);

        curent_time = LLTimer::getTotalSeconds();
        if (curent_time >= end_time)
            return;
    }
    mRefreshListState.Removed.clear();
    mRefreshListState.RemovedIterator = mRefreshListState.Removed.end();

    // Get changed items from inventory model and update outfit tabs
    // which might have been renamed.
    while (!mChangedItems.empty())
    {
        std::set<LLUUID>::const_iterator items_iter = mChangedItems.begin();
        LLViewerInventoryCategory *cat = gInventory.getCategory(*items_iter);
        mChangedItems.erase(items_iter);

        // Links aren't supposed to be allowed here, check only cats
        if (cat)
        {
            std::string name = cat->getName();
            updateChangedCategoryName(cat, name);
        }

        curent_time = LLTimer::getTotalSeconds();
        if (curent_time >= end_time)
            return;
    }

    sortOutfits();
    highlightBaseOutfit();

    gIdleCallbacks.deleteFunction(onIdle, this);
    mRefreshListState.CategoryUUID.setNull();

    LL_INFOS() << "done" << LL_ENDL;
}

void LLOutfitListBase::computeDifference(
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
    getCurrentCategories(vcur);

    LLCommonUtils::computeDifference(vnew, vcur, vadded, vremoved);
}

void LLOutfitListBase::sortOutfits()
{
}

void LLOutfitListBase::highlightBaseOutfit()
{
    // id of base outfit
    LLUUID base_id = LLAppearanceMgr::getInstance()->getBaseOutfitUUID();
    if (base_id != mHighlightedOutfitUUID)
    {
        LLUUID prev_id = mHighlightedOutfitUUID;
        mHighlightedOutfitUUID = base_id;
        onHighlightBaseOutfit(base_id, prev_id);
    }
}

void LLOutfitListBase::removeSelected()
{
    LLNotificationsUtil::add("DeleteOutfits", LLSD(), LLSD(), boost::bind(&LLOutfitListBase::onOutfitsRemovalConfirmation, this, _1, _2));
}

void LLOutfitListBase::onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) return; // canceled

    if (mSelectedOutfitUUID.notNull())
    {
        gInventory.removeCategory(mSelectedOutfitUUID);
    }
}

void LLOutfitListBase::setSelectedOutfitByUUID(const LLUUID& outfit_uuid)
{
    onSetSelectedOutfitByUUID(outfit_uuid);
}

boost::signals2::connection LLOutfitListBase::setSelectionChangeCallback(selection_change_callback_t cb)
{
    return mSelectionChangeSignal.connect(cb);
}

void LLOutfitListBase::signalSelectionOutfitUUID(const LLUUID& category_id)
{
    mSelectionChangeSignal(category_id);
}

void LLOutfitListBase::outfitRightClickCallBack(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id)
{
    onOutfitRightClick(ctrl, x, y, cat_id);
}

void LLOutfitListBase::ChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id)
{
    onChangeOutfitSelection(list, category_id);
    mSelectedOutfitUUID = category_id;
    signalSelectionOutfitUUID(category_id);
}

bool LLOutfitListBase::postBuild()
{
    mGearMenu = createGearMenu();

    LLMenuButton* menu_gear_btn = getChild<LLMenuButton>("options_gear_btn");

    menu_gear_btn->setMouseDownCallback(boost::bind(&LLOutfitListGearMenuBase::updateItemsVisibility, mGearMenu));
    menu_gear_btn->setMenu(mGearMenu->getMenu());
    return true;
}

void LLOutfitListBase::collapseAllFolders()
{
    onCollapseAllFolders();
}

void LLOutfitListBase::expandAllFolders()
{
    onExpandAllFolders();
}

void LLOutfitListBase::deselectOutfit(const LLUUID& category_id)
{
    // Reset selection if the outfit is selected.
    if (category_id == mSelectedOutfitUUID)
    {
        mSelectedOutfitUUID = LLUUID::null;
        signalSelectionOutfitUUID(mSelectedOutfitUUID);
    }
}

LLContextMenu* LLOutfitContextMenu::createMenu()
{
    LLUICtrl::ScopedRegistrarHelper registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
    LLUUID selected_id = mUUIDs.front();

    registrar.add("Outfit.WearReplace",
        boost::bind(&LLAppearanceMgr::replaceCurrentOutfit, &LLAppearanceMgr::instance(), selected_id));
    registrar.add("Outfit.WearAdd",
        boost::bind(&LLAppearanceMgr::addCategoryToCurrentOutfit, &LLAppearanceMgr::instance(), selected_id));
    registrar.add("Outfit.TakeOff",
        boost::bind(&LLAppearanceMgr::takeOffOutfit, &LLAppearanceMgr::instance(), selected_id));
    registrar.add("Outfit.Edit", boost::bind(editOutfit));
    registrar.add("Outfit.Rename", boost::bind(renameOutfit, selected_id), LLUICtrl::cb_info::UNTRUSTED_BLOCK);
    registrar.add("Outfit.Delete", boost::bind(&LLOutfitListBase::removeSelected, mOutfitList), LLUICtrl::cb_info::UNTRUSTED_BLOCK);
    registrar.add("Outfit.Thumbnail", boost::bind(&LLOutfitContextMenu::onThumbnail, this, selected_id));
    registrar.add("Outfit.Save", boost::bind(&LLOutfitContextMenu::onSave, this, selected_id));

    enable_registrar.add("Outfit.OnEnable", boost::bind(&LLOutfitContextMenu::onEnable, this, _2));
    enable_registrar.add("Outfit.OnVisible", boost::bind(&LLOutfitContextMenu::onVisible, this, _2));

    return createFromFile("menu_outfit_tab.xml");

}

bool LLOutfitContextMenu::onEnable(LLSD::String param)
{
    LLUUID outfit_cat_id = mUUIDs.back();

    if ("rename" == param)
    {
        return get_is_category_renameable(&gInventory, outfit_cat_id);
    }
    else if ("wear_replace" == param)
    {
        return LLAppearanceMgr::instance().getCanReplaceCOF(outfit_cat_id);
    }
    else if ("wear_add" == param)
    {
        return LLAppearanceMgr::getCanAddToCOF(outfit_cat_id);
    }
    else if ("take_off" == param)
    {
        return LLAppearanceMgr::getCanRemoveFromCOF(outfit_cat_id);
    }

    return true;
}

bool LLOutfitContextMenu::onVisible(LLSD::String param)
{
    LLUUID outfit_cat_id = mUUIDs.back();

    if ("edit" == param)
    {
        bool is_worn = LLAppearanceMgr::instance().getBaseOutfitUUID() == outfit_cat_id;
        return is_worn;
    }
    else if ("wear_replace" == param)
    {
        return true;
    }
    else if ("delete" == param)
    {
        return LLAppearanceMgr::instance().getCanRemoveOutfit(outfit_cat_id);
    }

    return true;
}

//static
void LLOutfitContextMenu::editOutfit()
{
    LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "edit_outfit"));
}

void LLOutfitContextMenu::renameOutfit(const LLUUID& outfit_cat_id)
{
    LLAppearanceMgr::instance().renameOutfit(outfit_cat_id);
}

void LLOutfitContextMenu::onThumbnail(const LLUUID &outfit_cat_id)
{
    if (outfit_cat_id.notNull())
    {
        LLSD data(outfit_cat_id);
        LLFloaterReg::showInstance("change_item_thumbnail", data);
    }
}

void LLOutfitContextMenu::onSave(const LLUUID &outfit_cat_id)
{
    if (outfit_cat_id.notNull())
    {
        LLNotificationsUtil::add("ConfirmOverwriteOutfit", LLSD(), LLSD(),
            [outfit_cat_id](const LLSD &notif, const LLSD &resp)
        {
            S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
            if (opt == 0)
            {
                LLAppearanceMgr::getInstance()->onOutfitFolderCreated(outfit_cat_id, true);
            }
        });
    }
}

LLOutfitListGearMenuBase::LLOutfitListGearMenuBase(LLOutfitListBase* olist)
    :   mOutfitList(olist),
        mMenu(NULL)
{
    llassert_always(mOutfitList);

    LLUICtrl::ScopedRegistrarHelper registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

    registrar.add("Gear.Wear", boost::bind(&LLOutfitListGearMenuBase::onWear, this));
    registrar.add("Gear.TakeOff", boost::bind(&LLOutfitListGearMenuBase::onTakeOff, this));
    registrar.add("Gear.Rename", boost::bind(&LLOutfitListGearMenuBase::onRename, this));
    registrar.add("Gear.Delete", boost::bind(&LLOutfitListBase::removeSelected, mOutfitList));
    registrar.add("Gear.Create", boost::bind(&LLOutfitListGearMenuBase::onCreate, this, _2));
    registrar.add("Gear.Collapse", boost::bind(&LLOutfitListBase::onCollapseAllFolders, mOutfitList));
    registrar.add("Gear.Expand", boost::bind(&LLOutfitListBase::onExpandAllFolders, mOutfitList));

    registrar.add("Gear.WearAdd", boost::bind(&LLOutfitListGearMenuBase::onAdd, this));
    registrar.add("Gear.Save", boost::bind(&LLOutfitListGearMenuBase::onSave, this));

    registrar.add("Gear.Thumbnail", boost::bind(&LLOutfitListGearMenuBase::onThumbnail, this));
    registrar.add("Gear.SortByName", boost::bind(&LLOutfitListGearMenuBase::onChangeSortOrder, this));

    enable_registrar.add("Gear.OnEnable", boost::bind(&LLOutfitListGearMenuBase::onEnable, this, _2));
    enable_registrar.add("Gear.OnVisible", boost::bind(&LLOutfitListGearMenuBase::onVisible, this, _2));

    mMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
        "menu_outfit_gear.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    llassert(mMenu);
}

LLOutfitListGearMenuBase::~LLOutfitListGearMenuBase()
{}

void LLOutfitListGearMenuBase::updateItemsVisibility()
{
    onUpdateItemsVisibility();
}

void LLOutfitListGearMenuBase::onUpdateItemsVisibility()
{
    if (!mMenu) return;

    bool have_selection = getSelectedOutfitID().notNull();
    mMenu->setItemVisible("wear_separator", have_selection);
    mMenu->arrangeAndClear(); // update menu height
}

LLToggleableMenu* LLOutfitListGearMenuBase::getMenu()
{
    return mMenu;
}
const LLUUID& LLOutfitListGearMenuBase::getSelectedOutfitID()
{
    return mOutfitList->getSelectedOutfitUUID();
}

LLViewerInventoryCategory* LLOutfitListGearMenuBase::getSelectedOutfit()
{
    const LLUUID& selected_outfit_id = getSelectedOutfitID();
    if (selected_outfit_id.isNull())
    {
        return NULL;
    }

    LLViewerInventoryCategory* cat = gInventory.getCategory(selected_outfit_id);
    return cat;
}

void LLOutfitListGearMenuBase::onWear()
{
    LLViewerInventoryCategory* selected_outfit = getSelectedOutfit();
    if (selected_outfit)
    {
        LLAppearanceMgr::instance().wearInventoryCategory(
            selected_outfit, /*copy=*/ false, /*append=*/ false);
    }
}

void LLOutfitListGearMenuBase::onAdd()
{
    const LLUUID& selected_id = getSelectedOutfitID();

    if (selected_id.notNull())
    {
        LLAppearanceMgr::getInstance()->addCategoryToCurrentOutfit(selected_id);
    }
}

void LLOutfitListGearMenuBase::onSave()
{
    const LLUUID &selected_id = getSelectedOutfitID();
    LLNotificationsUtil::add("ConfirmOverwriteOutfit", LLSD(), LLSD(),
        [selected_id](const LLSD &notif, const LLSD &resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            LLAppearanceMgr::getInstance()->onOutfitFolderCreated(selected_id, true);
        }
    });
}

void LLOutfitListGearMenuBase::onTakeOff()
{
    // Take off selected outfit.
    const LLUUID& selected_outfit_id = getSelectedOutfitID();
    if (selected_outfit_id.notNull())
    {
        LLAppearanceMgr::instance().takeOffOutfit(selected_outfit_id);
    }
}

void LLOutfitListGearMenuBase::onRename()
{
    const LLUUID& selected_outfit_id = getSelectedOutfitID();
    if (selected_outfit_id.notNull())
    {
        LLAppearanceMgr::instance().renameOutfit(selected_outfit_id);
    }
}

void LLOutfitListGearMenuBase::onCreate(const LLSD& data)
{
    LLWearableType::EType type = LLWearableType::getInstance()->typeNameToType(data.asString());
    if (type == LLWearableType::WT_NONE)
    {
        LL_WARNS() << "Invalid wearable type" << LL_ENDL;
        return;
    }

    LLAgentWearables::createWearable(type, true);
}

bool LLOutfitListGearMenuBase::onEnable(LLSD::String param)
{
    // Handle the "Wear - Replace Current Outfit" menu option specially
    // because LLOutfitList::isActionEnabled() checks whether it's allowed
    // to wear selected outfit OR selected items, while we're only
    // interested in the outfit (STORM-183).
    if ("wear" == param)
    {
        return LLAppearanceMgr::instance().getCanReplaceCOF(mOutfitList->getSelectedOutfitUUID());
    }

    return mOutfitList->isActionEnabled(param);
}

bool LLOutfitListGearMenuBase::onVisible(LLSD::String param)
{
    const LLUUID& selected_outfit_id = getSelectedOutfitID();
    if (selected_outfit_id.isNull()) // no selection or invalid outfit selected
    {
        return false;
    }

    return true;
}

void LLOutfitListGearMenuBase::onThumbnail()
{
    const LLUUID& selected_outfit_id = getSelectedOutfitID();
    LLSD data(selected_outfit_id);
    LLFloaterReg::showInstance("change_item_thumbnail", data);
}

void LLOutfitListGearMenuBase::onChangeSortOrder()
{

}

LLOutfitListGearMenu::LLOutfitListGearMenu(LLOutfitListBase* olist)
    : LLOutfitListGearMenuBase(olist)
{}

LLOutfitListGearMenu::~LLOutfitListGearMenu()
{}

void LLOutfitListGearMenu::onUpdateItemsVisibility()
{
    if (!mMenu) return;
    mMenu->setItemVisible("expand", true);
    mMenu->setItemVisible("collapse", true);
    mMenu->setItemVisible("thumbnail", getSelectedOutfitID().notNull());
    mMenu->setItemVisible("sepatator3", false);
    mMenu->setItemVisible("sort_folders_by_name", false);
    LLOutfitListGearMenuBase::onUpdateItemsVisibility();
}

bool LLOutfitAccordionCtrlTab::handleToolTip(S32 x, S32 y, MASK mask)
{
    if (y >= getLocalRect().getHeight() - getHeaderHeight())
    {
        LLSD params;
        params["inv_type"] = LLInventoryType::IT_CATEGORY;
        params["thumbnail_id"] = gInventory.getCategory(mFolderID)->getThumbnailUUID();
        params["item_id"] = mFolderID;

        LLToolTipMgr::instance().show(LLToolTip::Params()
                                    .message(getToolTip())
                                    .sticky_rect(calcScreenRect())
                                    .delay_time(LLView::getTooltipTimeout())
                                    .create_callback(boost::bind(&LLInspectTextureUtil::createInventoryToolTip, _1))
                                    .create_params(params));
        return true;
    }

    return LLAccordionCtrlTab::handleToolTip(x, y, mask);
}
// EOF
