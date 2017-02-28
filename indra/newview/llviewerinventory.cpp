/** 
 * @file llviewerinventory.cpp
 * @brief Implementation of the viewer side inventory objects.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#include "llaisapi.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llfloatersidepanelcontainer.h"
#include "llviewerfoldertype.h"
#include "llfloatersidepanelcontainer.h"
#include "llfolderview.h"
#include "llviewercontrol.h"
#include "llconsole.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llgesturemgr.h"

#include "llinventorybridge.h"
#include "llinventorypanel.h"
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
#include "llfavoritesbar.h"
#include "llfloaterperms.h"
#include "llclipboard.h"
#include "llhttpretrypolicy.h"

// do-nothing ops for use in callbacks.
void no_op_inventory_func(const LLUUID&) {} 
void no_op_llsd_func(const LLSD&) {}
void no_op() {}

static const char * const LOG_INV("Inventory");
static const char * const LOG_LOCAL("InventoryLocalize");
static const char * const LOG_NOTECARD("copy_inventory_from_notecard");

#if 1
// *TODO$: LLInventoryCallback should be deprecated to conform to the new boost::bind/coroutine model.
// temp code in transition
void doInventoryCb(LLPointer<LLInventoryCallback> cb, LLUUID id)
{
    if (cb.notNull())
        cb->fire(id);
}
#endif

///----------------------------------------------------------------------------
/// Helper class to store special inventory item names and their localized values.
///----------------------------------------------------------------------------
class LLLocalizedInventoryItemsDictionary : public LLSingleton<LLLocalizedInventoryItemsDictionary>
{
	LLSINGLETON(LLLocalizedInventoryItemsDictionary);
public:
	std::map<std::string, std::string> mInventoryItemsDict;

	/**
	 * Finds passed name in dictionary and replaces it with found localized value.
	 *
	 * @param object_name - string to be localized.
	 * @return true if passed name was found and localized, false otherwise.
	 */
	bool localizeInventoryObjectName(std::string& object_name)
	{
		LL_DEBUGS(LOG_LOCAL) << "Searching for localization: " << object_name << LL_ENDL;

		std::map<std::string, std::string>::const_iterator dictionary_iter = mInventoryItemsDict.find(object_name);

		bool found = dictionary_iter != mInventoryItemsDict.end();
		if(found)
		{
			object_name = dictionary_iter->second;
			LL_DEBUGS(LOG_LOCAL) << "Found, new name is: " << object_name << LL_ENDL;
		}
		return found;
	}
};

