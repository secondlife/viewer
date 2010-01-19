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

#include "llcallbacklist.h"
#include "llfloatercustomize.h"
#include "llinventorybridge.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llnotificationsutil.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llwearable.h"
#include "llwearablelist.h"
#include "llgesturemgr.h"
#include "llappearancemgr.h"
#include "lltexlayer.h"
#include "llsidetray.h"
#include "llpaneloutfitsinventory.h"
#include "llfolderview.h"
#include "llaccordionctrltab.h"

#include <boost/scoped_ptr.hpp>

#define USE_CURRENT_OUTFIT_FOLDER

//--------------------------------------------------------------------
// Classes for fetching initial wearables data
//--------------------------------------------------------------------
// Outfit folder fetching callback structure.
class LLInitialWearablesFetch : public LLInventoryFetchDescendentsObserver
{
public:
	LLInitialWearablesFetch() {}
	~LLInitialWearablesFetch();
	virtual void done();

	struct InitialWearableData
	{
		EWearableType mType;
		LLUUID mItemID;
		LLUUID mAssetID;
		InitialWearableData(EWearableType type, LLUUID& itemID, LLUUID& assetID) :
			mType(type),
			mItemID(itemID),
			mAssetID(assetID)
		{}
	};

	typedef std::vector<InitialWearableData> initial_wearable_data_vec_t;
	initial_wearable_data_vec_t mCOFInitialWearables; // Wearables from the Current Outfit Folder
	initial_wearable_data_vec_t mAgentInitialWearables; // Wearables from the old agent wearables msg

protected:
	void processWearablesMessage();
	void processContents();
};

class LLLibraryOutfitsFetch : public LLInventoryFetchDescendentsObserver
{
public:
	enum ELibraryOutfitFetchStep {
		LOFS_FOLDER = 0,
		LOFS_OUTFITS,
		LOFS_LIBRARY,
		LOFS_IMPORTED,
		LOFS_CONTENTS
	};
	LLLibraryOutfitsFetch() : mCurrFetchStep(LOFS_FOLDER), mOutfitsPopulated(false) 
	{
		mMyOutfitsID = LLUUID::null;
		mClothingID = LLUUID::null;
		mLibraryClothingID = LLUUID::null;
		mImportedClothingID = LLUUID::null;
		mImportedClothingName = "Imported Library Clothing";
	}
	~LLLibraryOutfitsFetch() {}
	virtual void done();
	void doneIdle();
	LLUUID mMyOutfitsID;
	void importedFolderFetch();
protected:
	void folderDone(void);
	void outfitsDone(void);
	void libraryDone(void);
	void importedFolderDone(void);
	void contentsDone(void);
	enum ELibraryOutfitFetchStep mCurrFetchStep;
	typedef std::vector< std::pair< LLUUID, std::string > > cloth_folder_vec_t;
	cloth_folder_vec_t mLibraryClothingFolders;
	cloth_folder_vec_t mImportedClothingFolders;
	bool mOutfitsPopulated;
	LLUUID mClothingID;
	LLUUID mLibraryClothingID;
	LLUUID mImportedClothingID;
	std::string mImportedClothingName;
};

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
	mWearablesLoaded(FALSE),
	mAvatarObject(NULL)
{
}

LLAgentWearables::~LLAgentWearables()
{
	cleanup();
}

