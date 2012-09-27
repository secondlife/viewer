/** 
 * @file llinventory.cpp
 * @brief Implementation of the inventory system.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"
#include "llinventory.h"

#include "lldbstrings.h"
#include "llinventorydefines.h"
#include "llxorcipher.h"
#include "llsd.h"
#include "message.h"
#include <boost/tokenizer.hpp>

#include "llsdutil.h"

///----------------------------------------------------------------------------
/// Exported functions
///----------------------------------------------------------------------------
static const std::string INV_ITEM_ID_LABEL("item_id");
static const std::string INV_FOLDER_ID_LABEL("folder_id");
static const std::string INV_PARENT_ID_LABEL("parent_id");
static const std::string INV_ASSET_TYPE_LABEL("type");
static const std::string INV_PREFERRED_TYPE_LABEL("preferred_type");
static const std::string INV_INVENTORY_TYPE_LABEL("inv_type");
static const std::string INV_NAME_LABEL("name");
static const std::string INV_DESC_LABEL("desc");
static const std::string INV_PERMISSIONS_LABEL("permissions");
static const std::string INV_SHADOW_ID_LABEL("shadow_id");
static const std::string INV_ASSET_ID_LABEL("asset_id");
static const std::string INV_SALE_INFO_LABEL("sale_info");
static const std::string INV_FLAGS_LABEL("flags");
static const std::string INV_CREATION_DATE_LABEL("created_at");

// key used by agent-inventory-service
static const std::string INV_ASSET_TYPE_LABEL_WS("type_default");
static const std::string INV_FOLDER_ID_LABEL_WS("category_id");

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

const LLUUID MAGIC_ID("3c115e51-04f4-523c-9fa6-98aff1034730");	

///----------------------------------------------------------------------------
/// Class LLInventoryObject
///----------------------------------------------------------------------------

LLInventoryObject::LLInventoryObject(const LLUUID& uuid,
									 const LLUUID& parent_uuid,
									 LLAssetType::EType type,
									 const std::string& name) :
	mUUID(uuid),
	mParentUUID(parent_uuid),
	mType(type),
	mName(name)
{
	correctInventoryName(mName);
}

LLInventoryObject::LLInventoryObject() :
	mType(LLAssetType::AT_NONE)
{
}

LLInventoryObject::~LLInventoryObject()
{
}

void LLInventoryObject::copyObject(const LLInventoryObject* other)
{
	mUUID = other->mUUID;
	mParentUUID = other->mParentUUID;
	mType = other->mType;
	mName = other->mName;
}

const LLUUID& LLInventoryObject::getUUID() const
{
	return mUUID;
}

const LLUUID& LLInventoryObject::getParentUUID() const
{
	return mParentUUID;
}

const std::string& LLInventoryObject::getName() const
{
	return mName;
}

// To bypass linked items, since llviewerinventory's getType
// will return the linked-to item's type instead of this object's type.
LLAssetType::EType LLInventoryObject::getActualType() const
{
	return mType;
}

BOOL LLInventoryObject::getIsLinkType() const
{
	return LLAssetType::lookupIsLinkType(mType);
}

// See LLInventoryItem override.
// virtual
const LLUUID& LLInventoryObject::getLinkedUUID() const
{
	return mUUID;
}

LLAssetType::EType LLInventoryObject::getType() const
{
	return mType;
}

void LLInventoryObject::setUUID(const LLUUID& new_uuid)
{
	mUUID = new_uuid;
}

void LLInventoryObject::rename(const std::string& n)
{
	std::string new_name(n);
	correctInventoryName(new_name);
	if( !new_name.empty() && new_name != mName )
	{
		mName = new_name;
	}
}

void LLInventoryObject::setParent(const LLUUID& new_parent)
{
	mParentUUID = new_parent;
}

void LLInventoryObject::setType(LLAssetType::EType type)
{
	mType = type;
}


// virtual
BOOL LLInventoryObject::importLegacyStream(std::istream& input_stream)
{
	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[MAX_STRING];	/* Flawfinder: ignore */
	char keyword[MAX_STRING];	/* Flawfinder: ignore */
	char valuestr[MAX_STRING];	/* Flawfinder: ignore */

	keyword[0] = '\0';
	valuestr[0] = '\0';
	while(input_stream.good())
	{
		input_stream.getline(buffer, MAX_STRING);
		sscanf(buffer, " %254s %254s", keyword, valuestr);	/* Flawfinder: ignore */
		if(0 == strcmp("{",keyword))
		{
			continue;
		}
		if(0 == strcmp("}", keyword))
		{
			break;
		}
		else if(0 == strcmp("obj_id", keyword))
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
		else if(0 == strcmp("name", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(	/* Flawfinder: ignore */
				buffer,
				" %254s %254[^|]",
				keyword, valuestr);
			mName.assign(valuestr);
			correctInventoryName(mName);
		}
		else
		{
			llwarns << "unknown keyword '" << keyword
					<< "' in LLInventoryObject::importLegacyStream() for object " << mUUID << llendl;
		}
	}
	return TRUE;
}

// exportFile should be replaced with exportLegacyStream
// not sure whether exportLegacyStream(llofstream(fp)) would work, fp may need to get icramented...
BOOL LLInventoryObject::exportFile(LLFILE* fp, BOOL) const
{
	std::string uuid_str;
	fprintf(fp, "\tinv_object\t0\n\t{\n");
	mUUID.toString(uuid_str);
	fprintf(fp, "\t\tobj_id\t%s\n", uuid_str.c_str());
	mParentUUID.toString(uuid_str);
	fprintf(fp, "\t\tparent_id\t%s\n", uuid_str.c_str());
	fprintf(fp, "\t\ttype\t%s\n", LLAssetType::lookup(mType));
	fprintf(fp, "\t\tname\t%s|\n", mName.c_str());
	fprintf(fp,"\t}\n");
	return TRUE;
}

BOOL LLInventoryObject::exportLegacyStream(std::ostream& output_stream, BOOL) const
{
	std::string uuid_str;
	output_stream <<  "\tinv_object\t0\n\t{\n";
	mUUID.toString(uuid_str);
	output_stream << "\t\tobj_id\t" << uuid_str << "\n";
	mParentUUID.toString(uuid_str);
	output_stream << "\t\tparent_id\t" << uuid_str << "\n";
	output_stream << "\t\ttype\t" << LLAssetType::lookup(mType) << "\n";
	output_stream << "\t\tname\t" << mName.c_str() << "|\n";
	output_stream << "\t}\n";
	return TRUE;
}


