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
#include "llagentwearables.h"

#include "llaccordionctrltab.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearablesfetch.h"
#include "llappearancemgr.h"
#include "llcallbacklist.h"
#include "llfloatercustomize.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llmd5.h"
#include "llnotificationsutil.h"
#include "llpaneloutfitsinventory.h"
#include "llsidetray.h"
#include "lltexlayer.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llwearable.h"
#include "llwearablelist.h"

#include <boost/scoped_ptr.hpp>

LLAgentWearables gAgentWearables;

BOOL LLAgentWearables::mInitialWearablesUpdateReceived = FALSE;

using namespace LLVOAvatarDefines;

// HACK: For EXT-3923: Pants item shows in inventory with skin icon and messes with "current look"
// Some db items are corrupted, have inventory flags = 0, implying wearable type = shape, even though
// wearable type stored in asset is some other value.
// Calling this function whenever a wearable is added to increase visibility if this problem
// turns up in other inventories.
void checkWearableAgainstInventory(LLWearable *wearable)
{
	if (wearable->getItemID().isNull())
		return;
	
	// Check for wearable type consistent with inventory item wearable type.
	LLViewerInventoryItem *item = gInventory.getItem(wearable->getItemID());
	if (item)
	{
		if (!item->isWearableType())
		{
			llwarns << "wearable associated with non-wearable item" << llendl;
		}
		if (item->getWearableType() != wearable->getType())
		{
			llwarns << "type mismatch: wearable " << wearable->getName()
					<< " has type " << wearable->getType()
					<< " but inventory item " << item->getName()
					<< " has type "  << item->getWearableType() << llendl;
		}
	}
	else
	{
		llwarns << "wearable inventory item not found" << wearable->getName()
				<< " itemID " << wearable->getItemID().asString() << llendl;
	}
}