void LLAgentWearables::cleanup()
{
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

	LLUUID old_item_id = getWearableItemID((EWearableType)type,index);
	if (wearable)
	{
		wearable->setItemID(item_id);
	}

	if (old_item_id.notNull())
	{	
		gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
		setWearable((EWearableType)type,index,wearable);
	}
	else
	{
		pushWearable((EWearableType)type,wearable);
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

		gAgent.getAvatarObject()->wearableUpdated( type, TRUE );

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
			addWearableToAgentInventoryCallback::CALL_UPDATE);
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

const LLWearable*	LLAgentWearables::getWearableFromAssetID(const LLUUID& asset_id) const
{
	for (S32 i=0; i < WT_COUNT; i++)
	{
		for (U32 j=0; j < getWearableCount((EWearableType)i); j++)
		{
			const LLWearable * curr_wearable = getWearable((EWearableType)i, j);
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
	mAvatarObject->wearableUpdated(wearable->getType(), TRUE);
	wearable->refreshName();
	wearable->setLabelUpdated();

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
		mAvatarObject->wearableUpdated(wearable->getType(), TRUE);
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
	
	// If this is the very first time the user has logged into viewer2+ (from a legacy viewer, or new account)
	// then auto-populate outfits from the library into the My Outfits folder.
	if (LLInventoryModel::getIsFirstTimeInViewer2() || gSavedSettings.getBOOL("MyOutfitsAutofill"))
	{
		gAgentWearables.populateMyOutfitsFolder();
	}

	LLUUID agent_id;
	gMessageSystem->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (avatar && (agent_id == avatar->getID()))
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
		
		LLInitialWearablesFetch* outfit = new LLInitialWearablesFetch();
		
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
				outfit->mAgentInitialWearables.push_back(wearable_data);
				
			}
			
			lldebugs << "       " << LLWearableDictionary::getTypeLabel(type) << llendl;
		}
		
		// Get the complete information on the items in the inventory and set up an observer
		// that will trigger when the complete information is fetched.
		LLInventoryFetchDescendentsObserver::folder_ref_t folders;
		folders.push_back(current_outfit_id);
		outfit->fetchDescendents(folders);
		if(outfit->isEverythingComplete())
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

// A single wearable that the avatar was wearing on start-up has arrived from the database.
// static
void LLAgentWearables::onInitialWearableAssetArrived(LLWearable* wearable, void* userdata)
{
	boost::scoped_ptr<LLInitialWearablesFetch::InitialWearableData> wear_data((LLInitialWearablesFetch::InitialWearableData*)userdata); 
	const EWearableType type = wear_data->mType;
	U32 index = 0;

	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if (!avatar)
	{
		return;
	}

	if (wearable)
	{
		llassert(type == wearable->getType());
		wearable->setItemID(wear_data->mItemID);
		index = gAgentWearables.pushWearable(type, wearable);
		gAgentWearables.mItemsAwaitingWearableUpdate.erase(wear_data->mItemID);

		// disable composites if initial textures are baked
		avatar->setupComposites();

		avatar->setCompositeUpdatesEnabled(TRUE);
		gInventory.addChangedMask(LLInventoryObserver::LABEL, wearable->getItemID());
	}
	else
	{
		// Somehow the asset doesn't exist in the database.
		gAgentWearables.recoverMissingWearable(type,index);
	}

	gInventory.notifyObservers();

	// Have all the wearables that the avatar was wearing at log-in arrived?
	// MULTI-WEARABLE: update when multiple wearables can arrive per type.

	gAgentWearables.updateWearablesLoaded();
	if (gAgentWearables.areWearablesLoaded())
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
	}
	LLLocalTextureObject* lto = new LLLocalTextureObject();
	wearable->setLocalTextureObject(texture_type, lto);
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
			llassert(getWearableCount((EWearableType)i) == 0);
			LLWearable* wearable = LLWearableList::instance().createNewWearable((EWearableType)i);
			U32 index = pushWearable((EWearableType)i,wearable);
			// no need to update here...
			LLPointer<LLInventoryCallback> cb =
				new addWearableToAgentInventoryCallback(
					donecb,
					i,
					index,
					wearable,
					addWearableToAgentInventoryCallback::CALL_CREATESTANDARDDONE);
			addWearableToAgentInventory(cb, wearable, LLUUID::null, FALSE);
		}
	}
}

void LLAgentWearables::createStandardWearablesDone(S32 type, U32 index)
{
	if (mAvatarObject)
	{
		mAvatarObject->updateVisualParams();
	}
}

void LLAgentWearables::createStandardWearablesAllDone()
{
	// ... because sendAgentWearablesUpdate will notify inventory
	// observers.
	mWearablesLoaded = TRUE; 
	checkWearablesLoaded();
	
	updateServer();

	// Treat this as the first texture entry message, if none received yet
	mAvatarObject->onFirstTEMessageReceived();
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
	if (mAvatarObject.isNull())
	{
		return;
	}

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
		if (outfit_panel)
		{
			outfit_panel->getRootFolder()->clearSelection();
			outfit_panel->getRootFolder()->setSelectionByID(mFolderID, TRUE);
		}
		LLAccordionCtrlTab* tab_outfits = outfit_panel ? outfit_panel->findChild<LLAccordionCtrlTab>("tab_outfits") : 0;
		if (tab_outfits && !tab_outfits->getDisplayChildren())
		{
			tab_outfits->changeOpenClose(tab_outfits->getDisplayChildren());
		}

		LLAppearanceManager::instance().updateIsDirty();
		LLAppearanceManager::instance().updatePanelOutfitName("");
	}
	
	virtual void fire(const LLUUID&)
	{
	}
	
