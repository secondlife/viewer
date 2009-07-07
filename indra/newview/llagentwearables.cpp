/** 
 * @file llagentwearables.cpp
 * @brief LLAgentWearables class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llagent.h" 
#include "llagentwearables.h"

#include "llfloatercustomize.h"
#include "llfloaterinventory.h"
#include "llinventorymodel.h"
#include "llnotify.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llwearable.h"
#include "llwearablelist.h"

LLAgentWearables gAgentWearables;

BOOL LLAgentWearables::mInitialWearablesUpdateReceived = FALSE;

using namespace LLVOAvatarDefines;

void LLAgentWearables::dump()
{
	llinfos << "LLAgentWearablesDump" << llendl;
	for (S32 i = 0; i < WT_COUNT; i++)
	{
		U32 count = getWearableCount((EWearableType)i);
		llinfos << "Type: " << i << " count " << count << llendl;
		for (U32 j=0; j<count; j++)
		{
			LLWearableInv* wearable_entry = getWearableInv((EWearableType)i,j);
			if (wearable_entry == NULL)
			{
				llinfos << "    " << j << " NULL entry" << llendl;
				continue;
			}
			if (wearable_entry->mWearable == NULL)
			{
				llinfos << "    " << j << " NULL wearable" << llendl;
				continue;
			}
			llinfos << "    " << j << " Name " << wearable_entry->mWearable->getName()
					<< " description " << wearable_entry->mWearable->getDescription() << llendl;
			
		}
	}
}

// MULTI-WEARABLE: debugging
struct LLAgentDumper
{
	LLAgentDumper(std::string name):
		mName(name)
	{
		llinfos << llendl;
		llinfos << "LLAgentDumper " << mName << llendl;
		gAgentWearables.dump();
	}

	~LLAgentDumper()
	{
		llinfos << llendl;
		llinfos << "~LLAgentDumper " << mName << llendl;
		gAgentWearables.dump();
	}

	std::string mName;
};

LLAgentWearables::LLAgentWearables() :
	mWearablesLoaded(FALSE),
	mAvatarObject(NULL)
{
	// MULTI-WEARABLE: TODO remove null entries.
	for (U32 i = 0; i < WT_COUNT; i++)
	{
		mWearableDatas[(EWearableType)i].push_back(new LLWearableInv);
	}
}

LLAgentWearables::~LLAgentWearables()
{
	cleanup();
}

void LLAgentWearables::cleanup()
{
	for (wearableentry_map_t::iterator iter = mWearableDatas.begin();
		 iter != mWearableDatas.end();
		 iter++)
	{
		wearableentry_vec_t &wearables = iter->second;
		for (U32 i = 0; i < wearables.size(); i++)
		{
			LLWearableInv *wearable = wearables[i];
			delete wearable;
			wearables[i] = NULL;
		}
	}
	mAvatarObject = NULL;
}

void LLAgentWearables::setAvatarObject(LLVOAvatarSelf *avatar)
{ 
	mAvatarObject = avatar;
	if (avatar)
	{
		sendAgentWearablesRequest();
	}
}

// wearables
LLAgentWearables::createStandardWearablesAllDoneCallback::~createStandardWearablesAllDoneCallback()
{
	gAgentWearables.createStandardWearablesAllDone();
}

LLAgentWearables::sendAgentWearablesUpdateCallback::~sendAgentWearablesUpdateCallback()
{
	gAgentWearables.sendAgentWearablesUpdate();
}

/**
 * @brief Construct a callback for dealing with the wearables.
 *
 * Would like to pass the agent in here, but we can't safely
 * count on it being around later.  Just use gAgent directly.
 * @param cb callback to execute on completion (??? unused ???)
 * @param type Type for the wearable in the agent
 * @param wearable The wearable data.
 * @param todo Bitmask of actions to take on completion.
 */
LLAgentWearables::addWearableToAgentInventoryCallback::addWearableToAgentInventoryCallback(
	LLPointer<LLRefCount> cb, S32 type, U32 index, LLWearable* wearable, U32 todo) :
	mType(type),
	mIndex(index),	
	mWearable(wearable),
	mTodo(todo),
	mCB(cb)
{
}

void LLAgentWearables::addWearableToAgentInventoryCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;

	gAgentWearables.addWearabletoAgentInventoryDone(mType, mIndex, inv_item, mWearable);

	if (mTodo & CALL_UPDATE)
	{
		gAgentWearables.sendAgentWearablesUpdate();
	}
	if (mTodo & CALL_RECOVERDONE)
	{
		gAgentWearables.recoverMissingWearableDone();
	}
	/*
	 * Do this for every one in the loop
	 */
	if (mTodo & CALL_CREATESTANDARDDONE)
	{
		gAgentWearables.createStandardWearablesDone(mType, mIndex);
	}
	if (mTodo & CALL_MAKENEWOUTFITDONE)
	{
		gAgentWearables.makeNewOutfitDone(mType, mIndex);
	}
}

void LLAgentWearables::addWearabletoAgentInventoryDone(const S32 type,
													   const U32 index,
													   const LLUUID& item_id,
													   LLWearable* wearable)
{
	if (item_id.isNull())
		return;

	LLWearableInv* wearable_entry = getWearableInv((EWearableType)type, index);

	LLUUID old_item_id = wearable_entry->mItemID;
	wearable_entry->mItemID = item_id;
	wearable_entry->mWearable = wearable;
	if (old_item_id.notNull())
		gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (item && wearable)
	{
		// We're changing the asset id, so we both need to set it
		// locally via setAssetUUID() and via setTransactionID() which
		// will be decoded on the server. JC
		item->setAssetUUID(wearable->getID());
		item->setTransactionID(wearable->getTransactionID());
		gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		item->updateServer(FALSE);
	}
	gInventory.notifyObservers();
}