void LLAgentWearables::dump()
{
	llinfos << "LLAgentWearablesDump" << llendl;
	for (S32 i = 0; i < WT_COUNT; i++)
	{
		U32 count = getWearableCount((EWearableType)i);
		llinfos << "Type: " << i << " count " << count << llendl;
		for (U32 j=0; j<count; j++)
		{
			LLWearable* wearable = getWearable((EWearableType)i,j);
			if (wearable == NULL)
			{
				llinfos << "    " << j << " NULL wearable" << llendl;
			}
			llinfos << "    " << j << " Name " << wearable->getName()
					<< " description " << wearable->getDescription() << llendl;
			
		}
	}
	llinfos << "Total items awaiting wearable update " << mItemsAwaitingWearableUpdate.size() << llendl;
	for (std::set<LLUUID>::iterator it = mItemsAwaitingWearableUpdate.begin();
		 it != mItemsAwaitingWearableUpdate.end();
		 ++it)
	{
		llinfos << (*it).asString() << llendl;
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
	mWearablesLoaded(FALSE)
{
}

LLAgentWearables::~LLAgentWearables()
{
	cleanup();
}

void LLAgentWearables::cleanup()
{
}

void LLAgentWearables::setAvatarObject(LLVOAvatarSelf *avatar)
{ 
	if (avatar)
	{
		sendAgentWearablesRequest();
	}
}

// wearables
LLAgentWearables::createStandardWearablesAllDoneCallback::~createStandardWearablesAllDoneCallback()
{
	llinfos << "destructor - all done?" << llendl;
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
	llinfos << "constructor" << llendl;
}

void LLAgentWearables::addWearableToAgentInventoryCallback::fire(const LLUUID& inv_item)
{
	if (mTodo & CALL_CREATESTANDARDDONE)
	{
		llinfos << "callback fired, inv_item " << inv_item.asString() << llendl;
	}

	if (inv_item.isNull())
		return;

	gAgentWearables.addWearabletoAgentInventoryDone(mType, mIndex, inv_item, mWearable);

	if (mTodo & CALL_UPDATE)
	{
		gAgentWearables.sendAgentWearablesUpdate();
	}
	if (mTodo & CALL_RECOVERDONE)
	{
		LLAppearanceMgr::instance().addCOFItemLink(inv_item,false);
		gAgentWearables.recoverMissingWearableDone();
	}
	/*
	 * Do this for every one in the loop
	 */
	if (mTodo & CALL_CREATESTANDARDDONE)
	{
		LLAppearanceMgr::instance().addCOFItemLink(inv_item,false);
		gAgentWearables.createStandardWearablesDone(mType, mIndex);
	}
	if (mTodo & CALL_MAKENEWOUTFITDONE)
	{
		gAgentWearables.makeNewOutfitDone(mType, mIndex);
	}
	if (mTodo & CALL_WEARITEM)
	{
		LLAppearanceMgr::instance().addCOFItemLink(inv_item, true);
	}
}

void LLAgentWearables::addWearabletoAgentInventoryDone(const S32 type,
													   const U32 index,
													   const LLUUID& item_id,
													   LLWearable* wearable)
{
	llinfos << "type " << type << " index " << index << " item " << item_id.asString() << llendl;

	if (item_id.isNull())
		return;

	LLUUID old_item_id = getWearableItemID((EWearableType)type,index);

	if (wearable)
	{
		wearable->setItemID(item_id);

		if (old_item_id.notNull())
		{	
			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
			setWearable((EWearableType)type,index,wearable);
		}
		else
		{
			pushWearable((EWearableType)type,wearable);
		}
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (item && wearable)
	{
		// We're changing the asset id, so we both need to set it
		// locally via setAssetUUID() and via setTransactionID() which
		// will be decoded on the server. JC
		item->setAssetUUID(wearable->getAssetID());
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
	for (S32 type=0; type < WT_COUNT; ++type)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)type); ++j)
		{
			LLWearable* wearable = getWearable((EWearableType)type,j);
			if (wearable)
			{
				if (wearable->getItemID().isNull())
				{
					LLPointer<LLInventoryCallback> cb =
						new addWearableToAgentInventoryCallback(
							LLPointer<LLRefCount>(NULL),
							type,
							j,
							wearable,
							addWearableToAgentInventoryCallback::CALL_NONE);
					addWearableToAgentInventory(cb, wearable);
				}
				else
				{
					gInventory.addChangedMask(LLInventoryObserver::LABEL,
											  wearable->getItemID());
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
	for (S32 type=0; type < WT_COUNT; ++type)
	{
		gMessageSystem->nextBlockFast(_PREHASH_WearableData);

		U8 type_u8 = (U8)type;
		gMessageSystem->addU8Fast(_PREHASH_WearableType, type_u8);

		// MULTI-WEARABLE: TODO: hacked index to 0, needs to loop over all once messages support this.
		LLWearable* wearable = getWearable((EWearableType)type, 0);
		if (wearable)
		{
			//llinfos << "Sending wearable " << wearable->getName() << llendl;
			LLUUID item_id = wearable->getItemID();
			const LLViewerInventoryItem *item = gInventory.getItem(item_id);
			if (item && item->getIsLinkType())
			{
				// Get the itemID that this item points to.  i.e. make sure
				// we are storing baseitems, not their links, in the database.
				item_id = item->getLinkedUUID();
			}
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, item_id);			
		}
		else
		{
			//llinfos << "Not wearing wearable type " << LLWearableDictionary::getInstance()->getWearable((EWearableType)i) << llendl;
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID::null);
		}

		lldebugs << "       " << LLWearableDictionary::getTypeLabel((EWearableType)type) << ": " << (wearable ? wearable->getAssetID() : LLUUID::null) << llendl;
	}
	gAgent.sendReliableMessage();
}

void LLAgentWearables::saveWearable(const EWearableType type, const U32 index, BOOL send_update)
{
	LLWearable* old_wearable = getWearable(type, index);
	if (old_wearable && (old_wearable->isDirty() || old_wearable->isOldVersion()))
	{
		LLUUID old_item_id = old_wearable->getItemID();
		LLWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
		new_wearable->setItemID(old_item_id); // should this be in LLWearable::copyDataFrom()?
		setWearable(type,index,new_wearable);

		// old_wearable may still be referred to by other inventory items. Revert
		// unsaved changes so other inventory items aren't affected by the changes
		// that were just saved.
		old_wearable->revertValues();

		LLInventoryItem* item = gInventory.getItem(old_item_id);
		if (item)
		{
			// Update existing inventory item
			LLPointer<LLViewerInventoryItem> template_item =
				new LLViewerInventoryItem(item->getUUID(),
										  item->getParentUUID(),
										  item->getPermissions(),
										  new_wearable->getAssetID(),
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

		gAgentAvatarp->wearableUpdated( type, TRUE );

		if (send_update)
		{
			sendAgentWearablesUpdate();
		}
	}
}

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
	LLWearable* old_wearable = getWearable(type, index);
	if (!old_wearable)
	{
		llwarns << "LLAgent::saveWearableAs() no old wearable." << llendl;
		return;
	}

	LLInventoryItem* item = gInventory.getItem(getWearableItemID(type,index));
	if (!item)
	{
		llwarns << "LLAgent::saveWearableAs() no inventory item." << llendl;
		return;
	}
	std::string trunc_name(new_name);
	LLStringUtil::truncate(trunc_name, DB_INV_ITEM_NAME_STR_LEN);
	LLWearable* new_wearable = LLWearableList::instance().createCopy(
		old_wearable,
		trunc_name);
	LLPointer<LLInventoryCallback> cb =
		new addWearableToAgentInventoryCallback(
			LLPointer<LLRefCount>(NULL),
			type,
			index,
			new_wearable,
			addWearableToAgentInventoryCallback::CALL_WEARITEM);
	LLUUID category_id;
	if (save_in_lost_and_found)
	{
		category_id = gInventory.findCategoryUUIDForType(
			LLFolderType::FT_LOST_AND_FOUND);
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

	// old_wearable may still be referred to by other inventory items. Revert
	// unsaved changes so other inventory items aren't affected by the changes
	// that were just saved.
	old_wearable->revertValues();
}

void LLAgentWearables::revertWearable(const EWearableType type, const U32 index)
{
	LLWearable* wearable = getWearable(type, index);
	wearable->revertValues();

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
			LLUUID curr_item_id = getWearableItemID((EWearableType)i,j);
			if (curr_item_id == item_id)
			{
				LLWearable* old_wearable = getWearable((EWearableType)i,j);
				llassert(old_wearable);

				std::string old_name = old_wearable->getName();
				old_wearable->setName(new_name);
				LLWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
				new_wearable->setItemID(item_id);
				LLInventoryItem* item = gInventory.getItem(item_id);
				if (item)
				{
					new_wearable->setPermissions(item->getPermissions());
				}
				old_wearable->setName(old_name);

				setWearable((EWearableType)i,j,new_wearable);
				sendAgentWearablesUpdate();
				break;
			}
		}
	}
}


BOOL LLAgentWearables::isWearableModifiable(EWearableType type, U32 index) const
{
	LLUUID item_id = getWearableItemID(type, index);
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
	LLUUID item_id = getWearableItemID(type, index);
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
  LLUUID item_id = getWearableItemID(type);
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
	LLUUID item_id = getWearableItemID(type,index);
	LLInventoryItem* item = NULL;
	if (item_id.notNull())
	{
		item = gInventory.getItem(item_id);
	}
	return item;
}

const LLWearable* LLAgentWearables::getWearableFromItemID(const LLUUID& item_id) const
{
	for (S32 i=0; i < WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)i); j++)
		{
			const LLWearable * curr_wearable = getWearable((EWearableType)i, j);
			if (curr_wearable && (curr_wearable->getItemID() == item_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}

LLWearable*	LLAgentWearables::getWearableFromAssetID(const LLUUID& asset_id) 
{
	for (S32 i=0; i < WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)i); j++)
		{
			LLWearable * curr_wearable = getWearable((EWearableType)i, j);
			if (curr_wearable && (curr_wearable->getAssetID() == asset_id))
			{
				return curr_wearable;
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

// static
BOOL LLAgentWearables::selfHasWearable(EWearableType type)
{
	return (gAgentWearables.getWearableCount(type) > 0);
}

LLWearable* LLAgentWearables::getWearable(const EWearableType type, U32 index)
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

void LLAgentWearables::setWearable(const EWearableType type, U32 index, LLWearable *wearable)
{

	LLWearable *old_wearable = getWearable(type,index);
	if (!old_wearable)
	{
		pushWearable(type,wearable);
		return;
	}
	
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		llwarns << "invalid type, type " << type << " index " << index << llendl; 
		return;
	}
	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index>=wearable_vec.size())
	{
		llwarns << "invalid index, type " << type << " index " << index << llendl; 
	}
	else
	{
		wearable_vec[index] = wearable;
		old_wearable->setLabelUpdated();
		wearableUpdated(wearable);
		checkWearableAgainstInventory(wearable);
	}
}

U32 LLAgentWearables::pushWearable(const EWearableType type, LLWearable *wearable)
{
	if (wearable == NULL)
	{
		// no null wearables please!
		llwarns << "Null wearable sent for type " << type << llendl;
		return MAX_WEARABLES_PER_TYPE;
	}
	if (type < WT_COUNT || mWearableDatas[type].size() < MAX_WEARABLES_PER_TYPE)
	{
		mWearableDatas[type].push_back(wearable);
		wearableUpdated(wearable);
		checkWearableAgainstInventory(wearable);
		return mWearableDatas[type].size()-1;
	}
	return MAX_WEARABLES_PER_TYPE;
}

void LLAgentWearables::wearableUpdated(LLWearable *wearable)
{
	gAgentAvatarp->wearableUpdated(wearable->getType(), TRUE);
	wearable->refreshName();
	wearable->setLabelUpdated();

	wearable->pullCrossWearableValues();

	// Hack pt 2. If the wearable we just loaded has definition version 24,
	// then force a re-save of this wearable after slamming the version number to 22.
	// This number was incorrectly incremented for internal builds before release, and
	// this fix will ensure that the affected wearables are re-saved with the right version number.
	// the versions themselves are compatible. This code can be removed before release.
	if( wearable->getDefinitionVersion() == 24 )
	{
		wearable->setDefinitionVersion(22);
		U32 index = getWearableIndex(wearable);
		llinfos << "forcing werable type " << wearable->getType() << " to version 22 from 24" << llendl;
		saveWearable(wearable->getType(),index,TRUE);
	}

}

void LLAgentWearables::popWearable(LLWearable *wearable)
{
	if (wearable == NULL)
	{
		// nothing to do here. move along.
		return;
	}

	U32 index = getWearableIndex(wearable);
	EWearableType type = wearable->getType();

	if (index < MAX_WEARABLES_PER_TYPE && index < getWearableCount(type))
	{
		popWearable(type, index);
	}
}

void LLAgentWearables::popWearable(const EWearableType type, U32 index)
{
	LLWearable *wearable = getWearable(type, index);
	if (wearable)
	{
		mWearableDatas[type].erase(mWearableDatas[type].begin() + index);
		gAgentAvatarp->wearableUpdated(wearable->getType(), TRUE);
		wearable->setLabelUpdated();
	}
}

U32	LLAgentWearables::getWearableIndex(LLWearable *wearable)
{
	if (wearable == NULL)
	{
		return MAX_WEARABLES_PER_TYPE;
	}

	const EWearableType type = wearable->getType();
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		llwarns << "tried to get wearable index with an invalid type!" << llendl;
		return MAX_WEARABLES_PER_TYPE;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	for(U32 index = 0; index < wearable_vec.size(); index++)
	{
		if (wearable_vec[index] == wearable)
		{
			return index;
		}
	}

	return MAX_WEARABLES_PER_TYPE;
}

const LLWearable* LLAgentWearables::getWearable(const EWearableType type, U32 index) const
{
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return NULL;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index>=wearable_vec.size())
	{
		return NULL;
	}
	else
	{
		return wearable_vec[index];
	}
}

LLWearable* LLAgentWearables::getTopWearable(const EWearableType type)
{
	U32 count = getWearableCount(type);
	if ( count == 0)
	{
		return NULL;
	}

	return getWearable(type, count-1);
}

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

U32 LLAgentWearables::getWearableCount(const U32 tex_index) const
{
	const EWearableType wearable_type = LLVOAvatarDictionary::getTEWearableType((LLVOAvatarDefines::ETextureIndex)tex_index);
	return getWearableCount(wearable_type);
}


BOOL LLAgentWearables::itemUpdatePending(const LLUUID& item_id) const
{
	return mItemsAwaitingWearableUpdate.find(item_id) != mItemsAwaitingWearableUpdate.end();
}

U32 LLAgentWearables::itemUpdatePendingCount() const
{
	return mItemsAwaitingWearableUpdate.size();
}

const LLUUID LLAgentWearables::getWearableItemID(EWearableType type, U32 index) const
{
	const LLWearable *wearable = getWearable(type,index);
	if (wearable)
		return wearable->getItemID();
	else
		return LLUUID();
}

const LLUUID LLAgentWearables::getWearableAssetID(EWearableType type, U32 index) const
{
	const LLWearable *wearable = getWearable(type,index);
	if (wearable)
		return wearable->getAssetID();
	else
		return LLUUID();
}

BOOL LLAgentWearables::isWearingItem(const LLUUID& item_id) const
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	if (getWearableFromItemID(base_item_id) != NULL) 
	{
		return TRUE;
	}
	return FALSE;
}