private:
	LLUUID mFolderID;
};

LLUUID LLAgentWearables::makeNewOutfitLinks(const std::string& new_folder_name)
{
	if (mAvatarObject.isNull())
	{
		return LLUUID::null;
	}

	// First, make a folder in the My Outfits directory.
	const LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
	LLUUID folder_id = gInventory.createNewCategory(
		parent_id,
		LLFolderType::FT_OUTFIT,
		new_folder_name);

	LLPointer<LLInventoryCallback> cb = new LLShowCreatedOutfit(folder_id);
	LLAppearanceManager::instance().shallowCopyCategory(LLAppearanceManager::instance().getCOF(),folder_id, cb);
	LLAppearanceManager::instance().createBaseOutfitLink(folder_id, cb);

	return folder_id;
}

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


// MULTI_WEARABLE: assuming one wearable per type.
// MULTI_WEARABLE: hardwiring 0th elt for now - notification needs to change.
// static 
bool LLAgentWearables::onRemoveWearableDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
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
	wearables_to_remove[WT_ALPHA]		= remove;
	wearables_to_remove[WT_TATTOO]		= remove;


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
		LLWearable* old_wearable = getWearable(type, 0);
		if (old_wearable)
		{
			const LLUUID& old_item_id = getWearableItemID(type, 0);
			if ((old_wearable->getAssetID() == new_wearable->getAssetID()) &&
				(old_item_id == new_item->getUUID()))
			{
				lldebugs << "No change to wearable asset and item: " << LLWearableDictionary::getInstance()->getWearableEntry(type) << llendl;
				continue;
			}

			// Assumes existing wearables are not dirty.
			if (old_wearable->isDirty())
			{
				llassert(0);
				continue;
			}
		}

		if (new_wearable)
			new_wearable->setItemID(new_item->getUUID());
		setWearable(type,0,new_wearable);
	}

	std::vector<LLWearable*> wearables_being_removed;

	for (i = 0; i < WT_COUNT; i++)
	{
		if (wearables_to_remove[i])
		{
			// MULTI_WEARABLE: assuming 0th
			LLWearable* wearable = getWearable((EWearableType)i, 0);
			const LLUUID &item_id = getWearableItemID((EWearableType)i,0);
			gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
			if (wearable)
			{
				wearables_being_removed.push_back(wearable);
			}
			removeWearable((EWearableType)i,true,0);
		}
	}

	gInventory.notifyObservers();


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

	if (mAvatarObject)
	{
		mAvatarObject->updateVisualParams();
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
		LLUUID hash;
		for (U8 i=0; i < baked_dict->mWearables.size(); i++)
		{
			const EWearableType baked_type = baked_dict->mWearables[i];
			// MULTI_WEARABLE: not order-dependent
			const U32 num_wearables = getWearableCount(baked_type);
			for (U32 index = 0; index < num_wearables; ++index)
			{
				const LLWearable* wearable = getWearable(baked_type,index);
				if (wearable)
				{
					hash ^= wearable->getAssetID();
				}
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
void LLAgentWearables::userRemoveWearable(EWearableType& type)
{
	if (!(type==WT_SHAPE || type==WT_SKIN || type==WT_HAIR)) //&&
		//!((!gAgent.isTeen()) && (type==WT_UNDERPANTS || type==WT_UNDERSHIRT)))
	{
		// MULTI_WEARABLE: fixed to 0th for now.
		gAgentWearables.removeWearable(type,false,0);
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

	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		llwarns << "No avatar found." << llendl;
		return;
	}

	std::set<LLUUID> requested_item_ids;
	std::set<LLUUID> current_item_ids;
	for (S32 i=0; i<obj_item_array.count(); i++)
		requested_item_ids.insert(obj_item_array[i].get()->getLinkedUUID());

	// Build up list of objects to be removed and items currently attached.
	llvo_vec_t objects_to_remove;
	for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
		 iter != avatarp->mAttachmentPoints.end();)
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
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		llwarns << "No avatar found." << llendl;
		return;
	}

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
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		llwarns << "No avatar found." << llendl;
		return;
	}

	llvo_vec_t objects_to_remove;
	
	for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
		 iter != avatarp->mAttachmentPoints.end();)
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
	LLLibraryOutfitsFetch* outfits = new LLLibraryOutfitsFetch();
	
	// Get the complete information on the items in the inventory and 
	// setup an observer that will wait for that to happen.
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	outfits->mMyOutfitsID = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

	folders.push_back(outfits->mMyOutfitsID);
	gInventory.addObserver(outfits);
	outfits->fetchDescendents(folders);
	if (outfits->isEverythingComplete())
	{
		outfits->done();
	}
}

