/** 
 * @file llinventoryobserver.cpp
 * @brief Implementation of the inventory observers used to track agent inventory.
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

#include "llviewerprecompiledheaders.h"

#include "llinventoryobserver.h"

#include "llassetstorage.h"
#include "llcrc.h"
#include "lldir.h"
#include "llsys.h"
#include "llxfermanager.h"
#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llfloater.h"
#include "llfocusmgr.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llviewerregion.h"
#include "llappviewer.h"
#include "lldbstrings.h"
#include "llviewerstats.h"
#include "llnotificationsutil.h"
#include "llcallbacklist.h"
#include "llpreview.h"
#include "llviewercontrol.h"
#include "llvoavatarself.h"
#include "llsdutil.h"
#include <deque>

// If the viewer gets a notification, your observer assumes
// that that notification is for itself and then tries to process
// the results.  The notification could be for something else (e.g.
// you're fetching an item and a notification gets triggered because
// you renamed some other item).  This counter is to specify how many
// notification to wait for before giving up.
static const U32 MAX_NUM_NOTIFICATIONS_TO_PROCESS = 20;

LLInventoryObserver::LLInventoryObserver()
{
}

// virtual
LLInventoryObserver::~LLInventoryObserver()
{
}

LLInventoryFetchObserver::LLInventoryFetchObserver(const LLUUID& id)
{
	mIDs.clear();
	if (id != LLUUID::null)
	{
		setFetchID(id);
	}
}

LLInventoryFetchObserver::LLInventoryFetchObserver(const uuid_vec_t& ids)
{
	setFetchIDs(ids);
}

BOOL LLInventoryFetchObserver::isFinished() const
{
	return mIncomplete.empty();
}

void LLInventoryFetchObserver::setFetchIDs(const uuid_vec_t& ids)
{
	mIDs = ids;
}
void LLInventoryFetchObserver::setFetchID(const LLUUID& id)
{
	mIDs.clear();
	mIDs.push_back(id);
}


void LLInventoryCompletionObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if (!mIncomplete.empty())
	{
		for (uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
		{
			const LLViewerInventoryItem* item = gInventory.getItem(*it);
			if (!item)
			{
				it = mIncomplete.erase(it);
				continue;
			}
			if (item->isFinished())
			{
				mComplete.push_back(*it);
				it = mIncomplete.erase(it);
				continue;
			}
			++it;
		}
		if (mIncomplete.empty())
		{
			done();
		}
	}
}

void LLInventoryCompletionObserver::watchItem(const LLUUID& id)
{
	if (id.notNull())
	{
		mIncomplete.push_back(id);
	}
}

LLInventoryFetchItemsObserver::LLInventoryFetchItemsObserver(const LLUUID& item_id) :
	LLInventoryFetchObserver(item_id),

	mNumTries(MAX_NUM_NOTIFICATIONS_TO_PROCESS)
{
	mIDs.clear();
	mIDs.push_back(item_id);
}

LLInventoryFetchItemsObserver::LLInventoryFetchItemsObserver(const uuid_vec_t& item_ids) :
	LLInventoryFetchObserver(item_ids),

	mNumTries(MAX_NUM_NOTIFICATIONS_TO_PROCESS)
{
}

void LLInventoryFetchItemsObserver::changed(U32 mask)
{
	BOOL any_items_missing = FALSE;

	// scan through the incomplete items and move or erase them as
	// appropriate.
	if (!mIncomplete.empty())
	{
		for (uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
		{
			const LLUUID& item_id = (*it);
			LLViewerInventoryItem* item = gInventory.getItem(item_id);
			if (!item)
			{
				any_items_missing = TRUE;
				if (mNumTries > 0)
				{
					// Keep trying.
					++it;
				}
				else
				{
					// Just concede that this item hasn't arrived in reasonable time and continue on.
					llwarns << "Fetcher timed out when fetching inventory item assetID:" << item_id << llendl;
					it = mIncomplete.erase(it);
				}
				continue;
			}
			if (item->isFinished())
			{
				mComplete.push_back(item_id);
				it = mIncomplete.erase(it);
				continue;
			}
			++it;
		}
		if (any_items_missing)
		{
			mNumTries--;
		}

		if (mIncomplete.empty())
		{
			mNumTries = MAX_NUM_NOTIFICATIONS_TO_PROCESS;
			done();
		}
	}
	//llinfos << "LLInventoryFetchItemsObserver::changed() mComplete size " << mComplete.size() << llendl;
	//llinfos << "LLInventoryFetchItemsObserver::changed() mIncomplete size " << mIncomplete.size() << llendl;
}

void fetch_items_from_llsd(const LLSD& items_llsd)
{
	if (!items_llsd.size() || gDisconnected) return;
	LLSD body;
	body[0]["cap_name"] = "FetchInventory";
	body[1]["cap_name"] = "FetchLib";
	for (S32 i=0; i<items_llsd.size();i++)
	{
		if (items_llsd[i]["owner_id"].asString() == gAgent.getID().asString())
		{
			body[0]["items"].append(items_llsd[i]);
			continue;
		}
		if (items_llsd[i]["owner_id"].asString() == ALEXANDRIA_LINDEN_ID.asString())
		{
			body[1]["items"].append(items_llsd[i]);
			continue;
		}
	}
		
	for (S32 i=0; i<body.size(); i++)
	{
		if(!gAgent.getRegion())
		{
			llwarns<<"Agent's region is null"<<llendl;
			break;
		}
		if (0 >= body[i].size()) continue;
		std::string url = gAgent.getRegion()->getCapability(body[i]["cap_name"].asString());

		if (!url.empty())
		{
			body[i]["agent_id"]	= gAgent.getID();
			LLHTTPClient::post(url, body[i], new LLInventoryModel::fetchInventoryResponder(body[i]));
			break;
		}

		LLMessageSystem* msg = gMessageSystem;
		BOOL start_new_message = TRUE;
		for (S32 j=0; j<body[i]["items"].size(); j++)
		{
			LLSD item_entry = body[i]["items"][j];
			if (start_new_message)
			{
				start_new_message = FALSE;
				msg->newMessageFast(_PREHASH_FetchInventory);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_OwnerID, item_entry["owner_id"].asUUID());
			msg->addUUIDFast(_PREHASH_ItemID, item_entry["item_id"].asUUID());
			if (msg->isSendFull(NULL))
			{
				start_new_message = TRUE;
				gAgent.sendReliableMessage();
			}
		}
		if (!start_new_message)
		{
			gAgent.sendReliableMessage();
		}
	}
}

void LLInventoryFetchItemsObserver::startFetch()
{
	LLUUID owner_id;
	LLSD items_llsd;
	for (uuid_vec_t::const_iterator it = mIDs.begin(); it < mIDs.end(); ++it)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if (item)
		{
			if (item->isFinished())
			{
				// It's complete, so put it on the complete container.
				mComplete.push_back(*it);
				continue;
			}
			else
			{
				owner_id = item->getPermissions().getOwner();
			}
		}
		else
		{
			// assume it's agent inventory.
			owner_id = gAgent.getID();
		}
		
		// It's incomplete, so put it on the incomplete container, and
		// pack this on the message.
		mIncomplete.push_back(*it);
		
		// Prepare the data to fetch
		LLSD item_entry;
		item_entry["owner_id"] = owner_id;
		item_entry["item_id"] = (*it);
		items_llsd.append(item_entry);
	}
	fetch_items_from_llsd(items_llsd);
}

LLInventoryFetchDescendentsObserver::LLInventoryFetchDescendentsObserver(const LLUUID& cat_id) :
	LLInventoryFetchObserver(cat_id)
{
}

LLInventoryFetchDescendentsObserver::LLInventoryFetchDescendentsObserver(const uuid_vec_t& cat_ids) :
	LLInventoryFetchObserver(cat_ids)
{
}

// virtual
void LLInventoryFetchDescendentsObserver::changed(U32 mask)
{
	for (uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end();)
	{
		const LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
		if (!cat)
		{
			it = mIncomplete.erase(it);
			continue;
		}
		if (isCategoryComplete(cat))
		{
			mComplete.push_back(*it);
			it = mIncomplete.erase(it);
			continue;
		}
		++it;
	}
	if (mIncomplete.empty())
	{
		done();
	}
}

void LLInventoryFetchDescendentsObserver::startFetch()
{
	for (uuid_vec_t::const_iterator it = mIDs.begin(); it != mIDs.end(); ++it)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
		if (!cat) continue;
		if (!isCategoryComplete(cat))
		{
			cat->fetch();		//blindly fetch it without seeing if anything else is fetching it.
			mIncomplete.push_back(*it);	//Add to list of things being downloaded for this observer.
		}
		else
		{
			mComplete.push_back(*it);
		}
	}
}

BOOL LLInventoryFetchDescendentsObserver::isCategoryComplete(const LLViewerInventoryCategory* cat) const
{
	const S32 version = cat->getVersion();
	const S32 expected_num_descendents = cat->getDescendentCount();
	if ((version == LLViewerInventoryCategory::VERSION_UNKNOWN) ||
		(expected_num_descendents == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN))
	{
		return FALSE;
	}
	// it might be complete - check known descendents against
	// currently available.
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(cat->getUUID(), cats, items);
	if (!cats || !items)
	{
		llwarns << "Category '" << cat->getName() << "' descendents corrupted, fetch failed." << llendl;
		// NULL means the call failed -- cats/items map doesn't exist (note: this does NOT mean
		// that the cat just doesn't have any items or subfolders).
		// Unrecoverable, so just return done so that this observer can be cleared
		// from memory.
		return TRUE;
	}
	const S32 current_num_known_descendents = cats->count() + items->count();
	
	// Got the number of descendents that we were expecting, so we're done.
	if (current_num_known_descendents == expected_num_descendents)
	{
		return TRUE;
	}

	// Error condition, but recoverable.  This happens if something was added to the
	// category before it was initialized, so accountForUpdate didn't update descendent
	// count and thus the category thinks it has fewer descendents than it actually has.
	if (current_num_known_descendents >= expected_num_descendents)
	{
		llwarns << "Category '" << cat->getName() << "' expected descendentcount:" << expected_num_descendents << " descendents but got descendentcount:" << current_num_known_descendents << llendl;
		const_cast<LLViewerInventoryCategory *>(cat)->setDescendentCount(current_num_known_descendents);
		return TRUE;
	}
	return FALSE;
}

LLInventoryFetchComboObserver::LLInventoryFetchComboObserver(const uuid_vec_t& folder_ids,
															 const uuid_vec_t& item_ids)
{
	mFetchDescendents = new LLInventoryFetchDescendentsObserver(folder_ids);

	uuid_vec_t pruned_item_ids;
	for (uuid_vec_t::const_iterator item_iter = item_ids.begin();
		 item_iter != item_ids.end();
		 ++item_iter)
	{
		const LLUUID& item_id = (*item_iter);
		const LLViewerInventoryItem* item = gInventory.getItem(item_id);
		if (item && std::find(folder_ids.begin(), folder_ids.end(), item->getParentUUID()) == folder_ids.end())
		{
			continue;
		}
		pruned_item_ids.push_back(item_id);
	}

	mFetchItems = new LLInventoryFetchItemsObserver(pruned_item_ids);
	mFetchDescendents = new LLInventoryFetchDescendentsObserver(folder_ids);
}

LLInventoryFetchComboObserver::~LLInventoryFetchComboObserver()
{
	mFetchItems->done();
	mFetchDescendents->done();
	delete mFetchItems;
	delete mFetchDescendents;
}

void LLInventoryFetchComboObserver::changed(U32 mask)
{
	mFetchItems->changed(mask);
	mFetchDescendents->changed(mask);
	if (mFetchItems->isFinished() && mFetchDescendents->isFinished())
	{
		done();
	}
}

void LLInventoryFetchComboObserver::startFetch()
{
	mFetchItems->startFetch();
	mFetchDescendents->startFetch();
}

void LLInventoryExistenceObserver::watchItem(const LLUUID& id)
{
	if (id.notNull())
	{
		mMIA.push_back(id);
	}
}

void LLInventoryExistenceObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if (!mMIA.empty())
	{
		for (uuid_vec_t::iterator it = mMIA.begin(); it < mMIA.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if (!item)
			{
				++it;
				continue;
			}
			mExist.push_back(*it);
			it = mMIA.erase(it);
		}
		if (mMIA.empty())
		{
			done();
		}
	}
}

void LLInventoryMoveFromWorldObserver::changed(U32 mask)
{
	if(!(mask & LLInventoryObserver::ADD))
	{
		return;
	}

	// nothing is watched
	if (mWatchedAssets.size() == 0)
	{
		return;
	}

	LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem;
	LLMessageSystem* msg = gMessageSystem;
	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for(S32 i = 0; i < num_blocks; ++i)
	{
		item->unpackMessage(msg, _PREHASH_InventoryData, i);
		const LLUUID& asset_uuid = item->getAssetUUID();
		if (item->getUUID().notNull() && asset_uuid.notNull())
		{
			if (isAssetWatched(asset_uuid))
			{
				LL_DEBUGS("Inventory_Move") << "Found asset UUID: " << asset_uuid << LL_ENDL;
				mAddedItems.push_back(item->getUUID());
			}
		}
	}

	if (mAddedItems.size() == mWatchedAssets.size())
	{
		done();
		LL_DEBUGS("Inventory_Move") << "All watched items are added & processed." << LL_ENDL;
		mAddedItems.clear();

		// Unable to clean watched items here due to somebody can require to check them in current frame.
		// set dirty state to clean them while next watch cycle.
		mIsDirty = true;
	}
}

void LLInventoryMoveFromWorldObserver::watchAsset(const LLUUID& asset_id)
{
	if(asset_id.notNull())
	{
		if (mIsDirty)
		{
			LL_DEBUGS("Inventory_Move") << "Watched items are dirty. Clean them." << LL_ENDL;
			mWatchedAssets.clear();
			mIsDirty = false;
		}

		mWatchedAssets.push_back(asset_id);
		onAssetAdded(asset_id);
	}
}

bool LLInventoryMoveFromWorldObserver::isAssetWatched( const LLUUID& asset_id )
{
	return std::find(mWatchedAssets.begin(), mWatchedAssets.end(), asset_id) != mWatchedAssets.end();
}

void LLInventoryAddedObserver::changed(U32 mask)
{
	if (!(mask & LLInventoryObserver::ADD))
	{
		return;
	}

	// *HACK: If this was in response to a packet off
	// the network, figure out which item was updated.
	LLMessageSystem* msg = gMessageSystem;

	std::string msg_name;
	if (mMessageName.empty())
	{
		msg_name = msg->getMessageName();
	}
	else
	{
		msg_name = mMessageName;
	}

	if (msg_name.empty())
	{
		return;
	}
	
	// We only want newly created inventory items. JC
	if ( msg_name != "UpdateCreateInventoryItem")
	{
		return;
	}

	LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for (S32 i = 0; i < num_blocks; ++i)
	{
		titem->unpackMessage(msg, _PREHASH_InventoryData, i);
		if (!(titem->getUUID().isNull()))
		{
			//we don't do anything with null keys
			mAdded.push_back(titem->getUUID());
		}
	}
	if (!mAdded.empty())
	{
		done();
	}
}

LLInventoryTransactionObserver::LLInventoryTransactionObserver(const LLTransactionID& transaction_id) :
	mTransactionID(transaction_id)
{
}

void LLInventoryTransactionObserver::changed(U32 mask)
{
	if (mask & LLInventoryObserver::ADD)
	{
		// This could be it - see if we are processing a bulk update
		LLMessageSystem* msg = gMessageSystem;
		if (msg->getMessageName()
		   && (0 == strcmp(msg->getMessageName(), "BulkUpdateInventory")))
		{
			// we have a match for the message - now check the
			// transaction id.
			LLUUID id;
			msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, id);
			if (id == mTransactionID)
			{
				// woo hoo, we found it
				uuid_vec_t folders;
				uuid_vec_t items;
				S32 count;
				count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
				S32 i;
				for (i = 0; i < count; ++i)
				{
					msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_FolderID, id, i);
					if (id.notNull())
					{
						folders.push_back(id);
					}
				}
				count = msg->getNumberOfBlocksFast(_PREHASH_ItemData);
				for (i = 0; i < count; ++i)
				{
					msg->getUUIDFast(_PREHASH_ItemData, _PREHASH_ItemID, id, i);
					if (id.notNull())
					{
						items.push_back(id);
					}
				}

				// call the derived class the implements this method.
				done(folders, items);
			}
		}
	}
}

void LLInventoryCategoriesObserver::changed(U32 mask)
{
	if (!mCategoryMap.size())
		return;

	for (category_map_t::iterator iter = mCategoryMap.begin();
		 iter != mCategoryMap.end();
		 ++iter)
	{
		LLViewerInventoryCategory* category = gInventory.getCategory((*iter).first);
		if (!category)
			continue;

		S32 version = category->getVersion();
		if (version != (*iter).second.mVersion)
		{
			// Update category version in map.
			(*iter).second.mVersion = version;
			(*iter).second.mCallback();
		}
	}
}

void LLInventoryCategoriesObserver::addCategory(const LLUUID& cat_id, callback_t cb)
{
	S32 version;
	LLViewerInventoryCategory* category = gInventory.getCategory(cat_id);
	if (category)
	{
		// Inventory category version is used to find out if some changes
		// to a category have been made.
		version = category->getVersion();
	}
	else
	{
		// If category could not be retrieved it might mean that
		// inventory is unusable at the moment so the category is
		// stored with VERSION_UNKNOWN and it may be updated later.
		version = LLViewerInventoryCategory::VERSION_UNKNOWN;
	}

	version = category->getVersion();
	mCategoryMap.insert(category_map_value_t(cat_id, LLCategoryData(cb, version)));
}

void LLInventoryCategoriesObserver::removeCategory(const LLUUID& cat_id)
{
	mCategoryMap.erase(cat_id);
}