// MULTI-WEARABLE: update for multiple
// static
// ! BACKWARDS COMPATIBILITY ! When we stop supporting viewer1.23, we can assume
// that viewers have a Current Outfit Folder and won't need this message, and thus
// we can remove/ignore this whole function.
void LLAgentWearables::processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys, void** user_data)
{
	// We should only receive this message a single time.  Ignore subsequent AgentWearablesUpdates
	// that may result from AgentWearablesRequest having been sent more than once.
	if (mInitialWearablesUpdateReceived)
		return;
	mInitialWearablesUpdateReceived = true;

	LLUUID agent_id;
	gMessageSystem->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);

	if (isAgentAvatarValid() && (agent_id == gAgentAvatarp->getID()))
	{
		gMessageSystem->getU32Fast(_PREHASH_AgentData, _PREHASH_SerialNum, gAgentQueryManager.mUpdateSerialNum);
		
		const S32 NUM_BODY_PARTS = 4;
		S32 num_wearables = gMessageSystem->getNumberOfBlocksFast(_PREHASH_WearableData);
		if (num_wearables < NUM_BODY_PARTS)
		{
			// Transitional state.  Avatars should always have at least their body parts (hair, eyes, shape and skin).
			// The fact that they don't have any here (only a dummy is sent) implies that either:
			// 1. This account existed before we had wearables
			// 2. The database has gotten messed up
			// 3. This is the account's first login (i.e. the wearables haven't been generated yet).
			return;
		}

		// Get the UUID of the current outfit folder (will be created if it doesn't exist)
		const LLUUID current_outfit_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
		LLInitialWearablesFetch* outfit = new LLInitialWearablesFetch(current_outfit_id);
		
		//lldebugs << "processAgentInitialWearablesUpdate()" << llendl;
		// Add wearables
		// MULTI-WEARABLE: TODO: update once messages change.  Currently use results to populate the zeroth element.
		gAgentWearables.mItemsAwaitingWearableUpdate.clear();
		for (S32 i=0; i < num_wearables; i++)
		{
			// Parse initial wearables data from message system
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
				
				// MULTI-WEARABLE: TODO: update once messages change.  Currently use results to populate the zeroth element.
				
				// Store initial wearables data until we know whether we have the current outfit folder or need to use the data.
				LLInitialWearablesFetch::InitialWearableData wearable_data(type, item_id, asset_id); // MULTI-WEARABLE: update
				outfit->add(wearable_data);
			}
			
			lldebugs << "       " << LLWearableDictionary::getTypeLabel(type) << llendl;
		}
		
		// Get the complete information on the items in the inventory and set up an observer
		// that will trigger when the complete information is fetched.
		outfit->startFetch();
		if(outfit->isFinished())
		{
			// everything is already here - call done.
			outfit->done();
		}
		else
		{
			// it's all on it's way - add an observer, and the inventory
			// will call done for us when everything is here.
			gInventory.addObserver(outfit);
		}
		
	}
}

