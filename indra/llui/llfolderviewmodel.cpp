/** 
 * @file llfolderviewmodel.cpp
 * @brief Implementation of the view model collection of classes.
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

#include "linden_common.h"

#include "llfolderviewmodel.h"
#include "lltrans.h"

// LLFolderViewModelItemCommon

LLFolderViewModelItemCommon::LLFolderViewModelItemCommon(LLFolderViewModelInterface& root_view_model)
    : mSortVersion(-1),
    mPassedFilter(true),
    mPassedFolderFilter(true),
    mStringMatchOffsetFilter(std::string::npos),
    mStringFilterSize(0),
    mFolderViewItem(NULL),
    mLastFilterGeneration(-1),
    mLastFolderFilterGeneration(-1),
    mMarkedDirtyGeneration(-1),
    mMostFilteredDescendantGeneration(-1),
    mParent(NULL),
    mRootViewModel(root_view_model)
{
    mChildren.clear(); //???
}

LLFolderViewModelItemCommon::~LLFolderViewModelItemCommon()
{
    // Children don't belong to model, but to LLFolderViewItem, just mark them as having no parent
    std::for_each(mChildren.begin(), mChildren.end(), [](LLFolderViewModelItem* c) {c->setParent(NULL); });

    // Don't leave dead pointer in parent
    if (mParent)
    {
        mParent->removeChild(this);
    }
}

void LLFolderViewModelItemCommon::dirtyFilter()
{
    if (mMarkedDirtyGeneration < 0)
    {
        mMarkedDirtyGeneration = mLastFilterGeneration;
    }
    mLastFilterGeneration = -1;
    mLastFolderFilterGeneration = -1;

    // bubble up dirty flag all the way to root
    if (mParent)
    {
        mParent->dirtyFilter();
    }
}

void LLFolderViewModelItemCommon::dirtyDescendantsFilter()
{
    mMostFilteredDescendantGeneration = -1;
    if (mParent)
    {
        mParent->dirtyDescendantsFilter();
    }
}

//virtual
void LLFolderViewModelItemCommon::addChild(LLFolderViewModelItem* child)
{
    mChildren.push_back(child);
    child->setParent(this);
    dirtyFilter();
    requestSort();
}

//virtual
void LLFolderViewModelItemCommon::removeChild(LLFolderViewModelItem* child)
{
    mChildren.remove(child);
    child->setParent(NULL);
    dirtyDescendantsFilter();
    dirtyFilter();
}

//virtual
void LLFolderViewModelItemCommon::clearChildren()
{
    // As this is cleaning the whole list of children wholesale, we do need to delete the pointed objects
    // This is different and not equivalent to calling removeChild() on each child
    std::for_each(mChildren.begin(), mChildren.end(), DeletePointer());
    mChildren.clear();
    dirtyDescendantsFilter();
    dirtyFilter();
}

void LLFolderViewModelItemCommon::setPassedFilter(bool passed, S32 filter_generation, std::string::size_type string_offset /*= std::string::npos*/, std::string::size_type string_size /*= 0*/)
{
    mPassedFilter = passed;
    mLastFilterGeneration = filter_generation;
    mStringMatchOffsetFilter = string_offset;
    mStringFilterSize = string_size;
    mMarkedDirtyGeneration = -1;
}

void LLFolderViewModelItemCommon::setPassedFolderFilter(bool passed, S32 filter_generation)
{
    mPassedFolderFilter = passed;
    mLastFolderFilterGeneration = filter_generation;
}

//virtual
bool LLFolderViewModelItemCommon::potentiallyVisible()
{
    return passedFilter() // we've passed the filter
        || (getLastFilterGeneration() < mRootViewModel.getFilter().getFirstSuccessGeneration()) // or we don't know yet
        || descendantsPassedFilter();
}

//virtual
bool LLFolderViewModelItemCommon::passedFilter(S32 filter_generation /*= -1*/)
{
    if (filter_generation < 0)
    {
        filter_generation = mRootViewModel.getFilter().getFirstSuccessGeneration();
    }
    bool passed_folder_filter = mPassedFolderFilter && (mLastFolderFilterGeneration >= filter_generation);
    bool passed_filter = mPassedFilter && (mLastFilterGeneration >= filter_generation);
    return passed_folder_filter && (passed_filter || descendantsPassedFilter(filter_generation));
}

//virtual
bool LLFolderViewModelItemCommon::descendantsPassedFilter(S32 filter_generation /*= -1*/)
{
    if (filter_generation < 0)
    {
        filter_generation = mRootViewModel.getFilter().getFirstSuccessGeneration();
    }
    return mMostFilteredDescendantGeneration >= filter_generation;
}

// LLFolderViewModelCommon

LLFolderViewModelCommon::LLFolderViewModelCommon()
    : mTargetSortVersion(0),
    mFolderView(NULL)
{}

//virtual
void LLFolderViewModelCommon::requestSortAll()
{
    // sort everything
    mTargetSortVersion++;
}

bool LLFolderViewModelCommon::needsSort(LLFolderViewModelItem* item)
{
	return item->getSortVersion() < mTargetSortVersion;
}

std::string LLFolderViewModelCommon::getStatusText()
{
	if (!contentsReady() || mFolderView->getViewModelItem()->getLastFilterGeneration() < getFilter().getCurrentGeneration())
	{
		return LLTrans::getString("Searching");
	}
	else
	{
		return getFilter().getEmptyLookupMessage();
	}
}

void LLFolderViewModelCommon::filter()
{
    static LLCachedControl<S32> filter_visible(*LLUI::getInstance()->mSettingGroups["config"], "FilterItemsMaxTimePerFrameVisible", 10);
    getFilter().resetTime(llclamp(filter_visible(), 1, 100));
	mFolderView->getViewModelItem()->filter(getFilter());
}

bool LLFolderViewModelItemCommon::hasFilterStringMatch()
{
	return mStringMatchOffsetFilter != std::string::npos;
}

std::string::size_type LLFolderViewModelItemCommon::getFilterStringOffset()
{
	return mStringMatchOffsetFilter;
}

std::string::size_type LLFolderViewModelItemCommon::getFilterStringSize()
{
	return mRootViewModel.getFilter().getFilterStringSize();
}
