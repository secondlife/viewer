/**
 * @file llfilteredwearablelist.cpp
 * @brief Functionality for showing filtered wearable flat list
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llfilteredwearablelist.h"

// newview
#include "llinventoryfunctions.h"
#include "llinventoryitemslist.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"


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
