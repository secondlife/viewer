/** 
 * @file llinventoryobserver.h
 * @brief LLInventoryObserver class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLINVENTORYOBSERVERS_H
#define LL_LLINVENTORYOBSERVERS_H

#include "lluuid.h"
#include "llmd5.h"
#include <string>
#include <vector>

class LLViewerInventoryCategory;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryObserver
//
//   A simple abstract base class that can relay messages when the inventory 
//   changes.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryObserver
{
public:
	// This enumeration is a way to refer to what changed in a more
	// human readable format. You can mask the value provided by
	// chaged() to see if the observer is interested in the change.
	enum 
	{
		NONE 			= 0,
		LABEL 			= 1,	// Name changed
		INTERNAL 		= 2,	// Internal change (e.g. asset uuid different)
		ADD 			= 4,	// Something added
		REMOVE 			= 8,	// Something deleted
		STRUCTURE 		= 16,	// Structural change (e.g. item or folder moved)
		CALLING_CARD	= 32,	// Calling card change (e.g. online, grant status, cancel)
		GESTURE 		= 64,
		REBUILD 		= 128, 	// Item UI changed (e.g. item type different)
		SORT 			= 256, 	// Folder needs to be resorted.
		ALL 			= 0xffffffff
	};
	LLInventoryObserver();
	virtual ~LLInventoryObserver();
	virtual void changed(U32 mask) = 0;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchObserver
//
//   Abstract class to handle fetching items, folders, etc.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchObserver(const LLUUID& id = LLUUID::null); // single item
	LLInventoryFetchObserver(const uuid_vec_t& ids); // multiple items
	void setFetchID(const LLUUID& id);
	void setFetchIDs(const uuid_vec_t& ids);

	BOOL isFinished() const;

	virtual void startFetch() = 0;
	virtual void changed(U32 mask) = 0;
	virtual void done() {};
protected:
	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
	uuid_vec_t mIDs;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchItemsObserver
//
//   Fetches inventory items, calls done() when all inventory has arrived. 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchItemsObserver : public LLInventoryFetchObserver
{
public:
	LLInventoryFetchItemsObserver(const LLUUID& item_id = LLUUID::null); 
	LLInventoryFetchItemsObserver(const uuid_vec_t& item_ids); 

	/*virtual*/ void startFetch();
	/*virtual*/ void changed(U32 mask);
private:
	LLTimer mFetchingPeriod;

	/**
	 * Period of waiting a notification when requested items get added into inventory.
	 */
	static const F32 FETCH_TIMER_EXPIRY;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchDescendentsObserver
//
//   Fetches children of a category/folder, calls done() when all 
//   inventory has arrived. 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchDescendentsObserver : public LLInventoryFetchObserver
{
public:
	LLInventoryFetchDescendentsObserver(const LLUUID& cat_id = LLUUID::null);
	LLInventoryFetchDescendentsObserver(const uuid_vec_t& cat_ids);

	/*virtual*/ void startFetch();
	/*virtual*/ void changed(U32 mask);
protected:
	BOOL isCategoryComplete(const LLViewerInventoryCategory* cat) const;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchComboObserver
//
//   Does an appropriate combination of fetch descendents and
//   item fetches based on completion of categories and items. This is optimized
//   to not fetch item_ids that are descendents of any of the folder_ids.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchComboObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchComboObserver(const uuid_vec_t& folder_ids,
								  const uuid_vec_t& item_ids);
	~LLInventoryFetchComboObserver();
	/*virtual*/ void changed(U32 mask);
	void startFetch();

	virtual void done() = 0;
protected:
	LLInventoryFetchItemsObserver *mFetchItems;
	LLInventoryFetchDescendentsObserver *mFetchDescendents;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryExistenceObserver
//
//   Used as a base class for doing something when all the
//   observed item ids exist in the inventory somewhere.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryExistenceObserver : public LLInventoryObserver
{
public:
	LLInventoryExistenceObserver() {}
	/*virtual*/ void changed(U32 mask);

	void watchItem(const LLUUID& id);
protected:
	virtual void done() = 0;
	uuid_vec_t mExist;
	uuid_vec_t mMIA;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryMovedObserver
//
// This class is used as a base class for doing something when all the
// item for observed asset ids were added into the inventory.
// Derive a class from this class and implement the done() method to do
// something useful.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryAddItemByAssetObserver : public LLInventoryObserver
{
public:
	LLInventoryAddItemByAssetObserver() : mIsDirty(false) {}
	virtual void changed(U32 mask);

	void watchAsset(const LLUUID& asset_id);
	bool isAssetWatched(const LLUUID& asset_id);

protected:
	virtual void onAssetAdded(const LLUUID& asset_id) {}
	virtual void done() = 0;

	typedef std::vector<LLUUID> item_ref_t;
	item_ref_t mAddedItems;
	item_ref_t mWatchedAssets;

private:
	bool mIsDirty;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryAddedObserver
//
//   Base class for doing something when a new item arrives in inventory.
//   It does not watch for a certain UUID, rather it acts when anything is added
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryAddedObserver : public LLInventoryObserver
{
public:
	LLInventoryAddedObserver() : mAdded() {}
	/*virtual*/ void changed(U32 mask);

protected:
	virtual void done() = 0;

	uuid_vec_t mAdded;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryTransactionObserver
//
//   Base class for doing something when an inventory transaction completes.
//   NOTE: This class is not quite complete. Avoid using unless you fix up its
//   functionality gaps.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryTransactionObserver : public LLInventoryObserver
{
public:
	LLInventoryTransactionObserver(const LLTransactionID& transaction_id);
	/*virtual*/ void changed(U32 mask);

protected:
	virtual void done(const uuid_vec_t& folders, const uuid_vec_t& items) = 0;

	LLTransactionID mTransactionID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCompletionObserver
//
//   Base class for doing something when when all observed items are locally 
//   complete.  Implements the changed() method of LLInventoryObserver 
//   and declares a new method named done() which is called when all watched items 
//   have complete information in the inventory model.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCompletionObserver : public LLInventoryObserver
{
public:
	LLInventoryCompletionObserver() {}
	/*virtual*/ void changed(U32 mask);

	void watchItem(const LLUUID& id);

protected:
	virtual void done() = 0;

	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCategoriesObserver
//
// This class is used for monitoring a list of inventory categories
// and firing a callback when there are changes in any of them.
// Categories are identified by their UUIDs.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCategoriesObserver : public LLInventoryObserver
{
public:
	typedef boost::function<void()> callback_t;

	LLInventoryCategoriesObserver() {};
	virtual void changed(U32 mask);

	/**
	 * Add cat_id to the list of observed categories with a
	 * callback fired on category being changed.
	 *
	 * @return "true" if category was added, "false" if it could
	 * not be found.
	 */
	bool addCategory(const LLUUID& cat_id, callback_t cb);
	void removeCategory(const LLUUID& cat_id);

protected:
	struct LLCategoryData
	{
		LLCategoryData(const LLUUID& cat_id, callback_t cb, S32 version, S32 num_descendents);

		callback_t	mCallback;
		S32			mVersion;
		S32			mDescendentsCount;
		LLMD5		mItemNameHash;
		bool		mIsNameHashInitialized;
		LLUUID		mCatID;
	};

	typedef	std::map<LLUUID, LLCategoryData>	category_map_t;
	typedef category_map_t::value_type			category_map_value_t;

	category_map_t				mCategoryMap;
};

#endif // LL_LLINVENTORYOBSERVERS_H
