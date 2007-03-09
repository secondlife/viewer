/** 
 * @file llviewquery.h
 * @brief Query algorithm for flattening and filtering the view hierarchy.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWQUERY_H
#define LL_LLVIEWQUERY_H

#include <list>	

#include "llmemory.h"

class LLView;

typedef std::list<LLView *>			viewList_t;
typedef std::pair<BOOL, BOOL>		filterResult_t;

// Abstract base class for all filters.
class LLQueryFilter : public LLRefCount
{
public:
	virtual filterResult_t operator() (const LLView* const view, const viewList_t & children) const =0;
};

class LLQuerySorter : public LLRefCount
{
public:
	virtual void operator() (LLView * parent, viewList_t &children) const;
};

class LLNoLeavesFilter : public LLQueryFilter, public LLSingleton<LLNoLeavesFilter>
{
	/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};
class LLVisibleFilter : public LLQueryFilter, public LLSingleton<LLVisibleFilter>
{
	/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};
class LLEnabledFilter : public LLQueryFilter, public LLSingleton<LLEnabledFilter>
{
	/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};
class LLTabStopFilter : public LLQueryFilter, public LLSingleton<LLTabStopFilter>
{
	/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};

// Algorithm for flattening
class LLViewQuery
{
public:
	typedef std::list<const LLQueryFilter*>		filterList_t;
	typedef filterList_t::iterator				filterList_iter_t;
	typedef filterList_t::const_iterator		filterList_const_iter_t;

	LLViewQuery();
	virtual ~LLViewQuery() {}

	void addPreFilter(const LLQueryFilter* prefilter);
	void addPostFilter(const LLQueryFilter* postfilter);
	const filterList_t & getPreFilters() const;
	const filterList_t & getPostFilters() const;

	void setSorter(const LLQuerySorter* sorter);
	const LLQuerySorter* getSorter() const;

	viewList_t run(LLView * view) const;
	// syntactic sugar
	viewList_t operator () (LLView * view) const { return run(view); }
protected:
	// override this method to provide iteration over other types of children
	virtual void filterChildren(LLView * view, viewList_t & filtered_children) const;
	filterResult_t runFilters(LLView * view, const viewList_t children, const filterList_t filters) const;
protected:
	filterList_t mPreFilters;
	filterList_t mPostFilters;
	const LLQuerySorter* mSorterp;
};

class LLCtrlQuery : public LLViewQuery
{
public:
	LLCtrlQuery();
};

#endif
