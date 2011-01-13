/** 
 * @file llviewerinventory.cpp
 * @brief Implementation of the viewer side inventory objects.
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

#include "llviewerprecompiledheaders.h"
#include "llviewerinventory.h"

#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "message.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llviewerfoldertype.h"
#include "llfolderview.h"
#include "llviewercontrol.h"
#include "llconsole.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llgesturemgr.h"
#include "llsidetray.h"

#include "llinventorybridge.h"
#include "llinventorypanel.h"
#include "llfloaterinventory.h"
#include "lllandmarkactions.h"

#include "llviewerassettype.h"
#include "llviewerregion.h"
#include "llviewerobjectlist.h"
#include "llpreviewgesture.h"
#include "llviewerwindow.h"
#include "lltrans.h"
#include "llappearancemgr.h"
#include "llcommandhandler.h"
#include "llviewermessage.h"
#include "llsidepanelappearance.h"
#include "llavatarnamecache.h"
#include "llavataractions.h"
#include "lllogininstance.h"

///----------------------------------------------------------------------------
/// Helper class to store special inventory item names and their localized values.
///----------------------------------------------------------------------------
class LLLocalizedInventoryItemsDictionary : public LLSingleton<LLLocalizedInventoryItemsDictionary>
{
public:
	std::map<std::string, std::string> mInventoryItemsDict;

	LLLocalizedInventoryItemsDictionary()
	{
		mInventoryItemsDict["New Shape"]		= LLTrans::getString("New Shape");
		mInventoryItemsDict["New Skin"]			= LLTrans::getString("New Skin");
		mInventoryItemsDict["New Hair"]			= LLTrans::getString("New Hair");
		mInventoryItemsDict["New Eyes"]			= LLTrans::getString("New Eyes");
		mInventoryItemsDict["New Shirt"]		= LLTrans::getString("New Shirt");
		mInventoryItemsDict["New Pants"]		= LLTrans::getString("New Pants");
		mInventoryItemsDict["New Shoes"]		= LLTrans::getString("New Shoes");
		mInventoryItemsDict["New Socks"]		= LLTrans::getString("New Socks");
		mInventoryItemsDict["New Jacket"]		= LLTrans::getString("New Jacket");
		mInventoryItemsDict["New Gloves"]		= LLTrans::getString("New Gloves");
		mInventoryItemsDict["New Undershirt"]	= LLTrans::getString("New Undershirt");
		mInventoryItemsDict["New Underpants"]	= LLTrans::getString("New Underpants");
		mInventoryItemsDict["New Skirt"]		= LLTrans::getString("New Skirt");
		mInventoryItemsDict["New Alpha"]		= LLTrans::getString("New Alpha");
		mInventoryItemsDict["New Tattoo"]		= LLTrans::getString("New Tattoo");
		mInventoryItemsDict["New Physics"]		= LLTrans::getString("New Physics");
		mInventoryItemsDict["Invalid Wearable"] = LLTrans::getString("Invalid Wearable");

		mInventoryItemsDict["New Gesture"]		= LLTrans::getString("New Gesture");
		mInventoryItemsDict["New Script"]		= LLTrans::getString("New Script");
		mInventoryItemsDict["New Folder"]		= LLTrans::getString("New Folder");
		mInventoryItemsDict["New Note"]			= LLTrans::getString("New Note");
		mInventoryItemsDict["Contents"]			= LLTrans::getString("Contents");

		mInventoryItemsDict["Gesture"]			= LLTrans::getString("Gesture");
		mInventoryItemsDict["Male Gestures"]	= LLTrans::getString("Male Gestures");
		mInventoryItemsDict["Female Gestures"]	= LLTrans::getString("Female Gestures");
		mInventoryItemsDict["Other Gestures"]	= LLTrans::getString("Other Gestures");
		mInventoryItemsDict["Speech Gestures"]	= LLTrans::getString("Speech Gestures");
		mInventoryItemsDict["Common Gestures"]	= LLTrans::getString("Common Gestures");

		//predefined gestures

		//male
		mInventoryItemsDict["Male - Excuse me"]			= LLTrans::getString("Male - Excuse me");
		mInventoryItemsDict["Male  - Get lost"]			= LLTrans::getString("Male - Get lost"); // double space after Male. EXT-8319
		mInventoryItemsDict["Male - Blow kiss"]			= LLTrans::getString("Male - Blow kiss");
		mInventoryItemsDict["Male - Boo"]				= LLTrans::getString("Male - Boo");
		mInventoryItemsDict["Male - Bored"]				= LLTrans::getString("Male - Bored");
		mInventoryItemsDict["Male - Hey"]				= LLTrans::getString("Male - Hey");
		mInventoryItemsDict["Male - Laugh"]				= LLTrans::getString("Male - Laugh");
		mInventoryItemsDict["Male - Repulsed"]			= LLTrans::getString("Male - Repulsed");
		mInventoryItemsDict["Male - Shrug"]				= LLTrans::getString("Male - Shrug");
		mInventoryItemsDict["Male - Stick tougue out"]	= LLTrans::getString("Male - Stick tougue out");
		mInventoryItemsDict["Male - Wow"]				= LLTrans::getString("Male - Wow");

		//female
		mInventoryItemsDict["Female - Chuckle"]			= LLTrans::getString("Female - Chuckle");
		mInventoryItemsDict["Female - Cry"]				= LLTrans::getString("Female - Cry");
		mInventoryItemsDict["Female - Embarrassed"]		= LLTrans::getString("Female - Embarrassed");
		mInventoryItemsDict["Female - Excuse me"]		= LLTrans::getString("Female - Excuse me");
		mInventoryItemsDict["Female  - Get lost"]		= LLTrans::getString("Female - Get lost"); // double space after Female. EXT-8319
		mInventoryItemsDict["Female - Blow kiss"]		= LLTrans::getString("Female - Blow kiss");
		mInventoryItemsDict["Female - Boo"]				= LLTrans::getString("Female - Boo");
		mInventoryItemsDict["Female - Bored"]			= LLTrans::getString("Female - Bored");
		mInventoryItemsDict["Female - Hey"]				= LLTrans::getString("Female - Hey");
		mInventoryItemsDict["Female - Hey baby"]		= LLTrans::getString("Female - Hey baby");
		mInventoryItemsDict["Female - Laugh"]			= LLTrans::getString("Female - Laugh");
		mInventoryItemsDict["Female - Looking good"]	= LLTrans::getString("Female - Looking good");
		mInventoryItemsDict["Female - Over here"]		= LLTrans::getString("Female - Over here");
		mInventoryItemsDict["Female - Please"]			= LLTrans::getString("Female - Please");
		mInventoryItemsDict["Female - Repulsed"]		= LLTrans::getString("Female - Repulsed");
		mInventoryItemsDict["Female - Shrug"]			= LLTrans::getString("Female - Shrug");
		mInventoryItemsDict["Female - Stick tougue out"]= LLTrans::getString("Female - Stick tougue out");
		mInventoryItemsDict["Female - Wow"]				= LLTrans::getString("Female - Wow");
		
	}

	/**
	 * Finds passed name in dictionary and replaces it with found localized value.
	 *
	 * @param object_name - string to be localized.
	 * @return true if passed name was found and localized, false otherwise.
	 */
	bool localizeInventoryObjectName(std::string& object_name)
	{
		LL_DEBUGS("InventoryLocalize") << "Searching for localization: " << object_name << LL_ENDL;

		std::map<std::string, std::string>::const_iterator dictionary_iter = mInventoryItemsDict.find(object_name);

		bool found = dictionary_iter != mInventoryItemsDict.end();
		if(found)
		{
			object_name = dictionary_iter->second;
			LL_DEBUGS("InventoryLocalize") << "Found, new name is: " << object_name << LL_ENDL;
		}
		return found;
	}
};


///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

class LLInventoryHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLInventoryHandler() : LLCommandHandler("inventory", UNTRUSTED_THROTTLE) { }
	
	bool handle(const LLSD& params, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (params.size() < 1)
		{
			return false;
		}

		// support secondlife:///app/inventory/show
		if (params[0].asString() == "show")
		{
			LLSideTray::getInstance()->showPanel("sidepanel_inventory", LLSD());
			return true;
		}

		// otherwise, we need a UUID and a verb...
		if (params.size() < 2) 
		{
			return false;
		}
		LLUUID inventory_id;
		if (!inventory_id.set(params[0], FALSE))
		{
			return false;
		}
		
		const std::string verb = params[1].asString();
		if (verb == "select")
		{
			uuid_vec_t items_to_open;
			items_to_open.push_back(inventory_id);
			//inventory_handler is just a stub, because we don't know from who this offer
			open_inventory_offer(items_to_open, "inventory_handler");
			return true;
		}
		
		return false;
	}
};
LLInventoryHandler gInventoryHandler;


///----------------------------------------------------------------------------
/// Class LLViewerInventoryItem
///----------------------------------------------------------------------------

LLViewerInventoryItem::LLViewerInventoryItem(const LLUUID& uuid,
											 const LLUUID& parent_uuid,
											 const LLPermissions& perm,
											 const LLUUID& asset_uuid,
											 LLAssetType::EType type,
											 LLInventoryType::EType inv_type,
											 const std::string& name,
											 const std::string& desc,
											 const LLSaleInfo& sale_info,
											 U32 flags,
											 time_t creation_date_utc) :
	LLInventoryItem(uuid, parent_uuid, perm, asset_uuid, type, inv_type,
					name, desc, sale_info, flags, creation_date_utc),
	mIsComplete(TRUE)
{
}

LLViewerInventoryItem::LLViewerInventoryItem(const LLUUID& item_id,
											 const LLUUID& parent_id,
											 const std::string& name,
											 LLInventoryType::EType inv_type) :
	LLInventoryItem(),
	mIsComplete(FALSE)
{
	mUUID = item_id;
	mParentUUID = parent_id;
	mInventoryType = inv_type;
	mName = name;
}

LLViewerInventoryItem::LLViewerInventoryItem() :
	LLInventoryItem(),
	mIsComplete(FALSE)
{
}

LLViewerInventoryItem::LLViewerInventoryItem(const LLViewerInventoryItem* other) :
	LLInventoryItem()
{
	copyViewerItem(other);
	if (!mIsComplete)
	{
		llwarns << "LLViewerInventoryItem copy constructor for incomplete item"
			<< mUUID << llendl;
	}
}

LLViewerInventoryItem::LLViewerInventoryItem(const LLInventoryItem *other) :
	LLInventoryItem(other),
	mIsComplete(TRUE)
{
}


LLViewerInventoryItem::~LLViewerInventoryItem()
{
}

void LLViewerInventoryItem::copyViewerItem(const LLViewerInventoryItem* other)
{
	LLInventoryItem::copyItem(other);
	mIsComplete = other->mIsComplete;
	mTransactionID = other->mTransactionID;
}

// virtual
void LLViewerInventoryItem::copyItem(const LLInventoryItem *other)
{
	LLInventoryItem::copyItem(other);
	mIsComplete = true;
	mTransactionID.setNull();
}

void LLViewerInventoryItem::cloneViewerItem(LLPointer<LLViewerInventoryItem>& newitem) const
{
	newitem = new LLViewerInventoryItem(this);
	if(newitem.notNull())
	{
		LLUUID item_id;
		item_id.generate();
		newitem->setUUID(item_id);
	}
}

void LLViewerInventoryItem::removeFromServer()
{
	llinfos << "Removing inventory item " << mUUID << " from server."
			<< llendl;

	LLInventoryModel::LLCategoryUpdate up(mParentUUID, -1);
	gInventory.accountForUpdate(up);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveInventoryItem);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID()); 
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addUUIDFast(_PREHASH_ItemID, mUUID);
	gAgent.sendReliableMessage();
}

void LLViewerInventoryItem::updateServer(BOOL is_new) const
{
	if(!mIsComplete)
	{
		// *FIX: deal with this better.
		// If we're crashing here then the UI is incorrectly enabled.
		llerrs << "LLViewerInventoryItem::updateServer() - for incomplete item"
			   << llendl;
		return;
	}
	if(gAgent.getID() != mPermissions.getOwner())
	{
		// *FIX: deal with this better.
		llwarns << "LLViewerInventoryItem::updateServer() - for unowned item"
				<< llendl;
		return;
	}
	LLInventoryModel::LLCategoryUpdate up(mParentUUID, is_new ? 1 : 0);
	gInventory.accountForUpdate(up);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateInventoryItem);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, mTransactionID);
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addU32Fast(_PREHASH_CallbackID, 0);
	packMessage(msg);
	gAgent.sendReliableMessage();
}

void LLViewerInventoryItem::fetchFromServer(void) const
{
	if(!mIsComplete)
	{
		std::string url; 

		LLViewerRegion* region = gAgent.getRegion();
		// we have to check region. It can be null after region was destroyed. See EXT-245
		if (region)
		{
		  if(gAgent.getID() != mPermissions.getOwner())
		    {
		      url = region->getCapability("FetchLib");
		    }
		  else
		    {	
		      url = region->getCapability("FetchInventory");
		    }
		}
		else
		{
			llwarns << "Agent Region is absent" << llendl;
		}

		if (!url.empty())
		{
			LLSD body;
			body["agent_id"]	= gAgent.getID();
			body["items"][0]["owner_id"]	= mPermissions.getOwner();
			body["items"][0]["item_id"]		= mUUID;

			LLHTTPClient::post(url, body, new LLInventoryModel::fetchInventoryResponder(body));
		}
		else
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessage("FetchInventory");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("InventoryData");
			msg->addUUID("OwnerID", mPermissions.getOwner());
			msg->addUUID("ItemID", mUUID);
			gAgent.sendReliableMessage();
		}
	}
	else
	{
		// *FIX: this can be removed after a bit.
		llwarns << "request to fetch complete item" << llendl;
	}
}

// virtual
BOOL LLViewerInventoryItem::unpackMessage(LLSD item)
{
	BOOL rv = LLInventoryItem::fromLLSD(item);
	mIsComplete = TRUE;
	return rv;
}

// virtual
BOOL LLViewerInventoryItem::unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num)
{
	BOOL rv = LLInventoryItem::unpackMessage(msg, block, block_num);

	LLLocalizedInventoryItemsDictionary::getInstance()->localizeInventoryObjectName(mName);

	mIsComplete = TRUE;
	return rv;
}

void LLViewerInventoryItem::setTransactionID(const LLTransactionID& transaction_id)
{
	mTransactionID = transaction_id;
}
// virtual
void LLViewerInventoryItem::packMessage(LLMessageSystem* msg) const
{
	msg->addUUIDFast(_PREHASH_ItemID, mUUID);
	msg->addUUIDFast(_PREHASH_FolderID, mParentUUID);
	mPermissions.packMessage(msg);
	msg->addUUIDFast(_PREHASH_TransactionID, mTransactionID);
	S8 type = static_cast<S8>(mType);
	msg->addS8Fast(_PREHASH_Type, type);
	type = static_cast<S8>(mInventoryType);
	msg->addS8Fast(_PREHASH_InvType, type);
	msg->addU32Fast(_PREHASH_Flags, mFlags);
	mSaleInfo.packMessage(msg);
	msg->addStringFast(_PREHASH_Name, mName);
	msg->addStringFast(_PREHASH_Description, mDescription);
	msg->addS32Fast(_PREHASH_CreationDate, mCreationDate);
	U32 crc = getCRC32();
	msg->addU32Fast(_PREHASH_CRC, crc);
}
// virtual
BOOL LLViewerInventoryItem::importFile(LLFILE* fp)
{
	BOOL rv = LLInventoryItem::importFile(fp);
	mIsComplete = TRUE;
	return rv;
}

