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
#include "llfasttimer.h"
#include "llinventorydefines.h"
#include "llxorcipher.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "message.h"
#include <boost/tokenizer.hpp>

#include "llsdutil.h"

///----------------------------------------------------------------------------
/// Exported functions
///----------------------------------------------------------------------------
// FIXME D567 - what's the point of these, especially if we don't even use them consistently?
static const std::string INV_ITEM_ID_LABEL("item_id");
static const std::string INV_FOLDER_ID_LABEL("cat_id");
static const std::string INV_PARENT_ID_LABEL("parent_id");
static const std::string INV_THUMBNAIL_LABEL("thumbnail");
static const std::string INV_THUMBNAIL_ID_LABEL("thumbnail_id");
static const std::string INV_ASSET_TYPE_LABEL("type");
static const std::string INV_PREFERRED_TYPE_LABEL("preferred_type");
static const std::string INV_INVENTORY_TYPE_LABEL("inv_type");
static const std::string INV_NAME_LABEL("name");
static const std::string INV_DESC_LABEL("desc");
static const std::string INV_PERMISSIONS_LABEL("permissions");
static const std::string INV_SHADOW_ID_LABEL("shadow_id");
static const std::string INV_ASSET_ID_LABEL("asset_id");
static const std::string INV_LINKED_ID_LABEL("linked_id");
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
                                     const std::string& name)
:   mUUID(uuid),
    mParentUUID(parent_uuid),
    mType(type),
    mName(name),
    mCreationDate(0)
{
    correctInventoryName(mName);
}

LLInventoryObject::LLInventoryObject()
:   mType(LLAssetType::AT_NONE),
    mCreationDate(0)
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
    mThumbnailUUID = other->mThumbnailUUID;
}

const LLUUID& LLInventoryObject::getUUID() const
{
    return mUUID;
}

const LLUUID& LLInventoryObject::getParentUUID() const
{
    return mParentUUID;
}

const LLUUID& LLInventoryObject::getThumbnailUUID() const
{
    return mThumbnailUUID;
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

bool LLInventoryObject::getIsLinkType() const
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

void LLInventoryObject::setThumbnailUUID(const LLUUID& thumbnail_uuid)
{
    mThumbnailUUID = thumbnail_uuid;
}

void LLInventoryObject::setType(LLAssetType::EType type)
{
    mType = type;
}


// virtual
bool LLInventoryObject::importLegacyStream(std::istream& input_stream)
{
    // *NOTE: Changing the buffer size will require changing the scanf
    // calls below.
    char buffer[MAX_STRING];    /* Flawfinder: ignore */
    char keyword[MAX_STRING];   /* Flawfinder: ignore */
    char valuestr[MAX_STRING];  /* Flawfinder: ignore */

    keyword[0] = '\0';
    valuestr[0] = '\0';
    while(input_stream.good())
    {
        input_stream.getline(buffer, MAX_STRING);
        sscanf(buffer, " %254s %254s", keyword, valuestr);  /* Flawfinder: ignore */
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
        else if (0 == strcmp("metadata", keyword))
        {
            LLSD metadata;
            if (strncmp("<llsd>", valuestr, 6) == 0)
            {
                std::istringstream stream(valuestr);
                LLSDSerialize::fromXML(metadata, stream);
            }
            else
            {
                // next line likely contains metadata, but at the moment is not supported
                // can do something like:
                // LLSDSerialize::fromNotation(metadata, input_stream, -1);
            }

            if (metadata.has("thumbnail"))
            {
                const LLSD& thumbnail = metadata["thumbnail"];
                if (thumbnail.has("asset_id"))
                {
                    setThumbnailUUID(thumbnail["asset_id"].asUUID());
                }
                else
                {
                    setThumbnailUUID(LLUUID::null);
                }
            }
            else
            {
                setThumbnailUUID(LLUUID::null);
            }
        }
        else if(0 == strcmp("name", keyword))
        {
            //strcpy(valuestr, buffer + strlen(keyword) + 3);
            // *NOTE: Not ANSI C, but widely supported.
            sscanf( /* Flawfinder: ignore */
                buffer,
                " %254s %254[^|]",
                keyword, valuestr);
            mName.assign(valuestr);
            correctInventoryName(mName);
        }
        else
        {
            LL_WARNS() << "unknown keyword '" << keyword
                    << "' in LLInventoryObject::importLegacyStream() for object " << mUUID << LL_ENDL;
        }
    }
    return true;
}

bool LLInventoryObject::exportLegacyStream(std::ostream& output_stream, bool) const
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
    return true;
}