void LLAgentWearables::sendAgentWearablesUpdate()
{
	// MULTI-WEARABLE: call i "type" or something.
	// First make sure that we have inventory items for each wearable
	for (S32 i=0; i < WT_COUNT; ++i)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)i); j++)
		{
			LLWearableInv* wearable_entry = getWearableInv((EWearableType)i,j);
			LLWearable* wearable = wearable_entry->mWearable;
			if (wearable)
			{
				if (wearable_entry->mItemID.isNull())
				{
					LLPointer<LLInventoryCallback> cb =
						new addWearableToAgentInventoryCallback(
							LLPointer<LLRefCount>(NULL),
							i,
							j,
							wearable,
							addWearableToAgentInventoryCallback::CALL_NONE);
					addWearableToAgentInventory(cb, wearable);
				}
				else
				{
					gInventory.addChangedMask(LLInventoryObserver::LABEL,
											  wearable_entry->mItemID);
				}
			}
		}
	}

	// Then make sure the inventory is in sync with the avatar.
	gInventory.notifyObservers();

	// Send the AgentIsNowWearing 
	gMessageSystem->newMessageFast(_PREHASH_AgentIsNowWearing);

	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	lldebugs << "sendAgentWearablesUpdate()" << llendl;
	// MULTI-WEARABLE: update for multi-wearables after server-side support is in.
	for (S32 i=0; i < WT_COUNT; ++i)
	{
		gMessageSystem->nextBlockFast(_PREHASH_WearableData);

		U8 type_u8 = (U8)i;
		gMessageSystem->addU8Fast(_PREHASH_WearableType, type_u8);

		// MULTI-WEARABLE: TODO: hacked index to 0, needs to loop over all once messages support this.
		LLWearableInv* wearable_entry = getWearableInv((EWearableType)i, 0);
		LLWearable* wearable = wearable_entry->mWearable;
		if (wearable)
		{
			//llinfos << "Sending wearable " << wearable->getName() << llendl;
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, wearable_entry->mItemID);
		}
		else
		{
			//llinfos << "Not wearing wearable type " << LLWearableDictionary::getInstance()->getWearable((EWearableType)i) << llendl;
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID::null);
		}

		lldebugs << "       " << LLWearableDictionary::getTypeLabel((EWearableType)i) << ": " << (wearable ? wearable->getID() : LLUUID::null) << llendl;
	}
	gAgent.sendReliableMessage();
}

// MULTI-WEARABLE: add index.
void LLAgentWearables::saveWearable(const EWearableType type, const U32 index, BOOL send_update)
{
	LLWearableInv* wearable_entry = getWearableInv(type, index);
	LLWearable* old_wearable = wearable_entry ? wearable_entry->mWearable : NULL;
	if (old_wearable && (old_wearable->isDirty() || old_wearable->isOldVersion()))
	{
		LLWearable* new_wearable = LLWearableList::instance().createCopyFromAvatar(old_wearable);
		wearable_entry->mWearable = new_wearable;

		LLInventoryItem* item = gInventory.getItem(wearable_entry->mItemID);
		if (item)
		{
			// Update existing inventory item
			LLPointer<LLViewerInventoryItem> template_item =
				new LLViewerInventoryItem(item->getUUID(),
										  item->getParentUUID(),
										  item->getPermissions(),
										  new_wearable->getID(),
										  new_wearable->getAssetType(),
										  item->getInventoryType(),
										  item->getName(),
										  item->getDescription(),
										  item->getSaleInfo(),
										  item->getFlags(),
										  item->getCreationDate());
			template_item->setTransactionID(new_wearable->getTransactionID());
			template_item->updateServer(FALSE);
			gInventory.updateItem(template_item);
		}
		else
		{
			// Add a new inventory item (shouldn't ever happen here)
			U32 todo = addWearableToAgentInventoryCallback::CALL_NONE;
			if (send_update)
			{
				todo |= addWearableToAgentInventoryCallback::CALL_UPDATE;
			}
			LLPointer<LLInventoryCallback> cb =
				new addWearableToAgentInventoryCallback(
					LLPointer<LLRefCount>(NULL),
					(S32)type,
					index,
					new_wearable,
					todo);
			addWearableToAgentInventory(cb, new_wearable);
			return;
		}

		gAgent.getAvatarObject()->wearableUpdated( type );

		if (send_update)
		{
			sendAgentWearablesUpdate();
		}
	}
}

// MULTI-WEARABLE: add index
void LLAgentWearables::saveWearableAs(const EWearableType type,
									  const U32 index,
									  const std::string& new_name,
									  BOOL save_in_lost_and_found)
{
	if (!isWearableCopyable(type, index))
	{
		llwarns << "LLAgent::saveWearableAs() not copyable." << llendl;
		return;
	}
	LLWearableInv* wearable_entry = getWearableInv(type, index);
	LLWearable* old_wearable = wearable_entry->mWearable;
	if (!old_wearable)
	{
		llwarns << "LLAgent::saveWearableAs() no old wearable." << llendl;
		return;
	}

	LLInventoryItem* item = gInventory.getItem(wearable_entry->mItemID);
	if (!item)
	{
		llwarns << "LLAgent::saveWearableAs() no inventory item." << llendl;
		return;
	}
	std::string trunc_name(new_name);
	LLStringUtil::truncate(trunc_name, DB_INV_ITEM_NAME_STR_LEN);
	LLWearable* new_wearable = LLWearableList::instance().createCopyFromAvatar(
		old_wearable,
		trunc_name);
	LLPointer<LLInventoryCallback> cb =
		new addWearableToAgentInventoryCallback(
			LLPointer<LLRefCount>(NULL),
			type,
			index,
			new_wearable,
			addWearableToAgentInventoryCallback::CALL_UPDATE);
	LLUUID category_id;
	if (save_in_lost_and_found)
	{
		category_id = gInventory.findCategoryUUIDForType(
			LLAssetType::AT_LOST_AND_FOUND);
	}
	else
	{
		// put in same folder as original
		category_id = item->getParentUUID();
	}

	copy_inventory_item(
		gAgent.getID(),
		item->getPermissions().getOwner(),
		item->getUUID(),
		category_id,
		new_name,
		cb);
}

void LLAgentWearables::revertWearable(const EWearableType type, const U32 index)
{
	LLWearableInv* wearable_entry = getWearableInv(type, index);
	LLWearable* wearable = wearable_entry->mWearable;
	if (wearable)
	{
		wearable->writeToAvatar(TRUE);
	}
	gAgent.sendAgentSetAppearance();
}

