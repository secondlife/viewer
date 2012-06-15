/** 
 * @file llfoldervieweventlistener.h
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
#ifndef LLFOLDERVIEWEVENTLISTENER_H
#define LLFOLDERVIEWEVENTLISTENER_H

#include "lldarray.h"	// *TODO: convert to std::vector
#include "llfoldertype.h"
#include "llfontgl.h"	// just for StyleFlags enum
#include "llinventorytype.h"
#include "llpermissionsflags.h"
#include "llpointer.h"
#include "llwearabletype.h"
#include "lltooldraganddrop.h"

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
	LLFolderViewFilter() {}
	virtual ~LLFolderViewFilter() {}

	// +-------------------------------------------------------------------+
	// + Execution And Results
	// +-------------------------------------------------------------------+
	virtual bool 				check(const LLFolderViewItem* item) = 0;
	virtual bool				check(const LLInventoryItem* item) = 0;
	virtual bool				checkFolder(const LLFolderViewFolder* folder) const = 0;
	virtual bool				checkFolder(const LLUUID& folder_id) const = 0;

	virtual void 				setEmptyLookupMessage(const std::string& message) = 0;
	const virtual std::string&	getEmptyLookupMessage() const = 0;

	// +-------------------------------------------------------------------+
	// + Status
	// +-------------------------------------------------------------------+
	virtual bool 				isActive() const = 0;
	virtual bool 				isModified() const = 0;
	virtual bool 				isModifiedAndClear() = 0;
	virtual void 				clearModified() = 0;
	virtual const std::string& 	getName() const = 0;
	virtual const std::string& 	getFilterText() = 0;
	//RN: this is public to allow system to externally force a global refilter
	virtual void 				setModified(EFilterBehavior behavior = FILTER_RESTART) = 0;

	// +-------------------------------------------------------------------+
	// + Count
	// +-------------------------------------------------------------------+
	virtual void 				setFilterCount(S32 count) = 0;
	virtual S32 				getFilterCount() const = 0;
	virtual void 				decrementFilterCount() = 0;

	// +-------------------------------------------------------------------+
	// + Default
	// +-------------------------------------------------------------------+
	virtual BOOL 				isNotDefault() const = 0;
	virtual void 				markDefault() = 0;
	virtual void 				resetDefault() = 0;

	// +-------------------------------------------------------------------+
	// + Generation
	// +-------------------------------------------------------------------+
	virtual S32 				getCurrentGeneration() const = 0;
	virtual S32 				getFirstSuccessGeneration() const = 0;
	virtual S32 				getFirstRequiredGeneration() const = 0;
};

struct LLFolderViewModelInterface
{
	virtual void requestSortAll() = 0;
	virtual void requestSort(class LLFolderViewFolder*) = 0;

	virtual void sort(class LLFolderViewFolder*) = 0;
	virtual void filter(class LLFolderViewFolder*) = 0;
};

struct LLFolderViewModelCommon : public LLFolderViewModelInterface
{
	LLFolderViewModelCommon()
	:	mTargetSortVersion(0)
	{}

	virtual void requestSortAll()
	{
		// sort everything
		mTargetSortVersion++;
	}

	virtual void requestSort(class LLFolderViewFolder* folder)
	{
		folder->requestSort();
	}
	
protected:
	bool needsSort(class LLFolderViewModelItem* item)
	{
		return item->getSortVersion() < mTargetSortVersion;
	}

	S32 mTargetSortVersion;
};


// This is am abstract base class that users of the folderview classes
// would use to bridge the folder view with the underlying data
class LLFolderViewModelItem
{
public:
	virtual ~LLFolderViewModelItem( void ) {};

	virtual void update() {}	//called when drawing
	virtual const std::string& getName() const = 0;
	virtual const std::string& getDisplayName() const = 0;

	virtual LLPointer<LLUIImage> getIcon() const = 0;
	virtual LLPointer<LLUIImage> getOpenIcon() const { return getIcon(); }

	virtual LLFontGL::StyleFlags getLabelStyle() const = 0;
	virtual std::string getLabelSuffix() const = 0;

	virtual void openItem( void ) = 0;
	virtual void closeItem( void ) = 0;
	virtual void selectItem(void) = 0;

	virtual BOOL isItemRenameable() const = 0;
	virtual BOOL renameItem(const std::string& new_name) = 0;

	virtual BOOL isItemMovable( void ) const = 0;		// Can be moved to another folder
	virtual void move( LLFolderViewModelItem* parent_listener ) = 0;

	virtual BOOL isItemRemovable( void ) const = 0;		// Can be destroyed
	virtual BOOL removeItem() = 0;
	virtual void removeBatch(std::vector<LLFolderViewModelItem*>& batch) = 0;

	virtual BOOL isItemCopyable() const = 0;
	virtual BOOL copyToClipboard() const = 0;
	virtual void cutToClipboard() = 0;

	virtual BOOL isClipboardPasteable() const = 0;
	virtual void pasteFromClipboard() = 0;
	virtual void pasteLinkFromClipboard() = 0;

	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) = 0;

	// This method should be called when a drag begins. returns TRUE
	// if the drag can begin, otherwise FALSE.
	virtual LLToolDragAndDrop::ESource getDragSource() const = 0;
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const = 0;

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
};

class LLFolderViewModelItemCommon : public LLFolderViewModelItem
{
public:
	LLFolderViewModelItemCommon()
	:	mSortVersion(-1)
	{}

	void requestSort() { mSortVersion = -1; }
	S32 getSortVersion() { return mSortVersion; }
	void setSortVersion(S32 version) { mSortVersion = VERSION;}

protected:
	S32 mSortVersion;
};

template <typename SORT_TYPE, typename ITEM_TYPE, typename FOLDER_TYPE, typename FILTER_TYPE>
class LLFolderViewModel : public LLFolderViewModelCommon
{
protected:
	LLFolderViewModel() {}
	
	typedef SORT_TYPE		SortType;
	typedef ITEM_TYPE		ItemType;
	typedef FOLDER_TYPE		FolderType;
	typedef FILTER_TYPE		FilterType;
	
	virtual const SortType& getSorter() const 		 { return mSorter; }
	virtual void setSorter(const SortType& type) 	 { mSorter = sorter; requestSortAll(); }
	virtual FilterType& getFilter() const 			 { return mFilter; }
	virtual void setFilter(const FilterType& filter) { mFilter = filter; }

public:

	struct ViewModelCompare()
	{
		ViewModelCompare(const SortType& sorter)
		:	mSorter(sorter)
		{}
		
		int operator () (const LLFolderViewItem* a, const LLFolderViewItem* b)
		{
			return mSorter(static_cast<const ItemType*>(a->getListener()), static_cast<const ItemType*>(b->getListener()));
		}

		int operator () (const LLFolderViewFolder* a, const LLFolderViewFolder* b)
		{
			return mSorter(static_cast<const ItemType*>(a->getListener()), static_cast<const ItemType*>(b->getListener()));
		}

		const SortType& mSorter;
	}

	void sort(LLFolderViewFolder* folder)
	{
		if (needsSort(folder))
		{
			std::sort(folder->getFoldersBegin(), folder->getFoldersEnd(), ViewModelCompare(getSorter()));
			std::sort(folder->getItemsBegin(), folder->getItemsEnd(), ViewModelCompare(getSorter()));
			folder->getListener()->setSortVersion(mTargetSortVersion);
			folder->requestArrange();
		}
	}

	void filter(LLFolderViewFolder* folder)
	{
		FilterType& filter = getFilter();
		for (std::list<LLFolderViewItem*>::iterator it = folder->getItemsBegin(), end_it = folder->getItemsEnd();
			it != end_it;
			++it)
		{
			LLFolderViewItem* child_item = *it;
			child_item->setFiltered(filter(static_cast<ItemType*>(child_item->getListener())), filter.getCurrentGeneration())
		}

		for (std::list<LLFolderViewFolder*>::iterator it = folder->getFoldersBegin(), end_it = folder->getFoldersEnd();
			it != end_it;
			++it)
		{
			LLFolderViewItem* child_folder = *it;
			child_folder->setFiltered(filter(static_cast<ItemType*>(child_folder->getListener())), filter.getCurrentGeneration())
		}
	}

protected:
	SortType	mSorter;
	FilterType	mFilter;
};





#endif