void LLInventoryObject::removeFromServer()
{
	// don't do nothin'
	llwarns << "LLInventoryObject::removeFromServer() called.  Doesn't do anything." << llendl;
}

void LLInventoryObject::updateParentOnServer(BOOL) const
{
	// don't do nothin'
	llwarns << "LLInventoryObject::updateParentOnServer() called.  Doesn't do anything." << llendl;
}

void LLInventoryObject::updateServer(BOOL) const
{
	// don't do nothin'
	llwarns << "LLInventoryObject::updateServer() called.  Doesn't do anything." << llendl;
}

inline
void LLInventoryObject::correctInventoryName(std::string& name)
{
	LLStringUtil::replaceNonstandardASCII(name, ' ');
	LLStringUtil::replaceChar(name, '|', ' ');
	LLStringUtil::trim(name);
	LLStringUtil::truncate(name, DB_INV_ITEM_NAME_STR_LEN);
}


///----------------------------------------------------------------------------
/// Class LLInventoryItem
///----------------------------------------------------------------------------

LLInventoryItem::LLInventoryItem(const LLUUID& uuid,
								 const LLUUID& parent_uuid,
								 const LLPermissions& permissions,
								 const LLUUID& asset_uuid,
								 LLAssetType::EType type,
								 LLInventoryType::EType inv_type,
								 const std::string& name, 
								 const std::string& desc,
								 const LLSaleInfo& sale_info,
								 U32 flags,
								 S32 creation_date_utc) :
	LLInventoryObject(uuid, parent_uuid, type, name),
	mPermissions(permissions),
	mAssetUUID(asset_uuid),
	mDescription(desc),
	mSaleInfo(sale_info),
	mInventoryType(inv_type),
	mFlags(flags),
	mCreationDate(creation_date_utc)
{
	LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
	LLStringUtil::replaceChar(mDescription, '|', ' ');
	mPermissions.initMasks(inv_type);
}

LLInventoryItem::LLInventoryItem() :
	LLInventoryObject(),
	mPermissions(),
	mAssetUUID(),
	mDescription(),
	mSaleInfo(),
	mInventoryType(LLInventoryType::IT_NONE),
	mFlags(0),
	mCreationDate(0)
{
}

LLInventoryItem::LLInventoryItem(const LLInventoryItem* other) :
	LLInventoryObject()
{
	copyItem(other);
}

LLInventoryItem::~LLInventoryItem()
{
}

// virtual
void LLInventoryItem::copyItem(const LLInventoryItem* other)
{
	copyObject(other);
	mPermissions = other->mPermissions;
	mAssetUUID = other->mAssetUUID;
	mDescription = other->mDescription;
	mSaleInfo = other->mSaleInfo;
	mInventoryType = other->mInventoryType;
	mFlags = other->mFlags;
	mCreationDate = other->mCreationDate;
}

// If this is a linked item, then the UUID of the base object is
// this item's assetID.
// virtual
const LLUUID& LLInventoryItem::getLinkedUUID() const
{
	if (LLAssetType::lookupIsLinkType(getActualType()))
	{
		return mAssetUUID;
	}

	return LLInventoryObject::getLinkedUUID();
}

const LLPermissions& LLInventoryItem::getPermissions() const
{
	return mPermissions;
}

const LLUUID& LLInventoryItem::getCreatorUUID() const
{
	return mPermissions.getCreator();
}

const LLUUID& LLInventoryItem::getAssetUUID() const
{
	return mAssetUUID;
}

void LLInventoryItem::setAssetUUID(const LLUUID& asset_id)
{
	mAssetUUID = asset_id;
}


const std::string& LLInventoryItem::getDescription() const
{
	return mDescription;
}

time_t LLInventoryItem::getCreationDate() const
{
	return mCreationDate;
}

U32 LLInventoryItem::getCRC32() const
{
	// *FIX: Not a real crc - more of a checksum.
	// *NOTE: We currently do not validate the name or description,
	// but if they change in transit, it's no big deal.
	U32 crc = mUUID.getCRC32();
	//lldebugs << "1 crc: " << std::hex << crc << std::dec << llendl;
	crc += mParentUUID.getCRC32();
	//lldebugs << "2 crc: " << std::hex << crc << std::dec << llendl;
	crc += mPermissions.getCRC32();
	//lldebugs << "3 crc: " << std::hex << crc << std::dec << llendl;
	crc += mAssetUUID.getCRC32();
	//lldebugs << "4 crc: " << std::hex << crc << std::dec << llendl;
	crc += mType;
	//lldebugs << "5 crc: " << std::hex << crc << std::dec << llendl;
	crc += mInventoryType;
	//lldebugs << "6 crc: " << std::hex << crc << std::dec << llendl;
	crc += mFlags;
	//lldebugs << "7 crc: " << std::hex << crc << std::dec << llendl;
	crc += mSaleInfo.getCRC32();
	//lldebugs << "8 crc: " << std::hex << crc << std::dec << llendl;
	crc += (U32)mCreationDate;
	//lldebugs << "9 crc: " << std::hex << crc << std::dec << llendl;
	return crc;
}


void LLInventoryItem::setDescription(const std::string& d)
{
	std::string new_desc(d);
	LLStringUtil::replaceNonstandardASCII(new_desc, ' ');
	LLStringUtil::replaceChar(new_desc, '|', ' ');
	if( new_desc != mDescription )
	{
		mDescription = new_desc;
	}
}

void LLInventoryItem::setPermissions(const LLPermissions& perm)
{
	mPermissions = perm;

	// Override permissions to unrestricted if this is a landmark
	mPermissions.initMasks(mInventoryType);
}

void LLInventoryItem::setInventoryType(LLInventoryType::EType inv_type)
{
	mInventoryType = inv_type;
}

void LLInventoryItem::setFlags(U32 flags)
{
	mFlags = flags;
}

void LLInventoryItem::setCreationDate(time_t creation_date_utc)
{
	mCreationDate = creation_date_utc;
}