void LLAgentWearables::saveAllWearables()
{
	//if (!gInventory.isLoaded())
	//{
	//	return;
	//}

	for (S32 i=0; i < WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)i); j++)
			saveWearable((EWearableType)i, j, FALSE);
	}
	sendAgentWearablesUpdate();
}

// Called when the user changes the name of a wearable inventory item that is currently being worn.
void LLAgentWearables::setWearableName(const LLUUID& item_id, const std::string& new_name)
{
	for (S32 i=0; i < WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)i); j++)
		{
			LLWearableInv* wearable_entry = getWearableInv((EWearableType)i,j);
			if (wearable_entry->mItemID == item_id)
			{
				LLWearable* old_wearable = wearable_entry->mWearable;
				llassert(old_wearable);

				std::string old_name = old_wearable->getName();
				old_wearable->setName(new_name);
				LLWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
				LLInventoryItem* item = gInventory.getItem(item_id);
				if (item)
				{
					new_wearable->setPermissions(item->getPermissions());
				}
				old_wearable->setName(old_name);

				wearable_entry->mWearable = new_wearable;
				sendAgentWearablesUpdate();
				break;
			}
		}
	}
}


BOOL LLAgentWearables::isWearableModifiable(EWearableType type, U32 index) const
{
	LLUUID item_id = getWearableItem(type, index);
	if (!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if (item && item->getPermissions().allowModifyBy(gAgent.getID(),
														 gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLAgentWearables::isWearableCopyable(EWearableType type, U32 index) const
{
	LLUUID item_id = getWearableItem(type, index);
	if (!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if (item && item->getPermissions().allowCopyBy(gAgent.getID(),
													   gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
  U32 LLAgentWearables::getWearablePermMask(EWearableType type)
  {
  LLUUID item_id = getWearableItem(type);
  if (!item_id.isNull())
  {
  LLInventoryItem* item = gInventory.getItem(item_id);
  if (item)
  {
  return item->getPermissions().getMaskOwner();
  }
  }
  return PERM_NONE;
  }
*/

LLInventoryItem* LLAgentWearables::getWearableInventoryItem(EWearableType type, U32 index)
{
	LLUUID item_id = getWearableItem(type,index);
	LLInventoryItem* item = NULL;
	if (item_id.notNull())
	{
		item = gInventory.getItem(item_id);
	}
	return item;
}

LLWearable* LLAgentWearables::getWearableFromWearableItem(const LLUUID& item_id) const
{
	for (S32 i=0; i < WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)i); j++)
		{
			const LLWearableInv* wearable_entry = getWearableInv((EWearableType)i, j);
			if (wearable_entry->mItemID == item_id)
			{
				return wearable_entry->mWearable;
			}
		}
	}
	return NULL;
}

void LLAgentWearables::sendAgentWearablesRequest()
{
	gMessageSystem->newMessageFast(_PREHASH_AgentWearablesRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gAgent.sendReliableMessage();
}

// MULTI-WEARABLE: update for multiple items per type.
// Used to enable/disable menu items.
// static
BOOL LLAgentWearables::selfHasWearable(void* userdata)
{
	EWearableType type = (EWearableType)(intptr_t)userdata;
	// MULTI-WEARABLE: TODO could be getWearableCount > 0, once null entries have been eliminated.
	return gAgentWearables.getWearableInv(type,0)->mWearable != NULL;
}

const LLAgentWearables::LLWearableInv LLAgentWearables::s_null_wearable;

LLWearable* LLAgentWearables::getWearable(const EWearableType type, U32 index)
{
	LLWearableInv* inv = getWearableInv(type,index);
	return inv->mWearable;
}

const LLWearable* LLAgentWearables::getWearable(const EWearableType type, U32 index) const
{
	const LLWearableInv* inv = getWearableInv(type,index);
	return inv->mWearable;
}

//MULTI-WEARABLE: this will give wrong values until we get rid of the "always one empty object" scheme.
U32 LLAgentWearables::getWearableCount(const EWearableType type) const
{
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return 0;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	return wearable_vec.size();
}
	
LLAgentWearables::LLWearableInv* LLAgentWearables::getWearableInv(const EWearableType type, U32 index)
{
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return NULL;
	}
	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index>=wearable_vec.size())
	{
		return NULL;
	}
	else
	{
		return wearable_vec[index];
	}
}

const LLAgentWearables::LLWearableInv* LLAgentWearables::getWearableInv(const EWearableType type, U32 index) const
{
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end()) return &s_null_wearable;
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index>=wearable_vec.size())
		return &s_null_wearable;
	else
		return wearable_vec[index];
}

const LLUUID& LLAgentWearables::getWearableItem(EWearableType type, U32 index) const
{
	return getWearableInv(type,index)->mItemID;
}


// Warning: include_linked_items = TRUE makes this operation expensive.
BOOL LLAgentWearables::isWearingItem(const LLUUID& item_id, BOOL include_linked_items) const
{
	if (getWearableFromWearableItem(item_id) != NULL) return TRUE;
	if (include_linked_items)
	{
		LLInventoryModel::item_array_t item_array;
		gInventory.collectLinkedItems(item_id, item_array);
		for (LLInventoryModel::item_array_t::iterator iter = item_array.begin();
			 iter != item_array.end();
			 iter++)
		{
			LLViewerInventoryItem *linked_item = (*iter);
			const LLUUID &item_id = linked_item->getUUID();
			if (getWearableFromWearableItem(item_id) != NULL) return TRUE;
		}
	}
	return FALSE;
}

// MULTI-WEARABLE: update for multiple
// static
void LLAgentWearables::processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys, void** user_data)
{
	// We should only receive this message a single time.  Ignore subsequent AgentWearablesUpdates
	// that may result from AgentWearablesRequest having been sent more than once.
	if (mInitialWearablesUpdateReceived)
		return;
	mInitialWearablesUpdateReceived = true;

	LLUUID agent_id;
	gMessageSystem->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (avatar && (agent_id == avatar->getID()))
	{
		gMessageSystem->getU32Fast(_PREHASH_AgentData, _PREHASH_SerialNum, gAgentQueryManager.mUpdateSerialNum);
		
		S32 num_wearables = gMessageSystem->getNumberOfBlocksFast(_PREHASH_WearableData);
		if (num_wearables < 4)
		{
			// Transitional state.  Avatars should always have at least their body parts (hair, eyes, shape and skin).
			// The fact that they don't have any here (only a dummy is sent) implies that this account existed
			// before we had wearables, or that the database has gotten messed up.
			return;
		}

		//lldebugs << "processAgentInitialWearablesUpdate()" << llendl;
		// Add wearables
		LLUUID asset_id_array[WT_COUNT];
		// MULTI-WEARABLE: TODO: update once messages change.  Currently use results to populate the zeroth element.
		for (S32 i=0; i < num_wearables; i++)
		{
			U8 type_u8 = 0;
			gMessageSystem->getU8Fast(_PREHASH_WearableData, _PREHASH_WearableType, type_u8, i);
			if (type_u8 >= WT_COUNT)
			{
				continue;
			}
			const EWearableType type = (EWearableType) type_u8;

			LLUUID item_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_ItemID, item_id, i);

			LLUUID asset_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_AssetID, asset_id, i);
			if (asset_id.isNull())
			{
				LLWearable::removeFromAvatar(type, FALSE);
			}
			else
			{
				LLAssetType::EType asset_type = LLWearableDictionary::getAssetType(type);
				if (asset_type == LLAssetType::AT_NONE)
				{
					continue;
				}
				// MULTI-WEARABLE: TODO FIXME: assumes zeroth element always exists and can be safely written to.
				LLWearableInv* wearable_entry = gAgentWearables.getWearableInv(type,0);
				wearable_entry->mItemID = item_id;
				asset_id_array[type] = asset_id;
			}
			
			lldebugs << "       " << LLWearableDictionary::getTypeLabel(type) << llendl;
		}

		// now that we have the asset ids...request the wearable assets
		for (S32 i = 0; i < WT_COUNT; i++)
		{
			// MULTI-WEARABLE: TODO: update once messages change.  Currently use results to populate the zeroth element.
			LLWearableInv* wearable_entry = gAgentWearables.getWearableInv((EWearableType)i, 0);
			if (!wearable_entry->mItemID.isNull())
			{
				LLWearableList::instance().getAsset(asset_id_array[i],
													LLStringUtil::null,
													LLWearableDictionary::getAssetType((EWearableType) i), 
													onInitialWearableAssetArrived, (void*)(intptr_t)i);
			}
		}
	}
}