// Normally, all wearables referred to "AgentWearablesUpdate" will correspond to actual assets in the
// database.  If for some reason, we can't load one of those assets, we can try to reconstruct it so that
// the user isn't left without a shape, for example.  (We can do that only after the inventory has loaded.)
void LLAgentWearables::recoverMissingWearable(const EWearableType type, U32 index)
{
	// Try to recover by replacing missing wearable with a new one.
	LLNotificationsUtil::add("ReplacedMissingWearable");
	lldebugs << "Wearable " << LLWearableDictionary::getTypeLabel(type) << " could not be downloaded.  Replaced inventory item with default wearable." << llendl;
	LLWearable* new_wearable = LLWearableList::instance().createNewWearable(type);

	S32 type_s32 = (S32) type;
	setWearable(type,index,new_wearable);
	//new_wearable->writeToAvatar(TRUE);

	// Add a new one in the lost and found folder.
	// (We used to overwrite the "not found" one, but that could potentially
	// destory content.) JC
	const LLUUID lost_and_found_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
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
	updateWearablesLoaded();
	if (areWearablesLoaded())
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

void LLAgentWearables::addLocalTextureObject(const EWearableType wearable_type, const LLVOAvatarDefines::ETextureIndex texture_type, U32 wearable_index)
{
	LLWearable* wearable = getWearable((EWearableType)wearable_type, wearable_index);
	if (!wearable)
	{
		llerrs << "Tried to add local texture object to invalid wearable with type " << wearable_type << " and index " << wearable_index << llendl;
		return;
	}
	LLLocalTextureObject lto;
	wearable->setLocalTextureObject(texture_type, lto);
}

class OnWearableItemCreatedCB: public LLInventoryCallback
{
public:
	OnWearableItemCreatedCB():
		mWearablesAwaitingItems(WT_COUNT,NULL)
	{
		llinfos << "created callback" << llendl;
	}
	/* virtual */ void fire(const LLUUID& inv_item)
	{
		llinfos << "One item created " << inv_item.asString() << llendl;
		LLViewerInventoryItem *item = gInventory.getItem(inv_item);
		mItemsToLink.put(item);
		updatePendingWearable(inv_item);
	}
	~OnWearableItemCreatedCB()
	{
		llinfos << "All items created" << llendl;
		LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;
		LLAppearanceMgr::instance().linkAll(LLAppearanceMgr::instance().getCOF(),
												mItemsToLink,
												link_waiter);
	}
	void addPendingWearable(LLWearable *wearable)
	{
		if (!wearable)
		{
			llwarns << "no wearable" << llendl;
			return;
		}
		EWearableType type = wearable->getType();
		if (type<WT_COUNT)
		{
			mWearablesAwaitingItems[type] = wearable;
		}
		else
		{
			llwarns << "invalid type " << type << llendl;
		}
	}
	void updatePendingWearable(const LLUUID& inv_item)
	{
		LLViewerInventoryItem *item = gInventory.getItem(inv_item);
		if (!item)
		{
			llwarns << "no item found" << llendl;
			return;
		}
		if (!item->isWearableType())
		{
			llwarns << "non-wearable item found" << llendl;
			return;
		}
		if (item && item->isWearableType())
		{
			EWearableType type = item->getWearableType();
			if (type < WT_COUNT)
			{
				LLWearable *wearable = mWearablesAwaitingItems[type];
				if (wearable)
					wearable->setItemID(inv_item);
			}
			else
			{
				llwarns << "invalid wearable type " << type << llendl;
			}
		}
	}
	
private:
	LLInventoryModel::item_array_t mItemsToLink;
	std::vector<LLWearable*> mWearablesAwaitingItems;
};

void LLAgentWearables::createStandardWearables(BOOL female)
{
	llwarns << "Creating Standard " << (female ? "female" : "male")
			<< " Wearables" << llendl;

	if (!isAgentAvatarValid()) return;

	gAgentAvatarp->setSex(female ? SEX_FEMALE : SEX_MALE);

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

	LLPointer<LLInventoryCallback> cb = new OnWearableItemCreatedCB;
	for (S32 i=0; i < WT_COUNT; i++)
	{
		if (create[i])
		{
			llassert(getWearableCount((EWearableType)i) == 0);
			LLWearable* wearable = LLWearableList::instance().createNewWearable((EWearableType)i);
			((OnWearableItemCreatedCB*)(&(*cb)))->addPendingWearable(wearable);
			// no need to update here...
			LLUUID category_id = LLUUID::null;
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
	}
}

void LLAgentWearables::createStandardWearablesDone(S32 type, U32 index)
{
	llinfos << "type " << type << " index " << index << llendl;

	if (!isAgentAvatarValid()) return;
	gAgentAvatarp->updateVisualParams();
}

void LLAgentWearables::createStandardWearablesAllDone()
{
	// ... because sendAgentWearablesUpdate will notify inventory
	// observers.
	llinfos << "all done?" << llendl;

	mWearablesLoaded = TRUE; 
	checkWearablesLoaded();
	
	updateServer();

	// Treat this as the first texture entry message, if none received yet
	gAgentAvatarp->onFirstTEMessageReceived();
}

// MULTI-WEARABLE: Properly handle multiwearables later.
void LLAgentWearables::getAllWearablesArray(LLDynamicArray<S32>& wearables)
{
	for( S32 i = 0; i < WT_COUNT; ++i )
	{
		if (getWearableCount((EWearableType) i) !=  0)
		{
			wearables.push_back(i);
		}
	}
}

// Note:	wearables_to_include should be a list of EWearableType types
//			attachments_to_include should be a list of attachment points
void LLAgentWearables::makeNewOutfit(const std::string& new_folder_name,
									 const LLDynamicArray<S32>& wearables_to_include,
									 const LLDynamicArray<S32>& attachments_to_include,
									 BOOL rename_clothing)
{
	if (!isAgentAvatarValid()) return;

	// First, make a folder in the Clothes directory.
	LLUUID folder_id = gInventory.createNewCategory(
		gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING),
		LLFolderType::FT_NONE,
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
				LLWearable* old_wearable = getWearable((EWearableType)type, j);
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

					LLViewerInventoryItem* item = gInventory.getItem(getWearableItemID((EWearableType)type,j));
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
					llassert(item);
					if (item)
					{
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
			LLViewerJointAttachment* attachment = get_if_there(gAgentAvatarp->mAttachmentPoints, attachment_pt, (LLViewerJointAttachment*)NULL);
			if (!attachment) continue;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				LLViewerObject *attached_object = (*attachment_iter);
				if(!attached_object) continue;
				const LLUUID& item_id = (*attachment_iter)->getItemID();
				if(item_id.isNull()) continue;
				LLInventoryItem* item = gInventory.getItem(item_id);
				if(!item) continue;
				if(!msg_started)
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
		}

		if (msg_started)
		{
			gAgent.sendReliableMessage();
		}

	} 
}

class LLShowCreatedOutfit: public LLInventoryCallback
{
public:
	LLShowCreatedOutfit(LLUUID& folder_id):
		mFolderID(folder_id)
	{
	}

	virtual ~LLShowCreatedOutfit()
	{
		LLSD key;
		LLSideTray::getInstance()->showPanel("panel_outfits_inventory", key);
		LLPanelOutfitsInventory *outfit_panel =
			dynamic_cast<LLPanelOutfitsInventory*>(LLSideTray::getInstance()->getPanel("panel_outfits_inventory"));
		// TODO: add handling "My Outfits" tab.
		if (outfit_panel && outfit_panel->isCOFPanelActive())
		{
			outfit_panel->getRootFolder()->clearSelection();
			outfit_panel->getRootFolder()->setSelectionByID(mFolderID, TRUE);
		}
		LLAccordionCtrlTab* tab_outfits = outfit_panel ? outfit_panel->findChild<LLAccordionCtrlTab>("tab_outfits") : 0;
		if (tab_outfits && !tab_outfits->getDisplayChildren())
		{
			tab_outfits->changeOpenClose(tab_outfits->getDisplayChildren());
		}

		LLAppearanceMgr::instance().updateIsDirty();
		LLAppearanceMgr::instance().updatePanelOutfitName("");
	}
	
	virtual void fire(const LLUUID&)
	{
	}
	
private:
	LLUUID mFolderID;
};

void LLAgentWearables::makeNewOutfitDone(S32 type, U32 index)
{
	LLUUID first_item_id = getWearableItemID((EWearableType)type, index);
	// Open the inventory and select the first item we added.
	if (first_item_id.notNull())
	{
		LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
		if (active_panel)
		{
			active_panel->setSelection(first_item_id, TAKE_FOCUS_NO);
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
	if (gAgent.isTeen() &&
		(type == WT_UNDERSHIRT || type == WT_UNDERPANTS))
	{
		// Can't take off underclothing in simple UI mode or on PG accounts
		// TODO: enable the removing of a single undershirt/underpants if multiple are worn. - Nyx
		return;
	}
	if (getWearableCount(type) == 0)
	{
		// no wearables to remove
		return;
	}

	if (do_remove_all)
	{
		removeWearableFinal(type, do_remove_all, index);
	}
	else
	{
		LLWearable* old_wearable = getWearable(type,index);
		
		if (old_wearable)
		{
			if (old_wearable->isDirty())
			{
				LLSD payload;
				payload["wearable_type"] = (S32)type;
				payload["wearable_index"] = (S32)index;
				// Bring up view-modal dialog: Save changes? Yes, No, Cancel
				LLNotificationsUtil::add("WearableSave", LLSD(), payload, &LLAgentWearables::onRemoveWearableDialog);
				return;
			}
			else
			{
				removeWearableFinal(type, do_remove_all, index);
			}
		}
	}
}


// static 
bool LLAgentWearables::onRemoveWearableDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	EWearableType type = (EWearableType)notification["payload"]["wearable_type"].asInteger();
	S32 index = (S32)notification["payload"]["wearable_index"].asInteger();
	switch(option)
	{
		case 0:  // "Save"
			gAgentWearables.saveWearable(type, index);
			gAgentWearables.removeWearableFinal(type, false, index);
			break;

		case 1:  // "Don't Save"
			gAgentWearables.removeWearableFinal(type, false, index);
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
			LLWearable* old_wearable = getWearable(type,i);
			//queryWearableCache(); // moved below
			if (old_wearable)
			{
				popWearable(old_wearable);
				old_wearable->removeFromAvatar(TRUE);
			}
		}
		mWearableDatas[type].clear();
	}
	else
	{
		LLWearable* old_wearable = getWearable(type, index);
		//queryWearableCache(); // moved below

		if (old_wearable)
		{
			popWearable(old_wearable);
			old_wearable->removeFromAvatar(TRUE);
		}
	}

	queryWearableCache();

	// Update the server
	updateServer();
	gInventory.notifyObservers();
}

// Assumes existing wearables are not dirty.
void LLAgentWearables::setWearableOutfit(const LLInventoryItem::item_array_t& items,
										 const LLDynamicArray< LLWearable* >& wearables,
										 BOOL remove)
{
	llinfos << "setWearableOutfit() start" << llendl;

	// TODO: Removed check for ensuring that teens don't remove undershirt and underwear. Handle later
	if (remove)
	{
		// note: shirt is the first non-body part wearable item. Update if wearable order changes.
		// This loop should remove all clothing, but not any body parts
		for (S32 type = 0; type < (S32)WT_COUNT; type++)
		{
			if (LLWearableDictionary::getAssetType((EWearableType)type) == LLAssetType::AT_CLOTHING)
			{
				removeWearable((EWearableType)type, true, 0);
			}
		}
	}

	S32 count = wearables.count();
	llassert(items.count() == count);

	S32 i;
	for (i = 0; i < count; i++)
	{
		LLWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];

		llassert(new_wearable);
		if (new_wearable)
		{
			const EWearableType type = new_wearable->getType();
		
			new_wearable->setName(new_item->getName());
			new_wearable->setItemID(new_item->getUUID());

			if (LLWearableDictionary::getAssetType(type) == LLAssetType::AT_BODYPART)
			{
				// exactly one wearable per body part
				setWearable(type,0,new_wearable);
			}
			else
			{
				pushWearable(type,new_wearable);
			}
			wearableUpdated(new_wearable);
			checkWearableAgainstInventory(new_wearable);
		}
	}

	gInventory.notifyObservers();

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);
		gAgentAvatarp->updateVisualParams();
		gAgentAvatarp->invalidateAll();
	}

	// Start rendering & update the server
	mWearablesLoaded = TRUE; 
	checkWearablesLoaded();
	queryWearableCache();
	updateServer();

	lldebugs << "setWearableOutfit() end" << llendl;
}