LLLocalizedInventoryItemsDictionary::LLLocalizedInventoryItemsDictionary()
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

	//common
	mInventoryItemsDict["/bow"]						= LLTrans::getString("/bow");
	mInventoryItemsDict["/clap"]					= LLTrans::getString("/clap");
	mInventoryItemsDict["/count"]					= LLTrans::getString("/count");
	mInventoryItemsDict["/extinguish"]				= LLTrans::getString("/extinguish");
	mInventoryItemsDict["/kmb"]						= LLTrans::getString("/kmb");
	mInventoryItemsDict["/muscle"]					= LLTrans::getString("/muscle");
	mInventoryItemsDict["/no"]						= LLTrans::getString("/no");
	mInventoryItemsDict["/no!"]						= LLTrans::getString("/no!");
	mInventoryItemsDict["/paper"]					= LLTrans::getString("/paper");
	mInventoryItemsDict["/pointme"]					= LLTrans::getString("/pointme");
	mInventoryItemsDict["/pointyou"]				= LLTrans::getString("/pointyou");
	mInventoryItemsDict["/rock"]					= LLTrans::getString("/rock");
	mInventoryItemsDict["/scissor"]					= LLTrans::getString("/scissor");
	mInventoryItemsDict["/smoke"]					= LLTrans::getString("/smoke");
	mInventoryItemsDict["/stretch"]					= LLTrans::getString("/stretch");
	mInventoryItemsDict["/whistle"]					= LLTrans::getString("/whistle");
	mInventoryItemsDict["/yes"]						= LLTrans::getString("/yes");
	mInventoryItemsDict["/yes!"]					= LLTrans::getString("/yes!");
	mInventoryItemsDict["afk"]						= LLTrans::getString("afk");
	mInventoryItemsDict["dance1"]					= LLTrans::getString("dance1");
	mInventoryItemsDict["dance2"]					= LLTrans::getString("dance2");
	mInventoryItemsDict["dance3"]					= LLTrans::getString("dance3");
	mInventoryItemsDict["dance4"]					= LLTrans::getString("dance4");
	mInventoryItemsDict["dance5"]					= LLTrans::getString("dance5");
	mInventoryItemsDict["dance6"]					= LLTrans::getString("dance6");
	mInventoryItemsDict["dance7"]					= LLTrans::getString("dance7");
	mInventoryItemsDict["dance8"]					= LLTrans::getString("dance8");
}

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

		if (!LLUI::sSettingGroups["config"]->getBOOL("EnableInventory"))
		{
				LLNotificationsUtil::add("NoInventory", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
				return true;
		}

		// support secondlife:///app/inventory/show
		if (params[0].asString() == "show")
		{
			LLFloaterSidePanelContainer::showPanel("inventory", LLSD());
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
		LL_WARNS(LOG_INV) << "LLViewerInventoryItem copy constructor for incomplete item"
						  << mUUID << LL_ENDL;
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

void LLViewerInventoryItem::updateServer(BOOL is_new) const
{
	if(!mIsComplete)
	{
		// *FIX: deal with this better.
		// If we're crashing here then the UI is incorrectly enabled.
		LL_ERRS(LOG_INV) << "LLViewerInventoryItem::updateServer() - for incomplete item"
						 << LL_ENDL;
		return;
	}
	if(gAgent.getID() != mPermissions.getOwner())
	{
		// *FIX: deal with this better.
		LL_WARNS(LOG_INV) << "LLViewerInventoryItem::updateServer() - for unowned item "
						  << ll_pretty_print_sd(this->asLLSD())
						  << LL_ENDL;
		return;
	}
	LLInventoryModel::LLCategoryUpdate up(mParentUUID, is_new ? 1 : 0);
	gInventory.accountForUpdate(up);

    LLSD updates = asLLSD();
    // Replace asset_id and/or shadow_id with transaction_id (hash_id)
    if (updates.has("asset_id"))
    {
        updates.erase("asset_id");
        if(getTransactionID().notNull())
        {
            updates["hash_id"] = getTransactionID();
        }
    }
    if (updates.has("shadow_id"))
    {
        updates.erase("shadow_id");
        if(getTransactionID().notNull())
        {
            updates["hash_id"] = getTransactionID();
        }
    }
    AISAPI::completion_t cr = boost::bind(&doInventoryCb, (LLPointer<LLInventoryCallback>)NULL, _1);
    AISAPI::UpdateItem(getUUID(), updates, cr);
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
		  if (gAgent.getID() != mPermissions.getOwner())
		  {
		      url = region->getCapability("FetchLib2");
		  }
		  else
		  {	
		      url = region->getCapability("FetchInventory2");
		  }
		}
		else
		{
			LL_WARNS(LOG_INV) << "Agent Region is absent" << LL_ENDL;
		}

		if (!url.empty())
		{
			LLSD body;
			body["agent_id"]	= gAgent.getID();
			body["items"][0]["owner_id"]	= mPermissions.getOwner();
			body["items"][0]["item_id"]		= mUUID;

            LLCore::HttpHandler::ptr_t handler(new LLInventoryModel::FetchItemHttpHandler(body));
			gInventory.requestPost(true, url, body, handler, "Inventory Item");
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
}

// virtual
BOOL LLViewerInventoryItem::unpackMessage(const LLSD& item)
{
	BOOL rv = LLInventoryItem::fromLLSD(item);

	LLLocalizedInventoryItemsDictionary::getInstance()->localizeInventoryObjectName(mName);

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
	setVersion(other->getVersion());
	mDescendentCount = other->mDescendentCount;
	mDescendentsRequested = other->mDescendentsRequested;
}


void LLViewerInventoryCategory::packMessage(LLMessageSystem* msg) const
{
	msg->addUUIDFast(_PREHASH_FolderID, mUUID);
	msg->addUUIDFast(_PREHASH_ParentID, mParentUUID);
	S8 type = static_cast<S8>(mPreferredType);
	msg->addS8Fast(_PREHASH_Type, type);
	msg->addStringFast(_PREHASH_Name, mName);
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

    LLSD new_llsd = asLLSD();
    AISAPI::completion_t cr = boost::bind(&doInventoryCb, (LLPointer<LLInventoryCallback>)NULL, _1);
    AISAPI::UpdateCategory(getUUID(), new_llsd, cr);
}

S32 LLViewerInventoryCategory::getVersion() const
{
	return mVersion;
}

void LLViewerInventoryCategory::setVersion(S32 version)
{
	mVersion = version;
}

bool LLViewerInventoryCategory::fetch()
{
	if((VERSION_UNKNOWN == getVersion())
	   && mDescendentsRequested.hasExpired())	//Expired check prevents multiple downloads.
	{
		LL_DEBUGS(LOG_INV) << "Fetching category children: " << mName << ", UUID: " << mUUID << LL_ENDL;
		const F32 FETCH_TIMER_EXPIRY = 10.0f;
		mDescendentsRequested.reset();
		mDescendentsRequested.setTimerExpirySec(FETCH_TIMER_EXPIRY);


		std::string url;
		if (gAgent.getRegion())
		{
			url = gAgent.getRegion()->getCapability("FetchInventoryDescendents2");
		}
		else
		{
			LL_WARNS(LOG_INV) << "agent region is null" << LL_ENDL;
		}
		if (!url.empty()) //Capability found.  Build up LLSD and use it.
		{
			LLInventoryModelBackgroundFetch::instance().start(mUUID, false);			
		}
		return true;
	}
	return false;
}

S32 LLViewerInventoryCategory::getViewerDescendentCount() const
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(getUUID(), cats, items);
	S32 descendents_actual = 0;
	if(cats && items)
	{
		descendents_actual = cats->size() + items->size();
	}
	return descendents_actual;
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
			LL_WARNS(LOG_INV) << "unknown keyword '" << keyword
							  << "' in inventory import category "  << mUUID << LL_ENDL;
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

bool LLViewerInventoryCategory::acceptItem(LLInventoryItem* inv_item)
{
    if (!inv_item)
    {
        return false;
    }

    // Only stock folders have limitation on which item they will accept
    bool accept = true;
    if (getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
    {
        // If the item is copyable (i.e. non stock) do not accept the drop in a stock folder
        if (inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            accept = false;
        }
        else
        {
            LLInventoryModel::cat_array_t* cat_array;
            LLInventoryModel::item_array_t* item_array;
            gInventory.getDirectDescendentsOf(getUUID(),cat_array,item_array);
            // Destination stock folder must be empty OR types of incoming and existing items must be identical and have the same permissions
            accept = (!item_array->size() ||
                      ((item_array->at(0)->getInventoryType() == inv_item->getInventoryType()) &&
                       (item_array->at(0)->getPermissions().getMaskNextOwner() == inv_item->getPermissions().getMaskNextOwner())));
        }
    }
    return accept;
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
		
    LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(folder_id,
                                                                                 parent_id,
                                                                                 new_folder_type,
                                                                                 name,
                                                                                 gAgent.getID());
        
        
    LLSD new_llsd = new_cat->asLLSD();
    AISAPI::completion_t cr = boost::bind(&doInventoryCb, (LLPointer<LLInventoryCallback>) NULL, _1);
    AISAPI::UpdateCategory(folder_id, new_llsd, cr);

	setPreferredType(new_folder_type);
	gInventory.addChangedMask(LLInventoryObserver::LABEL, folder_id);
}

void LLViewerInventoryCategory::localizeName()
{
	LLLocalizedInventoryItemsDictionary::getInstance()->localizeInventoryObjectName(mName);
}

// virtual
BOOL LLViewerInventoryCategory::unpackMessage(const LLSD& category)
{
	BOOL rv = LLInventoryCategory::fromLLSD(category);
	localizeName();
	return rv;
}

// virtual
void LLViewerInventoryCategory::unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num)
{
	LLInventoryCategory::unpackMessage(msg, block, block_num);
	localizeName();
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
		LL_WARNS(LOG_INV) << "LLInventoryCallbackManager::LLInventoryCallbackManager: unexpected multiple instances" << LL_ENDL;
		return;
	}
	sInstance = this;
}

LLInventoryCallbackManager::~LLInventoryCallbackManager()
{
	if( sInstance != this )
	{
		LL_WARNS(LOG_INV) << "LLInventoryCallbackManager::~LLInventoryCallbackManager: unexpected multiple instances" << LL_ENDL;
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

void rez_attachment_cb(const LLUUID& inv_item, LLViewerJointAttachment *attachmentp)
{
	if (inv_item.isNull())
		return;

	LLViewerInventoryItem *item = gInventory.getItem(inv_item);
	if (item)
	{
		rez_attachment(item, attachmentp);
	}
}

void activate_gesture_cb(const LLUUID& inv_item)
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

void set_default_permissions(LLViewerInventoryItem* item, std::string perm_type)
{
	llassert(item);
	LLPermissions perm = item->getPermissions();
	if (perm.getMaskEveryone() != LLFloaterPerms::getEveryonePerms(perm_type)
		|| perm.getMaskGroup() != LLFloaterPerms::getGroupPerms(perm_type))
	{
		perm.setMaskEveryone(LLFloaterPerms::getEveryonePerms(perm_type));
		perm.setMaskGroup(LLFloaterPerms::getGroupPerms(perm_type));

		item->setPermissions(perm);

		item->updateServer(FALSE);
	}
}

void create_script_cb(const LLUUID& inv_item)
{
	if (!inv_item.isNull())
	{
		LLViewerInventoryItem* item = gInventory.getItem(inv_item);
		if (item)
		{
			set_default_permissions(item, "Scripts");

			// item was just created, update even if permissions did not changed
			gInventory.updateItem(item);
			gInventory.notifyObservers();
		}
	}
}

void create_gesture_cb(const LLUUID& inv_item)
{
	if (!inv_item.isNull())
	{
		LLGestureMgr::instance().activateGesture(inv_item);
	
		LLViewerInventoryItem* item = gInventory.getItem(inv_item);
		if (item)
		{
			set_default_permissions(item, "Gestures");

			gInventory.updateItem(item);
			gInventory.notifyObservers();

			LLPreviewGesture* preview = LLPreviewGesture::show(inv_item,  LLUUID::null);
			// Force to be entirely onscreen.
			gFloaterView->adjustToFitScreen(preview, FALSE);
		}
	}
}


void create_notecard_cb(const LLUUID& inv_item)
{
	if (!inv_item.isNull())
		{
		LLViewerInventoryItem* item = gInventory.getItem(inv_item);
		if (item)
		{
			set_default_permissions(item, "Notecards");

			gInventory.updateItem(item);
			gInventory.notifyObservers();
		}
	}
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

// Create link to single inventory object.
void link_inventory_object(const LLUUID& category,
			 LLConstPointer<LLInventoryObject> baseobj,
			 LLPointer<LLInventoryCallback> cb)
{
	if (!baseobj)
	{
		LL_WARNS(LOG_INV) << "Attempt to link to non-existent object" << LL_ENDL;
		return;
	}

	LLInventoryObject::const_object_list_t obj_array;
	obj_array.push_back(baseobj);
	link_inventory_array(category, obj_array, cb);
}

void link_inventory_object(const LLUUID& category,
			 const LLUUID& id,
			 LLPointer<LLInventoryCallback> cb)
{
	LLConstPointer<LLInventoryObject> baseobj = gInventory.getObject(id);
	link_inventory_object(category, baseobj, cb);
}

// Create links to all listed inventory objects.
void link_inventory_array(const LLUUID& category,
			 LLInventoryObject::const_object_list_t& baseobj_array,
			 LLPointer<LLInventoryCallback> cb)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	const LLViewerInventoryCategory *cat = gInventory.getCategory(category);
	const std::string cat_name = cat ? cat->getName() : "CAT NOT FOUND";
#endif
	LLInventoryObject::const_object_list_t::const_iterator it = baseobj_array.begin();
	LLInventoryObject::const_object_list_t::const_iterator end = baseobj_array.end();
	LLSD links = LLSD::emptyArray();
	for (; it != end; ++it)
	{
		const LLInventoryObject* baseobj = *it;
		if (!baseobj)
		{
			LL_WARNS(LOG_INV) << "attempt to link to unknown object" << LL_ENDL;
			continue;
		}

		if (!LLAssetType::lookupCanLink(baseobj->getType()))
		{
			// Fail if item can be found but is of a type that can't be linked.
			// Arguably should fail if the item can't be found too, but that could
			// be a larger behavioral change.
			LL_WARNS(LOG_INV) << "attempt to link an unlinkable object, type = " << baseobj->getActualType() << LL_ENDL;
			continue;
		}
		
		LLInventoryType::EType inv_type = LLInventoryType::IT_NONE;
		LLAssetType::EType asset_type = LLAssetType::AT_NONE;
		std::string new_desc;
		LLUUID linkee_id;
		if (dynamic_cast<const LLInventoryCategory *>(baseobj))
		{
			inv_type = LLInventoryType::IT_CATEGORY;
			asset_type = LLAssetType::AT_LINK_FOLDER;
			linkee_id = baseobj->getUUID();
		}
		else
		{
			const LLViewerInventoryItem *baseitem = dynamic_cast<const LLViewerInventoryItem *>(baseobj);
			if (baseitem)
			{
				inv_type = baseitem->getInventoryType();
				new_desc = baseitem->getActualDescription();
				switch (baseitem->getActualType())
				{
					case LLAssetType::AT_LINK:
					case LLAssetType::AT_LINK_FOLDER:
						linkee_id = baseobj->getLinkedUUID();
						asset_type = baseitem->getActualType();
						break;
					default:
						linkee_id = baseobj->getUUID();
						asset_type = LLAssetType::AT_LINK;
						break;
				}
			}
			else
			{
				LL_WARNS(LOG_INV) << "could not convert object into an item or category: " << baseobj->getUUID() << LL_ENDL;
				continue;
			}
		}

		LLSD link = LLSD::emptyMap();
		link["linked_id"] = linkee_id;
		link["type"] = (S8)asset_type;
		link["inv_type"] = (S8)inv_type;
		link["name"] = baseobj->getName();
		link["desc"] = new_desc;
		links.append(link);

#ifndef LL_RELEASE_FOR_DOWNLOAD
		LL_DEBUGS(LOG_INV) << "Linking Object [ name:" << baseobj->getName() 
						   << " UUID:" << baseobj->getUUID() 
						   << " ] into Category [ name:" << cat_name 
						   << " UUID:" << category << " ] " << LL_ENDL;
#endif
	}
    LLSD new_inventory = LLSD::emptyMap();
    new_inventory["links"] = links;
    AISAPI::completion_t cr = boost::bind(&doInventoryCb, cb, _1);
    AISAPI::CreateInventory(category, new_inventory, cr);
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

// Should call this with an update_item that's been copied and
// modified from an original source item, rather than modifying the
// source item directly.
void update_inventory_item(
	LLViewerInventoryItem *update_item,
	LLPointer<LLInventoryCallback> cb)
{
	const LLUUID& item_id = update_item->getUUID();
  
    LLSD updates = update_item->asLLSD();
    // Replace asset_id and/or shadow_id with transaction_id (hash_id)
    if (updates.has("asset_id"))
    {
        updates.erase("asset_id");
        if (update_item->getTransactionID().notNull())
        {
            updates["hash_id"] = update_item->getTransactionID();
        }
    }
    if (updates.has("shadow_id"))
    {
        updates.erase("shadow_id");
        if (update_item->getTransactionID().notNull())
        {
            updates["hash_id"] = update_item->getTransactionID();
        }
    }
    AISAPI::completion_t cr = boost::bind(&doInventoryCb, cb, _1);
    AISAPI::UpdateItem(item_id, updates, cr);
}

// Note this only supports updating an existing item. Goes through AISv3
// code path where available. Not all uses of item->updateServer() can
// easily be switched to this paradigm.
void update_inventory_item(
	const LLUUID& item_id,
	const LLSD& updates,
	LLPointer<LLInventoryCallback> cb)
{
    AISAPI::completion_t cr = boost::bind(&doInventoryCb, cb, _1);
    AISAPI::UpdateItem(item_id, updates, cr);
}

void update_inventory_category(
	const LLUUID& cat_id,
	const LLSD& updates,
	LLPointer<LLInventoryCallback> cb)
{
	LLPointer<LLViewerInventoryCategory> obj = gInventory.getCategory(cat_id);
	LL_DEBUGS(LOG_INV) << "cat_id: [" << cat_id << "] name " << (obj ? obj->getName() : "(NOT FOUND)") << LL_ENDL;
	if(obj)
	{
		if (LLFolderType::lookupIsProtectedType(obj->getPreferredType()))
		{
			LLNotificationsUtil::add("CannotModifyProtectedCategories");
			return;
		}

		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(obj);
		new_cat->fromLLSD(updates);
        LLSD new_llsd = new_cat->asLLSD();
        AISAPI::completion_t cr = boost::bind(&doInventoryCb, cb, _1);
        AISAPI::UpdateCategory(cat_id, new_llsd, cr);
	}
}

void remove_inventory_items(
	LLInventoryObject::object_list_t& items_to_kill,
	LLPointer<LLInventoryCallback> cb
	)
{
	for (LLInventoryObject::object_list_t::iterator it = items_to_kill.begin();
		 it != items_to_kill.end();
		 ++it)
	{
		remove_inventory_item(*it, cb);
	}
}

void remove_inventory_item(
	const LLUUID& item_id,
	LLPointer<LLInventoryCallback> cb,
	bool immediate_delete)
{
	LLPointer<LLInventoryObject> obj = gInventory.getItem(item_id);
	if (obj)
	{
		remove_inventory_item(obj, cb, immediate_delete);
	}
	else
	{
		LL_DEBUGS(LOG_INV) << "item_id: [" << item_id << "] name " << "(NOT FOUND)" << LL_ENDL;
	}
}

void remove_inventory_item(
	LLPointer<LLInventoryObject> obj,
	LLPointer<LLInventoryCallback> cb,
	bool immediate_delete)
{
	if(obj)
	{
		const LLUUID item_id(obj->getUUID());
		LL_DEBUGS(LOG_INV) << "item_id: [" << item_id << "] name " << obj->getName() << LL_ENDL;
        if (AISAPI::isAvailable())
		{
            AISAPI::completion_t cr = (cb) ? boost::bind(&doInventoryCb, cb, _1) : AISAPI::completion_t();
            AISAPI::RemoveItem(item_id, cr);

			if (immediate_delete)
			{
				gInventory.onObjectDeletedFromServer(item_id);
			}
		}
		else // no cap
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_RemoveInventoryItem);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID()); 
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_ItemID, item_id);
			gAgent.sendReliableMessage();

			// Update inventory and call callback immediately since
			// message-based system has no callback mechanism (!)
			gInventory.onObjectDeletedFromServer(item_id);
			if (cb)
			{
				cb->fire(item_id);
			}
		}
	}
	else
	{
		// *TODO: Clean up callback?
		LL_WARNS(LOG_INV) << "remove_inventory_item called for invalid or nonexistent item." << LL_ENDL;
	}
}