// A single wearable that the avatar was wearing on start-up has arrived from the database.
// static
void LLAgentWearables::onInitialWearableAssetArrived(LLWearable* wearable, void* userdata)
{
	const EWearableType type = (EWearableType)(intptr_t)userdata;

	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if (!avatar)
	{
		return;
	}

	if (wearable)
	{
		llassert(type == wearable->getType());
		// MULTI-WEARABLE: is this always zeroth element?  Change sometime.
		LLWearableInv* wearable_entry = gAgentWearables.getWearableInv(type,0);
		wearable_entry->mWearable = wearable;

		// disable composites if initial textures are baked
		avatar->setupComposites();
		gAgentWearables.queryWearableCache();

		wearable->writeToAvatar(FALSE);
		avatar->setCompositeUpdatesEnabled(TRUE);
		gInventory.addChangedMask(LLInventoryObserver::LABEL, wearable_entry->mItemID);
	}
	else
	{
		// Somehow the asset doesn't exist in the database.
		// MULTI-WEARABLE: assuming zeroth elt
		gAgentWearables.recoverMissingWearable(type,0);
	}

	gInventory.notifyObservers();

	// Have all the wearables that the avatar was wearing at log-in arrived?
	// MULTI-WEARABLE: update when multiple wearables can arrive per type.
	if (!gAgentWearables.mWearablesLoaded)
	{
		gAgentWearables.mWearablesLoaded = TRUE;
		for (S32 i = 0; i < WT_COUNT; i++)
		{
			LLWearableInv* wearable_entry = gAgentWearables.getWearableInv((EWearableType)i,0);
			if (!wearable_entry->mItemID.isNull() && !wearable_entry->mWearable)
			{
				gAgentWearables.mWearablesLoaded = FALSE;
				break;
			}
		}
	}

	if (gAgentWearables.mWearablesLoaded)
	{

		// Can't query cache until all wearables have arrived, so calling this earlier is a no-op.
		gAgentWearables.queryWearableCache();

		// Make sure that the server's idea of the avatar's wearables actually match the wearables.
		gAgent.sendAgentSetAppearance();

		// Check to see if there are any baked textures that we hadn't uploaded before we logged off last time.
		// If there are any, schedule them to be uploaded as soon as the layer textures they depend on arrive.
		if (gAgent.cameraCustomizeAvatar())
		{
			avatar->requestLayerSetUploads();
		}
	}
}

// Normally, all wearables referred to "AgentWearablesUpdate" will correspond to actual assets in the
// database.  If for some reason, we can't load one of those assets, we can try to reconstruct it so that
// the user isn't left without a shape, for example.  (We can do that only after the inventory has loaded.)
void LLAgentWearables::recoverMissingWearable(const EWearableType type, U32 index)
{
	// Try to recover by replacing missing wearable with a new one.
	LLNotifications::instance().add("ReplacedMissingWearable");
	lldebugs << "Wearable " << LLWearableDictionary::getTypeLabel(type) << " could not be downloaded.  Replaced inventory item with default wearable." << llendl;
	LLWearable* new_wearable = LLWearableList::instance().createNewWearable(type);

	S32 type_s32 = (S32) type;
	LLWearableInv* wearable_entry = getWearableInv(type, index);
	wearable_entry->mWearable = new_wearable;
	new_wearable->writeToAvatar(TRUE);

	// Add a new one in the lost and found folder.
	// (We used to overwrite the "not found" one, but that could potentially
	// destory content.) JC
	LLUUID lost_and_found_id = 
		gInventory.findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
	LLPointer<LLInventoryCallback> cb =
		new addWearableToAgentInventoryCallback(
			LLPointer<LLRefCount>(NULL),
			type_s32,
			index,
			new_wearable,
			addWearableToAgentInventoryCallback::CALL_RECOVERDONE);
	addWearableToAgentInventory(cb, new_wearable, lost_and_found_id, TRUE);
}