// User has picked "wear on avatar" from a menu.
void LLAgentWearables::setWearableItem(LLInventoryItem* new_item, LLWearable* new_wearable, bool do_append)
{
	//LLAgentDumper dumper("setWearableItem");
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
		LLWearable* old_wearable = getWearable(type,0);
		if (old_wearable)
		{
			const LLUUID& old_item_id = old_wearable->getItemID();
			if ((old_wearable->getAssetID() == new_wearable->getAssetID()) &&
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
				LLNotificationsUtil::add("WearableSave", LLSD(), payload, boost::bind(onSetWearableDialog, _1, _2, new_wearable));
				return;
			}
		}
	}

	setWearableFinal(new_item, new_wearable, do_append);
}

// static 
bool LLAgentWearables::onSetWearableDialog(const LLSD& notification, const LLSD& response, LLWearable* wearable)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLInventoryItem* new_item = gInventory.getItem(notification["payload"]["item_id"].asUUID());
	U32 index = gAgentWearables.getWearableIndex(wearable);
	if (!new_item)
	{
		delete wearable;
		return false;
	}

	switch(option)
	{
		case 0:  // "Save"
			gAgentWearables.saveWearable(wearable->getType(),index);
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

// Called from setWearableItem() and onSetWearableDialog() to actually set the wearable.
// MULTI_WEARABLE: unify code after null objects are gone.
void LLAgentWearables::setWearableFinal(LLInventoryItem* new_item, LLWearable* new_wearable, bool do_append)
{
	const EWearableType type = new_wearable->getType();

	if (do_append && getWearableItemID(type,0).notNull())
	{
		new_wearable->setItemID(new_item->getUUID());
		mWearableDatas[type].push_back(new_wearable);
		llinfos << "Added additional wearable for type " << type
				<< " size is now " << mWearableDatas[type].size() << llendl;
		checkWearableAgainstInventory(new_wearable);
	}
	else
	{
		// Replace the old wearable with a new one.
		llassert(new_item->getAssetUUID() == new_wearable->getAssetID());

		LLWearable *old_wearable = getWearable(type,0);
		LLUUID old_item_id;
		if (old_wearable)
		{
			old_item_id = old_wearable->getItemID();
		}
		new_wearable->setItemID(new_item->getUUID());
		setWearable(type,0,new_wearable);

		if (old_item_id.notNull())
		{
			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
			gInventory.notifyObservers();
		}
		llinfos << "Replaced current element 0 for type " << type
				<< " size is now " << mWearableDatas[type].size() << llendl;
	}

	//llinfos << "LLVOAvatar::setWearableItem()" << llendl;
	queryWearableCache();
	//new_wearable->writeToAvatar(TRUE);

	updateServer();
}

void LLAgentWearables::queryWearableCache()
{
	if (!areWearablesLoaded())
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
		LLMD5 hash;
		bool hash_computed = false;
		for (U8 i=0; i < baked_dict->mWearables.size(); i++)
		{
			const EWearableType baked_type = baked_dict->mWearables[i];
			const U32 num_wearables = getWearableCount(baked_type);
			for (U32 index = 0; index < num_wearables; ++index)
			{
				const LLWearable* wearable = getWearable(baked_type,index);
				if (wearable)
				{
					LLUUID asset_id = wearable->getAssetID();
					hash.update((const unsigned char*)asset_id.mData, UUID_BYTES);
					hash_computed = true;
				}
			}
		}
		hash.finalize();
		if (hash_computed)
		{
			LLUUID hash_id;
			hash.raw_digest(hash_id.mData);
			hash_id ^= baked_dict->mWearablesHashID;
			num_queries++;
			// *NOTE: make sure at least one request gets packed

			//llinfos << "Requesting texture for hash " << hash << " in baked texture slot " << baked_index << llendl;
			gMessageSystem->nextBlockFast(_PREHASH_WearableData);
			gMessageSystem->addUUIDFast(_PREHASH_ID, hash_id);
			gMessageSystem->addU8Fast(_PREHASH_TextureIndex, (U8)baked_index);
		}

		gAgentQueryManager.mActiveCacheQueries[baked_index] = gAgentQueryManager.mWearablesCacheQueryID;
	}

	llinfos << "Requesting texture cache entry for " << num_queries << " baked textures" << llendl;
	gMessageSystem->sendReliable(gAgent.getRegion()->getHost());
	gAgentQueryManager.mNumPendingQueries++;
	gAgentQueryManager.mWearablesCacheQueryID++;
}

