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
    static LLCachedControl<S32> max_time(*LLUI::getInstance()->mSettingGroups["config"], "FilterItemsMaxTimePerFrameVisible", 10);
    getFilter().resetTime(llclamp(max_time(), 1, 100));
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