class LLRemoveCategoryOnDestroy: public LLInventoryCallback
{
public:
	LLRemoveCategoryOnDestroy(const LLUUID& cat_id, LLPointer<LLInventoryCallback> cb):
		mID(cat_id),
		mCB(cb)
	{
	}
	/* virtual */ void fire(const LLUUID& item_id) {}
	~LLRemoveCategoryOnDestroy()
	{
		LLInventoryModel::EHasChildren children = gInventory.categoryHasChildren(mID);
		if(children != LLInventoryModel::CHILDREN_NO)
		{
			LL_WARNS(LOG_INV) << "remove descendents failed, cannot remove category " << LL_ENDL;
		}
		else
		{
			remove_inventory_category(mID, mCB);
		}
	}
private:
	LLUUID mID;
	LLPointer<LLInventoryCallback> mCB;
};

void remove_inventory_category(
	const LLUUID& cat_id,
	LLPointer<LLInventoryCallback> cb)
{
	LL_DEBUGS(LOG_INV) << "cat_id: [" << cat_id << "] " << LL_ENDL;
	LLPointer<LLViewerInventoryCategory> obj = gInventory.getCategory(cat_id);
	if(obj)
	{
		if(LLFolderType::lookupIsProtectedType(obj->getPreferredType()))
		{
			LLNotificationsUtil::add("CannotRemoveProtectedCategories");
			return;
		}
        AISAPI::completion_t cr = boost::bind(&doInventoryCb, cb, _1);
        AISAPI::RemoveCategory(cat_id, cr);
	}
	else
	{
		LL_WARNS(LOG_INV) << "remove_inventory_category called for invalid or nonexistent item " << cat_id << LL_ENDL;
	}
}