// User has picked "remove from avatar" from a menu.
// static
void LLAgentWearables::userRemoveWearable(const EWearableType &type, const U32 &index)
{
	if (!(type==WT_SHAPE || type==WT_SKIN || type==WT_HAIR || type==WT_EYES)) //&&
		//!((!gAgent.isTeen()) && (type==WT_UNDERPANTS || type==WT_UNDERSHIRT)))
	{
		gAgentWearables.removeWearable(type,false,index);
	}
}

//static 
void LLAgentWearables::userRemoveWearablesOfType(const EWearableType &type)
{
	if (!(type==WT_SHAPE || type==WT_SKIN || type==WT_HAIR || type==WT_EYES)) //&&
		//!((!gAgent.isTeen()) && (type==WT_UNDERPANTS || type==WT_UNDERSHIRT)))
	{
		gAgentWearables.removeWearable(type,true,0);
	}
}

// static
void LLAgentWearables::userRemoveAllClothes()
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
		gAgentWearables.removeWearable(WT_ALPHA,true,0);
		gAgentWearables.removeWearable(WT_TATTOO,true,0);
	}
}

// Combines userRemoveAllAttachments() and userAttachMultipleAttachments() logic to
// get attachments into desired state with minimal number of adds/removes.
void LLAgentWearables::userUpdateAttachments(LLInventoryModel::item_array_t& obj_item_array)
{
	// Possible cases:
	// already wearing but not in request set -> take off.
	// already wearing and in request set -> leave alone.
	// not wearing and in request set -> put on.

	if (!isAgentAvatarValid()) return;

	std::set<LLUUID> requested_item_ids;
	std::set<LLUUID> current_item_ids;
	for (S32 i=0; i<obj_item_array.count(); i++)
		requested_item_ids.insert(obj_item_array[i].get()->getLinkedUUID());

	// Build up list of objects to be removed and items currently attached.
	llvo_vec_t objects_to_remove;
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end();)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *objectp = (*attachment_iter);
			if (objectp)
			{
				LLUUID object_item_id = objectp->getItemID();
				if (requested_item_ids.find(object_item_id) != requested_item_ids.end())
				{
					// Object currently worn, was requested.
					// Flag as currently worn so we won't have to add it again.
					current_item_ids.insert(object_item_id);
				}
				else
				{
					// object currently worn, not requested.
					objects_to_remove.push_back(objectp);
				}
			}
		}
	}

	LLInventoryModel::item_array_t items_to_add;
	for (LLInventoryModel::item_array_t::iterator it = obj_item_array.begin();
		 it != obj_item_array.end();
		 ++it)
	{
		LLUUID linked_id = (*it).get()->getLinkedUUID();
		if (current_item_ids.find(linked_id) != current_item_ids.end())
		{
			// Requested attachment is already worn.
		}
		else
		{
			// Requested attachment is not worn yet.
			items_to_add.push_back(*it);
		}
	}
	// S32 remove_count = objects_to_remove.size();
	// S32 add_count = items_to_add.size();
	// llinfos << "remove " << remove_count << " add " << add_count << llendl;

	// Remove everything in objects_to_remove
	userRemoveMultipleAttachments(objects_to_remove);

	// Add everything in items_to_add
	userAttachMultipleAttachments(items_to_add);
}

