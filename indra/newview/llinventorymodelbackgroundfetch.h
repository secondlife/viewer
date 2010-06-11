/** 
 * @file llinventorymodelbackgroundfetch.h
 * @brief LLInventoryModelBackgroundFetch class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

	BOOL backgroundFetchActive() const;
	bool isEverythingFetched() const; // completing the fetch once per session should be sufficient

	bool libraryFetchStarted() const;
	bool libraryFetchCompleted() const;
	bool libraryFetchInProgress() const;
	
	bool inventoryFetchStarted() const;
	bool inventoryFetchCompleted() const;
	bool inventoryFetchInProgress() const;

    void findLostItems();	
protected:
	void incrBulkFetch(S16 fetching);
	bool isBulkFetchProcessingComplete() const;
	void bulkFetch(std::string url);

	void backgroundFetch();
	static void backgroundFetchCB(void*); // background fetch idle function
	void stopBackgroundFetch(); // stop fetch process

	void setAllFoldersFetched();
	bool fetchQueueContainsNoDescendentsOf(const LLUUID& cat_id) const;
private:
 	BOOL mRecursiveInventoryFetchStarted;
	BOOL mRecursiveLibraryFetchStarted;
	BOOL mAllFoldersFetched;

	BOOL mBackgroundFetchActive;
	S16 mBulkFetchCount;
	BOOL mTimelyFetchPending;
	S32 mNumFetchRetries;

	LLFrameTimer mFetchTimer;
	F32 mMinTimeBetweenFetches;
	F32 mMaxTimeBetweenFetches;

	struct FetchQueueInfo
	{
		FetchQueueInfo(const LLUUID& id, BOOL recursive) :
			mCatUUID(id), mRecursive(recursive)
		{
		}
		LLUUID mCatUUID;
		BOOL mRecursive;
	};
	typedef std::deque<FetchQueueInfo> fetch_queue_t;
	fetch_queue_t mFetchQueue;
};

#endif // LL_LLINVENTORYMODELBACKGROUNDFETCH_H

