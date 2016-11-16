/** 
 * @file llfolderviewmodel.h
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
#ifndef LLFOLDERVIEWMODEL_H
#define LLFOLDERVIEWMODEL_H

#include "llfontgl.h"	// just for StyleFlags enum
#include "llfolderview.h"

// These are grouping of inventory types.
// Order matters when sorting system folders to the top.
enum EInventorySortGroup
{
	SG_SYSTEM_FOLDER,
	SG_TRASH_FOLDER,
	SG_NORMAL_FOLDER,
	SG_ITEM
};

class LLFontGL;
class LLInventoryModel;
class LLMenuGL;
class LLUIImage;
class LLUUID;
class LLFolderViewItem;
class LLFolderViewFolder;

class LLFolderViewFilter
{
public:
	enum EFilterModified
	{
		FILTER_NONE,				// nothing to do, already filtered
		FILTER_RESTART,				// restart filtering from scratch
		FILTER_LESS_RESTRICTIVE,	// existing filtered items will certainly pass this filter
		FILTER_MORE_RESTRICTIVE		// if you didn't pass the previous filter, you definitely won't pass this one
	};

public:

	LLFolderViewFilter() {}
	virtual ~LLFolderViewFilter() {}

	// +-------------------------------------------------------------------+
	// + Execution And Results
	// +-------------------------------------------------------------------+
	virtual bool 				check(const LLFolderViewModelItem* item) = 0;
	virtual bool				checkFolder(const LLFolderViewModelItem* folder) const = 0;

	virtual void 				setEmptyLookupMessage(const std::string& message) = 0;
	virtual std::string			getEmptyLookupMessage() const = 0;

	virtual bool				showAllResults() const = 0;

	virtual std::string::size_type getStringMatchOffset(LLFolderViewModelItem* item) const = 0;
	virtual std::string::size_type getFilterStringSize() const = 0;
	// +-------------------------------------------------------------------+
	// + Status
	// +-------------------------------------------------------------------+
	virtual bool 				isActive() const = 0;
	virtual bool 				isModified() const = 0;
	virtual void 				clearModified() = 0;
	virtual const std::string& 	getName() const = 0;
	virtual const std::string& 	getFilterText() = 0;
	//RN: this is public to allow system to externally force a global refilter
	virtual void 				setModified(EFilterModified behavior = FILTER_RESTART) = 0;

	// +-------------------------------------------------------------------+
	// + Time
	// +-------------------------------------------------------------------+
	virtual void 				resetTime(S32 timeout) = 0;
    virtual bool                isTimedOut() = 0;
    
	// +-------------------------------------------------------------------+
	// + Default
	// +-------------------------------------------------------------------+
	virtual bool 				isDefault() const = 0;
	virtual bool 				isNotDefault() const = 0;
	virtual void 				markDefault() = 0;
	virtual void 				resetDefault() = 0;

	// +-------------------------------------------------------------------+
	// + Generation
	// +-------------------------------------------------------------------+
	virtual S32 				getCurrentGeneration() const = 0;
	virtual S32 				getFirstSuccessGeneration() const = 0;
	virtual S32 				getFirstRequiredGeneration() const = 0;
};

class LLFolderViewModelInterface : public LLTrace::MemTrackable<LLFolderViewModelInterface>
{
public:
	LLFolderViewModelInterface() 
	:	LLTrace::MemTrackable<LLFolderViewModelInterface>("LLFolderViewModelInterface") 
	{}

	virtual ~LLFolderViewModelInterface() {}
	virtual void requestSortAll() = 0;

	virtual void sort(class LLFolderViewFolder*) = 0;
	virtual void filter() = 0;

	virtual bool contentsReady() = 0;
	virtual bool isFolderComplete(class LLFolderViewFolder*) = 0;
	virtual void setFolderView(LLFolderView* folder_view) = 0;
	virtual LLFolderViewFilter& getFilter() = 0;
	virtual const LLFolderViewFilter& getFilter() const = 0;
	virtual std::string getStatusText() = 0;

	virtual bool startDrag(std::vector<LLFolderViewModelItem*>& items) = 0;
};

// This is an abstract base class that users of the folderview classes
// would use to bridge the folder view with the underlying data
class LLFolderViewModelItem : public LLRefCount, public LLTrace::MemTrackable<LLFolderViewModelItem>
{
public:
	LLFolderViewModelItem() 
	:	LLTrace::MemTrackable<LLFolderViewModelItem>("LLFolderViewModelItem") 
	{}

	virtual ~LLFolderViewModelItem() { }

	virtual void update() {}	//called when drawing
	virtual const std::string& getName() const = 0;
	virtual const std::string& getDisplayName() const = 0;
	virtual const std::string& getSearchableName() const = 0;

	virtual LLPointer<LLUIImage> getIcon() const = 0;
	virtual LLPointer<LLUIImage> getIconOpen() const { return getIcon(); }
	virtual LLPointer<LLUIImage> getIconOverlay() const { return NULL; }

	virtual LLFontGL::StyleFlags getLabelStyle() const = 0;
	virtual std::string getLabelSuffix() const = 0;

	virtual void openItem( void ) = 0;
	virtual void closeItem( void ) = 0;
	virtual void selectItem(void) = 0;
    
    virtual BOOL isItemWearable() const { return FALSE; }

	virtual BOOL isItemRenameable() const = 0;
	virtual BOOL renameItem(const std::string& new_name) = 0;

	virtual BOOL isItemMovable( void ) const = 0;		// Can be moved to another folder
	virtual void move( LLFolderViewModelItem* parent_listener ) = 0;

	virtual BOOL isItemRemovable( void ) const = 0;		// Can be destroyed
	virtual BOOL removeItem() = 0;
	virtual void removeBatch(std::vector<LLFolderViewModelItem*>& batch) = 0;

	virtual BOOL isItemCopyable() const = 0;
	virtual BOOL copyToClipboard() const = 0;
	virtual BOOL cutToClipboard() = 0;
	virtual bool isCutToClipboard() { return false; };

	virtual BOOL isClipboardPasteable() const = 0;
	virtual void pasteFromClipboard() = 0;
	virtual void pasteLinkFromClipboard() = 0;

	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) = 0;
	
	virtual bool potentiallyVisible() = 0; // is the item definitely visible or we haven't made up our minds yet?

	virtual bool filter( LLFolderViewFilter& filter) = 0;
	virtual bool passedFilter(S32 filter_generation = -1) = 0;
	virtual bool descendantsPassedFilter(S32 filter_generation = -1) = 0;
	virtual void setPassedFilter(bool passed, S32 filter_generation, std::string::size_type string_offset = std::string::npos, std::string::size_type string_size = 0) = 0;
	virtual void setPassedFolderFilter(bool passed, S32 filter_generation) = 0;
	virtual void dirtyFilter() = 0;
	virtual void dirtyDescendantsFilter() = 0;
	virtual bool hasFilterStringMatch() = 0;
	virtual std::string::size_type getFilterStringOffset() = 0;
	virtual std::string::size_type getFilterStringSize() = 0;

	virtual S32	getLastFilterGeneration() const = 0;
	virtual S32 getMarkedDirtyGeneration() const = 0;

	virtual bool hasChildren() const = 0;
	virtual void addChild(LLFolderViewModelItem* child) = 0;
	virtual void removeChild(LLFolderViewModelItem* child) = 0;

	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. Returns TRUE if a drop is possible/happened,
	// otherwise FALSE.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg) = 0;

	virtual void requestSort() = 0;
	virtual S32 getSortVersion() = 0;
	virtual void setSortVersion(S32 version) = 0;
	virtual void setParent(LLFolderViewModelItem* parent) = 0;
	virtual bool hasParent() = 0;

protected:

	friend class LLFolderViewItem;
	virtual void setFolderViewItem(LLFolderViewItem* folder_view_item) = 0;

};


class LLFolderViewModelItemCommon : public LLFolderViewModelItem
{
public:
	LLFolderViewModelItemCommon(LLFolderViewModelInterface& root_view_model)
	:	mSortVersion(-1),
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
		mChildren.clear();
	}

	void requestSort() { mSortVersion = -1; }
	S32 getSortVersion() { return mSortVersion; }
	void setSortVersion(S32 version) { mSortVersion = version;}

	S32	getLastFilterGeneration() const { return mLastFilterGeneration; }
	S32	getLastFolderFilterGeneration() const { return mLastFolderFilterGeneration; }
	S32	getMarkedDirtyGeneration() const { return mMarkedDirtyGeneration; }
	void dirtyFilter()
	{
		if(mMarkedDirtyGeneration < 0)
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
	void dirtyDescendantsFilter()
	{
		mMostFilteredDescendantGeneration = -1;
		if (mParent)
		{
			mParent->dirtyDescendantsFilter();
		}
	}
	bool hasFilterStringMatch();
	std::string::size_type getFilterStringOffset();
	std::string::size_type getFilterStringSize();
	
	typedef std::list<LLFolderViewModelItem*> child_list_t;

	virtual void addChild(LLFolderViewModelItem* child) 
	{ 
		// Avoid duplicates: bail out if that child is already present in the list
		// Note: this happens when models are created before views
		child_list_t::const_iterator iter;
		for (iter = mChildren.begin(); iter != mChildren.end(); iter++)
		{
			if (child == *iter)
			{
				return;
			}
		}
		mChildren.push_back(child);
		child->setParent(this); 
		dirtyFilter();
		requestSort();
	}
	virtual void removeChild(LLFolderViewModelItem* child) 
	{ 
		mChildren.remove(child); 
		child->setParent(NULL);
		dirtyDescendantsFilter();
		dirtyFilter();
	}
	
	virtual void clearChildren()
	{
		// As this is cleaning the whole list of children wholesale, we do need to delete the pointed objects
		// This is different and not equivalent to calling removeChild() on each child
		std::for_each(mChildren.begin(), mChildren.end(), DeletePointer());
		mChildren.clear();
		dirtyDescendantsFilter();
		dirtyFilter();
	}
	
	child_list_t::const_iterator getChildrenBegin() const { return mChildren.begin(); }
	child_list_t::const_iterator getChildrenEnd() const { return mChildren.end(); }
	child_list_t::size_type getChildrenCount() const { return mChildren.size(); }
	
	void setPassedFilter(bool passed, S32 filter_generation, std::string::size_type string_offset = std::string::npos, std::string::size_type string_size = 0)
	{
		mPassedFilter = passed;
		mLastFilterGeneration = filter_generation;
		mStringMatchOffsetFilter = string_offset;
		mStringFilterSize = string_size;
		mMarkedDirtyGeneration = -1;
	}

	void setPassedFolderFilter(bool passed, S32 filter_generation)
	{
		mPassedFolderFilter = passed;
		mLastFolderFilterGeneration = filter_generation;
	}

	virtual bool potentiallyVisible()
	{
		return passedFilter() // we've passed the filter
			|| (getLastFilterGeneration() < mRootViewModel.getFilter().getFirstSuccessGeneration()) // or we don't know yet
			|| descendantsPassedFilter();
	}

	virtual bool passedFilter(S32 filter_generation = -1) 
	{ 
		if (filter_generation < 0)
        {
			filter_generation = mRootViewModel.getFilter().getFirstSuccessGeneration();
        }
		bool passed_folder_filter = mPassedFolderFilter && (mLastFolderFilterGeneration >= filter_generation);
		bool passed_filter = mPassedFilter && (mLastFilterGeneration >= filter_generation);
		return passed_folder_filter && (passed_filter || descendantsPassedFilter(filter_generation));
	}

	virtual bool descendantsPassedFilter(S32 filter_generation = -1)
	{ 
		if (filter_generation < 0)
        {
            filter_generation = mRootViewModel.getFilter().getFirstSuccessGeneration();
        }
		return mMostFilteredDescendantGeneration >= filter_generation;
	}


protected:
	virtual void setParent(LLFolderViewModelItem* parent) { mParent = parent; }
	virtual bool hasParent() { return mParent != NULL; }

	S32							mSortVersion;
	bool						mPassedFilter;
	bool						mPassedFolderFilter;
	std::string::size_type		mStringMatchOffsetFilter;
	std::string::size_type		mStringFilterSize;

	S32							mLastFilterGeneration,
								mLastFolderFilterGeneration,
								mMostFilteredDescendantGeneration,
								mMarkedDirtyGeneration;

	child_list_t				mChildren;
	LLFolderViewModelItem*		mParent;
	LLFolderViewModelInterface& mRootViewModel;

	void setFolderViewItem(LLFolderViewItem* folder_view_item) { mFolderViewItem = folder_view_item;}
	LLFolderViewItem*		mFolderViewItem;
};



class LLFolderViewModelCommon : public LLFolderViewModelInterface
{
public:
	LLFolderViewModelCommon()
	:	mTargetSortVersion(0),
		mFolderView(NULL)
	{}

	virtual void requestSortAll()
	{
		// sort everything
		mTargetSortVersion++;
	}
	virtual std::string getStatusText();
	virtual void filter();

	void setFolderView(LLFolderView* folder_view) { mFolderView = folder_view;}

protected:
	bool needsSort(class LLFolderViewModelItem* item);

	S32 mTargetSortVersion;
	LLFolderView* mFolderView;

};

template <typename SORT_TYPE, typename ITEM_TYPE, typename FOLDER_TYPE, typename FILTER_TYPE>
class LLFolderViewModel : public LLFolderViewModelCommon
{
public:
	typedef SORT_TYPE		SortType;
	typedef ITEM_TYPE		ItemType;
	typedef FOLDER_TYPE		FolderType;
	typedef FILTER_TYPE		FilterType;

	LLFolderViewModel(SortType* sorter, FilterType* filter) 
	:	mSorter(sorter),
		mFilter(filter)
	{}

	virtual ~LLFolderViewModel() 
	{
		delete mSorter;
		mSorter = NULL;
		delete mFilter;
		mFilter = NULL;
	}

	virtual SortType& getSorter()					 { return *mSorter; }
	virtual const SortType& getSorter() const 		 { return *mSorter; }
	virtual void setSorter(const SortType& sorter) 	 { mSorter = new SortType(sorter); requestSortAll(); }

	virtual FilterType& getFilter() 				 { return *mFilter; }
	virtual const FilterType& getFilter() const		 { return *mFilter; }
	virtual void setFilter(const FilterType& filter) { mFilter = new FilterType(filter); }

	// By default, we assume the content is available. If a network fetch mechanism is implemented for the model,
	// this method needs to be overloaded and return the relevant fetch status.
	virtual bool contentsReady()					{ return true; }
	virtual bool isFolderComplete(LLFolderViewFolder* folder)					{ return true; }

	struct ViewModelCompare
	{
		ViewModelCompare(const SortType& sorter)
		:	mSorter(sorter)
		{}

		bool operator () (const LLFolderViewItem* a, const LLFolderViewItem* b) const
		{
			return mSorter(static_cast<const ItemType*>(a->getViewModelItem()), static_cast<const ItemType*>(b->getViewModelItem()));
		}

		bool operator () (const LLFolderViewFolder* a, const LLFolderViewFolder* b) const
		{
			return mSorter(static_cast<const ItemType*>(a->getViewModelItem()), static_cast<const ItemType*>(b->getViewModelItem()));
		}

		const SortType& mSorter;
	};

	void sort(LLFolderViewFolder* folder)
	{
		if (needsSort(folder->getViewModelItem()))
		{
			folder->sortFolders(ViewModelCompare(getSorter()));
			folder->sortItems(ViewModelCompare(getSorter()));
			folder->getViewModelItem()->setSortVersion(mTargetSortVersion);
			folder->requestArrange();
		}
	}

protected:
	SortType*		mSorter;
	FilterType*		mFilter;
};

#endif // LLFOLDERVIEWMODEL_H