void remove_inventory_object(
	const LLUUID& object_id,
	LLPointer<LLInventoryCallback> cb)
{
	if (gInventory.getCategory(object_id))
	{
		remove_inventory_category(object_id, cb);
	}
	else
	{
		remove_inventory_item(object_id, cb);
	}
}

// This is a method which collects the descendents of the id
// provided. If the category is not found, no action is
// taken. This method goes through the long winded process of
// cancelling any calling cards, removing server representation of
// folders, items, etc in a fairly efficient manner.
void purge_descendents_of(const LLUUID& id, LLPointer<LLInventoryCallback> cb)
{
	LLInventoryModel::EHasChildren children = gInventory.categoryHasChildren(id);
	if(children == LLInventoryModel::CHILDREN_NO)
	{
		LL_DEBUGS(LOG_INV) << "No descendents to purge for " << id << LL_ENDL;
		return;
	}
	LLPointer<LLViewerInventoryCategory> cat = gInventory.getCategory(id);
	if (cat.notNull())
	{
		if (LLClipboard::instance().hasContents() && LLClipboard::instance().isCutMode())
		{
			// Something on the clipboard is in "cut mode" and needs to be preserved
			LL_DEBUGS(LOG_INV) << "purge_descendents_of clipboard case " << cat->getName()
							   << " iterate and purge non hidden items" << LL_ENDL;
			LLInventoryModel::cat_array_t* categories;
			LLInventoryModel::item_array_t* items;
			// Get the list of direct descendants in tha categoy passed as argument
			gInventory.getDirectDescendentsOf(id, categories, items);
			std::vector<LLUUID> list_uuids;
			// Make a unique list with all the UUIDs of the direct descendants (items and categories are not treated differently)
			// Note: we need to do that shallow copy as purging things will invalidate the categories or items lists
			for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin(); it != categories->end(); ++it)
			{
				list_uuids.push_back((*it)->getUUID());
			}
			for (LLInventoryModel::item_array_t::const_iterator it = items->begin(); it != items->end(); ++it)
			{
				list_uuids.push_back((*it)->getUUID());
			}
			// Iterate through the list and only purge the UUIDs that are not on the clipboard
			for (std::vector<LLUUID>::const_iterator it = list_uuids.begin(); it != list_uuids.end(); ++it)
			{
				if (!LLClipboard::instance().isOnClipboard(*it))
				{
					remove_inventory_object(*it, NULL);
				}
			}
		}
		else
		{
            if (AISAPI::isAvailable())
			{
                AISAPI::completion_t cr = (cb) ? boost::bind(&doInventoryCb, cb, _1) : AISAPI::completion_t();
                AISAPI::PurgeDescendents(id, cr);
			}
			else // no cap
			{
				// Fast purge
				LL_DEBUGS(LOG_INV) << "purge_descendents_of fast case " << cat->getName() << LL_ENDL;

				// send it upstream
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessage("PurgeInventoryDescendents");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID", gAgent.getID());
				msg->addUUID("SessionID", gAgent.getSessionID());
				msg->nextBlock("InventoryData");
				msg->addUUID("FolderID", id);
				gAgent.sendReliableMessage();

				// Update model immediately because there is no callback mechanism.
				gInventory.onDescendentsPurgedFromServer(id);
				if (cb)
				{
					cb->fire(id);
				}
			}
		}
	}
}

