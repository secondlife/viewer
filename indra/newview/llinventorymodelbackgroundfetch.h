/** 
 * @file llinventorymodelbackgroundfetch.h
 * @brief LLInventoryModelBackgroundFetch class header file
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

#ifndef LL_LLINVENTORYMODELBACKGROUNDFETCH_H
#define LL_LLINVENTORYMODELBACKGROUNDFETCH_H

#include "llsingleton.h"
#include "lluuid.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryModelBackgroundFetch
//
// This class handles background fetches, which are fetches of
// inventory folder.  Fetches can be recursive or not.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryModelBackgroundFetch : public LLSingleton<LLInventoryModelBackgroundFetch>
{
	friend class LLInventoryModelFetchDescendentsResponder;

public:
	LLInventoryModelBackgroundFetch();
	~LLInventoryModelBackgroundFetch();

	// Start and stop background breadth-first fetching of inventory contents.
	// This gets triggered when performing a filter-search.
	void start(const LLUUID& cat_id = LLUUID::null, BOOL recursive = TRUE);

	BOOL folderFetchActive() const;
	bool isEverythingFetched() const; // completing the fetch once per session should be sufficient

	bool libraryFetchStarted() const;
	bool libraryFetchCompleted() const;
	bool libraryFetchInProgress() const;
	
	bool inventoryFetchStarted() const;
	bool inventoryFetchCompleted() const;
	bool inventoryFetchInProgress() const;

    void findLostItems();	
	void incrFetchCount(S16 fetching);
protected:
	bool isBulkFetchProcessingComplete() const;
	void bulkFetch();

	void backgroundFetch();
	static void backgroundFetchCB(void*); // background fetch idle function

	void setAllFoldersFetched();
	bool fetchQueueContainsNoDescendentsOf(const LLUUID& cat_id) const;
private:
 	BOOL mRecursiveInventoryFetchStarted;
	BOOL mRecursiveLibraryFetchStarted;
	BOOL mAllFoldersFetched;

	BOOL mBackgroundFetchActive;
	bool mFolderFetchActive;
	S16 mFetchCount;
	BOOL mTimelyFetchPending;
	S32 mNumFetchRetries;

	LLFrameTimer mFetchTimer;
	F32 mMinTimeBetweenFetches;
	F32 mMaxTimeBetweenFetches;

	struct FetchQueueInfo
	{
		FetchQueueInfo(const LLUUID& id, BOOL recursive, bool is_category = true) :
			mUUID(id), mRecursive(recursive), mIsCategory(is_category)
		{}
		LLUUID mUUID;
		bool mIsCategory;
		BOOL mRecursive;
	};
	typedef std::deque<FetchQueueInfo> fetch_queue_t;
	fetch_queue_t mFetchQueue;
};

#endif // LL_LLINVENTORYMODELBACKGROUNDFETCH_H