// Currently only used in the Viewer to handle calling cards
// where the creator is actually used to store the target.
void LLInventoryItem::setCreator(const LLUUID& creator)
{ 
	mPermissions.setCreator(creator); 
}

void LLInventoryItem::accumulatePermissionSlamBits(const LLInventoryItem& old_item)
{
	// Remove any pre-existing II_FLAGS_PERM_OVERWRITE_MASK flags 
	// because we now detect when they should be set.
	setFlags( old_item.getFlags() | (getFlags() & ~(LLInventoryItemFlags::II_FLAGS_PERM_OVERWRITE_MASK)) );

	// Enforce the PERM_OVERWRITE flags for any masks that are different
	// but only for AT_OBJECT's since that is the only asset type that can 
	// exist in-world (instead of only in-inventory or in-object-contents).
	if (LLAssetType::AT_OBJECT == getType())
	{
		LLPermissions old_permissions = old_item.getPermissions();
		U32 flags_to_be_set = 0;
		if(old_permissions.getMaskNextOwner() != getPermissions().getMaskNextOwner())
		{
			flags_to_be_set |= LLInventoryItemFlags::II_FLAGS_OBJECT_SLAM_PERM;
		}
		if(old_permissions.getMaskEveryone() != getPermissions().getMaskEveryone())
		{
			flags_to_be_set |= LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
		}
		if(old_permissions.getMaskGroup() != getPermissions().getMaskGroup())
		{
			flags_to_be_set |= LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
		}
		LLSaleInfo old_sale_info = old_item.getSaleInfo();
		if(old_sale_info != getSaleInfo())
		{
			flags_to_be_set |= LLInventoryItemFlags::II_FLAGS_OBJECT_SLAM_SALE;
		}
		setFlags(getFlags() | flags_to_be_set);
	}
}

const LLSaleInfo& LLInventoryItem::getSaleInfo() const
{
	return mSaleInfo;
}

void LLInventoryItem::setSaleInfo(const LLSaleInfo& sale_info)
{
	mSaleInfo = sale_info;
}

LLInventoryType::EType LLInventoryItem::getInventoryType() const
{
	return mInventoryType;
}

U32 LLInventoryItem::getFlags() const
{
	return mFlags;
}

// virtual
void LLInventoryItem::packMessage(LLMessageSystem* msg) const
{
	msg->addUUIDFast(_PREHASH_ItemID, mUUID);
	msg->addUUIDFast(_PREHASH_FolderID, mParentUUID);
	mPermissions.packMessage(msg);
	msg->addUUIDFast(_PREHASH_AssetID, mAssetUUID);
	S8 type = static_cast<S8>(mType);
	msg->addS8Fast(_PREHASH_Type, type);
	type = static_cast<S8>(mInventoryType);
	msg->addS8Fast(_PREHASH_InvType, type);
	msg->addU32Fast(_PREHASH_Flags, mFlags);
	mSaleInfo.packMessage(msg);
	msg->addStringFast(_PREHASH_Name, mName);
	msg->addStringFast(_PREHASH_Description, mDescription);
	msg->addS32Fast(_PREHASH_CreationDate, (S32)mCreationDate);
	U32 crc = getCRC32();
	msg->addU32Fast(_PREHASH_CRC, crc);
}

// virtual
BOOL LLInventoryItem::unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num)
{
	msg->getUUIDFast(block, _PREHASH_ItemID, mUUID, block_num);
	msg->getUUIDFast(block, _PREHASH_FolderID, mParentUUID, block_num);
	mPermissions.unpackMessage(msg, block, block_num);
	msg->getUUIDFast(block, _PREHASH_AssetID, mAssetUUID, block_num);

	S8 type;
	msg->getS8Fast(block, _PREHASH_Type, type, block_num);
	mType = static_cast<LLAssetType::EType>(type);
	msg->getS8(block, "InvType", type, block_num);
	mInventoryType = static_cast<LLInventoryType::EType>(type);
	mPermissions.initMasks(mInventoryType);

	msg->getU32Fast(block, _PREHASH_Flags, mFlags, block_num);

	mSaleInfo.unpackMultiMessage(msg, block, block_num);

	msg->getStringFast(block, _PREHASH_Name, mName, block_num);
	LLStringUtil::replaceNonstandardASCII(mName, ' ');

	msg->getStringFast(block, _PREHASH_Description, mDescription, block_num);
	LLStringUtil::replaceNonstandardASCII(mDescription, ' ');

	S32 date;
	msg->getS32(block, "CreationDate", date, block_num);
	mCreationDate = date;

	U32 local_crc = getCRC32();
	U32 remote_crc = 0;
	msg->getU32(block, "CRC", remote_crc, block_num);
//#define CRC_CHECK
#ifdef CRC_CHECK
	if(local_crc == remote_crc)
	{
		lldebugs << "crc matches" << llendl;
		return TRUE;
	}
	else
	{
		llwarns << "inventory crc mismatch: local=" << std::hex << local_crc
				<< " remote=" << remote_crc << std::dec << llendl;
		return FALSE;
	}
#else
	return (local_crc == remote_crc);
#endif
}