const LLUUID get_folder_by_itemtype(const LLInventoryItem *src)
{
	LLUUID retval = LLUUID::null;
	
	if (src)
	{
		retval = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(src->getType()));
	}
	
	return retval;
}

void copy_inventory_from_notecard(const LLUUID& destination_id,
								  const LLUUID& object_id,
								  const LLUUID& notecard_inv_id,
								  const LLInventoryItem *src,
								  U32 callback_id)
{
	if (NULL == src)
	{
		LL_WARNS(LOG_NOTECARD) << "Null pointer to item was passed for object_id "
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
        LL_WARNS(LOG_NOTECARD) << "Can't find region from object_id "
							   << object_id << " or gAgent"
							   << LL_ENDL;
        return;
    }

    LLSD body;
    body["notecard-id"] = notecard_inv_id;
    body["object-id"] = object_id;
    body["item-id"] = src->getUUID();
	body["folder-id"] = destination_id;
    body["callback-id"] = (LLSD::Integer)callback_id;

    /// *TODO: RIDER: This posts the request under the agents policy.  
    /// When I convert the inventory over this call should be moved under that 
    /// policy as well.
    if (!gAgent.requestPostCapability("CopyInventoryFromNotecard", body))
    {
        LL_WARNS() << "SIM does not have the capability to copy from notecard." << LL_ENDL;
    }
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

	LLPointer<LLInventoryCallback> cb = NULL;

	switch (inv_type)
	{
		case LLInventoryType::IT_LSL:
		{
			cb = new LLBoostFuncInventoryCallback(create_script_cb);
			next_owner_perm = LLFloaterPerms::getNextOwnerPerms("Scripts");
			break;
		}

		case LLInventoryType::IT_GESTURE:
		{
			cb = new LLBoostFuncInventoryCallback(create_gesture_cb);
			next_owner_perm = LLFloaterPerms::getNextOwnerPerms("Gestures");
			break;
		}

		case LLInventoryType::IT_NOTECARD:
		{
			cb = new LLBoostFuncInventoryCallback(create_notecard_cb);
			next_owner_perm = LLFloaterPerms::getNextOwnerPerms("Notecards");
			break;
		}
		default:
			break;
	}

	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  parent_id,
						  LLTransactionID::tnull,
						  name,
						  desc,
						  asset_type,
						  inv_type,
						  NOT_WEARABLE,
						  next_owner_perm,
						  cb);
}	

