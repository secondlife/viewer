/** 
 * @file llviewquery.cpp
 * @brief Implementation of view query class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

#include "llview.h"
#include "lluictrl.h"
#include "llviewquery.h"

void LLQuerySorter::operator() (LLView * parent, viewList_t &children) const {}

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

filterResult_t LLWidgetTypeFilter::operator() (const LLView* const view, const viewList_t & children) const
{
	return filterResult_t(view->getWidgetType() == mType, TRUE);
} 

// LLViewQuery

LLViewQuery::LLViewQuery(): mPreFilters(), mPostFilters(), mSorterp() 
{
}

void LLViewQuery::addPreFilter(const LLQueryFilter* prefilter) { mPreFilters.push_back(prefilter); }

void LLViewQuery::addPostFilter(const LLQueryFilter* postfilter) {	mPostFilters.push_back(postfilter); }

const LLViewQuery::filterList_t & LLViewQuery::getPreFilters() const { return mPreFilters; }

const LLViewQuery::filterList_t & LLViewQuery::getPostFilters() const { return mPostFilters; }

void LLViewQuery::setSorter(const LLQuerySorter* sorterp) { mSorterp = sorterp; }
const LLQuerySorter* LLViewQuery::getSorter() const { return mSorterp; }

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

void LLViewQuery::filterChildren(LLView * view, viewList_t & filtered_children) const
{
	LLView::child_list_t views(*(view->getChildList()));
	if (mSorterp)
	{
		(*mSorterp)(view, views); // sort the children per the sorter
	}
	for(LLView::child_list_iter_t iter = views.begin();
			iter != views.end();
			iter++)
		{
			viewList_t indiv_children = this->run(*iter);
			filtered_children.insert(filtered_children.end(), indiv_children.begin(), indiv_children.end());
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

class SortByTabOrder : public LLQuerySorter, public LLSingleton<SortByTabOrder>
{
	/*virtual*/ void operator() (LLView * parent, LLView::child_list_t &children) const 
	{
		children.sort(LLCompareByTabOrder(parent->getCtrlOrder()));
	}
};

LLCtrlQuery::LLCtrlQuery() : 
	LLViewQuery()
{
	setSorter(SortByTabOrder::getInstance());
}