// virtual
BOOL LLViewerInventoryItem::importLegacyStream(std::istream& input_stream)
{
	BOOL rv = LLInventoryItem::importLegacyStream(input_stream);
	mIsComplete = TRUE;
	return rv;
}

bool LLViewerInventoryItem::importFileLocal(LLFILE* fp)
{
	// TODO: convert all functions that return BOOL to return bool
	bool rv = (LLInventoryItem::importFile(fp) ? true : false);
	mIsComplete = false;
	return rv;
}

bool LLViewerInventoryItem::exportFileLocal(LLFILE* fp) const
{
	std::string uuid_str;
	fprintf(fp, "\tinv_item\t0\n\t{\n");
	mUUID.toString(uuid_str);
	fprintf(fp, "\t\titem_id\t%s\n", uuid_str.c_str());
	mParentUUID.toString(uuid_str);
	fprintf(fp, "\t\tparent_id\t%s\n", uuid_str.c_str());
	mPermissions.exportFile(fp);
	fprintf(fp, "\t\ttype\t%s\n", LLAssetType::lookup(mType));
	const std::string inv_type_str = LLInventoryType::lookup(mInventoryType);
	if(!inv_type_str.empty()) fprintf(fp, "\t\tinv_type\t%s\n", inv_type_str.c_str());
	fprintf(fp, "\t\tname\t%s|\n", mName.c_str());
	fprintf(fp, "\t\tcreation_date\t%d\n", (S32) mCreationDate);
	fprintf(fp,"\t}\n");
	return true;
}

void LLViewerInventoryItem::updateParentOnServer(BOOL restamp) const
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoveInventoryItem);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addBOOLFast(_PREHASH_Stamp, restamp);
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addUUIDFast(_PREHASH_ItemID, mUUID);
	msg->addUUIDFast(_PREHASH_FolderID, mParentUUID);
	msg->addString("NewName", NULL);
	gAgent.sendReliableMessage();
}

//void LLViewerInventoryItem::setCloneCount(S32 clones)
//{
//	mClones = clones;
//}

//S32 LLViewerInventoryItem::getCloneCount() const
//{
//	return mClones;
//}

///----------------------------------------------------------------------------
/// Class LLViewerInventoryCategory
///----------------------------------------------------------------------------

LLViewerInventoryCategory::LLViewerInventoryCategory(const LLUUID& uuid,
													 const LLUUID& parent_uuid,
													 LLFolderType::EType pref,
													 const std::string& name,
													 const LLUUID& owner_id) :
	LLInventoryCategory(uuid, parent_uuid, pref, name),
	mOwnerID(owner_id),
	mVersion(LLViewerInventoryCategory::VERSION_UNKNOWN),
	mDescendentCount(LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN)
{
	mDescendentsRequested.reset();
}

LLViewerInventoryCategory::LLViewerInventoryCategory(const LLUUID& owner_id) :
	mOwnerID(owner_id),
	mVersion(LLViewerInventoryCategory::VERSION_UNKNOWN),
	mDescendentCount(LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN)
{
	mDescendentsRequested.reset();
}

LLViewerInventoryCategory::LLViewerInventoryCategory(const LLViewerInventoryCategory* other)
{
	copyViewerCategory(other);
}

LLViewerInventoryCategory::~LLViewerInventoryCategory()
{
}

void LLViewerInventoryCategory::copyViewerCategory(const LLViewerInventoryCategory* other)
{
	copyCategory(other);
	mOwnerID = other->mOwnerID;
	mVersion = other->mVersion;
	mDescendentCount = other->mDescendentCount;
	mDescendentsRequested = other->mDescendentsRequested;
}


void LLViewerInventoryCategory::updateParentOnServer(BOOL restamp) const
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoveInventoryFolder);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	msg->addBOOL("Stamp", restamp);
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addUUIDFast(_PREHASH_FolderID, mUUID);
	msg->addUUIDFast(_PREHASH_ParentID, mParentUUID);
	gAgent.sendReliableMessage();
}

void LLViewerInventoryCategory::updateServer(BOOL is_new) const
{
	// communicate that change with the server.

	if (LLFolderType::lookupIsProtectedType(mPreferredType))
	{
		LLNotificationsUtil::add("CannotModifyProtectedCategories");
		return;
	}

	LLInventoryModel::LLCategoryUpdate up(mParentUUID, is_new ? 1 : 0);
	gInventory.accountForUpdate(up);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateInventoryFolder);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_FolderData);
	packMessage(msg);
	gAgent.sendReliableMessage();
}

void LLViewerInventoryCategory::removeFromServer( void )
{
	llinfos << "Removing inventory category " << mUUID << " from server."
			<< llendl;
	// communicate that change with the server.
	if(LLFolderType::lookupIsProtectedType(mPreferredType))
	{
		LLNotificationsUtil::add("CannotRemoveProtectedCategories");
		return;
	}

	LLInventoryModel::LLCategoryUpdate up(mParentUUID, -1);
	gInventory.accountForUpdate(up);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveInventoryFolder);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_FolderData);
	msg->addUUIDFast(_PREHASH_FolderID, mUUID);
	gAgent.sendReliableMessage();
}

bool LLViewerInventoryCategory::fetch()
{
	if((VERSION_UNKNOWN == mVersion)
	   && mDescendentsRequested.hasExpired())	//Expired check prevents multiple downloads.
	{
		LL_DEBUGS("InventoryFetch") << "Fetching category children: " << mName << ", UUID: " << mUUID << LL_ENDL;
		const F32 FETCH_TIMER_EXPIRY = 10.0f;
		mDescendentsRequested.reset();
		mDescendentsRequested.setTimerExpirySec(FETCH_TIMER_EXPIRY);

		// bitfield
		// 1 = by date
		// 2 = folders by date
		// Need to mask off anything but the first bit.
		// This comes from LLInventoryFilter from llfolderview.h
		U32 sort_order = gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER) & 0x1;

		// *NOTE: For bug EXT-2879, originally commented out
		// gAgent.getRegion()->getCapability in order to use the old
		// message-based system.  This has been uncommented now that
		// AIS folks are aware of the issue and have a fix in process.
		// see ticket for details.

		std::string url;
		if (gAgent.getRegion())
		{
			url = gAgent.getRegion()->getCapability("WebFetchInventoryDescendents");
		}
		else
		{
			llwarns << "agent region is null" << llendl;
		}
		if (!url.empty()) //Capability found.  Build up LLSD and use it.
		{
			LLInventoryModelBackgroundFetch::instance().start(mUUID, false);			
		}
		else
		{	//Deprecated, but if we don't have a capability, use the old system.
			llinfos << "WebFetchInventoryDescendents capability not found.  Using deprecated UDP message." << llendl;
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessage("FetchInventoryDescendents");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("InventoryData");
			msg->addUUID("FolderID", mUUID);
			msg->addUUID("OwnerID", mOwnerID);

			msg->addS32("SortOrder", sort_order);
			msg->addBOOL("FetchFolders", FALSE);
			msg->addBOOL("FetchItems", TRUE);
			gAgent.sendReliableMessage();
		}
		return true;
	}
	return false;
}

