/** 
 * @file llviewquery.cpp
 * @brief Implementation of view query class.
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

#include "llview.h"
#include "lluictrl.h"
#include "llviewquery.h"

void LLQuerySorter::sort(LLView * parent, viewList_t &children) const {}

filterResult_t LLLeavesFilter::operator() (const LLView* const view, const viewList_t & children) const 
{
    return filterResult_t(children.empty(), TRUE);
}

filterResult_t LLRootsFilter::operator() (const LLView* const view, const viewList_t & children) const 
{
    return filterResult_t(TRUE, FALSE);
}

filterResult_t LLVisibleFilter::operator() (const LLView* const view, const viewList_t & children) const 
{
    return filterResult_t(view->getVisible(), view->getVisible());
}
filterResult_t LLEnabledFilter::operator() (const LLView* const view, const viewList_t & children) const 
{
    return filterResult_t(view->getEnabled(), view->getEnabled());
}
filterResult_t LLTabStopFilter::operator() (const LLView* const view, const viewList_t & children) const 
{
    return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl*>(view)->hasTabStop(),
                        view->canFocusChildren());
}

filterResult_t LLCtrlFilter::operator() (const LLView* const view, const viewList_t & children) const 
{
    return filterResult_t(view->isCtrl(),TRUE);
}

//
// LLViewQuery
//

viewList_t LLViewQuery::run(LLView* view) const
{
    viewList_t result;

    // prefilter gets immediate children of view
    filterResult_t pre = runFilters(view, *view->getChildList(), mPreFilters);
    if(!pre.first && !pre.second)
    {
        // not including ourselves or the children
        // nothing more to do
        return result;
    }

    viewList_t filtered_children;
    filterResult_t post(TRUE, TRUE);
    if(pre.second)
    {
        // run filters on children
        filterChildren(view, filtered_children);
        // only run post filters if this element passed pre filters
        // so if you failed to pass the pre filter, you can't filter out children in post
        if (pre.first)
        {
            post = runFilters(view, filtered_children, mPostFilters);
        }
    }

    if(pre.first && post.first) 
    {
        result.push_back(view);
    }

    if(pre.second && post.second)
    {
        result.insert(result.end(), filtered_children.begin(), filtered_children.end());
    }

    return result;
}

void LLViewQuery::filterChildren(LLView* parent_view, viewList_t & filtered_children) const
{
    LLView::child_list_t views(*(parent_view->getChildList()));
    if (mSorterp)
    {
        mSorterp->sort(parent_view, views); // sort the children per the sorter
    }
    for(LLView::child_list_iter_t iter = views.begin();
        iter != views.end();
        iter++)
    {
        viewList_t indiv_children = this->run(*iter);
        filtered_children.splice(filtered_children.end(), indiv_children);
    }
}

filterResult_t LLViewQuery::runFilters(LLView * view, const viewList_t children, const filterList_t filters) const
{
    filterResult_t result = filterResult_t(TRUE, TRUE);
    for(filterList_const_iter_t iter = filters.begin();
        iter != filters.end();
        iter++)
    {
        filterResult_t filtered = (**iter)(view, children);
        result.first = result.first && filtered.first;
        result.second = result.second && filtered.second;
    }
    return result;
}
