/**
 * @file llgltffolderviews.cpp
 * @author Andrey Kleshchev
 * @brief LLFloaterFontTest class implementation
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llgltffolderviews.h"
#include "llinventoryicon.h"

/// LLGLTFItem

LLGLTFItem::LLGLTFItem(std::string display_name, LLFolderViewModelInterface& root_view_model)
    : LLFolderViewModelItemCommon(root_view_model)
    , mName(display_name)
{
    init();
}

LLGLTFItem::LLGLTFItem(LLFolderViewModelInterface& root_view_model)
    : LLFolderViewModelItemCommon(root_view_model)
{
    init();
}

LLGLTFItem::~LLGLTFItem()
{

}

void LLGLTFItem::init()
{
    pIcon = LLInventoryIcon::getIcon(LLInventoryType::ICONNAME_OBJECT);
}


bool LLGLTFItem::filterChildItem(LLFolderViewModelItem* item, LLFolderViewFilter& filter)
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
            LLGLTFItem* view_model = this;

            while (view_model && view_model->mMostFilteredDescendantGeneration < filter_generation)
            {
                view_model->mMostFilteredDescendantGeneration = filter_generation;
                view_model = static_cast<LLGLTFItem*>(view_model->mParent);
            }
        }
    }
    return continue_filtering;
}

bool LLGLTFItem::filter(LLFolderViewFilter& filter)
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

    bool is_folder = true;
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
            LLGLTFItem* view_model = this;
            while (view_model && view_model->mMostFilteredDescendantGeneration < filter_generation)
            {
                view_model->mMostFilteredDescendantGeneration = filter_generation;
                view_model = static_cast<LLGLTFItem*>(view_model->mParent);
            }
        }
        setPassedFilter(passed_filter, filter_generation, filter.getStringMatchOffset(this), filter.getFilterStringSize());
        continue_filtering = !filter.isTimedOut();
    }
    return continue_filtering;
}