void LLAgentWearables::recoverMissingWearableDone()
{
	// Have all the wearables that the avatar was wearing at log-in arrived or been fabricated?
	mWearablesLoaded = TRUE;
	for (S32 i = 0; i < WT_COUNT; i++)
	{
		// MULTI-WEARABLE: assuming zeroth elt - fix when messages change.
		LLWearableInv* wearable_entry = getWearableInv((EWearableType)i,0);
		if (!wearable_entry->mItemID.isNull() && !wearable_entry->mWearable)
		{
			mWearablesLoaded = FALSE;
			break;
		}
	}

	if (mWearablesLoaded)
	{
		// Make sure that the server's idea of the avatar's wearables actually match the wearables.
		gAgent.sendAgentSetAppearance();
	}
	else
	{
		gInventory.addChangedMask(LLInventoryObserver::LABEL, LLUUID::null);
		gInventory.notifyObservers();
	}
}

void LLAgentWearables::createStandardWearables(BOOL female)
{
	llwarns << "Creating Standard " << (female ? "female" : "male")
			<< " Wearables" << llendl;

	if (mAvatarObject.isNull())
	{
		return;
	}

	mAvatarObject->setSex(female ? SEX_FEMALE : SEX_MALE);

	const BOOL create[WT_COUNT] = 
		{
			TRUE,  //WT_SHAPE
			TRUE,  //WT_SKIN
			TRUE,  //WT_HAIR
			TRUE,  //WT_EYES
			TRUE,  //WT_SHIRT
			TRUE,  //WT_PANTS
			TRUE,  //WT_SHOES
			TRUE,  //WT_SOCKS
			FALSE, //WT_JACKET
			FALSE, //WT_GLOVES
			TRUE,  //WT_UNDERSHIRT
			TRUE,  //WT_UNDERPANTS
			FALSE  //WT_SKIRT
		};

	for (S32 i=0; i < WT_COUNT; i++)
	{
		bool once = false;
		LLPointer<LLRefCount> donecb = NULL;
		if (create[i])
		{
			if (!once)
			{
				once = true;
				donecb = new createStandardWearablesAllDoneCallback;
			}
			// MULTI_WEARABLE: only elt 0, may be the right thing?
			LLWearableInv* wearable_entry = getWearableInv((EWearableType)i,0);
			llassert(wearable_entry->mWearable == NULL);
			LLWearable* wearable = LLWearableList::instance().createNewWearable((EWearableType)i);
			wearable_entry->mWearable = wearable;
			// no need to update here...
			// MULTI_WEARABLE: hardwired index = 0 here.
			LLPointer<LLInventoryCallback> cb =
				new addWearableToAgentInventoryCallback(
					donecb,
					i,
					0,
					wearable,
					addWearableToAgentInventoryCallback::CALL_CREATESTANDARDDONE);
			addWearableToAgentInventory(cb, wearable, LLUUID::null, FALSE);
		}
	}
}

void LLAgentWearables::createStandardWearablesDone(S32 type, U32 index)
{
	LLWearable* wearable = getWearable((EWearableType)type, index);

	if (wearable)
	{
		wearable->writeToAvatar(TRUE);
	}
}

void LLAgentWearables::createStandardWearablesAllDone()
{
	// ... because sendAgentWearablesUpdate will notify inventory
	// observers.
	mWearablesLoaded = TRUE; 
	updateServer();

	// Treat this as the first texture entry message, if none received yet
	mAvatarObject->onFirstTEMessageReceived();
}

void LLAgentWearables::makeNewOutfit(const std::string& new_folder_name,
									 const LLDynamicArray<S32>& wearables_to_include,
									 const LLDynamicArray<S32>& attachments_to_include,
									 BOOL rename_clothing)
{
	if (mAvatarObject.isNull())
	{
		return;
	}

	// First, make a folder in the Clothes directory.
	LLUUID folder_id = gInventory.createNewCategory(
		gInventory.findCategoryUUIDForType(LLAssetType::AT_CLOTHING),
		LLAssetType::AT_NONE,
		new_folder_name);

	bool found_first_item = false;

	///////////////////
	// Wearables

	if (wearables_to_include.count())
	{
		// Then, iterate though each of the wearables and save copies of them in the folder.
		S32 i;
		S32 count = wearables_to_include.count();
		LLDynamicArray<LLUUID> delete_items;
		LLPointer<LLRefCount> cbdone = NULL;
		for (i = 0; i < count; ++i)
		{
			const S32 type = wearables_to_include[i];
			for (U32 j=0; j<getWearableCount((EWearableType)i); j++)
			{
				LLWearableInv* wearable_entry = getWearableInv((EWearableType)type, j);
				LLWearable* old_wearable = wearable_entry->mWearable;
				if (old_wearable)
				{
					std::string new_name;
					LLWearable* new_wearable;
					new_wearable = LLWearableList::instance().createCopy(old_wearable);
					if (rename_clothing)
					{
						new_name = new_folder_name;
						new_name.append(" ");
						new_name.append(old_wearable->getTypeLabel());
						LLStringUtil::truncate(new_name, DB_INV_ITEM_NAME_STR_LEN);
						new_wearable->setName(new_name);
					}

					LLViewerInventoryItem* item = gInventory.getItem(wearable_entry->mItemID);
					S32 todo = addWearableToAgentInventoryCallback::CALL_NONE;
					if (!found_first_item)
					{
						found_first_item = true;
						/* set the focus to the first item */
						todo |= addWearableToAgentInventoryCallback::CALL_MAKENEWOUTFITDONE;
						/* send the agent wearables update when done */
						cbdone = new sendAgentWearablesUpdateCallback;
					}
					LLPointer<LLInventoryCallback> cb =
						new addWearableToAgentInventoryCallback(
							cbdone,
							type,
							j,
							new_wearable,
							todo);
					if (isWearableCopyable((EWearableType)type, j))
					{
						copy_inventory_item(
							gAgent.getID(),
							item->getPermissions().getOwner(),
							item->getUUID(),
							folder_id,
							new_name,
							cb);
					}
					else
					{
						move_inventory_item(
							gAgent.getID(),
							gAgent.getSessionID(),
							item->getUUID(),
							folder_id,
							new_name,
							cb);
					}
				}
			}
		}
		gInventory.notifyObservers();
	}


	///////////////////
	// Attachments

	if (attachments_to_include.count())
	{
		BOOL msg_started = FALSE;
		LLMessageSystem* msg = gMessageSystem;
		for (S32 i = 0; i < attachments_to_include.count(); i++)
		{
			S32 attachment_pt = attachments_to_include[i];
			LLViewerJointAttachment* attachment = get_if_there(mAvatarObject->mAttachmentPoints, attachment_pt, (LLViewerJointAttachment*)NULL);
			if (!attachment) continue;
			LLViewerObject* attached_object = attachment->getObject();
			if (!attached_object) continue;
			const LLUUID& item_id = attachment->getItemID();
			if (item_id.isNull()) continue;
			LLInventoryItem* item = gInventory.getItem(item_id);
			if (!item) continue;
			if (!msg_started)
			{
				msg_started = TRUE;
				msg->newMessage("CreateNewOutfitAttachments");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID", gAgent.getID());
				msg->addUUID("SessionID", gAgent.getSessionID());
				msg->nextBlock("HeaderData");
				msg->addUUID("NewFolderID", folder_id);
			}
			msg->nextBlock("ObjectData");
			msg->addUUID("OldItemID", item_id);
			msg->addUUID("OldFolderID", item->getParentUUID());
		}

		if (msg_started)
		{
			gAgent.sendReliableMessage();
		}

	} 
}

