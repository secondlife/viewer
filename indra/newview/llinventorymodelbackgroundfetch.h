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

// Seraph clean this up
#include "llassettype.h"
#include "llfoldertype.h"
#include "lldarray.h"
#include "llframetimer.h"
#include "llhttpclient.h"
#include "lluuid.h"
#include "llpermissionsflags.h"
#include "llstring.h"
#include <map>
#include <set>
#include <string>
#include <vector>

// Seraph clean this up
class LLInventoryObserver;
class LLInventoryObject;
class LLInventoryItem;
class LLInventoryCategory;
class LLViewerInventoryItem;
class LLViewerInventoryCategory;
class LLViewerInventoryItem;
class LLViewerInventoryCategory;
class LLMessageSystem;
class LLInventoryCollectFunctor;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryModelBackgroundFetch
//
// This class handles background fetch.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryModelBackgroundFetch : public LLSingleton<LLInventoryModelBackgroundFetch>
{
public:
	LLInventoryModelBackgroundFetch();
	~LLInventoryModelBackgroundFetch();

	// Start and stop background breadth-first fetching of inventory contents.
	// This gets triggered when performing a filter-search
	void start(const LLUUID& cat_id = LLUUID::null);
	BOOL backgroundFetchActive();
	bool isEverythingFetched();
	void incrBulkFetch(S16 fetching);
	void stopBackgroundFetch(); // stop fetch process
	bool isBulkFetchProcessingComplete();

	// Add categories to a list to be fetched in bulk.
	void bulkFetch(std::string url);

	bool libraryFetchStarted();
	bool libraryFetchCompleted();
	bool libraryFetchInProgress();
	
	bool inventoryFetchStarted();
	bool inventoryFetchCompleted();
	bool inventoryFetchInProgress();
    void findLostItems();

	void setAllFoldersFetched();

	static void backgroundFetchCB(void*); // background fetch idle function
	void backgroundFetch();
	
private:
 	BOOL mInventoryFetchStarted;
	BOOL mLibraryFetchStarted;
	BOOL mAllFoldersFetched;

	// completing the fetch once per session should be sufficient
	BOOL mBackgroundFetchActive;
	S16 mBulkFetchCount;
	BOOL mTimelyFetchPending;
	S32 mNumFetchRetries;

	LLFrameTimer mFetchTimer;
	F32 mMinTimeBetweenFetches;
	F32 mMaxTimeBetweenFetches;

};

#endif // LL_LLINVENTORYMODELBACKGROUNDFETCH_H