void slam_inventory_folder(const LLUUID& folder_id,
						   const LLSD& contents,
						   LLPointer<LLInventoryCallback> cb)
{
    LL_DEBUGS(LOG_INV) << "using AISv3 to slam folder, id " << folder_id
                       << " new contents: " << ll_pretty_print_sd(contents) << LL_ENDL;

    AISAPI::completion_t cr = boost::bind(&doInventoryCb, cb, _1);
    AISAPI::SlamFolder(folder_id, contents, cr);
}

void remove_folder_contents(const LLUUID& category, bool keep_outfit_links,
							LLPointer<LLInventoryCallback> cb)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(category, cats, items,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i = 0; i < items.size(); ++i)
	{
		LLViewerInventoryItem *item = items.at(i);
		if (keep_outfit_links && (item->getActualType() == LLAssetType::AT_LINK_FOLDER))
			continue;
		if (item->getIsLinkType())
		{
			remove_inventory_item(item->getUUID(), cb);
		}
	}
}

const std::string NEW_LSL_NAME = "New Script"; // *TODO:Translate? (probably not)
const std::string NEW_NOTECARD_NAME = "New Note"; // *TODO:Translate? (probably not)
const std::string NEW_GESTURE_NAME = "New Gesture"; // *TODO:Translate? (probably not)

