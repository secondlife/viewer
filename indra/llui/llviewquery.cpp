/** 
 * @file llviewquery.cpp
 * @brief Implementation of view query class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llview.h"
#include "lluictrl.h"
#include "llviewquery.h"

void LLQuerySorter::operator() (LLView * parent, viewList_t &children) const {}

filterResult_t LLNoLeavesFilter::operator() (const LLView* const view, const viewList_t & children) const 
{
	return filterResult_t(!(view->getChildList()->size() == 0), TRUE);
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
	return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl* const>(view)->hasTabStop(),
						view->canFocusChildren());
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

viewList_t LLViewQuery::run(LLView * view) const
{
	viewList_t result;

	filterResult_t pre = runFilters(view, viewList_t(), mPreFilters);
	if(!pre.first && !pre.second)
	{
		// skip post filters completely if we're not including ourselves or the children
		return result;
	}
	if(pre.second)
	{
		// run filters on children
		viewList_t filtered_children;
		filterChildren(view, filtered_children);
		filterResult_t post = runFilters(view, filtered_children, mPostFilters);
		if(pre.first && post.first)
		{
			result.push_back(view);
		}
		if(post.second)
		{
			result.insert(result.end(), filtered_children.begin(), filtered_children.end());
		}
	}
	else 
	{
		if(pre.first) 
		{
			result.push_back(view);
		}
	}
	return result;
}

void LLViewQuery::filterChildren(LLView * view, viewList_t & filtered_children) const
{
	LLView::child_list_t views(*(view->getChildList()));
	(*mSorterp)(view, views); // sort the children per the sorter
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