void LLInventoryObject::updateParentOnServer(bool) const
{
    // don't do nothin'
    LL_WARNS() << "LLInventoryObject::updateParentOnServer() called.  Doesn't do anything." << LL_ENDL;
}

void LLInventoryObject::updateServer(bool) const
{
    // don't do nothin'
    LL_WARNS() << "LLInventoryObject::updateServer() called.  Doesn't do anything." << LL_ENDL;
}

// static
void LLInventoryObject::correctInventoryName(std::string& name)
{
    LLStringUtil::replaceNonstandardASCII(name, ' ');
    LLStringUtil::replaceChar(name, '|', ' ');
    LLStringUtil::trim(name);
    LLStringUtil::truncate(name, DB_INV_ITEM_NAME_STR_LEN);
}

time_t LLInventoryObject::getCreationDate() const
{
    return mCreationDate;
}

void LLInventoryObject::setCreationDate(time_t creation_date_utc)
{
    mCreationDate = creation_date_utc;
}


const std::string& LLInventoryItem::getDescription() const
{
    return mDescription;
}

const std::string& LLInventoryItem::getActualDescription() const
{
    return mDescription;
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
    mFlags(flags)
{
    mCreationDate = creation_date_utc;

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
    mFlags(0)
{
    mCreationDate = 0;
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
    mThumbnailUUID = other->mThumbnailUUID;
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


U32 LLInventoryItem::getCRC32() const
{
    // *FIX: Not a real crc - more of a checksum.
    // *NOTE: We currently do not validate the name or description,
    // but if they change in transit, it's no big deal.
    U32 crc = mUUID.getCRC32();
    //LL_DEBUGS() << "1 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mParentUUID.getCRC32();
    //LL_DEBUGS() << "2 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mPermissions.getCRC32();
    //LL_DEBUGS() << "3 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mAssetUUID.getCRC32();
    //LL_DEBUGS() << "4 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mType;
    //LL_DEBUGS() << "5 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mInventoryType;
    //LL_DEBUGS() << "6 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mFlags;
    //LL_DEBUGS() << "7 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mSaleInfo.getCRC32();
    //LL_DEBUGS() << "8 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += (U32)mCreationDate;
    //LL_DEBUGS() << "9 crc: " << std::hex << crc << std::dec << LL_ENDL;
    crc += mThumbnailUUID.getCRC32();
    return crc;
}

// static
void LLInventoryItem::correctInventoryDescription(std::string& desc)
{
    LLStringUtil::replaceNonstandardASCII(desc, ' ');
    LLStringUtil::replaceChar(desc, '|', ' ');
}

void LLInventoryItem::setDescription(const std::string& d)
{
    std::string new_desc(d);
    LLInventoryItem::correctInventoryDescription(new_desc);
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

time_t LLInventoryItem::getCreationDate() const
{
    return mCreationDate;
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
bool LLInventoryItem::unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num)
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
        LL_DEBUGS() << "crc matches" << LL_ENDL;
        return true;
    }
    else
    {
        LL_WARNS() << "inventory crc mismatch: local=" << std::hex << local_crc
                << " remote=" << remote_crc << std::dec << LL_ENDL;
        return false;
    }
#else
    return (local_crc == remote_crc);
#endif
}

// virtual
bool LLInventoryItem::importLegacyStream(std::istream& input_stream)
{
    // *NOTE: Changing the buffer size will require changing the scanf
    // calls below.
    char buffer[MAX_STRING];    /* Flawfinder: ignore */
    char keyword[MAX_STRING];   /* Flawfinder: ignore */
    char valuestr[MAX_STRING];  /* Flawfinder: ignore */
    char junk[MAX_STRING];  /* Flawfinder: ignore */
    bool success = true;

    keyword[0] = '\0';
    valuestr[0] = '\0';

    mInventoryType = LLInventoryType::IT_NONE;
    mAssetUUID.setNull();
    while(success && input_stream.good())
    {
        input_stream.getline(buffer, MAX_STRING);
        sscanf( /* Flawfinder: ignore */
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
            success = mPermissions.importLegacyStream(input_stream);
        }
        else if(0 == strcmp("sale_info", keyword))
        {
            // Sale info used to contain next owner perm. It is now in
            // the permissions. Thus, we read that out, and fix legacy
            // objects. It's possible this op would fail, but it
            // should pick up the vast majority of the tasks.
            bool has_perm_mask = false;
            U32 perm_mask = 0;
            success = mSaleInfo.importLegacyStream(input_stream, has_perm_mask, perm_mask);
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
        else if (0 == strcmp("metadata", keyword))
        {
            LLSD metadata;
            if (strncmp("<llsd>", valuestr, 6) == 0)
            {
                std::istringstream stream(valuestr);
                LLSDSerialize::fromXML(metadata, stream);
            }
            else
            {
                // next line likely contains metadata, but at the moment is not supported
                // can do something like:
                // LLSDSerialize::fromNotation(metadata, input_stream, -1);
            }

            if (metadata.has("thumbnail"))
            {
                const LLSD& thumbnail = metadata["thumbnail"];
                if (thumbnail.has("asset_id"))
                {
                    setThumbnailUUID(thumbnail["asset_id"].asUUID());
                }
                else
                {
                    setThumbnailUUID(LLUUID::null);
                }
            }
            else
            {
                setThumbnailUUID(LLUUID::null);
            }
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
            sscanf( /* Flawfinder: ignore */
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
            sscanf( /* Flawfinder: ignore */
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
                LL_ERRS() << "Donkey" << LL_ENDL;
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
            LL_WARNS() << "unknown keyword '" << keyword
                    << "' in inventory import of item " << mUUID << LL_ENDL;
        }
    }

    // Need to convert 1.0 simstate files to a useful inventory type
    // and potentially deal with bad inventory tyes eg, a landmark
    // marked as a texture.
    if((LLInventoryType::IT_NONE == mInventoryType)
       || !inventory_and_asset_types_match(mInventoryType, mType))
    {
        LL_DEBUGS() << "Resetting inventory type for " << mUUID << LL_ENDL;
        mInventoryType = LLInventoryType::defaultForAssetType(mType);
    }

    mPermissions.initMasks(mInventoryType);

    return success;
}

bool LLInventoryItem::exportLegacyStream(std::ostream& output_stream, bool include_asset_key) const
{
    std::string uuid_str;
    output_stream << "\tinv_item\t0\n\t{\n";
    mUUID.toString(uuid_str);
    output_stream << "\t\titem_id\t" << uuid_str << "\n";
    mParentUUID.toString(uuid_str);
    output_stream << "\t\tparent_id\t" << uuid_str << "\n";
    mPermissions.exportLegacyStream(output_stream);

    if (mThumbnailUUID.notNull())
    {
        // Max length is 255 chars, will have to export differently if it gets more data
        // Ex: use newline and toNotation (uses {}) for unlimited size
        LLSD metadata;
        metadata["thumbnail"] = LLSD().with("asset_id", mThumbnailUUID);

        output_stream << "\t\tmetadata\t";
        LLSDSerialize::toXML(metadata, output_stream);
        output_stream << "|\n";
    }

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
    mSaleInfo.exportLegacyStream(output_stream);
    output_stream << "\t\tname\t" << mName.c_str() << "|\n";
    output_stream << "\t\tdesc\t" << mDescription.c_str() << "|\n";
    output_stream << "\t\tcreation_date\t" << mCreationDate << "\n";
    output_stream << "\t}\n";
    return true;
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

    if (mThumbnailUUID.notNull())
    {
        sd[INV_THUMBNAIL_LABEL] = LLSD().with(INV_ASSET_ID_LABEL, mThumbnailUUID);
    }

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

bool LLInventoryItem::fromLLSD(const LLSD& sd, bool is_new)
{
    LL_PROFILE_ZONE_SCOPED;
    if (is_new)
    {
        // If we're adding LLSD to an existing object, need avoid
        // clobbering these fields.
        mInventoryType = LLInventoryType::IT_NONE;
        mAssetUUID.setNull();
    }

    // TODO - figure out if this should be moved into the noclobber fields above
    mThumbnailUUID.setNull();

    // iterate as map to avoid making unnecessary temp copies of everything
    LLSD::map_const_iterator i, end;
    end = sd.endMap();
    for (i = sd.beginMap(); i != end; ++i)
    {
        if (i->first == INV_ITEM_ID_LABEL)
        {
            mUUID = i->second;
            continue;
        }

        if (i->first == INV_PARENT_ID_LABEL)
        {
            mParentUUID = i->second;
            continue;
        }

        if (i->first == INV_THUMBNAIL_LABEL)
        {
            const LLSD &thumbnail_map = i->second;
            const std::string w = INV_ASSET_ID_LABEL;
            if (thumbnail_map.has(w))
            {
                mThumbnailUUID = thumbnail_map[w];
            }
            /* Example:
                <key> asset_id </key>
                <uuid> acc0ec86 - 17f2 - 4b92 - ab41 - 6718b1f755f7 </uuid>
                <key> perms </key>
                <integer> 8 </integer>
                <key>service</key>
                <integer> 3 </integer>
                <key>version</key>
                <integer> 1 </key>
            */
          continue;
      }

        if (i->first == INV_THUMBNAIL_ID_LABEL)
        {
            mThumbnailUUID = i->second.asUUID();
            continue;
        }

        if (i->first == INV_PERMISSIONS_LABEL)
        {
            mPermissions = ll_permissions_from_sd(i->second);
            continue;
        }

        if (i->first == INV_SALE_INFO_LABEL)
        {
            // Sale info used to contain next owner perm. It is now in
            // the permissions. Thus, we read that out, and fix legacy
            // objects. It's possible this op would fail, but it
            // should pick up the vast majority of the tasks.
            bool has_perm_mask = false;
            U32  perm_mask     = 0;
            if (!mSaleInfo.fromLLSD(i->second, has_perm_mask, perm_mask))
            {
                return false;
            }
            if (has_perm_mask)
            {
                if (perm_mask == PERM_NONE)
                {
                    perm_mask = mPermissions.getMaskOwner();
                }
                // fair use fix.
                if (!(perm_mask & PERM_COPY))
                {
                    perm_mask |= PERM_TRANSFER;
                }
                mPermissions.setMaskNext(perm_mask);
            }
            continue;
        }

        if (i->first == INV_SHADOW_ID_LABEL)
        {
            mAssetUUID = i->second;
            LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
            cipher.decrypt(mAssetUUID.mData, UUID_BYTES);
            continue;
        }

        if (i->first == INV_ASSET_ID_LABEL)
        {
            mAssetUUID = i->second;
            continue;
        }

        if (i->first == INV_LINKED_ID_LABEL)
        {
            mAssetUUID = i->second;
            continue;
        }

        if (i->first == INV_ASSET_TYPE_LABEL)
        {
            LLSD const &label = i->second;
            if (label.isString())
            {
                mType = LLAssetType::lookup(label.asString().c_str());
            }
            else if (label.isInteger())
            {
                S8 type = (U8) label.asInteger();
                mType   = static_cast<LLAssetType::EType>(type);
            }
            continue;
        }

        if (i->first == INV_INVENTORY_TYPE_LABEL)
        {
            LLSD const &label = i->second;
            if (label.isString())
            {
                mInventoryType = LLInventoryType::lookup(label.asString().c_str());
            }
            else if (label.isInteger())
            {
                S8 type        = (U8) label.asInteger();
                mInventoryType = static_cast<LLInventoryType::EType>(type);
            }
            continue;
        }

        if (i->first == INV_FLAGS_LABEL)
        {
            LLSD const &label = i->second;
            if (label.isBinary())
            {
                mFlags = ll_U32_from_sd(label);
            }
            else if (label.isInteger())
            {
                mFlags = label.asInteger();
            }
            continue;
        }

        if (i->first == INV_NAME_LABEL)
        {
            mName = i->second.asString();
            LLStringUtil::replaceNonstandardASCII(mName, ' ');
            LLStringUtil::replaceChar(mName, '|', ' ');
            continue;
        }

        if (i->first == INV_DESC_LABEL)
        {
            mDescription = i->second.asString();
            LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
            continue;
        }

        if (i->first == INV_CREATION_DATE_LABEL)
        {
            mCreationDate = i->second.asInteger();
            continue;
        }
    }

    // Need to convert 1.0 simstate files to a useful inventory type
    // and potentially deal with bad inventory tyes eg, a landmark
    // marked as a texture.
    if((LLInventoryType::IT_NONE == mInventoryType)
       || !inventory_and_asset_types_match(mInventoryType, mType))
    {
        LL_DEBUGS() << "Resetting inventory type for " << mUUID << LL_ENDL;
        mInventoryType = LLInventoryType::defaultForAssetType(mType);
    }

    mPermissions.initMasks(mInventoryType);

    return true;
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
    sd[INV_ITEM_ID_LABEL]   = mUUID;
    sd[INV_PARENT_ID_LABEL] = mParentUUID;
    S8 type = static_cast<S8>(mPreferredType);
    sd[INV_ASSET_TYPE_LABEL] = type;
    sd[INV_NAME_LABEL] = mName;

    if (mThumbnailUUID.notNull())
    {
        sd[INV_THUMBNAIL_LABEL] = LLSD().with(INV_ASSET_ID_LABEL, mThumbnailUUID);
    }

    return sd;
}

LLSD LLInventoryCategory::asAISCreateCatLLSD() const
{
    LLSD sd                 = LLSD();
    sd[INV_FOLDER_ID_LABEL_WS]  = mUUID;
    sd[INV_PARENT_ID_LABEL] = mParentUUID;
    S8 type                 = static_cast<S8>(mPreferredType);
    sd[INV_ASSET_TYPE_LABEL_WS] = type;
    sd[INV_NAME_LABEL] = mName;
    if (mThumbnailUUID.notNull())
    {
        sd[INV_THUMBNAIL_LABEL] = LLSD().with(INV_ASSET_ID_LABEL, mThumbnailUUID);
    }

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
    mThumbnailUUID.setNull();
    w = INV_THUMBNAIL_LABEL;
    if (sd.has(w))
    {
        const LLSD &thumbnail_map = sd[w];
        w = INV_ASSET_ID_LABEL;
        if (thumbnail_map.has(w))
        {
            mThumbnailUUID = thumbnail_map[w];
        }
    }
    else
    {
        w = INV_THUMBNAIL_ID_LABEL;
        if (sd.has(w))
        {
            mThumbnailUUID = sd[w];
        }
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
bool LLInventoryCategory::importLegacyStream(std::istream& input_stream)
{
    // *NOTE: Changing the buffer size will require changing the scanf
    // calls below.
    char buffer[MAX_STRING];    /* Flawfinder: ignore */
    char keyword[MAX_STRING];   /* Flawfinder: ignore */
    char valuestr[MAX_STRING];  /* Flawfinder: ignore */

    keyword[0] = '\0';
    valuestr[0] = '\0';
    while(input_stream.good())
    {
        input_stream.getline(buffer, MAX_STRING);
        sscanf( /* Flawfinder: ignore */
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
            sscanf( /* Flawfinder: ignore */
                buffer,
                " %254s %254[^|]",
                keyword, valuestr);
            mName.assign(valuestr);
            LLStringUtil::replaceNonstandardASCII(mName, ' ');
            LLStringUtil::replaceChar(mName, '|', ' ');
        }
        else if (0 == strcmp("metadata", keyword))
        {
            LLSD metadata;
            if (strncmp("<llsd>", valuestr, 6) == 0)
            {
                std::istringstream stream(valuestr);
                LLSDSerialize::fromXML(metadata, stream);
            }
            else
            {
                // next line likely contains metadata, but at the moment is not supported
                // can do something like:
                // LLSDSerialize::fromNotation(metadata, input_stream, -1);
            }

            if (metadata.has("thumbnail"))
            {
                const LLSD& thumbnail = metadata["thumbnail"];
                if (thumbnail.has("asset_id"))
                {
                    setThumbnailUUID(thumbnail["asset_id"].asUUID());
                }
                else
                {
                    setThumbnailUUID(LLUUID::null);
                }
            }
            else
            {
                setThumbnailUUID(LLUUID::null);
            }
        }
        else
        {
            LL_WARNS() << "unknown keyword '" << keyword
                    << "' in inventory import category "  << mUUID << LL_ENDL;
        }
    }
    return true;
}

bool LLInventoryCategory::exportLegacyStream(std::ostream& output_stream, bool) const
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
    if (mThumbnailUUID.notNull())
    {
        // Only up to 255 chars
        LLSD metadata;
        metadata["thumbnail"] = LLSD().with("asset_id", mThumbnailUUID);
        output_stream << "\t\tmetadata\t";
        LLSDSerialize::toXML(metadata, output_stream);
        output_stream << "|\n";
    }
    output_stream << "\t}\n";
    return true;
}

LLSD LLInventoryCategory::exportLLSD() const
{
    LLSD cat_data;
    cat_data[INV_FOLDER_ID_LABEL] = mUUID;
    cat_data[INV_PARENT_ID_LABEL] = mParentUUID;
    cat_data[INV_ASSET_TYPE_LABEL] = LLAssetType::lookup(mType);
    cat_data[INV_PREFERRED_TYPE_LABEL] = LLFolderType::lookup(mPreferredType);
    cat_data[INV_NAME_LABEL] = mName;

    if (mThumbnailUUID.notNull())
    {
        cat_data[INV_THUMBNAIL_LABEL] = LLSD().with(INV_ASSET_ID_LABEL, mThumbnailUUID);
    }

    return cat_data;
}

bool LLInventoryCategory::importLLSD(const LLSD& cat_data)
{
    if (cat_data.has(INV_FOLDER_ID_LABEL))
    {
        setUUID(cat_data[INV_FOLDER_ID_LABEL].asUUID());
    }
    if (cat_data.has(INV_PARENT_ID_LABEL))
    {
        setParent(cat_data[INV_PARENT_ID_LABEL].asUUID());
    }
    if (cat_data.has(INV_ASSET_TYPE_LABEL))
    {
        setType(LLAssetType::lookup(cat_data[INV_ASSET_TYPE_LABEL].asString()));
    }
    if (cat_data.has(INV_PREFERRED_TYPE_LABEL))
    {
        setPreferredType(LLFolderType::lookup(cat_data[INV_PREFERRED_TYPE_LABEL].asString()));
    }
    if (cat_data.has(INV_THUMBNAIL_LABEL))
    {
        LLUUID thumbnail_uuid;
        const LLSD &thumbnail_data = cat_data[INV_THUMBNAIL_LABEL];
        if (thumbnail_data.has(INV_ASSET_ID_LABEL))
        {
            thumbnail_uuid = thumbnail_data[INV_ASSET_ID_LABEL].asUUID();
        }
        setThumbnailUUID(thumbnail_uuid);
    }
    if (cat_data.has(INV_NAME_LABEL))
    {
        mName = cat_data[INV_NAME_LABEL].asString();
        LLStringUtil::replaceNonstandardASCII(mName, ' ');
        LLStringUtil::replaceChar(mName, '|', ' ');
    }

    return true;
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
        LL_WARNS() << "ll_create_sd_from_inventory_item() for item with AT_NONE"
            << LL_ENDL;
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
        LL_WARNS() << "ll_create_sd_from_inventory_category() for cat with AT_NONE"
            << LL_ENDL;
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