// ! REFACTOR ! Really need to refactor this so that it's not a bunch of if-then statements...
void menu_create_inventory_item(LLInventoryPanel* panel, LLFolderBridge *bridge, const LLSD& userdata, const LLUUID& default_parent_uuid)
{
	std::string type_name = userdata.asString();
	
	if (("inbox" == type_name) || ("outbox" == type_name) || ("category" == type_name) || ("current" == type_name) || ("outfit" == type_name) || ("my_otfts" == type_name))
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
		panel->setSelectionByID(category, TRUE);
	}
	else if ("lsl" == type_name)
	{
		const LLUUID parent_id = bridge ? bridge->getUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_LSL_TEXT);
		create_new_item(NEW_LSL_NAME,
					  parent_id,
					  LLAssetType::AT_LSL_TEXT,
					  LLInventoryType::IT_LSL,
					  PERM_MOVE | PERM_TRANSFER);	// overridden in create_new_item
	}
	else if ("notecard" == type_name)
	{
		const LLUUID parent_id = bridge ? bridge->getUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_NOTECARD);
		create_new_item(NEW_NOTECARD_NAME,
					  parent_id,
					  LLAssetType::AT_NOTECARD,
					  LLInventoryType::IT_NOTECARD,
					  PERM_ALL);	// overridden in create_new_item
	}
	else if ("gesture" == type_name)
	{
		const LLUUID parent_id = bridge ? bridge->getUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE);
		create_new_item(NEW_GESTURE_NAME,
					  parent_id,
					  LLAssetType::AT_GESTURE,
					  LLInventoryType::IT_GESTURE,
					  PERM_ALL);	// overridden in create_new_item
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
			LL_WARNS(LOG_INV) << "Can't create unrecognized type " << type_name << LL_ENDL;
		}
	}
	panel->getRootFolder()->setNeedsAutoRename(TRUE);	
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

S32 LLViewerInventoryItem::getSortField() const
{
	return LLFavoritesOrderStorage::instance().getSortIndex(mUUID);
}

//void LLViewerInventoryItem::setSortField(S32 sortField)
//{
//	LLFavoritesOrderStorage::instance().setSortIndex(mUUID, sortField);
//	getSLURL();
//}

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
	return LLWearableType::inventoryFlagsToWearableType(getFlags());
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
			LL_WARNS(LOG_INV) << "Warning: Accessing link to link" << LL_ENDL;
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