bool LLViewerInventoryCategory::importFileLocal(LLFILE* fp)
{
	// *NOTE: This buffer size is hard coded into scanf() below.
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	char keyword[MAX_STRING];		/* Flawfinder: ignore */
	char valuestr[MAX_STRING];		/* Flawfinder: ignore */

	keyword[0] = '\0';
	valuestr[0] = '\0';
	while(!feof(fp))
	{
		if (fgets(buffer, MAX_STRING, fp) == NULL)
		{
			buffer[0] = '\0';
		}
		
		sscanf(	/* Flawfinder: ignore */
			buffer, " %254s %254s", keyword, valuestr); 
		if(0 == strcmp("{",keyword))
		{
			continue;
		}
		if(0 == strcmp("}", keyword))
		{
			break;
		}
		else if(0 == strcmp("cat_id", keyword))
		{
			mUUID.set(valuestr);
		}
		else if(0 == strcmp("parent_id", keyword))
		{
			mParentUUID.set(valuestr);
		}
		else if(0 == strcmp("type", keyword))
		{
			mType = LLAssetType::lookup(valuestr);
		}
		else if(0 == strcmp("pref_type", keyword))
		{
			mPreferredType = LLFolderType::lookup(valuestr);
		}
		else if(0 == strcmp("name", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(	/* Flawfinder: ignore */
				buffer, " %254s %254[^|]", keyword, valuestr);
			mName.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
		else if(0 == strcmp("owner_id", keyword))
		{
			mOwnerID.set(valuestr);
		}
		else if(0 == strcmp("version", keyword))
		{
			sscanf(valuestr, "%d", &mVersion);
		}
		else
		{
			llwarns << "unknown keyword '" << keyword
					<< "' in inventory import category "  << mUUID << llendl;
		}
	}
	return true;
}

bool LLViewerInventoryCategory::exportFileLocal(LLFILE* fp) const
{
	std::string uuid_str;
	fprintf(fp, "\tinv_category\t0\n\t{\n");
	mUUID.toString(uuid_str);
	fprintf(fp, "\t\tcat_id\t%s\n", uuid_str.c_str());
	mParentUUID.toString(uuid_str);
	fprintf(fp, "\t\tparent_id\t%s\n", uuid_str.c_str());
	fprintf(fp, "\t\ttype\t%s\n", LLAssetType::lookup(mType));
	fprintf(fp, "\t\tpref_type\t%s\n", LLFolderType::lookup(mPreferredType).c_str());
	fprintf(fp, "\t\tname\t%s|\n", mName.c_str());
	mOwnerID.toString(uuid_str);
	fprintf(fp, "\t\towner_id\t%s\n", uuid_str.c_str());
	fprintf(fp, "\t\tversion\t%d\n", mVersion);
	fprintf(fp,"\t}\n");
	return true;
}

void LLViewerInventoryCategory::determineFolderType()
{
	/* Do NOT uncomment this code.  This is for future 2.1 support of ensembles.
	llassert(FALSE);
	LLFolderType::EType original_type = getPreferredType();
	if (LLFolderType::lookupIsProtectedType(original_type))
		return;

	U64 folder_valid = 0;
	U64 folder_invalid = 0;
	LLInventoryModel::cat_array_t category_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(getUUID(),category_array,item_array,FALSE);

	// For ensembles
	if (category_array.empty())
	{
		for (LLInventoryModel::item_array_t::iterator item_iter = item_array.begin();
			 item_iter != item_array.end();
			 item_iter++)
		{
			const LLViewerInventoryItem *item = (*item_iter);
			if (item->getIsLinkType())
				return;
			if (item->isWearableType())
			{
				const LLWearableType::EType wearable_type = item->getWearableType();
				const std::string& wearable_name = LLWearableType::getTypeName(wearable_type);
				U64 valid_folder_types = LLViewerFolderType::lookupValidFolderTypes(wearable_name);
				folder_valid |= valid_folder_types;
				folder_invalid |= ~valid_folder_types;
			}
		}
		for (U8 i = LLFolderType::FT_ENSEMBLE_START; i <= LLFolderType::FT_ENSEMBLE_END; i++)
		{
			if ((folder_valid & (1LL << i)) &&
				!(folder_invalid & (1LL << i)))
			{
				changeType((LLFolderType::EType)i);
				return;
			}
		}
	}
	if (LLFolderType::lookupIsEnsembleType(original_type))
	{
		changeType(LLFolderType::FT_NONE);
	}
	llassert(FALSE);
	*/
}

void LLViewerInventoryCategory::changeType(LLFolderType::EType new_folder_type)
{
	const LLUUID &folder_id = getUUID();
	const LLUUID &parent_id = getParentUUID();
	const std::string &name = getName();
		
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateInventoryFolder);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_FolderData);
	msg->addUUIDFast(_PREHASH_FolderID, folder_id);
	msg->addUUIDFast(_PREHASH_ParentID, parent_id);
	msg->addS8Fast(_PREHASH_Type, new_folder_type);
	msg->addStringFast(_PREHASH_Name, name);
	gAgent.sendReliableMessage();

	setPreferredType(new_folder_type);
	gInventory.addChangedMask(LLInventoryObserver::LABEL, folder_id);
}

void LLViewerInventoryCategory::localizeName()
{
	LLLocalizedInventoryItemsDictionary::getInstance()->localizeInventoryObjectName(mName);
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------

LLInventoryCallbackManager *LLInventoryCallbackManager::sInstance = NULL;

LLInventoryCallbackManager::LLInventoryCallbackManager() :
	mLastCallback(0)
{
	if( sInstance != NULL )
	{
		llwarns << "LLInventoryCallbackManager::LLInventoryCallbackManager: unexpected multiple instances" << llendl;
		return;
	}
	sInstance = this;
}

LLInventoryCallbackManager::~LLInventoryCallbackManager()
{
	if( sInstance != this )
	{
		llwarns << "LLInventoryCallbackManager::~LLInventoryCallbackManager: unexpected multiple instances" << llendl;
		return;
	}
	sInstance = NULL;
}

//static 
void LLInventoryCallbackManager::destroyClass()
{
	if (sInstance)
	{
		for (callback_map_t::iterator it = sInstance->mMap.begin(), end_it = sInstance->mMap.end(); it != end_it; ++it)
		{
			// drop LLPointer reference to callback
			it->second = NULL;
		}
		sInstance->mMap.clear();
	}
}


U32 LLInventoryCallbackManager::registerCB(LLPointer<LLInventoryCallback> cb)
{
	if (cb.isNull())
		return 0;

	mLastCallback++;
	if (!mLastCallback)
		mLastCallback++;

	mMap[mLastCallback] = cb;
	return mLastCallback;
}

void LLInventoryCallbackManager::fire(U32 callback_id, const LLUUID& item_id)
{
	if (!callback_id || item_id.isNull())
		return;

	std::map<U32, LLPointer<LLInventoryCallback> >::iterator i;

	i = mMap.find(callback_id);
	if (i != mMap.end())
	{
		(*i).second->fire(item_id);
		mMap.erase(i);
	}
}

void WearOnAvatarCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;

	LLViewerInventoryItem *item = gInventory.getItem(inv_item);
	if (item)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(inv_item, true, mReplace);
	}
}

void ModifiedCOFCallback::fire(const LLUUID& inv_item)
{
	LLAppearanceMgr::instance().updateAppearanceFromCOF();

	// Start editing the item if previously requested.
	gAgentWearables.editWearableIfRequested(inv_item);

	// TODO: camera mode may not be changed if a debug setting is tweaked
	if( gAgentCamera.cameraCustomizeAvatar() )
	{
		// If we're in appearance editing mode, the current tab may need to be refreshed
		LLSidepanelAppearance *panel = dynamic_cast<LLSidepanelAppearance*>(LLSideTray::getInstance()->getPanel("sidepanel_appearance"));
		if (panel)
		{
			panel->showDefaultSubpart();
		}
	}
}

RezAttachmentCallback::RezAttachmentCallback(LLViewerJointAttachment *attachmentp)
{
	mAttach = attachmentp;
}
RezAttachmentCallback::~RezAttachmentCallback()
{
}

void RezAttachmentCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;

	LLViewerInventoryItem *item = gInventory.getItem(inv_item);
	if (item)
	{
		rez_attachment(item, mAttach);
	}
}

void ActivateGestureCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;
	LLViewerInventoryItem* item = gInventory.getItem(inv_item);
	if (!item)
		return;
	if (item->getType() != LLAssetType::AT_GESTURE)
		return;

	LLGestureMgr::instance().activateGesture(inv_item);
}

void CreateGestureCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;

	LLGestureMgr::instance().activateGesture(inv_item);
	
	LLViewerInventoryItem* item = gInventory.getItem(inv_item);
	if (!item) return;
    gInventory.updateItem(item);
    gInventory.notifyObservers();

	LLPreviewGesture* preview = LLPreviewGesture::show(inv_item,  LLUUID::null);
	// Force to be entirely onscreen.
	gFloaterView->adjustToFitScreen(preview, FALSE);
}

void AddFavoriteLandmarkCallback::fire(const LLUUID& inv_item_id)
{
	if (mTargetLandmarkId.isNull()) return;

	gInventory.rearrangeFavoriteLandmarks(inv_item_id, mTargetLandmarkId);
}

LLInventoryCallbackManager gInventoryCallbacks;

void create_inventory_item(const LLUUID& agent_id, const LLUUID& session_id,
						   const LLUUID& parent, const LLTransactionID& transaction_id,
						   const std::string& name,
						   const std::string& desc, LLAssetType::EType asset_type,
						   LLInventoryType::EType inv_type, LLWearableType::EType wtype,
						   U32 next_owner_perm,
						   LLPointer<LLInventoryCallback> cb)
{
	//check if name is equal to one of special inventory items names
	//EXT-5839
	std::string server_name = name;

	{
		std::map<std::string, std::string>::const_iterator dictionary_iter;

		for (dictionary_iter = LLLocalizedInventoryItemsDictionary::getInstance()->mInventoryItemsDict.begin();
			 dictionary_iter != LLLocalizedInventoryItemsDictionary::getInstance()->mInventoryItemsDict.end();
			 dictionary_iter++)
		{
			const std::string& localized_name = dictionary_iter->second;
			if(localized_name == name)
			{
				server_name = dictionary_iter->first;
			}
		}
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_CreateInventoryItem);
	msg->nextBlock(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, agent_id);
	msg->addUUIDFast(_PREHASH_SessionID, session_id);
	msg->nextBlock(_PREHASH_InventoryBlock);
	msg->addU32Fast(_PREHASH_CallbackID, gInventoryCallbacks.registerCB(cb));
	msg->addUUIDFast(_PREHASH_FolderID, parent);
	msg->addUUIDFast(_PREHASH_TransactionID, transaction_id);
	msg->addU32Fast(_PREHASH_NextOwnerMask, next_owner_perm);
	msg->addS8Fast(_PREHASH_Type, (S8)asset_type);
	msg->addS8Fast(_PREHASH_InvType, (S8)inv_type);
	msg->addU8Fast(_PREHASH_WearableType, (U8)wtype);
	msg->addStringFast(_PREHASH_Name, server_name);
	msg->addStringFast(_PREHASH_Description, desc);
	
	gAgent.sendReliableMessage();
}

void create_inventory_callingcard(const LLUUID& avatar_id, const LLUUID& parent /*= LLUUID::null*/, LLPointer<LLInventoryCallback> cb/*=NULL*/)
{
	std::string item_desc = avatar_id.asString();
	std::string item_name;
	gCacheName->getFullName(avatar_id, item_name);
	create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
						  parent, LLTransactionID::tnull, item_name, item_desc, LLAssetType::AT_CALLINGCARD,
						  LLInventoryType::IT_CALLINGCARD, NOT_WEARABLE, PERM_MOVE | PERM_TRANSFER, cb);
}

void copy_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& current_owner,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	LLPointer<LLInventoryCallback> cb)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_CopyInventoryItem);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, agent_id);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addU32Fast(_PREHASH_CallbackID, gInventoryCallbacks.registerCB(cb));
	msg->addUUIDFast(_PREHASH_OldAgentID, current_owner);
	msg->addUUIDFast(_PREHASH_OldItemID, item_id);
	msg->addUUIDFast(_PREHASH_NewFolderID, parent_id);
	msg->addStringFast(_PREHASH_NewName, new_name);
	gAgent.sendReliableMessage();
}

void link_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	const std::string& new_description,
	const LLAssetType::EType asset_type,
	LLPointer<LLInventoryCallback> cb)
{
	const LLInventoryObject *baseobj = gInventory.getObject(item_id);
	if (!baseobj)
	{
		llwarns << "attempt to link to unknown item, linked-to-item's itemID " << item_id << llendl;
		return;
	}
	if (baseobj && baseobj->getIsLinkType())
	{
		llwarns << "attempt to create a link to a link, linked-to-item's itemID " << item_id << llendl;
		return;
	}

	if (baseobj && !LLAssetType::lookupCanLink(baseobj->getType()))
	{
		// Fail if item can be found but is of a type that can't be linked.
		// Arguably should fail if the item can't be found too, but that could
		// be a larger behavioral change.
		llwarns << "attempt to link an unlinkable item, type = " << baseobj->getActualType() << llendl;
		return;
	}
	
	LLUUID transaction_id;
	LLInventoryType::EType inv_type = LLInventoryType::IT_NONE;
	if (dynamic_cast<const LLInventoryCategory *>(baseobj))
	{
		inv_type = LLInventoryType::IT_CATEGORY;
	}
	else
	{
		const LLViewerInventoryItem *baseitem = dynamic_cast<const LLViewerInventoryItem *>(baseobj);
		if (baseitem)
		{
			inv_type = baseitem->getInventoryType();
		}
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_LinkInventoryItem);
	msg->nextBlock(_PREHASH_AgentData);
	{
		msg->addUUIDFast(_PREHASH_AgentID, agent_id);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	}
	msg->nextBlock(_PREHASH_InventoryBlock);
	{
		msg->addU32Fast(_PREHASH_CallbackID, gInventoryCallbacks.registerCB(cb));
		msg->addUUIDFast(_PREHASH_FolderID, parent_id);
		msg->addUUIDFast(_PREHASH_TransactionID, transaction_id);
		msg->addUUIDFast(_PREHASH_OldItemID, item_id);
		msg->addS8Fast(_PREHASH_Type, (S8)asset_type);
		msg->addS8Fast(_PREHASH_InvType, (S8)inv_type);
		msg->addStringFast(_PREHASH_Name, new_name);
		msg->addStringFast(_PREHASH_Description, new_description);
	}
	gAgent.sendReliableMessage();
}

void move_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& session_id,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	LLPointer<LLInventoryCallback> cb)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoveInventoryItem);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, agent_id);
	msg->addUUIDFast(_PREHASH_SessionID, session_id);
	msg->addBOOLFast(_PREHASH_Stamp, FALSE);
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addUUIDFast(_PREHASH_ItemID, item_id);
	msg->addUUIDFast(_PREHASH_FolderID, parent_id);
	msg->addStringFast(_PREHASH_NewName, new_name);
	gAgent.sendReliableMessage();
}