void LLAgentWearables::makeNewOutfitDone(S32 type, U32 index)
{
	LLWearableInv* wearable_entry = getWearableInv((EWearableType)type, index);
	LLUUID first_item_id = wearable_entry->mItemID;
	// Open the inventory and select the first item we added.
	if (first_item_id.notNull())
	{
		LLFloaterInventory* view = LLFloaterInventory::getActiveInventory();
		if (view)
		{
			view->getPanel()->setSelection(first_item_id, TAKE_FOCUS_NO);
		}
	}
}


void LLAgentWearables::addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												   LLWearable* wearable,
												   const LLUUID& category_id,
												   BOOL notify)
{
	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  category_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  wearable->getAssetType(),
						  LLInventoryType::IT_WEARABLE,
						  wearable->getType(),
						  wearable->getPermissions().getMaskNextOwner(),
						  cb);
}

void LLAgentWearables::removeWearable(const EWearableType type, bool do_remove_all, U32 index)
{

	if (do_remove_all)
	{
		removeWearableFinal(type, do_remove_all, index);
	}
	else
	{
// MULTI_WEARABLE: handle vector changes from arbitrary removal.
		LLWearable* old_wearable = getWearable(type,index);
		
		if ((gAgent.isTeen())
			&& (type == WT_UNDERSHIRT || type == WT_UNDERPANTS))
		{
			// Can't take off underclothing in simple UI mode or on PG accounts
			return;
		}
		
		if (old_wearable)
		{
			if (old_wearable->isDirty())
			{
				LLSD payload;
				payload["wearable_type"] = (S32)type;
				// Bring up view-modal dialog: Save changes? Yes, No, Cancel
				LLNotifications::instance().add("WearableSave", LLSD(), payload, &LLAgentWearables::onRemoveWearableDialog);
				return;
			}
			else
			{
				removeWearableFinal(type, do_remove_all, index);
			}
		}
	}
}


// MULTI_WEARABLE: assuming one wearable per type.
// MULTI_WEARABLE: hardwiring 0th elt for now - notification needs to change.
// static 
bool LLAgentWearables::onRemoveWearableDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	EWearableType type = (EWearableType)notification["payload"]["wearable_type"].asInteger();
	switch(option)
	{
		case 0:  // "Save"
			gAgentWearables.saveWearable(type, 0);
			gAgentWearables.removeWearableFinal(type, false, 0);
			break;

		case 1:  // "Don't Save"
			gAgentWearables.removeWearableFinal(type, false, 0);
			break;

		case 2: // "Cancel"
			break;

		default:
			llassert(0);
			break;
	}
	return false;
}

// Called by removeWearable() and onRemoveWearableDialog() to actually do the removal.
void LLAgentWearables::removeWearableFinal(const EWearableType type, bool do_remove_all, U32 index)
{
	//LLAgentDumper dumper("removeWearable");
	if (do_remove_all)
	{
		S32 max_entry = mWearableDatas[type].size()-1;
		for (S32 i=max_entry; i>=0; i--)
		{
			LLWearableInv *wearable_entry = getWearableInv(type,i);
			LLWearable* old_wearable = wearable_entry->mWearable;
			gInventory.addChangedMask(LLInventoryObserver::LABEL, wearable_entry->mItemID);
			wearable_entry->mWearable = NULL;
			wearable_entry->mItemID.setNull();
			//queryWearableCache(); // BAP moved below
			// MULTI_WEARABLE: FIXME - currently we keep a null entry, so can't delete the last one.
			if (i>0)
			{
				mWearableDatas[type].pop_back();
				delete wearable_entry;
			}
			if (old_wearable)
			{
				old_wearable->removeFromAvatar(TRUE);
			}
		}
	}
	else
	{
		LLWearableInv* wearable_entry = getWearableInv(type, index);
		LLWearable* old_wearable = wearable_entry->mWearable;

		gInventory.addChangedMask(LLInventoryObserver::LABEL, wearable_entry->mItemID);

		wearable_entry->mWearable = NULL;
		wearable_entry->mItemID.setNull();

		//queryWearableCache(); // BAP moved below

		if (old_wearable)
		{
			old_wearable->removeFromAvatar(TRUE);
		}

		// MULTI_WEARABLE: logic changes if null entries go away
		if (getWearableCount(type)>1)
		{
			// Have to shrink the vector and clean up the item.
			wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
			llassert_always(wearable_iter != mWearableDatas.end());
			wearableentry_vec_t& wearable_vec = wearable_iter->second;
			wearable_vec.erase( wearable_vec.begin() + index );
			delete(wearable_entry);
		}
	}

	queryWearableCache();

	// Update the server
	updateServer();
	gInventory.notifyObservers();
}