void LLLibraryOutfitsFetch::done()
{
	// Delay this until idle() routine, since it's a heavy operation and
	// we also can't have it run within notifyObservers.
	doOnIdle(boost::bind(&LLLibraryOutfitsFetch::doneIdle,this));
	gInventory.removeObserver(this); // Prevent doOnIdle from being added twice.
}

void LLLibraryOutfitsFetch::doneIdle()
{
	gInventory.addObserver(this); // Add this back in since it was taken out during ::done()
	
	switch (mCurrFetchStep)
	{
		case LOFS_FOLDER:
			folderDone();
			mCurrFetchStep = LOFS_OUTFITS;
			break;
		case LOFS_OUTFITS:
			outfitsDone();
			mCurrFetchStep = LOFS_LIBRARY;
			break;
		case LOFS_LIBRARY:
			libraryDone();
			mCurrFetchStep = LOFS_IMPORTED;
			break;
		case LOFS_IMPORTED:
			importedFolderDone();
			mCurrFetchStep = LOFS_CONTENTS;
			break;
		case LOFS_CONTENTS:
			contentsDone();
			break;
		default:
			llwarns << "Got invalid state for outfit fetch: " << mCurrFetchStep << llendl;
			mOutfitsPopulated = TRUE;
			break;
	}

	// We're completely done.  Cleanup.
	if (mOutfitsPopulated)
	{
		gInventory.removeObserver(this);
		delete this;
		return;
	}
}

void LLLibraryOutfitsFetch::folderDone(void)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	gInventory.collectDescendents(mMyOutfitsID, cat_array, wearable_array, 
								  LLInventoryModel::EXCLUDE_TRASH);
	
	// Early out if we already have items in My Outfits.
	if (cat_array.count() > 0 || wearable_array.count() > 0)
	{
		mOutfitsPopulated = true;
		return;
	}
	
	mClothingID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
	mLibraryClothingID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING, false, true);
	
	mCompleteFolders.clear();
	
	// Get the complete information on the items in the inventory.
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	folders.push_back(mClothingID);
	folders.push_back(mLibraryClothingID);
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

void LLLibraryOutfitsFetch::outfitsDone(void)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	
	// Collect the contents of the Library's Clothing folder
	gInventory.collectDescendents(mLibraryClothingID, cat_array, wearable_array, 
								  LLInventoryModel::EXCLUDE_TRASH);
	
	llassert(cat_array.count() > 0);
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
		 ++iter)
	{
		const LLViewerInventoryCategory *cat = iter->get();
		
		// Get the names and id's of every outfit in the library, except for ruth and other "misc" outfits.
		if (cat->getName() != "More Outfits" && cat->getName() != "Ruth")
		{
			// Get the name of every outfit in the library 
			folders.push_back(cat->getUUID());
			mLibraryClothingFolders.push_back(std::make_pair(cat->getUUID(), cat->getName()));
		}
	}
	
	// Collect the contents of your Inventory Clothing folder
	cat_array.clear();
	wearable_array.clear();
	gInventory.collectDescendents(mClothingID, cat_array, wearable_array, 
								  LLInventoryModel::EXCLUDE_TRASH);

	// Check if you already have an "Imported Library Clothing" folder
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
		 ++iter)
	{
		const LLViewerInventoryCategory *cat = iter->get();
		if (cat->getName() == mImportedClothingName)
		{
			mImportedClothingID = cat->getUUID();
		}
	}
	
	mCompleteFolders.clear();
	
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

class LLLibraryOutfitsCopyDone: public LLInventoryCallback
{
public:
	LLLibraryOutfitsCopyDone(LLLibraryOutfitsFetch * fetcher):
	mFireCount(0), mLibraryOutfitsFetcher(fetcher)
	{
	}
	