void copy_inventory_from_notecard(const LLUUID& object_id, const LLUUID& notecard_inv_id, const LLInventoryItem *src, U32 callback_id)
{
	if (NULL == src)
	{
		LL_WARNS("copy_inventory_from_notecard") << "Null pointer to item was passed for object_id "
												 << object_id << " and notecard_inv_id "
												 << notecard_inv_id << LL_ENDL;
		return;
	}

	LLViewerRegion* viewer_region = NULL;
    LLViewerObject* vo = NULL;
	if (object_id.notNull() && (vo = gObjectList.findObject(object_id)) != NULL)
    {
        viewer_region = vo->getRegion();
	}

	// Fallback to the agents region if for some reason the 
	// object isn't found in the viewer.
	if (! viewer_region)
	{
		viewer_region = gAgent.getRegion();
	}

	if (! viewer_region)
	{
        LL_WARNS("copy_inventory_from_notecard") << "Can't find region from object_id "
                                                 << object_id << " or gAgent"
                                                 << LL_ENDL;
        return;
    }

	// check capability to prevent a crash while LL_ERRS in LLCapabilityListener::capListener. See EXT-8459.
	std::string url = viewer_region->getCapability("CopyInventoryFromNotecard");
	if (url.empty())
	{
        LL_WARNS("copy_inventory_from_notecard") << "There is no 'CopyInventoryFromNotecard' capability"
												 << " for region: " << viewer_region->getName()
                                                 << LL_ENDL;
		return;
	}

    LLSD request, body;
    body["notecard-id"] = notecard_inv_id;
    body["object-id"] = object_id;
    body["item-id"] = src->getUUID();
	body["folder-id"] = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(src->getType()));
    body["callback-id"] = (LLSD::Integer)callback_id;

    request["message"] = "CopyInventoryFromNotecard";
    request["payload"] = body;

    viewer_region->getCapAPI().post(request);
}

void create_new_item(const std::string& name,
				   const LLUUID& parent_id,
				   LLAssetType::EType asset_type,
				   LLInventoryType::EType inv_type,
				   U32 next_owner_perm)
{
	std::string desc;
	LLViewerAssetType::generateDescriptionFor(asset_type, desc);
	next_owner_perm = (next_owner_perm) ? next_owner_perm : PERM_MOVE | PERM_TRANSFER;

	
	if (inv_type == LLInventoryType::IT_GESTURE)
	{
		LLPointer<LLInventoryCallback> cb = new CreateGestureCallback();
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							  parent_id, LLTransactionID::tnull, name, desc, asset_type, inv_type,
							  NOT_WEARABLE, next_owner_perm, cb);
	}
	else
	{
		LLPointer<LLInventoryCallback> cb = NULL;
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							  parent_id, LLTransactionID::tnull, name, desc, asset_type, inv_type,
							  NOT_WEARABLE, next_owner_perm, cb);
	}
	
}	

const std::string NEW_LSL_NAME = "New Script"; // *TODO:Translate? (probably not)
const std::string NEW_NOTECARD_NAME = "New Note"; // *TODO:Translate? (probably not)
const std::string NEW_GESTURE_NAME = "New Gesture"; // *TODO:Translate? (probably not)

// ! REFACTOR ! Really need to refactor this so that it's not a bunch of if-then statements...
void menu_create_inventory_item(LLFolderView* root, LLFolderBridge *bridge, const LLSD& userdata, const LLUUID& default_parent_uuid)
{
	std::string type_name = userdata.asString();
	
	if (("category" == type_name) || ("current" == type_name) || ("outfit" == type_name) || ("my_otfts" == type_name))
	{
		LLFolderType::EType preferred_type = LLFolderType::lookup(type_name);

		LLUUID parent_id;
		if (bridge)
		{
			parent_id = bridge->getUUID();
		}
		else if (default_parent_uuid.notNull())
		{
			parent_id = default_parent_uuid;
		}
		else
		{
			parent_id = gInventory.getRootFolderID();
		}

		LLUUID category = gInventory.createNewCategory(parent_id, preferred_type, LLStringUtil::null);
		gInventory.notifyObservers();
		root->setSelectionByID(category, TRUE);
	}
	else if ("lsl" == type_name)
	{
		const LLUUID parent_id = bridge ? bridge->getUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_LSL_TEXT);
		create_new_item(NEW_LSL_NAME,
					  parent_id,
					  LLAssetType::AT_LSL_TEXT,
					  LLInventoryType::IT_LSL,
					  PERM_MOVE | PERM_TRANSFER);
	}
	else if ("notecard" == type_name)
	{
		const LLUUID parent_id = bridge ? bridge->getUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_NOTECARD);
		create_new_item(NEW_NOTECARD_NAME,
					  parent_id,
					  LLAssetType::AT_NOTECARD,
					  LLInventoryType::IT_NOTECARD,
					  PERM_ALL);
	}
	else if ("gesture" == type_name)
	{
		const LLUUID parent_id = bridge ? bridge->getUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE);
		create_new_item(NEW_GESTURE_NAME,
					  parent_id,
					  LLAssetType::AT_GESTURE,
					  LLInventoryType::IT_GESTURE,
					  PERM_ALL);
	}
	else
	{
		// Use for all clothing and body parts.  Adding new wearable types requires updating LLWearableDictionary.
		LLWearableType::EType wearable_type = LLWearableType::typeNameToType(type_name);
		if (wearable_type >= LLWearableType::WT_SHAPE && wearable_type < LLWearableType::WT_COUNT)
		{
			const LLUUID parent_id = bridge ? bridge->getUUID() : LLUUID::null;
			LLAgentWearables::createWearable(wearable_type, false, parent_id);
		}
		else
		{
			llwarns << "Can't create unrecognized type " << type_name << llendl;
		}
	}
	root->setNeedsAutoRename(TRUE);	
}

LLAssetType::EType LLViewerInventoryItem::getType() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getType();
	}
	if (const LLViewerInventoryCategory *linked_category = getLinkedCategory())
	{
		return linked_category->getType();
	}	
	return LLInventoryItem::getType();
}

const LLUUID& LLViewerInventoryItem::getAssetUUID() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getAssetUUID();
	}

	return LLInventoryItem::getAssetUUID();
}

const LLUUID& LLViewerInventoryItem::getProtectedAssetUUID() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getProtectedAssetUUID();
	}

	// check for conditions under which we may return a visible UUID to the user
	bool item_is_fullperm = getIsFullPerm();
	bool agent_is_godlike = gAgent.isGodlikeWithoutAdminMenuFakery();
	if (item_is_fullperm || agent_is_godlike)
	{
		return LLInventoryItem::getAssetUUID();
	}

	return LLUUID::null;
}

const bool LLViewerInventoryItem::getIsFullPerm() const
{
	LLPermissions item_permissions = getPermissions();

	// modify-ok & copy-ok & transfer-ok
	return ( item_permissions.allowOperationBy(PERM_MODIFY,
						   gAgent.getID(),
						   gAgent.getGroupID()) &&
		 item_permissions.allowOperationBy(PERM_COPY,
						   gAgent.getID(),
						   gAgent.getGroupID()) &&
		 item_permissions.allowOperationBy(PERM_TRANSFER,
						   gAgent.getID(),
						   gAgent.getGroupID()) );
}

const std::string& LLViewerInventoryItem::getName() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getName();
	}
	if (const LLViewerInventoryCategory *linked_category = getLinkedCategory())
	{
		return linked_category->getName();
	}

	return  LLInventoryItem::getName();
}

/**
 * Class to store sorting order of favorites landmarks in a local file. EXT-3985.
 * It replaced previously implemented solution to store sort index in landmark's name as a "<N>@" prefix.
 * Data are stored in user home directory.
 */