void LLAgentWearables::userRemoveMultipleAttachments(llvo_vec_t& objects_to_remove)
{
	if (!isAgentAvatarValid()) return;

	if (objects_to_remove.empty())
		return;

	gMessageSystem->newMessage("ObjectDetach");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	
	for (llvo_vec_t::iterator it = objects_to_remove.begin();
		 it != objects_to_remove.end();
		 ++it)
	{
		LLViewerObject *objectp = *it;
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, objectp->getLocalID());
	}
	gMessageSystem->sendReliable(gAgent.getRegionHost());
}

void LLAgentWearables::userRemoveAllAttachments()
{
	if (!isAgentAvatarValid()) return;

	llvo_vec_t objects_to_remove;
	
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end();)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *attached_object = (*attachment_iter);
			if (attached_object)
			{
				objects_to_remove.push_back(attached_object);
			}
		}
	}
	userRemoveMultipleAttachments(objects_to_remove);
}

void LLAgentWearables::userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array)
{
	// Build a compound message to send all the objects that need to be rezzed.
	S32 obj_count = obj_item_array.count();

	// Limit number of packets to send
	const S32 MAX_PACKETS_TO_SEND = 10;
	const S32 OBJECTS_PER_PACKET = 4;
	const S32 MAX_OBJECTS_TO_SEND = MAX_PACKETS_TO_SEND * OBJECTS_PER_PACKET;
	if( obj_count > MAX_OBJECTS_TO_SEND )
	{
		obj_count = MAX_OBJECTS_TO_SEND;
	}
				
	// Create an id to keep the parts of the compound message together
	LLUUID compound_msg_id;
	compound_msg_id.generate();
	LLMessageSystem* msg = gMessageSystem;

	for(S32 i = 0; i < obj_count; ++i)
	{
		if( 0 == (i % OBJECTS_PER_PACKET) )
		{
			// Start a new message chunk
			msg->newMessageFast(_PREHASH_RezMultipleAttachmentsFromInv);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_HeaderData);
			msg->addUUIDFast(_PREHASH_CompoundMsgID, compound_msg_id );
			msg->addU8Fast(_PREHASH_TotalObjects, obj_count );
			msg->addBOOLFast(_PREHASH_FirstDetachAll, false );
		}

		const LLInventoryItem* item = obj_item_array.get(i).get();
		msg->nextBlockFast(_PREHASH_ObjectData );
		msg->addUUIDFast(_PREHASH_ItemID, item->getLinkedUUID());
		msg->addUUIDFast(_PREHASH_OwnerID, item->getPermissions().getOwner());
		msg->addU8Fast(_PREHASH_AttachmentPt, 0 );	// Wear at the previous or default attachment point
		pack_permissions_slam(msg, item->getFlags(), item->getPermissions());
		msg->addStringFast(_PREHASH_Name, item->getName());
		msg->addStringFast(_PREHASH_Description, item->getDescription());

		if( (i+1 == obj_count) || ((OBJECTS_PER_PACKET-1) == (i % OBJECTS_PER_PACKET)) )
		{
			// End of message chunk
			msg->sendReliable( gAgent.getRegion()->getHost() );
		}
	}
}

