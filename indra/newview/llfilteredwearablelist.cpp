/**
 * @file llfilteredwearablelist.cpp
 * @brief Functionality for showing filtered wearable flat list
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewerprecompiledheaders.h"
#include "llfilteredwearablelist.h"

// newview
#include "llinventoryfunctions.h"
#include "llinventoryitemslist.h"
#include "llinventorymodel.h"


LLFilteredWearableListManager::LLFilteredWearableListManager(LLInventoryItemsList* list, LLInventoryCollectFunctor* collector)
: mWearableList(list)
, mCollector(collector)
{
	llassert(mWearableList);
	gInventory.addObserver(this);
	gInventory.fetchDescendentsOf(gInventory.getRootFolderID());
}

LLFilteredWearableListManager::~LLFilteredWearableListManager()
{
	gInventory.removeObserver(this);
}

void LLFilteredWearableListManager::changed(U32 mask)
{
	if (LLInventoryObserver::CALLING_CARD == mask
			|| LLInventoryObserver::GESTURE == mask
			|| LLInventoryObserver::SORT == mask
			)
	{
		// skip non-related changes
		return;
	}

	if(!gInventory.isInventoryUsable())
	{
		return;
	}

	populateList();
}

void LLFilteredWearableListManager::setFilterCollector(LLInventoryCollectFunctor* collector)
{
	mCollector = collector;
	populateList();
}

void LLFilteredWearableListManager::populateList()
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;

	if(mCollector)
	{
		gInventory.collectDescendentsIf(
				gInventory.getRootFolderID(),
				cat_array,
				item_array,
				LLInventoryModel::EXCLUDE_TRASH,
				*mCollector);
	}

	// Probably will also need to get items from Library (waiting for reply in EXT-6724).

	mWearableList->refreshList(item_array);
}

// EOF