class LLFavoritesOrderStorage : public LLSingleton<LLFavoritesOrderStorage>
	, public LLDestroyClass<LLFavoritesOrderStorage>
{
public:
	/**
	 * Sets sort index for specified with LLUUID favorite landmark
	 */
	void setSortIndex(const LLUUID& inv_item_id, S32 sort_index);

	/**
	 * Gets sort index for specified with LLUUID favorite landmark
	 */
	S32 getSortIndex(const LLUUID& inv_item_id);
	void removeSortIndex(const LLUUID& inv_item_id);

	void getSLURL(const LLUUID& asset_id);

	/**
	 * Implementation of LLDestroyClass. Calls cleanup() instance method.
	 *
	 * It is important this callback is called before gInventory is cleaned.
	 * For now it is called from LLAppViewer::cleanup() -> LLAppViewer::disconnectViewer(),
	 * Inventory is cleaned later from LLAppViewer::cleanup() after LLAppViewer::disconnectViewer() is called.
	 * @see cleanup()
	 */
	static void destroyClass();

	const static S32 NO_INDEX;
private:
	friend class LLSingleton<LLFavoritesOrderStorage>;
	LLFavoritesOrderStorage() : mIsDirty(false) { load(); }
	~LLFavoritesOrderStorage() { save(); }

	/**
	 * Removes sort indexes for items which are not in Favorites bar for now.
	 */
	void cleanup();

	const static std::string SORTING_DATA_FILE_NAME;

	void load();
	void save();

	void saveFavoritesSLURLs();

	void onLandmarkLoaded(const LLUUID& asset_id, LLLandmark* landmark);
	void storeFavoriteSLURL(const LLUUID& asset_id, std::string& slurl);

	typedef std::map<LLUUID, S32> sort_index_map_t;
	sort_index_map_t mSortIndexes;

	typedef std::map<LLUUID, std::string> slurls_map_t;
	slurls_map_t mSLURLs;

	bool mIsDirty;

	struct IsNotInFavorites
	{
		IsNotInFavorites(const LLInventoryModel::item_array_t& items)
			: mFavoriteItems(items)
		{

		}

		/**
		 * Returns true if specified item is not found among inventory items
		 */
		bool operator()(const sort_index_map_t::value_type& id_index_pair) const
		{
			LLPointer<LLViewerInventoryItem> item = gInventory.getItem(id_index_pair.first);
			if (item.isNull()) return true;

			LLInventoryModel::item_array_t::const_iterator found_it =
				std::find(mFavoriteItems.begin(), mFavoriteItems.end(), item);

			return found_it == mFavoriteItems.end();
		}
	private:
		LLInventoryModel::item_array_t mFavoriteItems;
	};

};

const std::string LLFavoritesOrderStorage::SORTING_DATA_FILE_NAME = "landmarks_sorting.xml";
const S32 LLFavoritesOrderStorage::NO_INDEX = -1;

void LLFavoritesOrderStorage::setSortIndex(const LLUUID& inv_item_id, S32 sort_index)
{
	mSortIndexes[inv_item_id] = sort_index;
	mIsDirty = true;
}

S32 LLFavoritesOrderStorage::getSortIndex(const LLUUID& inv_item_id)
{
	sort_index_map_t::const_iterator it = mSortIndexes.find(inv_item_id);
	if (it != mSortIndexes.end())
	{
		return it->second;
	}
	return NO_INDEX;
}

void LLFavoritesOrderStorage::removeSortIndex(const LLUUID& inv_item_id)
{
	mSortIndexes.erase(inv_item_id);
	mIsDirty = true;
}

void LLFavoritesOrderStorage::getSLURL(const LLUUID& asset_id)
{
	slurls_map_t::iterator slurl_iter = mSLURLs.find(asset_id);
	if (slurl_iter != mSLURLs.end()) return; // SLURL for current landmark is already cached

	LLLandmark* lm = gLandmarkList.getAsset(asset_id,
			boost::bind(&LLFavoritesOrderStorage::onLandmarkLoaded, this, asset_id, _1));
	if (lm)
	{
		onLandmarkLoaded(asset_id, lm);
	}
}

// static
void LLFavoritesOrderStorage::destroyClass()
{
	LLFavoritesOrderStorage::instance().cleanup();
	if (gSavedPerAccountSettings.getBOOL("ShowFavoritesOnLogin"))
	{
		LLFavoritesOrderStorage::instance().saveFavoritesSLURLs();
	}
}

void LLFavoritesOrderStorage::load()
{
	// load per-resident sorting information
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SORTING_DATA_FILE_NAME);

	LLSD settings_llsd;
	llifstream file;
	file.open(filename);
	if (file.is_open())
	{
		LLSDSerialize::fromXML(settings_llsd, file);
	}

	for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
		iter != settings_llsd.endMap(); ++iter)
	{
		mSortIndexes.insert(std::make_pair(LLUUID(iter->first), (S32)iter->second.asInteger()));
	}
}

void LLFavoritesOrderStorage::saveFavoritesSLURLs()
{
	// Do not change the file if we are not logged in yet.
	if (!LLLoginInstance::getInstance()->authSuccess()) return;
	
	std::string user_dir = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "");
	if (user_dir.empty()) return;

	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	llifstream in_file;
	in_file.open(filename);
	LLSD fav_llsd;
	if (in_file.is_open())
	{
		LLSDSerialize::fromXML(fav_llsd, in_file);
	}

	const LLUUID fav_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(fav_id, cats, items, LLInventoryModel::EXCLUDE_TRASH);

	LLSD user_llsd;
	for (LLInventoryModel::item_array_t::iterator it = items.begin(); it != items.end(); it++)
	{
		LLSD value;
		value["name"] = (*it)->getName();
		value["asset_id"] = (*it)->getAssetUUID();

		slurls_map_t::iterator slurl_iter = mSLURLs.find(value["asset_id"]);
		if (slurl_iter != mSLURLs.end())
		{
			value["slurl"] = slurl_iter->second;
			user_llsd[(*it)->getSortField()] = value;
		}
	}

	LLAvatarName av_name;
	LLAvatarNameCache::get( gAgentID, &av_name );
	fav_llsd[av_name.getLegacyName()] = user_llsd;

	llofstream file;
	file.open(filename);
	LLSDSerialize::toPrettyXML(fav_llsd, file);
}

void LLFavoritesOrderStorage::onLandmarkLoaded(const LLUUID& asset_id, LLLandmark* landmark)
{
	if (!landmark) return;

	LLVector3d pos_global;
	if (!landmark->getGlobalPos(pos_global))
	{
		// If global position was unknown on first getGlobalPos() call
		// it should be set for the subsequent calls.
		landmark->getGlobalPos(pos_global);
	}

	if (!pos_global.isExactlyZero())
	{
		LLLandmarkActions::getSLURLfromPosGlobal(pos_global,
				boost::bind(&LLFavoritesOrderStorage::storeFavoriteSLURL, this, asset_id, _1));
	}
}

void LLFavoritesOrderStorage::storeFavoriteSLURL(const LLUUID& asset_id, std::string& slurl)
{
	mSLURLs[asset_id] = slurl;
}

void LLFavoritesOrderStorage::save()
{
	// nothing to save if clean
	if (!mIsDirty) return;

	// If we quit from the login screen we will not have an SL account
	// name.  Don't try to save, otherwise we'll dump a file in
	// C:\Program Files\SecondLife\ or similar. JC
	std::string user_dir = gDirUtilp->getLindenUserDir();
	if (!user_dir.empty())
	{
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SORTING_DATA_FILE_NAME);
		LLSD settings_llsd;

		for(sort_index_map_t::const_iterator iter = mSortIndexes.begin(); iter != mSortIndexes.end(); ++iter)
		{
			settings_llsd[iter->first.asString()] = iter->second;
		}

		llofstream file;
		file.open(filename);
		LLSDSerialize::toPrettyXML(settings_llsd, file);
	}
}

void LLFavoritesOrderStorage::cleanup()
{
	// nothing to clean
	if (!mIsDirty) return;

	const LLUUID fav_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(fav_id, cats, items, LLInventoryModel::EXCLUDE_TRASH);

	IsNotInFavorites is_not_in_fav(items);

	sort_index_map_t  aTempMap;
	//copy unremoved values from mSortIndexes to aTempMap
	std::remove_copy_if(mSortIndexes.begin(), mSortIndexes.end(), 
		inserter(aTempMap, aTempMap.begin()),
		is_not_in_fav);

	//Swap the contents of mSortIndexes and aTempMap
	mSortIndexes.swap(aTempMap);
}


S32 LLViewerInventoryItem::getSortField() const
{
	return LLFavoritesOrderStorage::instance().getSortIndex(mUUID);
}