// virtual
BOOL LLInventoryItem::importFile(LLFILE* fp)
{
	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[MAX_STRING];	/* Flawfinder: ignore */
	char keyword[MAX_STRING];	/* Flawfinder: ignore */	
	char valuestr[MAX_STRING];	/* Flawfinder: ignore */
	char junk[MAX_STRING];	/* Flawfinder: ignore */
	BOOL success = TRUE;

	keyword[0] = '\0';
	valuestr[0] = '\0';

	mInventoryType = LLInventoryType::IT_NONE;
	mAssetUUID.setNull();
	while(success && (!feof(fp)))
	{
		if (fgets(buffer, MAX_STRING, fp) == NULL)
		{
			buffer[0] = '\0';
		}
		
		sscanf(buffer, " %254s %254s", keyword, valuestr);	/* Flawfinder: ignore */
		if(0 == strcmp("{",keyword))
		{
			continue;
		}
		if(0 == strcmp("}", keyword))
		{
			break;
		}
		else if(0 == strcmp("item_id", keyword))
		{
			mUUID.set(valuestr);
		}
		else if(0 == strcmp("parent_id", keyword))
		{
			mParentUUID.set(valuestr);
		}
		else if(0 == strcmp("permissions", keyword))
		{
			success = mPermissions.importFile(fp);
		}
		else if(0 == strcmp("sale_info", keyword))
		{
			// Sale info used to contain next owner perm. It is now in
			// the permissions. Thus, we read that out, and fix legacy
			// objects. It's possible this op would fail, but it
			// should pick up the vast majority of the tasks.
			BOOL has_perm_mask = FALSE;
			U32 perm_mask = 0;
			success = mSaleInfo.importFile(fp, has_perm_mask, perm_mask);
			if(has_perm_mask)
			{
				if(perm_mask == PERM_NONE)
				{
					perm_mask = mPermissions.getMaskOwner();
				}
				// fair use fix.
				if(!(perm_mask & PERM_COPY))
				{
					perm_mask |= PERM_TRANSFER;
				}
				mPermissions.setMaskNext(perm_mask);
			}
		}
		else if(0 == strcmp("shadow_id", keyword))
		{
			mAssetUUID.set(valuestr);
			LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
			cipher.decrypt(mAssetUUID.mData, UUID_BYTES);
		}
		else if(0 == strcmp("asset_id", keyword))
		{
			mAssetUUID.set(valuestr);
		}
		else if(0 == strcmp("type", keyword))
		{
			mType = LLAssetType::lookup(valuestr);
		}
		else if(0 == strcmp("inv_type", keyword))
		{
			mInventoryType = LLInventoryType::lookup(std::string(valuestr));
		}
		else if(0 == strcmp("flags", keyword))
		{
			sscanf(valuestr, "%x", &mFlags);
		}
		else if(0 == strcmp("name", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(	/* Flawfinder: ignore */
				buffer,
				" %254s%254[\t]%254[^|]",
				keyword, junk, valuestr);

			// IW: sscanf chokes and puts | in valuestr if there's no name
			if (valuestr[0] == '|')
			{
				valuestr[0] = '\000';
			}

			mName.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
		else if(0 == strcmp("desc", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(	/* Flawfinder: ignore */
				buffer,
				" %254s%254[\t]%254[^|]",
				keyword, junk, valuestr);

			if (valuestr[0] == '|')
			{
				valuestr[0] = '\000';
			}

			mDescription.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
			/* TODO -- ask Ian about this code
			const char *donkey = mDescription.c_str();
			if (donkey[0] == '|')
			{
				llerrs << "Donkey" << llendl;
			}
			*/
		}
		else if(0 == strcmp("creation_date", keyword))
		{
			S32 date;
			sscanf(valuestr, "%d", &date);
			mCreationDate = date;
		}
		else
		{
			llwarns << "unknown keyword '" << keyword
					<< "' in inventory import of item " << mUUID << llendl;
		}
	}

	// Need to convert 1.0 simstate files to a useful inventory type
	// and potentially deal with bad inventory tyes eg, a landmark
	// marked as a texture.
	if((LLInventoryType::IT_NONE == mInventoryType)
	   || !inventory_and_asset_types_match(mInventoryType, mType))
	{
		lldebugs << "Resetting inventory type for " << mUUID << llendl;
		mInventoryType = LLInventoryType::defaultForAssetType(mType);
	}

	mPermissions.initMasks(mInventoryType);

	return success;
}

BOOL LLInventoryItem::exportFile(LLFILE* fp, BOOL include_asset_key) const
{
	std::string uuid_str;
	fprintf(fp, "\tinv_item\t0\n\t{\n");
	mUUID.toString(uuid_str);
	fprintf(fp, "\t\titem_id\t%s\n", uuid_str.c_str());
	mParentUUID.toString(uuid_str);
	fprintf(fp, "\t\tparent_id\t%s\n", uuid_str.c_str());
	mPermissions.exportFile(fp);

	// Check for permissions to see the asset id, and if so write it
	// out as an asset id. Otherwise, apply our cheesy encryption.
	if(include_asset_key)
	{
		U32 mask = mPermissions.getMaskBase();
		if(((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		   || (mAssetUUID.isNull()))
		{
			mAssetUUID.toString(uuid_str);
			fprintf(fp, "\t\tasset_id\t%s\n", uuid_str.c_str());
		}
		else
		{
			LLUUID shadow_id(mAssetUUID);
			LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
			cipher.encrypt(shadow_id.mData, UUID_BYTES);
			shadow_id.toString(uuid_str);
			fprintf(fp, "\t\tshadow_id\t%s\n", uuid_str.c_str());
		}
	}
	else
	{
		LLUUID::null.toString(uuid_str);
		fprintf(fp, "\t\tasset_id\t%s\n", uuid_str.c_str());
	}
	fprintf(fp, "\t\ttype\t%s\n", LLAssetType::lookup(mType));
	const std::string inv_type_str = LLInventoryType::lookup(mInventoryType);
	if(!inv_type_str.empty()) fprintf(fp, "\t\tinv_type\t%s\n", inv_type_str.c_str());
	fprintf(fp, "\t\tflags\t%08x\n", mFlags);
	mSaleInfo.exportFile(fp);
	fprintf(fp, "\t\tname\t%s|\n", mName.c_str());
	fprintf(fp, "\t\tdesc\t%s|\n", mDescription.c_str());
	fprintf(fp, "\t\tcreation_date\t%d\n", (S32) mCreationDate);
	fprintf(fp,"\t}\n");
	return TRUE;
}

// virtual
BOOL LLInventoryItem::importLegacyStream(std::istream& input_stream)
{
	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[MAX_STRING];	/* Flawfinder: ignore */
	char keyword[MAX_STRING];	/* Flawfinder: ignore */
	char valuestr[MAX_STRING];	/* Flawfinder: ignore */
	char junk[MAX_STRING];	/* Flawfinder: ignore */
	BOOL success = TRUE;

	keyword[0] = '\0';
	valuestr[0] = '\0';

	mInventoryType = LLInventoryType::IT_NONE;
	mAssetUUID.setNull();
	while(success && input_stream.good())
	{
		input_stream.getline(buffer, MAX_STRING);
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %254s %254s",
			keyword, valuestr);
		if(0 == strcmp("{",keyword))
		{
			continue;
		}
		if(0 == strcmp("}", keyword))
		{
			break;
		}
		else if(0 == strcmp("item_id", keyword))
		{
			mUUID.set(valuestr);
		}
		else if(0 == strcmp("parent_id", keyword))
		{
			mParentUUID.set(valuestr);
		}
		else if(0 == strcmp("permissions", keyword))
		{
			success = mPermissions.importStream(input_stream);
		}
		else if(0 == strcmp("sale_info", keyword))
		{
			// Sale info used to contain next owner perm. It is now in
			// the permissions. Thus, we read that out, and fix legacy
			// objects. It's possible this op would fail, but it
			// should pick up the vast majority of the tasks.
			BOOL has_perm_mask = FALSE;
			U32 perm_mask = 0;
			success = mSaleInfo.importStream(input_stream, has_perm_mask, perm_mask);
			if(has_perm_mask)
			{
				if(perm_mask == PERM_NONE)
				{
					perm_mask = mPermissions.getMaskOwner();
				}
				// fair use fix.
				if(!(perm_mask & PERM_COPY))
				{
					perm_mask |= PERM_TRANSFER;
				}
				mPermissions.setMaskNext(perm_mask);
			}
		}
		else if(0 == strcmp("shadow_id", keyword))
		{
			mAssetUUID.set(valuestr);
			LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
			cipher.decrypt(mAssetUUID.mData, UUID_BYTES);
		}
		else if(0 == strcmp("asset_id", keyword))
		{
			mAssetUUID.set(valuestr);
		}
		else if(0 == strcmp("type", keyword))
		{
			mType = LLAssetType::lookup(valuestr);
		}
		else if(0 == strcmp("inv_type", keyword))
		{
			mInventoryType = LLInventoryType::lookup(std::string(valuestr));
		}
		else if(0 == strcmp("flags", keyword))
		{
			sscanf(valuestr, "%x", &mFlags);
		}
		else if(0 == strcmp("name", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(	/* Flawfinder: ignore */
				buffer,
				" %254s%254[\t]%254[^|]",
				keyword, junk, valuestr);

			// IW: sscanf chokes and puts | in valuestr if there's no name
			if (valuestr[0] == '|')
			{
				valuestr[0] = '\000';
			}

			mName.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
		else if(0 == strcmp("desc", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(	/* Flawfinder: ignore */
				buffer,
				" %254s%254[\t]%254[^|]",
				keyword, junk, valuestr);

			if (valuestr[0] == '|')
			{
				valuestr[0] = '\000';
			}

			mDescription.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
			/* TODO -- ask Ian about this code
			const char *donkey = mDescription.c_str();
			if (donkey[0] == '|')
			{
				llerrs << "Donkey" << llendl;
			}
			*/
		}
		else if(0 == strcmp("creation_date", keyword))
		{
			S32 date;
			sscanf(valuestr, "%d", &date);
			mCreationDate = date;
		}
		else
		{
			llwarns << "unknown keyword '" << keyword
					<< "' in inventory import of item " << mUUID << llendl;
		}
	}

	// Need to convert 1.0 simstate files to a useful inventory type
	// and potentially deal with bad inventory tyes eg, a landmark
	// marked as a texture.
	if((LLInventoryType::IT_NONE == mInventoryType)
	   || !inventory_and_asset_types_match(mInventoryType, mType))
	{
		lldebugs << "Resetting inventory type for " << mUUID << llendl;
		mInventoryType = LLInventoryType::defaultForAssetType(mType);
	}

	mPermissions.initMasks(mInventoryType);

	return success;
}

BOOL LLInventoryItem::exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key) const
{
	std::string uuid_str;
	output_stream << "\tinv_item\t0\n\t{\n";
	mUUID.toString(uuid_str);
	output_stream << "\t\titem_id\t" << uuid_str << "\n";
	mParentUUID.toString(uuid_str);
	output_stream << "\t\tparent_id\t" << uuid_str << "\n";
	mPermissions.exportStream(output_stream);

	// Check for permissions to see the asset id, and if so write it
	// out as an asset id. Otherwise, apply our cheesy encryption.
	if(include_asset_key)
	{
		U32 mask = mPermissions.getMaskBase();
		if(((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		   || (mAssetUUID.isNull()))
		{
			mAssetUUID.toString(uuid_str);
			output_stream << "\t\tasset_id\t" << uuid_str << "\n";
		}
		else
		{
			LLUUID shadow_id(mAssetUUID);
			LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
			cipher.encrypt(shadow_id.mData, UUID_BYTES);
			shadow_id.toString(uuid_str);
			output_stream << "\t\tshadow_id\t" << uuid_str << "\n";
		}
	}
	else
	{
		LLUUID::null.toString(uuid_str);
		output_stream << "\t\tasset_id\t" << uuid_str << "\n";
	}
	output_stream << "\t\ttype\t" << LLAssetType::lookup(mType) << "\n";
	const std::string inv_type_str = LLInventoryType::lookup(mInventoryType);
	if(!inv_type_str.empty()) 
		output_stream << "\t\tinv_type\t" << inv_type_str << "\n";
	std::string buffer;
	buffer = llformat( "\t\tflags\t%08x\n", mFlags);
	output_stream << buffer;
	mSaleInfo.exportStream(output_stream);
	output_stream << "\t\tname\t" << mName.c_str() << "|\n";
	output_stream << "\t\tdesc\t" << mDescription.c_str() << "|\n";
	output_stream << "\t\tcreation_date\t" << mCreationDate << "\n";
	output_stream << "\t}\n";
	return TRUE;
}

LLSD LLInventoryItem::asLLSD() const
{
	LLSD sd = LLSD();
	asLLSD(sd);
	return sd;
}

void LLInventoryItem::asLLSD( LLSD& sd ) const
{
	sd[INV_ITEM_ID_LABEL] = mUUID;
	sd[INV_PARENT_ID_LABEL] = mParentUUID;
	sd[INV_PERMISSIONS_LABEL] = ll_create_sd_from_permissions(mPermissions);

	U32 mask = mPermissions.getMaskBase();
	if(((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		|| (mAssetUUID.isNull()))
	{
		sd[INV_ASSET_ID_LABEL] = mAssetUUID;
	}
	else
	{
		// *TODO: get rid of this. Phoenix 2008-01-30
		LLUUID shadow_id(mAssetUUID);
		LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
		cipher.encrypt(shadow_id.mData, UUID_BYTES);
		sd[INV_SHADOW_ID_LABEL] = shadow_id;
	}
	sd[INV_ASSET_TYPE_LABEL] = LLAssetType::lookup(mType);
	sd[INV_INVENTORY_TYPE_LABEL] = mInventoryType;
	const std::string inv_type_str = LLInventoryType::lookup(mInventoryType);
	if(!inv_type_str.empty())
	{
		sd[INV_INVENTORY_TYPE_LABEL] = inv_type_str;
	}
	//sd[INV_FLAGS_LABEL] = (S32)mFlags;
	sd[INV_FLAGS_LABEL] = ll_sd_from_U32(mFlags);
	sd[INV_SALE_INFO_LABEL] = mSaleInfo;
	sd[INV_NAME_LABEL] = mName;
	sd[INV_DESC_LABEL] = mDescription;
	sd[INV_CREATION_DATE_LABEL] = (S32) mCreationDate;
}

LLFastTimer::DeclareTimer FTM_INVENTORY_SD_DESERIALIZE("Inventory SD Deserialize");

bool LLInventoryItem::fromLLSD(const LLSD& sd)
{
	LLFastTimer _(FTM_INVENTORY_SD_DESERIALIZE);
	mInventoryType = LLInventoryType::IT_NONE;
	mAssetUUID.setNull();
	std::string w;

	w = INV_ITEM_ID_LABEL;
	if (sd.has(w))
	{
		mUUID = sd[w];
	}
	w = INV_PARENT_ID_LABEL;
	if (sd.has(w))
	{
		mParentUUID = sd[w];
	}
	w = INV_PERMISSIONS_LABEL;
	if (sd.has(w))
	{
		mPermissions = ll_permissions_from_sd(sd[w]);
	}
	w = INV_SALE_INFO_LABEL;
	if (sd.has(w))
	{
		// Sale info used to contain next owner perm. It is now in
		// the permissions. Thus, we read that out, and fix legacy
		// objects. It's possible this op would fail, but it
		// should pick up the vast majority of the tasks.
		BOOL has_perm_mask = FALSE;
		U32 perm_mask = 0;
		if (!mSaleInfo.fromLLSD(sd[w], has_perm_mask, perm_mask))
		{
			goto fail;
		}
		if (has_perm_mask)
		{
			if(perm_mask == PERM_NONE)
			{
				perm_mask = mPermissions.getMaskOwner();
			}
			// fair use fix.
			if(!(perm_mask & PERM_COPY))
			{
				perm_mask |= PERM_TRANSFER;
			}
			mPermissions.setMaskNext(perm_mask);
		}
	}
	w = INV_SHADOW_ID_LABEL;
	if (sd.has(w))
	{
		mAssetUUID = sd[w];
		LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
		cipher.decrypt(mAssetUUID.mData, UUID_BYTES);
	}
	w = INV_ASSET_ID_LABEL;
	if (sd.has(w))
	{
		mAssetUUID = sd[w];
	}
	w = INV_ASSET_TYPE_LABEL;
	if (sd.has(w))
	{
		if (sd[w].isString())
		{
			mType = LLAssetType::lookup(sd[w].asString().c_str());
		}
		else if (sd[w].isInteger())
		{
			S8 type = (U8)sd[w].asInteger();
			mType = static_cast<LLAssetType::EType>(type);
		}
	}
	w = INV_INVENTORY_TYPE_LABEL;
	if (sd.has(w))
	{
		if (sd[w].isString())
		{
			mInventoryType = LLInventoryType::lookup(sd[w].asString().c_str());
		}
		else if (sd[w].isInteger())
		{
			S8 type = (U8)sd[w].asInteger();
			mInventoryType = static_cast<LLInventoryType::EType>(type);
		}
	}
	w = INV_FLAGS_LABEL;
	if (sd.has(w))
	{
		if (sd[w].isBinary())
		{
			mFlags = ll_U32_from_sd(sd[w]);
		}
		else if(sd[w].isInteger())
		{
			mFlags = sd[w].asInteger();
		}
	}
	w = INV_NAME_LABEL;
	if (sd.has(w))
	{
		mName = sd[w].asString();
		LLStringUtil::replaceNonstandardASCII(mName, ' ');
		LLStringUtil::replaceChar(mName, '|', ' ');
	}
	w = INV_DESC_LABEL;
	if (sd.has(w))
	{
		mDescription = sd[w].asString();
		LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
	}
	w = INV_CREATION_DATE_LABEL;
	if (sd.has(w))
	{
		mCreationDate = sd[w].asInteger();
	}

	// Need to convert 1.0 simstate files to a useful inventory type
	// and potentially deal with bad inventory tyes eg, a landmark
	// marked as a texture.
	if((LLInventoryType::IT_NONE == mInventoryType)
	   || !inventory_and_asset_types_match(mInventoryType, mType))
	{
		lldebugs << "Resetting inventory type for " << mUUID << llendl;
		mInventoryType = LLInventoryType::defaultForAssetType(mType);
	}

	mPermissions.initMasks(mInventoryType);

	return true;
fail:
	return false;

}

// Deleted LLInventoryItem::exportFileXML() and LLInventoryItem::importXML()
// because I can't find any non-test code references to it. 2009-05-04 JC

S32 LLInventoryItem::packBinaryBucket(U8* bin_bucket, LLPermissions* perm_override) const
{
	// Figure out which permissions to use.
	LLPermissions perm;
	if (perm_override)
	{
		// Use the permissions override.
		perm = *perm_override;
	}
	else
	{
		// Use the current permissions.
		perm = getPermissions();
	}

	// describe the inventory item
	char* buffer = (char*) bin_bucket;
	std::string creator_id_str;

	perm.getCreator().toString(creator_id_str);
	std::string owner_id_str;
	perm.getOwner().toString(owner_id_str);
	std::string last_owner_id_str;
	perm.getLastOwner().toString(last_owner_id_str);
	std::string group_id_str;
	perm.getGroup().toString(group_id_str);
	std::string asset_id_str;
	getAssetUUID().toString(asset_id_str);
	S32 size = sprintf(buffer,	/* Flawfinder: ignore */
					   "%d|%d|%s|%s|%s|%s|%s|%x|%x|%x|%x|%x|%s|%s|%d|%d|%x",
					   getType(),
					   getInventoryType(),
					   getName().c_str(),
					   creator_id_str.c_str(),
					   owner_id_str.c_str(),
					   last_owner_id_str.c_str(),
					   group_id_str.c_str(),
					   perm.getMaskBase(),
					   perm.getMaskOwner(),
					   perm.getMaskGroup(),
					   perm.getMaskEveryone(),
					   perm.getMaskNextOwner(),
					   asset_id_str.c_str(),
					   getDescription().c_str(),
					   getSaleInfo().getSaleType(),
					   getSaleInfo().getSalePrice(),
					   getFlags()) + 1;

	return size;
}

void LLInventoryItem::unpackBinaryBucket(U8* bin_bucket, S32 bin_bucket_size)
{	
	// Early exit on an empty binary bucket.
	if (bin_bucket_size <= 1) return;

	if (NULL == bin_bucket)
	{
		llerrs << "unpackBinaryBucket failed.  bin_bucket is NULL." << llendl;
		return;
	}

	// Convert the bin_bucket into a string.
	std::vector<char> item_buffer(bin_bucket_size+1);
	memcpy(&item_buffer[0], bin_bucket, bin_bucket_size);	/* Flawfinder: ignore */
	item_buffer[bin_bucket_size] = '\0';
	std::string str(&item_buffer[0]);

	lldebugs << "item buffer: " << str << llendl;

	// Tokenize the string.
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
	tokenizer tokens(str, sep);
	tokenizer::iterator iter = tokens.begin();

	// Extract all values.
	LLUUID item_id;
	item_id.generate();
	setUUID(item_id);

	LLAssetType::EType type;
	type = (LLAssetType::EType)(atoi((*(iter++)).c_str()));
	setType( type );
	
	LLInventoryType::EType inv_type;
	inv_type = (LLInventoryType::EType)(atoi((*(iter++)).c_str()));
	setInventoryType( inv_type );

	std::string name((*(iter++)).c_str());
	rename( name );
	
	LLUUID creator_id((*(iter++)).c_str());
	LLUUID owner_id((*(iter++)).c_str());
	LLUUID last_owner_id((*(iter++)).c_str());
	LLUUID group_id((*(iter++)).c_str());
	PermissionMask mask_base = strtoul((*(iter++)).c_str(), NULL, 16);
	PermissionMask mask_owner = strtoul((*(iter++)).c_str(), NULL, 16);
	PermissionMask mask_group = strtoul((*(iter++)).c_str(), NULL, 16);
	PermissionMask mask_every = strtoul((*(iter++)).c_str(), NULL, 16);
	PermissionMask mask_next = strtoul((*(iter++)).c_str(), NULL, 16);
	LLPermissions perm;
	perm.init(creator_id, owner_id, last_owner_id, group_id);
	perm.initMasks(mask_base, mask_owner, mask_group, mask_every, mask_next);
	setPermissions(perm);
	//lldebugs << "perm: " << perm << llendl;

	LLUUID asset_id((*(iter++)).c_str());
	setAssetUUID(asset_id);

	std::string desc((*(iter++)).c_str());
	setDescription(desc);
	
	LLSaleInfo::EForSale sale_type;
	sale_type = (LLSaleInfo::EForSale)(atoi((*(iter++)).c_str()));
	S32 price = atoi((*(iter++)).c_str());
	LLSaleInfo sale_info(sale_type, price);
	setSaleInfo(sale_info);
	
	U32 flags = strtoul((*(iter++)).c_str(), NULL, 16);
	setFlags(flags);

	time_t now = time(NULL);
	setCreationDate(now);
}

///----------------------------------------------------------------------------
/// Class LLInventoryCategory
///----------------------------------------------------------------------------

LLInventoryCategory::LLInventoryCategory(const LLUUID& uuid,
										 const LLUUID& parent_uuid,
										 LLFolderType::EType preferred_type,
										 const std::string& name) :
	LLInventoryObject(uuid, parent_uuid, LLAssetType::AT_CATEGORY, name),
	mPreferredType(preferred_type)
{
}

LLInventoryCategory::LLInventoryCategory() :
	mPreferredType(LLFolderType::FT_NONE)
{
	mType = LLAssetType::AT_CATEGORY;
}

LLInventoryCategory::LLInventoryCategory(const LLInventoryCategory* other) :
	LLInventoryObject()
{
	copyCategory(other);
}

LLInventoryCategory::~LLInventoryCategory()
{
}

// virtual
void LLInventoryCategory::copyCategory(const LLInventoryCategory* other)
{
	copyObject(other);
	mPreferredType = other->mPreferredType;
}

LLFolderType::EType LLInventoryCategory::getPreferredType() const
{
	return mPreferredType;
}

void LLInventoryCategory::setPreferredType(LLFolderType::EType type)
{
	mPreferredType = type;
}

LLSD LLInventoryCategory::asLLSD() const
{
    LLSD sd = LLSD();
    sd["item_id"] = mUUID;
    sd["parent_id"] = mParentUUID;
    S8 type = static_cast<S8>(mPreferredType);
    sd["type"]      = type;
    sd["name"] = mName;

    return sd;
}


// virtual
void LLInventoryCategory::packMessage(LLMessageSystem* msg) const
{
	msg->addUUIDFast(_PREHASH_FolderID, mUUID);
	msg->addUUIDFast(_PREHASH_ParentID, mParentUUID);
	S8 type = static_cast<S8>(mPreferredType);
	msg->addS8Fast(_PREHASH_Type, type);
	msg->addStringFast(_PREHASH_Name, mName);
}

bool LLInventoryCategory::fromLLSD(const LLSD& sd)
{
    std::string w;

    w = INV_FOLDER_ID_LABEL_WS;
    if (sd.has(w))
    {
        mUUID = sd[w];
    }
    w = INV_PARENT_ID_LABEL;
    if (sd.has(w))
    {
        mParentUUID = sd[w];
    }
    w = INV_ASSET_TYPE_LABEL;
    if (sd.has(w))
    {
        S8 type = (U8)sd[w].asInteger();
        mPreferredType = static_cast<LLFolderType::EType>(type);
    }
	w = INV_ASSET_TYPE_LABEL_WS;
	if (sd.has(w))
	{
		S8 type = (U8)sd[w].asInteger();
        mPreferredType = static_cast<LLFolderType::EType>(type);
	}

    w = INV_NAME_LABEL;
    if (sd.has(w))
    {
        mName = sd[w].asString();
        LLStringUtil::replaceNonstandardASCII(mName, ' ');
        LLStringUtil::replaceChar(mName, '|', ' ');
    }
    return true;
}

// virtual
void LLInventoryCategory::unpackMessage(LLMessageSystem* msg,
										const char* block,
										S32 block_num)
{
	msg->getUUIDFast(block, _PREHASH_FolderID, mUUID, block_num);
	msg->getUUIDFast(block, _PREHASH_ParentID, mParentUUID, block_num);
	S8 type;
	msg->getS8Fast(block, _PREHASH_Type, type, block_num);
	mPreferredType = static_cast<LLFolderType::EType>(type);
	msg->getStringFast(block, _PREHASH_Name, mName, block_num);
	LLStringUtil::replaceNonstandardASCII(mName, ' ');
}
	
// virtual
BOOL LLInventoryCategory::importFile(LLFILE* fp)
{
	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[MAX_STRING];	/* Flawfinder: ignore */
	char keyword[MAX_STRING];	/* Flawfinder: ignore */
	char valuestr[MAX_STRING];	/* Flawfinder: ignore */

	keyword[0] = '\0';
	valuestr[0] = '\0';
	while(!feof(fp))
	{
		if (fgets(buffer, MAX_STRING, fp) == NULL)
		{
			buffer[0] = '\0';
		}
		
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %254s %254s",
			keyword, valuestr);
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
				buffer,
				" %254s %254[^|]",
				keyword, valuestr);
			mName.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
		else
		{
			llwarns << "unknown keyword '" << keyword
					<< "' in inventory import category "  << mUUID << llendl;
		}
	}
	return TRUE;
}

BOOL LLInventoryCategory::exportFile(LLFILE* fp, BOOL) const
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
	fprintf(fp,"\t}\n");
	return TRUE;
}

	
// virtual
BOOL LLInventoryCategory::importLegacyStream(std::istream& input_stream)
{
	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[MAX_STRING];	/* Flawfinder: ignore */
	char keyword[MAX_STRING];	/* Flawfinder: ignore */
	char valuestr[MAX_STRING];	/* Flawfinder: ignore */

	keyword[0] = '\0';
	valuestr[0] = '\0';
	while(input_stream.good())
	{
		input_stream.getline(buffer, MAX_STRING);
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %254s %254s",
			keyword, valuestr);
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
				buffer,
				" %254s %254[^|]",
				keyword, valuestr);
			mName.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
		else
		{
			llwarns << "unknown keyword '" << keyword
					<< "' in inventory import category "  << mUUID << llendl;
		}
	}
	return TRUE;
}

BOOL LLInventoryCategory::exportLegacyStream(std::ostream& output_stream, BOOL) const
{
	std::string uuid_str;
	output_stream << "\tinv_category\t0\n\t{\n";
	mUUID.toString(uuid_str);
	output_stream << "\t\tcat_id\t" << uuid_str << "\n";
	mParentUUID.toString(uuid_str);
	output_stream << "\t\tparent_id\t" << uuid_str << "\n";
	output_stream << "\t\ttype\t" << LLAssetType::lookup(mType) << "\n";
	output_stream << "\t\tpref_type\t" << LLFolderType::lookup(mPreferredType) << "\n";
	output_stream << "\t\tname\t" << mName.c_str() << "|\n";
	output_stream << "\t}\n";
	return TRUE;
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------

LLSD ll_create_sd_from_inventory_item(LLPointer<LLInventoryItem> item)
{
	LLSD rv;
	if(item.isNull()) return rv;
	if (item->getType() == LLAssetType::AT_NONE)
	{
		llwarns << "ll_create_sd_from_inventory_item() for item with AT_NONE"
			<< llendl;
		return rv;
	}
	rv[INV_ITEM_ID_LABEL] =  item->getUUID();
	rv[INV_PARENT_ID_LABEL] = item->getParentUUID();
	rv[INV_NAME_LABEL] = item->getName();
	rv[INV_ASSET_TYPE_LABEL] = LLAssetType::lookup(item->getType());
	rv[INV_ASSET_ID_LABEL] = item->getAssetUUID();
	rv[INV_DESC_LABEL] = item->getDescription();
	rv[INV_SALE_INFO_LABEL] = ll_create_sd_from_sale_info(item->getSaleInfo());
	rv[INV_PERMISSIONS_LABEL] =
		ll_create_sd_from_permissions(item->getPermissions());
	rv[INV_INVENTORY_TYPE_LABEL] =
		LLInventoryType::lookup(item->getInventoryType());
	rv[INV_FLAGS_LABEL] = (S32)item->getFlags();
	rv[INV_CREATION_DATE_LABEL] = (S32)item->getCreationDate();
	return rv;
}

LLSD ll_create_sd_from_inventory_category(LLPointer<LLInventoryCategory> cat)
{
	LLSD rv;
	if(cat.isNull()) return rv;
	if (cat->getType() == LLAssetType::AT_NONE)
	{
		llwarns << "ll_create_sd_from_inventory_category() for cat with AT_NONE"
			<< llendl;
		return rv;
	}
	rv[INV_FOLDER_ID_LABEL] = cat->getUUID();
	rv[INV_PARENT_ID_LABEL] = cat->getParentUUID();
	rv[INV_NAME_LABEL] = cat->getName();
	rv[INV_ASSET_TYPE_LABEL] = LLAssetType::lookup(cat->getType());
	if(LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
	{
		rv[INV_PREFERRED_TYPE_LABEL] =
			LLFolderType::lookup(cat->getPreferredType()).c_str();
	}
	return rv;
}

LLPointer<LLInventoryCategory> ll_create_category_from_sd(const LLSD& sd_cat)
{
	LLPointer<LLInventoryCategory> rv = new LLInventoryCategory;
	rv->setUUID(sd_cat[INV_FOLDER_ID_LABEL].asUUID());
	rv->setParent(sd_cat[INV_PARENT_ID_LABEL].asUUID());
	rv->rename(sd_cat[INV_NAME_LABEL].asString());
	rv->setType(
		LLAssetType::lookup(sd_cat[INV_ASSET_TYPE_LABEL].asString()));
	rv->setPreferredType(
			LLFolderType::lookup(
				sd_cat[INV_PREFERRED_TYPE_LABEL].asString()));
	return rv;
}
