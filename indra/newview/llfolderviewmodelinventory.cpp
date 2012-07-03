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
#include "llinventorypanel.h"

//
// class LLFolderViewModelInventory
//
static LLFastTimer::DeclareTimer FTM_INVENTORY_SORT("Sort");

void LLFolderViewModelInventory::sort( LLFolderViewFolder* folder )
{
	LLFastTimer _(FTM_INVENTORY_SORT);

	if (!needsSort(folder->getViewModelItem())) return;

	LLFolderViewModelItemInventory* modelp =   static_cast<LLFolderViewModelItemInventory*>(folder->getViewModelItem());
	if (modelp->getUUID().isNull()) return;

	for (std::list<LLFolderViewFolder*>::iterator it =   folder->getFoldersBegin(), end_it = folder->getFoldersEnd();
		it != end_it;
		++it)
	{
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

void LLFolderViewModelItemInventory::requestSort()
{
	LLFolderViewModelItemCommon::requestSort();
	if (mRootViewModel->getSorter().isByDate())
	{
		// sort by date potentially affects parent folders which use a date
		// derived from newest item in them
		if (mParent)
		{
			mParent->requestSort();
		}
	}
}

bool LLFolderViewModelItemInventory::potentiallyVisible()
{
	return passedFilter() // we've passed the filter
		|| getLastFilterGeneration() < mRootViewModel->getFilter()->getFirstSuccessGeneration() // or we don't know yet
		|| descendantsPassedFilter();
}

bool LLFolderViewModelItemInventory::passedFilter(S32 filter_generation) 
{ 
	if (filter_generation < 0 && mRootViewModel) 
		filter_generation = mRootViewModel->getFilter()->getFirstSuccessGeneration();

	return mPassedFolderFilter 
		&& mLastFilterGeneration >= filter_generation
		&& (mPassedFilter || descendantsPassedFilter(filter_generation));
}

bool LLFolderViewModelItemInventory::descendantsPassedFilter(S32 filter_generation)
{ 
	if (filter_generation < 0) filter_generation = mRootViewModel->getFilter()->getFirstSuccessGeneration();
	return mMostFilteredDescendantGeneration >= filter_generation; 
}

void LLFolderViewModelItemInventory::setPassedFilter(bool passed, bool passed_folder, S32 filter_generation)
{
	mPassedFilter = passed;
	mPassedFolderFilter = passed_folder;
	mLastFilterGeneration = filter_generation;
}

bool LLFolderViewModelItemInventory::filterChildItem( LLFolderViewModelItem* item, LLFolderViewFilter& filter )
{
	bool passed_filter_before = item->passedFilter();
	S32 filter_generation = filter.getCurrentGeneration();
	S32 must_pass_generation = filter.getFirstRequiredGeneration();

	if (item->getLastFilterGeneration() < filter_generation)
	{
		if (item->getLastFilterGeneration() >= must_pass_generation 
			&& !item->passedFilter(must_pass_generation))
		{
			// failed to pass an earlier filter that was a subset of the current one
			// go ahead and flag this item as done
			item->filter(filter);
			if (item->passedFilter())
			{
				llerrs << "Invalid shortcut in inventory filtering!" << llendl;
			}
			item->setPassedFilter(false, false, filter_generation);
		}
		else
		{
			item->filter( filter );
		}
	}

	// track latest generation to pass any child items, for each folder up to root
	if (item->passedFilter())
	{
		LLFolderViewModelItemInventory* view_model = this;
		
		while(view_model && view_model->mMostFilteredDescendantGeneration < filter_generation)
		{
			view_model->mMostFilteredDescendantGeneration = filter_generation;
			view_model = static_cast<LLFolderViewModelItemInventory*>(view_model->mParent);
		}
		
		return !passed_filter_before;
	}
	else // !item->passedfilter()
	{
		return passed_filter_before;
	}
}

bool LLFolderViewModelItemInventory::filter( LLFolderViewFilter& filter)
{
	bool changed = false;

	if(!mChildren.empty()
		&& (getLastFilterGeneration() < filter.getFirstRequiredGeneration() // haven't checked descendants against minimum required generation to pass
			|| descendantsPassedFilter(filter.getFirstRequiredGeneration()))) // or at least one descendant has passed the minimum requirement
	{
		// now query children
		for (child_list_t::iterator iter = mChildren.begin();
			iter != mChildren.end() && filter.getFilterCount() > 0;
			++iter)
		{
			changed |= filterChildItem((*iter), filter);
		}
	}

	if (changed)
	{
		//TODO RN: ensure this still happens, but without dependency on folderview
		LLFolderViewFolder* folder = static_cast<LLFolderViewFolder*>(mFolderViewItem);
		folder->requestArrange();
	}

	// if we didn't use all filter iterations
	// that means we filtered all of our descendants
	// so filter ourselves now
	if (filter.getFilterCount() > 0)
	{
		filter.decrementFilterCount();

		const BOOL passed_filter = filter.check(this);
		const BOOL passed_filter_folder = (getInventoryType() == LLInventoryType::IT_CATEGORY) 
								? filter.checkFolder(this)
								: true;

		setPassedFilter(passed_filter, passed_filter_folder, filter.getCurrentGeneration());
		//TODO RN: create interface for string highlighting
		//mStringMatchOffset = filter.getStringMatchOffset(this);
	}
	return changed;
}

LLFolderViewModelInventory* LLInventoryPanel::getFolderViewModel()
{
	return &mInventoryViewModel;
}


const LLFolderViewModelInventory* LLInventoryPanel::getFolderViewModel() const
{
	return &mInventoryViewModel;
}

bool LLInventorySort::operator()(const LLFolderViewModelItemInventory* const& a, const LLFolderViewModelItemInventory* const& b) const
{
	// ignore sort order for landmarks in the Favorites folder.
	// they should be always sorted as in Favorites bar. See EXT-719
	//TODO RN: fix sorting in favorites folder
	//if (a->getSortGroup() == SG_ITEM
	//	&& b->getSortGroup() == SG_ITEM
	//	&& a->getInventoryType() == LLInventoryType::IT_LANDMARK
	//	&& b->getInventoryType() == LLInventoryType::IT_LANDMARK)
	//{

	//	static const LLUUID& favorites_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);

	//	LLUUID a_uuid = a->getParentFolder()->getUUID();
	//	LLUUID b_uuid = b->getParentFolder()->getUUID();

	//	if ((a_uuid == favorites_folder_id && b_uuid == favorites_folder_id))
	//	{
	//		// *TODO: mantipov: probably it is better to add an appropriate method to LLFolderViewItem
	//		// or to LLInvFVBridge
	//		LLViewerInventoryItem* aitem = (static_cast<const LLItemBridge*>(a))->getItem();
	//		LLViewerInventoryItem* bitem = (static_cast<const LLItemBridge*>(b))->getItem();
	//		if (!aitem || !bitem)
	//			return false;
	//		S32 a_sort = aitem->getSortField();
	//		S32 b_sort = bitem->getSortField();
	//		return a_sort < b_sort;
	//	}
	//}

	// We sort by name if we aren't sorting by date
	// OR if these are folders and we are sorting folders by name.
	bool by_name = (!mByDate 
		|| (mFoldersByName 
		&& (a->getSortGroup() != SG_ITEM)));

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

