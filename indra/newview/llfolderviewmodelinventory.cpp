/* 
 * @file llfolderviewmodelinventory.cpp
 * @brief Implementation of the inventory-specific view model
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llfolderviewmodelinventory.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "lltooldraganddrop.h"
#include "llfavoritesbar.h"

//
// class LLFolderViewModelInventory
//
static LLTrace::BlockTimerStatHandle FTM_INVENTORY_SORT("Inventory Sort");

bool LLFolderViewModelInventory::startDrag(std::vector<LLFolderViewModelItem*>& items)
{
    std::vector<EDragAndDropType> types;
    uuid_vec_t cargo_ids;
    std::vector<LLFolderViewModelItem*>::iterator item_it;
    bool can_drag = true;
    if (!items.empty())
    {
        for (item_it = items.begin(); item_it != items.end(); ++item_it)
        {
            EDragAndDropType type = DAD_NONE;
            LLUUID id = LLUUID::null;
            can_drag = can_drag && static_cast<LLFolderViewModelItemInventory*>(*item_it)->startDrag(&type, &id);

            types.push_back(type);
            cargo_ids.push_back(id);
        }

        LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, 
            static_cast<LLFolderViewModelItemInventory*>(items.front())->getDragSource(), mTaskID); 
    }
    return can_drag;
}


void LLFolderViewModelInventory::sort( LLFolderViewFolder* folder )
{
    LL_RECORD_BLOCK_TIME(FTM_INVENTORY_SORT);

    if (!folder->areChildrenInited() || !needsSort(folder->getViewModelItem())) return;

    LLFolderViewModelItemInventory* modelp =   static_cast<LLFolderViewModelItemInventory*>(folder->getViewModelItem());
    if (modelp->getUUID().isNull()) return;

    for (std::list<LLFolderViewFolder*>::iterator it =   folder->getFoldersBegin(), end_it = folder->getFoldersEnd();
        it != end_it;
        ++it)
    {
        // Recursive call to sort() on child (CHUI-849)
        LLFolderViewFolder* child_folderp = *it;
        sort(child_folderp);

        if (child_folderp->getFoldersCount() > 0)
        {
            time_t most_recent_folder_time =
                static_cast<LLFolderViewModelItemInventory*>((*child_folderp->getFoldersBegin())->getViewModelItem())->getCreationDate();
            LLFolderViewModelItemInventory* modelp =   static_cast<LLFolderViewModelItemInventory*>(child_folderp->getViewModelItem());
            if (most_recent_folder_time > modelp->getCreationDate())
            {
                modelp->setCreationDate(most_recent_folder_time);
            }
        }
        if (child_folderp->getItemsCount() > 0)         
        {
            time_t most_recent_item_time =
                static_cast<LLFolderViewModelItemInventory*>((*child_folderp->getItemsBegin())->getViewModelItem())->getCreationDate();

            LLFolderViewModelItemInventory* modelp =   static_cast<LLFolderViewModelItemInventory*>(child_folderp->getViewModelItem());
            if (most_recent_item_time > modelp->getCreationDate())
            {
                modelp->setCreationDate(most_recent_item_time);
            }
        }
    }
    base_t::sort(folder);
}

bool LLFolderViewModelInventory::contentsReady()
{
    return !LLInventoryModelBackgroundFetch::instance().folderFetchActive();
}

bool LLFolderViewModelInventory::isFolderComplete(LLFolderViewFolder* folder)
{
    LLFolderViewModelItemInventory* modelp = static_cast<LLFolderViewModelItemInventory*>(folder->getViewModelItem());
    LLUUID cat_id = modelp->getUUID();
    if (cat_id.isNull())
    {
        return false;
    }
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
    if (cat)
    {
        // don't need to check version - descendents_server == -1 if we have no data
        S32 descendents_server = cat->getDescendentCount();
        S32 descendents_actual = cat->getViewerDescendentCount();
        if (descendents_server == descendents_actual
            || (descendents_actual > 0 && descendents_server == -1)) // content was loaded in previous session
        {
            return true;
        }
    }
    return false;
}

//virtual
void LLFolderViewModelItemInventory::addChild(LLFolderViewModelItem* child)
{
    LLFolderViewModelItemInventory* model_child = static_cast<LLFolderViewModelItemInventory*>(child);
    mLastAddedChildCreationDate = model_child->getCreationDate();

    // this will requestSort()
    LLFolderViewModelItemCommon::addChild(child);
}

void LLFolderViewModelItemInventory::requestSort()
{
    LLFolderViewModelItemCommon::requestSort();
    LLFolderViewFolder* folderp = dynamic_cast<LLFolderViewFolder*>(mFolderViewItem);
    if (folderp)
    {
        folderp->requestArrange();
    }
    LLInventorySort sorter = static_cast<LLFolderViewModelInventory&>(mRootViewModel).getSorter();

    // Sort by date potentially affects parent folders which use a date
    // derived from newest item in them
    if (sorter.isByDate() && mParent)
    {
        // If this is an item, parent needs to be resorted
        // This case shouldn't happen, unless someone calls item->requestSort()
        if (!folderp)
        {
            mParent->requestSort();
        }
        // if this is a folder, check sort rules for folder first
        else if (sorter.isFoldersByDate())
        {
            if (mLastAddedChildCreationDate == -1  // nothing was added, some other reason for resort
                || mLastAddedChildCreationDate > getCreationDate()) // newer child
            {
                LLFolderViewModelItemInventory* model_parent = static_cast<LLFolderViewModelItemInventory*>(mParent);
                model_parent->mLastAddedChildCreationDate = mLastAddedChildCreationDate;
                mParent->requestSort();
            }
        }
    }
    mLastAddedChildCreationDate = -1;
}

void LLFolderViewModelItemInventory::setPassedFilter(bool passed, S32 filter_generation, std::string::size_type string_offset, std::string::size_type string_size)
{
    bool generation_skip = mMarkedDirtyGeneration >= 0
        && mPrevPassedAllFilters
        && mMarkedDirtyGeneration < mRootViewModel.getFilter().getFirstSuccessGeneration();
    LLFolderViewModelItemCommon::setPassedFilter(passed, filter_generation, string_offset, string_size);
    bool before = mPrevPassedAllFilters;
    mPrevPassedAllFilters = passedFilter(filter_generation);

    if (before != mPrevPassedAllFilters || generation_skip)
    {
        // Need to rearrange the folder if the filtered state of the item changed,
        // previously passed item skipped filter generation changes while being dirty
        // or previously passed not yet filtered item was marked dirty
        LLFolderViewFolder* parent_folder = mFolderViewItem->getParentFolder();
        if (parent_folder)
        {
            parent_folder->requestArrange();
        }
    }
}




bool LLFolderViewModelItemInventory::filterChildItem( LLFolderViewModelItem* item, LLFolderViewFilter& filter )
{
    S32 filter_generation = filter.getCurrentGeneration();

    bool continue_filtering = true;
    if (item)
    {
        if (item->getLastFilterGeneration() < filter_generation)
        {
            // Recursive application of the filter for child items (CHUI-849)
            continue_filtering = item->filter(filter);
        }

        // Update latest generation to pass filter in parent and propagate up to root
        if (item->passedFilter())
        {
            LLFolderViewModelItemInventory* view_model = this;

            while (view_model && view_model->mMostFilteredDescendantGeneration < filter_generation)
            {
                view_model->mMostFilteredDescendantGeneration = filter_generation;
                view_model = static_cast<LLFolderViewModelItemInventory*>(view_model->mParent);
            }
        }
    }
    return continue_filtering;
}

bool LLFolderViewModelItemInventory::filter( LLFolderViewFilter& filter)
{
    const S32 filter_generation = filter.getCurrentGeneration();
    const S32 must_pass_generation = filter.getFirstRequiredGeneration();

    if (getLastFilterGeneration() >= must_pass_generation
        && getLastFolderFilterGeneration() >= must_pass_generation
        && !passedFilter(must_pass_generation))
    {
        // failed to pass an earlier filter that was a subset of the current one
        // go ahead and flag this item as not pass
        setPassedFilter(false, filter_generation);
        setPassedFolderFilter(false, filter_generation);
        return true;
    }

    // *TODO : Revise the logic for fast pass on less restrictive filter case
    /*
     const S32 sufficient_pass_generation = filter.getFirstSuccessGeneration();
    if (getLastFilterGeneration() >= sufficient_pass_generation
        && getLastFolderFilterGeneration() >= sufficient_pass_generation
        && passedFilter(sufficient_pass_generation))
    {
        // passed an earlier filter that was a superset of the current one
        // go ahead and flag this item as pass
        setPassedFilter(true, filter_generation);
        setPassedFolderFilter(true, filter_generation);
        return true;
    }
     */

    bool is_folder = (getInventoryType() == LLInventoryType::IT_CATEGORY);
    const bool passed_filter_folder = is_folder ? filter.checkFolder(this) : true;
    setPassedFolderFilter(passed_filter_folder, filter_generation);

    bool continue_filtering = true;

    if (!mChildren.empty()
        && (getLastFilterGeneration() < must_pass_generation // haven't checked descendants against minimum required generation to pass
            || descendantsPassedFilter(must_pass_generation))) // or at least one descendant has passed the minimum requirement
    {
        // now query children
        for (child_list_t::iterator iter = mChildren.begin(), end_iter = mChildren.end(); iter != end_iter; ++iter)
        {
            continue_filtering = filterChildItem((*iter), filter);
            if (!continue_filtering)
            {
                break;
            }
        }
    }

    // If we didn't use all the filter time that means we filtered all of our descendants so we can filter ourselves now
    if (continue_filtering)
    {
        // This is where filter check on the item done (CHUI-849)
        const bool passed_filter = filter.check(this);
        if (passed_filter && mChildren.empty() && is_folder) // Update the latest filter generation for empty folders
        {
            LLFolderViewModelItemInventory* view_model = this;
            while (view_model && view_model->mMostFilteredDescendantGeneration < filter_generation)
            {
                view_model->mMostFilteredDescendantGeneration = filter_generation;
                view_model = static_cast<LLFolderViewModelItemInventory*>(view_model->mParent);
            }
        }
        setPassedFilter(passed_filter, filter_generation, filter.getStringMatchOffset(this), filter.getFilterStringSize());
        continue_filtering = !filter.isTimedOut();
    }
    return continue_filtering;
}