void LLViewerInventoryItem::setSortField(S32 sortField)
{
	LLFavoritesOrderStorage::instance().setSortIndex(mUUID, sortField);
	getSLURL();
}

void LLViewerInventoryItem::getSLURL()
{
	LLFavoritesOrderStorage::instance().getSLURL(mAssetUUID);
}

const LLPermissions& LLViewerInventoryItem::getPermissions() const
{
	// Use the actual permissions of the symlink, not its parent.
	return LLInventoryItem::getPermissions();	
}

const LLUUID& LLViewerInventoryItem::getCreatorUUID() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getCreatorUUID();
	}

	return LLInventoryItem::getCreatorUUID();
}

const std::string& LLViewerInventoryItem::getDescription() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getDescription();
	}

	return LLInventoryItem::getDescription();
}

const LLSaleInfo& LLViewerInventoryItem::getSaleInfo() const
{	
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getSaleInfo();
	}

	return LLInventoryItem::getSaleInfo();
}

LLInventoryType::EType LLViewerInventoryItem::getInventoryType() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getInventoryType();
	}

	// Categories don't have types.  If this item is an AT_FOLDER_LINK,
	// treat it as a category.
	if (getLinkedCategory())
	{
		return LLInventoryType::IT_CATEGORY;
	}

	return LLInventoryItem::getInventoryType();
}

U32 LLViewerInventoryItem::getFlags() const
{
	if (const LLViewerInventoryItem *linked_item = getLinkedItem())
	{
		return linked_item->getFlags();
	}
	return LLInventoryItem::getFlags();
}

bool LLViewerInventoryItem::isWearableType() const
{
	return (getInventoryType() == LLInventoryType::IT_WEARABLE);
}

LLWearableType::EType LLViewerInventoryItem::getWearableType() const
{
	if (!isWearableType())
	{
		return LLWearableType::WT_INVALID;
	}
	return LLWearableType::EType(getFlags() & LLInventoryItemFlags::II_FLAGS_WEARABLES_MASK);
}


time_t LLViewerInventoryItem::getCreationDate() const
{
	return LLInventoryItem::getCreationDate();
}

U32 LLViewerInventoryItem::getCRC32() const
{
	return LLInventoryItem::getCRC32();	
}

// *TODO: mantipov: should be removed with LMSortPrefix patch in llinventorymodel.cpp, EXT-3985
static char getSeparator() { return '@'; }
BOOL LLViewerInventoryItem::extractSortFieldAndDisplayName(const std::string& name, S32* sortField, std::string* displayName)
{
	using std::string;
	using std::stringstream;

	const char separator = getSeparator();
	const string::size_type separatorPos = name.find(separator, 0);

	BOOL result = FALSE;

	if (separatorPos < string::npos)
	{
		if (sortField)
		{
			/*
			 * The conversion from string to S32 is made this way instead of old plain
			 * atoi() to ensure portability. If on some other platform S32 will not be
			 * defined to be signed int, this conversion will still work because of
			 * operators overloading, but atoi() may fail.
			 */
			stringstream ss(name.substr(0, separatorPos));
			ss >> *sortField;
		}

		if (displayName)
		{
			*displayName = name.substr(separatorPos + 1, string::npos);
		}

		result = TRUE;
	}

	return result;
}

// This returns true if the item that this item points to 
// doesn't exist in memory (i.e. LLInventoryModel).  The baseitem
// might still be in the database but just not loaded yet.
bool LLViewerInventoryItem::getIsBrokenLink() const
{
	// If the item's type resolves to be a link, that means either:
	// A. It wasn't able to perform indirection, i.e. the baseobj doesn't exist in memory.
	// B. It's pointing to another link, which is illegal.
	return LLAssetType::lookupIsLinkType(getType());
}

LLViewerInventoryItem *LLViewerInventoryItem::getLinkedItem() const
{
	if (mType == LLAssetType::AT_LINK)
	{
		LLViewerInventoryItem *linked_item = gInventory.getItem(mAssetUUID);
		if (linked_item && linked_item->getIsLinkType())
		{
			llwarns << "Warning: Accessing link to link" << llendl;
			return NULL;
		}
		return linked_item;
	}
	return NULL;
}

LLViewerInventoryCategory *LLViewerInventoryItem::getLinkedCategory() const
{
	if (mType == LLAssetType::AT_LINK_FOLDER)
	{
		LLViewerInventoryCategory *linked_category = gInventory.getCategory(mAssetUUID);
		return linked_category;
	}
	return NULL;
}

bool LLViewerInventoryItem::checkPermissionsSet(PermissionMask mask) const
{
	const LLPermissions& perm = getPermissions();
	PermissionMask curr_mask = PERM_NONE;
	if(perm.getOwner() == gAgent.getID())
	{
		curr_mask = perm.getMaskBase();
	}
	else if(gAgent.isInGroup(perm.getGroup()))
	{
		curr_mask = perm.getMaskGroup();
	}
	else
	{
		curr_mask = perm.getMaskEveryone();
	}
	return ((curr_mask & mask) == mask);
}

PermissionMask LLViewerInventoryItem::getPermissionMask() const
{
	const LLPermissions& permissions = getPermissions();

	BOOL copy = permissions.allowCopyBy(gAgent.getID());
	BOOL mod = permissions.allowModifyBy(gAgent.getID());
	BOOL xfer = permissions.allowOperationBy(PERM_TRANSFER, gAgent.getID());
	PermissionMask perm_mask = 0;
	if (copy) perm_mask |= PERM_COPY;
	if (mod)  perm_mask |= PERM_MODIFY;
	if (xfer) perm_mask |= PERM_TRANSFER;
	return perm_mask;
}

//----------

void LLViewerInventoryItem::onCallingCardNameLookup(const LLUUID& id, const std::string& name, bool is_group)
{
	rename(name);
	gInventory.addChangedMask(LLInventoryObserver::LABEL, getUUID());
	gInventory.notifyObservers();
}

class LLRegenerateLinkCollector : public LLInventoryCollectFunctor
{
public:
	LLRegenerateLinkCollector(const LLViewerInventoryItem *target_item) : mTargetItem(target_item) {}
	virtual ~LLRegenerateLinkCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if (item)
		{
			if ((item->getName() == mTargetItem->getName()) &&
				(item->getInventoryType() == mTargetItem->getInventoryType()) &&
				(!item->getIsLinkType()))
			{
				return true;
			}
		}
		return false;
	}
protected:
	const LLViewerInventoryItem* mTargetItem;
};

LLUUID find_possible_item_for_regeneration(const LLViewerInventoryItem *target_item)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;

	LLRegenerateLinkCollector candidate_matches(target_item);
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									candidate_matches);
	for (LLViewerInventoryItem::item_array_t::const_iterator item_iter = items.begin();
		 item_iter != items.end();
		 ++item_iter)
	{
	    const LLViewerInventoryItem *item = (*item_iter);
		if (true) return item->getUUID();
	}
	return LLUUID::null;
}

// This currently dosen't work, because the sim does not allow us 
// to change an item's assetID.
BOOL LLViewerInventoryItem::regenerateLink()
{
	const LLUUID target_item_id = find_possible_item_for_regeneration(this);
	if (target_item_id.isNull())
		return FALSE;
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLAssetIDMatches asset_id_matches(getAssetUUID());
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									asset_id_matches);
	for (LLViewerInventoryItem::item_array_t::iterator item_iter = items.begin();
		 item_iter != items.end();
		 item_iter++)
	{
	    LLViewerInventoryItem *item = (*item_iter);
		item->setAssetUUID(target_item_id);
		item->updateServer(FALSE);
		gInventory.addChangedMask(LLInventoryObserver::REBUILD, item->getUUID());
	}
	gInventory.notifyObservers();
	return TRUE;
}