	virtual ~LLLibraryOutfitsCopyDone()
	{
		if (mLibraryOutfitsFetcher)
		{
			gInventory.addObserver(mLibraryOutfitsFetcher);
			mLibraryOutfitsFetcher->done();
		}
	}
	
	/* virtual */ void fire(const LLUUID& inv_item)
	{
		mFireCount++;
	}
private:
	U32 mFireCount;
	LLLibraryOutfitsFetch * mLibraryOutfitsFetcher;
};

void LLLibraryOutfitsFetch::libraryDone(void)
{
	// Copy the clothing folders from the library into the imported clothing folder if necessary.
	if (mImportedClothingID == LLUUID::null)
	{
		gInventory.removeObserver(this);
		LLPointer<LLInventoryCallback> copy_waiter = new LLLibraryOutfitsCopyDone(this);
		mImportedClothingID = gInventory.createNewCategory(mClothingID,
														   LLFolderType::FT_NONE,
														   mImportedClothingName);
		
		for (cloth_folder_vec_t::const_iterator iter = mLibraryClothingFolders.begin();
			 iter != mLibraryClothingFolders.end();
			 ++iter)
		{
			LLUUID folder_id = gInventory.createNewCategory(mImportedClothingID,
															LLFolderType::FT_NONE,
															iter->second);
			LLAppearanceManager::getInstance()->shallowCopyCategory(iter->first, folder_id, copy_waiter);
		}
	}
	else
	{
		// Skip straight to fetching the contents of the imported folder
		importedFolderFetch();
	}
}

void LLLibraryOutfitsFetch::importedFolderFetch(void)
{
	// Fetch the contents of the Imported Clothing Folder
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	folders.push_back(mImportedClothingID);
	
	mCompleteFolders.clear();
	
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

void LLLibraryOutfitsFetch::importedFolderDone(void)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	
	// Collect the contents of the Imported Clothing folder
	gInventory.collectDescendents(mImportedClothingID, cat_array, wearable_array, 
								  LLInventoryModel::EXCLUDE_TRASH);
	
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
		 ++iter)
	{
		const LLViewerInventoryCategory *cat = iter->get();
		
		// Get the name of every imported outfit
		folders.push_back(cat->getUUID());
		mImportedClothingFolders.push_back(std::make_pair(cat->getUUID(), cat->getName()));
	}
	
	mCompleteFolders.clear();
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

void LLLibraryOutfitsFetch::contentsDone(void)
{		
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	
	for (cloth_folder_vec_t::const_iterator folder_iter = mImportedClothingFolders.begin();
		 folder_iter != mImportedClothingFolders.end();
		 ++folder_iter)
	{
		// First, make a folder in the My Outfits directory.
		LLUUID new_outfit_folder_id = gInventory.createNewCategory(mMyOutfitsID, LLFolderType::FT_OUTFIT, folder_iter->second);
		
		cat_array.clear();
		wearable_array.clear();
		// Collect the contents of each imported clothing folder, so we can create new outfit links for it
		gInventory.collectDescendents(folder_iter->first, cat_array, wearable_array, 
									  LLInventoryModel::EXCLUDE_TRASH);
		
		for (LLInventoryModel::item_array_t::const_iterator wearable_iter = wearable_array.begin();
			 wearable_iter != wearable_array.end();
			 ++wearable_iter)
		{
			const LLViewerInventoryItem *item = wearable_iter->get();
			link_inventory_item(gAgent.getID(),
								item->getLinkedUUID(),
								new_outfit_folder_id,
								item->getName(),
								LLAssetType::AT_LINK,
								NULL);
		}
	}

	mOutfitsPopulated = true;
}

//--------------------------------------------------------------------
// InitialWearablesFetch
// 
// This grabs contents from the COF and processes them.
// The processing is handled in idle(), i.e. outside of done(),
// to avoid gInventory.notifyObservers recursion.
//--------------------------------------------------------------------

LLInitialWearablesFetch::~LLInitialWearablesFetch()
{
}

// virtual
void LLInitialWearablesFetch::done()
{
	// Delay processing the actual results of this so it's not handled within
	// gInventory.notifyObservers.  The results will be handled in the next
	// idle tick instead.
	gInventory.removeObserver(this);
	doOnIdle(boost::bind(&LLInitialWearablesFetch::processContents,this));
}