void LLAgentWearables::checkWearablesLoaded() const
{
#ifdef SHOW_ASSERT
	U32 item_pend_count = itemUpdatePendingCount();
	if (mWearablesLoaded)
	{
		llassert(item_pend_count==0);
	}
#endif
}

BOOL LLAgentWearables::areWearablesLoaded() const
{
	checkWearablesLoaded();
	return mWearablesLoaded;
}

// MULTI-WEARABLE: update for multiple indices.
void LLAgentWearables::updateWearablesLoaded()
{
	mWearablesLoaded = (itemUpdatePendingCount()==0);
}

bool LLAgentWearables::canWearableBeRemoved(const LLWearable* wearable) const
{
	if (!wearable) return false;
	
	EWearableType type = wearable->getType();
	// Make sure the user always has at least one shape, skin, eyes, and hair type currently worn.
	return !(((type == WT_SHAPE) || (type == WT_SKIN) || (type == WT_HAIR) || (type == WT_EYES))
			 && (getWearableCount(type) <= 1) );		  
}
void LLAgentWearables::animateAllWearableParams(F32 delta, BOOL upload_bake)
{
	for( S32 type = 0; type < WT_COUNT; ++type )
	{
		for (S32 count = 0; count < (S32)getWearableCount((EWearableType)type); ++count)
		{
			LLWearable *wearable = getWearable((EWearableType)type,count);
			wearable->animateParams(delta, upload_bake);
		}
	}
}

void LLAgentWearables::updateServer()
{
	sendAgentWearablesUpdate();
	gAgent.sendAgentSetAppearance();
}

void LLAgentWearables::populateMyOutfitsFolder(void)
{	
	llinfos << "starting outfit population" << llendl;

	const LLUUID& my_outfits_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
	LLLibraryOutfitsFetch* outfits = new LLLibraryOutfitsFetch(my_outfits_id);
	outfits->mMyOutfitsID = my_outfits_id;
	
	// Get the complete information on the items in the inventory and 
	// setup an observer that will wait for that to happen.
	gInventory.addObserver(outfits);
	outfits->startFetch();
	if (outfits->isFinished())
	{
		outfits->done();
	}
}