// Assumes existing wearables are not dirty.
// MULTI_WEARABLE: assumes one wearable per type.
void LLAgentWearables::setWearableOutfit(const LLInventoryItem::item_array_t& items,
										 const LLDynamicArray< LLWearable* >& wearables,
										 BOOL remove)
{
	lldebugs << "setWearableOutfit() start" << llendl;

	BOOL wearables_to_remove[WT_COUNT];
	wearables_to_remove[WT_SHAPE]		= FALSE;
	wearables_to_remove[WT_SKIN]		= FALSE;
	wearables_to_remove[WT_HAIR]		= FALSE;
	wearables_to_remove[WT_EYES]		= FALSE;
	wearables_to_remove[WT_SHIRT]		= remove;
	wearables_to_remove[WT_PANTS]		= remove;
	wearables_to_remove[WT_SHOES]		= remove;
	wearables_to_remove[WT_SOCKS]		= remove;
	wearables_to_remove[WT_JACKET]		= remove;
	wearables_to_remove[WT_GLOVES]		= remove;
	wearables_to_remove[WT_UNDERSHIRT]	= (!gAgent.isTeen()) & remove;
	wearables_to_remove[WT_UNDERPANTS]	= (!gAgent.isTeen()) & remove;
	wearables_to_remove[WT_SKIRT]		= remove;

	S32 count = wearables.count();
	llassert(items.count() == count);

	S32 i;
	for (i = 0; i < count; i++)
	{
		LLWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];

		const EWearableType type = new_wearable->getType();
		wearables_to_remove[type] = FALSE;

		// MULTI_WEARABLE: using 0th
		LLWearableInv* old_wearable_entry = getWearableInv(type, 0);
		LLWearable* old_wearable = old_wearable_entry->mWearable;
		if (old_wearable)
		{
			const LLUUID& old_item_id = old_wearable_entry->mItemID;
			if ((old_wearable->getID() == new_wearable->getID()) &&
				(old_item_id == new_item->getUUID()))
			{
				lldebugs << "No change to wearable asset and item: " << LLWearableDictionary::getInstance()->getWearableEntry(type) << llendl;
				continue;
			}

			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);

			// Assumes existing wearables are not dirty.
			if (old_wearable->isDirty())
			{
				llassert(0);
				continue;
			}
		}

		old_wearable_entry->mItemID = new_item->getUUID();
		old_wearable_entry->mWearable = new_wearable;
	}

	std::vector<LLWearable*> wearables_being_removed;

	for (i = 0; i < WT_COUNT; i++)
	{
		if (wearables_to_remove[i])
		{
			// MULTI_WEARABLE: assuming 0th
			LLWearableInv* wearable_entry = getWearableInv((EWearableType)i, 0);
			wearables_being_removed.push_back(wearable_entry->mWearable);
			wearable_entry->mWearable = NULL;
			
			gInventory.addChangedMask(LLInventoryObserver::LABEL, wearable_entry->mItemID);
			wearable_entry->mItemID.setNull();
		}
	}

	gInventory.notifyObservers();

	queryWearableCache();

	std::vector<LLWearable*>::iterator wearable_iter;

	for (wearable_iter = wearables_being_removed.begin(); 
		 wearable_iter != wearables_being_removed.end();
		 ++wearable_iter)
	{
		LLWearable* wearablep = *wearable_iter;
		if (wearablep)
		{
			wearablep->removeFromAvatar(TRUE);
		}
	}

	for (i = 0; i < count; i++)
	{
		wearables[i]->writeToAvatar(TRUE);
	}

	// Start rendering & update the server
	mWearablesLoaded = TRUE; 
	updateServer();

	lldebugs << "setWearableOutfit() end" << llendl;
}


// User has picked "wear on avatar" from a menu.
void LLAgentWearables::setWearable(LLInventoryItem* new_item, LLWearable* new_wearable, bool do_append)
{
	//LLAgentDumper dumper("setWearable");
	if (isWearingItem(new_item->getUUID()))
	{
		llwarns << "wearable " << new_item->getUUID() << " is already worn" << llendl;
		return;
	}
	
	const EWearableType type = new_wearable->getType();

	if (!do_append)
	{
		// Remove old wearable, if any
		// MULTI_WEARABLE: hardwired to 0
		LLWearableInv* old_wearable_entry = getWearableInv(type,0);
		LLWearable* old_wearable = old_wearable_entry->mWearable;
		if (old_wearable)
		{
			const LLUUID& old_item_id = old_wearable_entry->mItemID;
			if ((old_wearable->getID() == new_wearable->getID()) &&
				(old_item_id == new_item->getUUID()))
			{
				lldebugs << "No change to wearable asset and item: " << LLWearableDictionary::getInstance()->getWearableEntry(type) << llendl;
				return;
			}
			
			if (old_wearable->isDirty())
			{
				// Bring up modal dialog: Save changes? Yes, No, Cancel
				LLSD payload;
				payload["item_id"] = new_item->getUUID();
				LLNotifications::instance().add("WearableSave", LLSD(), payload, boost::bind(onSetWearableDialog, _1, _2, new_wearable));
				return;
			}
		}
	}

	setWearableFinal(new_item, new_wearable, do_append);
}

// static 
bool LLAgentWearables::onSetWearableDialog(const LLSD& notification, const LLSD& response, LLWearable* wearable)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLInventoryItem* new_item = gInventory.getItem(notification["payload"]["item_id"].asUUID());
	if (!new_item)
	{
		delete wearable;
		return false;
	}

	switch(option)
	{
		case 0:  // "Save"
// MULTI_WEARABLE: assuming 0th
			gAgentWearables.saveWearable(wearable->getType(),0);
			gAgentWearables.setWearableFinal(new_item, wearable);
			break;

		case 1:  // "Don't Save"
			gAgentWearables.setWearableFinal(new_item, wearable);
			break;

		case 2: // "Cancel"
			break;

		default:
			llassert(0);
			break;
	}

	delete wearable;
	return false;
}

