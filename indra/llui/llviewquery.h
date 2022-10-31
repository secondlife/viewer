/** 
 * @file llviewquery.h
 * @brief Query algorithm for flattening and filtering the view hierarchy.
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

#ifndef LL_LLVIEWQUERY_H
#define LL_LLVIEWQUERY_H

#include <list> 

#include "llsingleton.h"
#include "llui.h"

class LLView;

typedef std::list<LLView *>         viewList_t;
typedef std::pair<BOOL, BOOL>       filterResult_t;

// Abstract base class for all query filters.
class LLQueryFilter
{
public:
    virtual ~LLQueryFilter() {};
    virtual filterResult_t operator() (const LLView* const view, const viewList_t & children) const = 0;
};

class LLQuerySorter
{
public:
    virtual ~LLQuerySorter() {};
    virtual void sort(LLView * parent, viewList_t &children) const;
};

class LLLeavesFilter : public LLQueryFilter, public LLSingleton<LLLeavesFilter>
{
    LLSINGLETON_EMPTY_CTOR(LLLeavesFilter);
    /*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};

class LLRootsFilter : public LLQueryFilter, public LLSingleton<LLRootsFilter>
{
    LLSINGLETON_EMPTY_CTOR(LLRootsFilter);
    /*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};

class LLVisibleFilter : public LLQueryFilter, public LLSingleton<LLVisibleFilter>
{
    LLSINGLETON_EMPTY_CTOR(LLVisibleFilter);
    /*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};

class LLEnabledFilter : public LLQueryFilter, public LLSingleton<LLEnabledFilter>
{
    LLSINGLETON_EMPTY_CTOR(LLEnabledFilter);
    /*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};

class LLTabStopFilter : public LLQueryFilter, public LLSingleton<LLTabStopFilter>
{
    LLSINGLETON_EMPTY_CTOR(LLTabStopFilter);
    /*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};

class LLCtrlFilter : public LLQueryFilter, public LLSingleton<LLCtrlFilter>
{
    LLSINGLETON_EMPTY_CTOR(LLCtrlFilter);
    /*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const;
};

template <class T>
class LLWidgetTypeFilter : public LLQueryFilter
{
    /*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const
    {
        return filterResult_t(dynamic_cast<const T*>(view) != NULL, TRUE);
    }

};

// Algorithm for flattening
class LLViewQuery
{
public:
    typedef std::list<const LLQueryFilter*>     filterList_t;
    typedef filterList_t::iterator              filterList_iter_t;
    typedef filterList_t::const_iterator        filterList_const_iter_t;

    LLViewQuery() : mPreFilters(), mPostFilters(), mSorterp() {}
    virtual ~LLViewQuery() {}

    void addPreFilter(const LLQueryFilter* prefilter) { mPreFilters.push_back(prefilter); }
    void addPostFilter(const LLQueryFilter* postfilter) { mPostFilters.push_back(postfilter); }
    const filterList_t & getPreFilters() const { return mPreFilters; }
    const filterList_t & getPostFilters() const { return mPostFilters; }

    void setSorter(const LLQuerySorter* sorter) { mSorterp = sorter; }
    const LLQuerySorter* getSorter() const { return mSorterp; }

    viewList_t run(LLView * view) const;
    // syntactic sugar
    viewList_t operator () (LLView * view) const { return run(view); }

    // override this method to provide iteration over other types of children
    virtual void filterChildren(LLView * view, viewList_t& filtered_children) const;

private:

    filterResult_t runFilters(LLView * view, const viewList_t children, const filterList_t filters) const;

    filterList_t mPreFilters;
    filterList_t mPostFilters;
    const LLQuerySorter* mSorterp;
};


#endif // LL_LLVIEWQUERY_H