void LLInitialWearablesFetch::processContents()
{
	// Fetch the wearable items from the Current Outfit Folder
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	LLFindWearables is_wearable;
	gInventory.collectDescendentsIf(mCompleteFolders.front(), cat_array, wearable_array, 
									LLInventoryModel::EXCLUDE_TRASH, is_wearable);

	LLAppearanceManager::instance().setAttachmentInvLinkEnable(true);
	if (wearable_array.count() > 0)
	{
		LLAppearanceManager::instance().updateAppearanceFromCOF();
	}
	else
	{
		// if we're constructing the COF from the wearables message, we don't have a proper outfit link
		LLAppearanceManager::instance().setOutfitDirty(true);
		processWearablesMessage();
	}
	delete this;
}

class LLFetchAndLinkObserver: public LLInventoryFetchObserver
{
public:
	LLFetchAndLinkObserver(LLInventoryFetchObserver::item_ref_t& ids):
		m_ids(ids),
		LLInventoryFetchObserver(true)
	{
	}
	~LLFetchAndLinkObserver()
	{
	}
	virtual void done()
	{
		gInventory.removeObserver(this);
		// Link to all fetched items in COF.
		for (LLInventoryFetchObserver::item_ref_t::iterator it = m_ids.begin();
			 it != m_ids.end();
			 ++it)
		{
			LLUUID id = *it;
			LLViewerInventoryItem *item = gInventory.getItem(*it);
			if (!item)
			{
				llwarns << "fetch failed!" << llendl;
				continue;
			}
			link_inventory_item(gAgent.getID(), item->getLinkedUUID(), LLAppearanceManager::instance().getCOF(), item->getName(),
								LLAssetType::AT_LINK, LLPointer<LLInventoryCallback>(NULL));
		}
	}
private:
	LLInventoryFetchObserver::item_ref_t m_ids;
};

void LLInitialWearablesFetch::processWearablesMessage()
{
	if (!mAgentInitialWearables.empty()) // We have an empty current outfit folder, use the message data instead.
	{
		const LLUUID current_outfit_id = LLAppearanceManager::instance().getCOF();
		LLInventoryFetchObserver::item_ref_t ids;
		for (U8 i = 0; i < mAgentInitialWearables.size(); ++i)
		{
			// Populate the current outfit folder with links to the wearables passed in the message
			InitialWearableData *wearable_data = new InitialWearableData(mAgentInitialWearables[i]); // This will be deleted in the callback.
			
			if (wearable_data->mAssetID.notNull())
			{
#ifdef USE_CURRENT_OUTFIT_FOLDER
				ids.push_back(wearable_data->mItemID);
#endif
				// Fetch the wearables
				LLWearableList::instance().getAsset(wearable_data->mAssetID,
													LLStringUtil::null,
													LLWearableDictionary::getAssetType(wearable_data->mType),
													LLAgentWearables::onInitialWearableAssetArrived, (void*)(wearable_data));
			}
			else
			{
				llinfos << "Invalid wearable, type " << wearable_data->mType << " itemID "
				<< wearable_data->mItemID << " assetID " << wearable_data->mAssetID << llendl;
			}
		}

		// Add all current attachments to the requested items as well.
		LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
		if( avatar )
		{
			for (LLVOAvatar::attachment_map_t::const_iterator iter = avatar->mAttachmentPoints.begin(); 
				 iter != avatar->mAttachmentPoints.end(); ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;
				if (!attachment) continue;
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					LLViewerObject* attached_object = (*attachment_iter);
					if (!attached_object) continue;
					const LLUUID& item_id = attached_object->getItemID();
					if (item_id.isNull()) continue;
					ids.push_back(item_id);
				}
			}
		}

		// Need to fetch the inventory items for ids, then create links to them after they arrive.
		LLFetchAndLinkObserver *fetcher = new LLFetchAndLinkObserver(ids);
		fetcher->fetchItems(ids);
		// If no items to be fetched, done will never be triggered.
		// TODO: Change LLInventoryFetchObserver::fetchItems to trigger done() on this condition.
		if (fetcher->isEverythingComplete())
		{
			fetcher->done();
		}
		else
		{
			gInventory.addObserver(fetcher);
		}
	}
	else
	{
		LL_WARNS("Wearables") << "No current outfit folder items found and no initial wearables fallback message received." << LL_ENDL;
	}
}