//LLFolderViewModelInventory* LLInventoryPanel::getFolderViewModel()
//{
//  return &mInventoryViewModel;
//}
//
//
//const LLFolderViewModelInventory* LLInventoryPanel::getFolderViewModel() const
//{
//  return &mInventoryViewModel;
//}

bool LLInventorySort::operator()(const LLFolderViewModelItemInventory* const& a, const LLFolderViewModelItemInventory* const& b) const
{
    // Ignore sort order for landmarks in the Favorites folder.
    // In that folder, landmarks should be always sorted as in the Favorites bar. See EXT-719
    if (a->getSortGroup() == SG_ITEM
        && b->getSortGroup() == SG_ITEM
        && a->getInventoryType() == LLInventoryType::IT_LANDMARK
        && b->getInventoryType() == LLInventoryType::IT_LANDMARK)
    {
        static const LLUUID& favorites_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
        // If both landmarks are in the Favorites folder...
        if (gInventory.isObjectDescendentOf(a->getUUID(), favorites_folder_id) && gInventory.isObjectDescendentOf(b->getUUID(), favorites_folder_id))
        {
            // Get their index in that folder
            S32 a_sort = LLFavoritesOrderStorage::instance().getSortIndex(a->getUUID());
            S32 b_sort = LLFavoritesOrderStorage::instance().getSortIndex(b->getUUID());
            // Note: this test is a bit overkill: since they are both in the Favorites folder, we shouldn't get negative index values...
            if (!((a_sort < 0) && (b_sort < 0)))
            {
                return a_sort < b_sort;
            }
        }
    }

    // We sort by name if we aren't sorting by date
    // OR if these are folders and we are sorting folders by name.
    bool by_name = ((!mByDate || (mFoldersByName && (a->getSortGroup() != SG_ITEM))) && !mFoldersByWeight);

    if (a->getSortGroup() != b->getSortGroup())
    {
        if (mSystemToTop)
        {
            // Group order is System Folders, Trash, Normal Folders, Items
            return (a->getSortGroup() < b->getSortGroup());
        }
        else if (mByDate)
        {
            // Trash needs to go to the bottom if we are sorting by date
            if ( (a->getSortGroup() == SG_TRASH_FOLDER)
                || (b->getSortGroup() == SG_TRASH_FOLDER))
            {
                return (b->getSortGroup() == SG_TRASH_FOLDER);
            }
        }
    }

    if (by_name)
    {
        S32 compare = LLStringUtil::compareDict(a->getDisplayName(), b->getDisplayName());
        if (0 == compare)
        {
            return (a->getCreationDate() > b->getCreationDate());
        }
        else
        {
            return (compare < 0);
        }
    }
    else if (mFoldersByWeight)
    {
        S32 weight_a = compute_stock_count(a->getUUID());
        S32 weight_b = compute_stock_count(b->getUUID());
        if (weight_a == weight_b)
        {
            // Equal weight -> use alphabetical order
            return (LLStringUtil::compareDict(a->getDisplayName(), b->getDisplayName()) < 0);
        }
        else if (weight_a == COMPUTE_STOCK_INFINITE)
        {
            // No stock -> move a at the end of the list
            return false;
        }
        else if (weight_b == COMPUTE_STOCK_INFINITE)
        {
            // No stock -> move b at the end of the list
            return true;
        }
        else
        {
            // Lighter is first (sorted in increasing order of weight)
            return (weight_a < weight_b);
        }
    }
    else
    {
        time_t first_create = a->getCreationDate();
        time_t second_create = b->getCreationDate();
        if (first_create == second_create)
        {
            return (LLStringUtil::compareDict(a->getDisplayName(), b->getDisplayName()) < 0);
        }
        else
        {
            return (first_create > second_create);
        }
    }
}

LLFolderViewModelItemInventory::LLFolderViewModelItemInventory( class LLFolderViewModelInventory& root_view_model ) :
    LLFolderViewModelItemCommon(root_view_model),
    mPrevPassedAllFilters(false),
    mLastAddedChildCreationDate(-1)
{
}