// Called from setWearable() and onSetWearableDialog() to actually set the wearable.
// MULTI_WEARABLE: unify code after null objects are gone.
void LLAgentWearables::setWearableFinal(LLInventoryItem* new_item, LLWearable* new_wearable, bool do_append)
{
	const EWearableType type = new_wearable->getType();

	if (do_append && getWearableInv(type,0)->mItemID.notNull())
	{
		LLWearableInv *new_wearable_entry = new LLWearableInv;
		new_wearable_entry->mItemID = new_item->getUUID();
		new_wearable_entry->mWearable = new_wearable;
		mWearableDatas[type].push_back(new_wearable_entry);
		llinfos << "Added additional wearable for type " << type
				<< " size is now " << mWearableDatas[type].size() << llendl;
	}
	else
	{
		LLWearableInv* wearable_entry = getWearableInv(type,0);
		// Replace the old wearable with a new one.
		llassert(new_item->getAssetUUID() == new_wearable->getID());
		LLUUID old_item_id = wearable_entry->mItemID;
		wearable_entry->mItemID = new_item->getUUID();
		wearable_entry->mWearable = new_wearable;

		if (old_item_id.notNull())
		{
			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
			gInventory.notifyObservers();
		}
		llinfos << "Replaced current element 0 for type " << type
				<< " size is now " << mWearableDatas[type].size() << llendl;
	}

	//llinfos << "LLVOAvatar::setWearable()" << llendl;
	queryWearableCache();
	new_wearable->writeToAvatar(TRUE);

	updateServer();
}

void LLAgentWearables::queryWearableCache()
{
	if (!mWearablesLoaded)
	{
		return;
	}

	// Look up affected baked textures.
	// If they exist:
	//		disallow updates for affected layersets (until dataserver responds with cache request.)
	//		If cache miss, turn updates back on and invalidate composite.
	//		If cache hit, modify baked texture entries.
	//
	// Cache requests contain list of hashes for each baked texture entry.
	// Response is list of valid baked texture assets. (same message)

	gMessageSystem->newMessageFast(_PREHASH_AgentCachedTexture);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addS32Fast(_PREHASH_SerialNum, gAgentQueryManager.mWearablesCacheQueryID);

	S32 num_queries = 0;
	for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
	{
		const LLVOAvatarDictionary::BakedEntry *baked_dict = LLVOAvatarDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)baked_index);
		LLUUID hash;
		for (U8 i=0; i < baked_dict->mWearables.size(); i++)
		{
			// EWearableType baked_type = gBakedWearableMap[baked_index][baked_num];
			const EWearableType baked_type = baked_dict->mWearables[i];
			// MULTI_WEARABLE: assuming 0th
			const LLWearable* wearable = getWearableInv(baked_type,0)->mWearable;
			if (wearable)
			{
				hash ^= wearable->getID();
			}
		}
		if (hash.notNull())
		{
			hash ^= baked_dict->mWearablesHashID;
			num_queries++;
			// *NOTE: make sure at least one request gets packed

			//llinfos << "Requesting texture for hash " << hash << " in baked texture slot " << baked_index << llendl;
			gMessageSystem->nextBlockFast(_PREHASH_WearableData);
			gMessageSystem->addUUIDFast(_PREHASH_ID, hash);
			gMessageSystem->addU8Fast(_PREHASH_TextureIndex, (U8)baked_index);
		}

		gAgentQueryManager.mActiveCacheQueries[baked_index] = gAgentQueryManager.mWearablesCacheQueryID;
	}

	llinfos << "Requesting texture cache entry for " << num_queries << " baked textures" << llendl;
	gMessageSystem->sendReliable(gAgent.getRegion()->getHost());
	gAgentQueryManager.mNumPendingQueries++;
	gAgentQueryManager.mWearablesCacheQueryID++;
}

// MULTI_WEARABLE: need a way to specify by wearable rather than by type.
// User has picked "remove from avatar" from a menu.
// static
void LLAgentWearables::userRemoveWearable(void* userdata)
{
	EWearableType type = (EWearableType)(intptr_t)userdata;
	
	if (!(type==WT_SHAPE || type==WT_SKIN || type==WT_HAIR)) //&&
		//!((!gAgent.isTeen()) && (type==WT_UNDERPANTS || type==WT_UNDERSHIRT)))
	{
		// MULTI_WEARABLE: fixed to 0th for now.
		gAgentWearables.removeWearable(type,false,0);
	}
}

// static
void LLAgentWearables::userRemoveAllClothes(void* userdata)
{
	// We have to do this up front to avoid having to deal with the case of multiple wearables being dirty.
	if (gFloaterCustomize)
	{
		gFloaterCustomize->askToSaveIfDirty(userRemoveAllClothesStep2);
	}
	else
	{
		userRemoveAllClothesStep2(TRUE);
	}
}

// static
// MULTI_WEARABLE: removing all here.
void LLAgentWearables::userRemoveAllClothesStep2(BOOL proceed)
{
	if (proceed)
	{
		gAgentWearables.removeWearable(WT_SHIRT,true,0);
		gAgentWearables.removeWearable(WT_PANTS,true,0);
		gAgentWearables.removeWearable(WT_SHOES,true,0);
		gAgentWearables.removeWearable(WT_SOCKS,true,0);
		gAgentWearables.removeWearable(WT_JACKET,true,0);
		gAgentWearables.removeWearable(WT_GLOVES,true,0);
		gAgentWearables.removeWearable(WT_UNDERSHIRT,true,0);
		gAgentWearables.removeWearable(WT_UNDERPANTS,true,0);
		gAgentWearables.removeWearable(WT_SKIRT,true,0);
	}
}

void LLAgentWearables::userRemoveAllAttachments(void* userdata)
{
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		llwarns << "No avatar found." << llendl;
		return;
	}

	gMessageSystem->newMessage("ObjectDetach");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
		 iter != avatarp->mAttachmentPoints.end();)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		LLViewerObject* objectp = attachment->getObject();
		if (objectp)
		{
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, objectp->getLocalID());
		}
	}
	gMessageSystem->sendReliable(gAgent.getRegionHost());
}

void LLAgentWearables::updateServer()
{
	sendAgentWearablesUpdate();
	gAgent.sendAgentSetAppearance();
}
