/** 
 * @file llinventory.h
 * @brief LLInventoryItem and LLInventoryCategory class declaration.
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

#ifndef LL_LLINVENTORY_H
#define LL_LLINVENTORY_H

#include "lldarray.h"
#include "llfoldertype.h"
#include "llinventorytype.h"
#include "llpermissions.h"
#include "llrefcount.h"
#include "llsaleinfo.h"
#include "llsd.h"
#include "lluuid.h"

class LLMessageSystem;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryObject
//
//   Base class for anything in the user's inventory.   Handles the common code 
//   between items and categories. 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryObject : public LLRefCount
{
public:
	typedef std::list<LLPointer<LLInventoryObject> > object_list_t;

	//--------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------
public:
	LLInventoryObject();
	LLInventoryObject(const LLUUID& uuid, 
					  const LLUUID& parent_uuid,
					  LLAssetType::EType type, 
					  const std::string& name);
	void copyObject(const LLInventoryObject* other); // LLRefCount requires custom copy
protected:
	virtual ~LLInventoryObject();

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	virtual const LLUUID& getUUID() const; // inventoryID that this item points to
	virtual const LLUUID& getLinkedUUID() const; // inventoryID that this item points to, else this item's inventoryID
	const LLUUID& getParentUUID() const;
	virtual const std::string& getName() const;
	virtual LLAssetType::EType getType() const;
	LLAssetType::EType getActualType() const; // bypasses indirection for linked items
	BOOL getIsLinkType() const;
	
	//--------------------------------------------------------------------
	// Mutators
	//   Will not call updateServer
	//--------------------------------------------------------------------
public:
	void setUUID(const LLUUID& new_uuid);
	virtual void rename(const std::string& new_name);
	void setParent(const LLUUID& new_parent);
	void setType(LLAssetType::EType type);

private:
	// in place correction for inventory name string
	void correctInventoryName(std::string& name);

	//--------------------------------------------------------------------
	// File Support
	//   Implemented here so that a minimal information set can be transmitted
	//   between simulator and viewer.
	//--------------------------------------------------------------------
public:
	// virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;
	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;

	virtual void removeFromServer();
	virtual void updateParentOnServer(BOOL) const;
	virtual void updateServer(BOOL) const;

	//--------------------------------------------------------------------
	// Member Variables
	//--------------------------------------------------------------------
protected:
	LLUUID mUUID;
	LLUUID mParentUUID; // Parent category.  Root categories have LLUUID::NULL.
	LLAssetType::EType mType;
	std::string mName;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryItem
//
//   An item in the current user's inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryItem : public LLInventoryObject
{
public:
	typedef LLDynamicArray<LLPointer<LLInventoryItem> > item_array_t;

	//--------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------
public:
	LLInventoryItem(const LLUUID& uuid,
					const LLUUID& parent_uuid,
					const LLPermissions& permissions,
					const LLUUID& asset_uuid,
					LLAssetType::EType type,
					LLInventoryType::EType inv_type,
					const std::string& name, 
					const std::string& desc,
					const LLSaleInfo& sale_info,
					U32 flags,
					S32 creation_date_utc);
	LLInventoryItem();
	// Create a copy of an inventory item from a pointer to another item
	// Note: Because InventoryItems are ref counted, reference copy (a = b)
	// is prohibited
	LLInventoryItem(const LLInventoryItem* other);
	virtual void copyItem(const LLInventoryItem* other); // LLRefCount requires custom copy
	void generateUUID() { mUUID.generate(); }
protected:
	~LLInventoryItem(); // ref counted
	
	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	virtual const LLUUID& getLinkedUUID() const;
	virtual const LLPermissions& getPermissions() const;
	virtual const LLUUID& getCreatorUUID() const;
	virtual const LLUUID& getAssetUUID() const;
	virtual const std::string& getDescription() const;
	virtual const LLSaleInfo& getSaleInfo() const;
	virtual LLInventoryType::EType getInventoryType() const;
	virtual U32 getFlags() const;
	virtual time_t getCreationDate() const;
	virtual U32 getCRC32() const; // really more of a checksum.
	
	//--------------------------------------------------------------------
	// Mutators
	//   Will not call updateServer and will never fail
	//   (though it may correct to sane values)
	//--------------------------------------------------------------------
public:
	void setAssetUUID(const LLUUID& asset_id);
	void setDescription(const std::string& new_desc);
	void setSaleInfo(const LLSaleInfo& sale_info);
	void setPermissions(const LLPermissions& perm);
	void setInventoryType(LLInventoryType::EType inv_type);
	void setFlags(U32 flags);
	void setCreationDate(time_t creation_date_utc);
	void setCreator(const LLUUID& creator); // only used for calling cards

	// Check for changes in permissions masks and sale info
	// and set the corresponding bits in mFlags.
	void accumulatePermissionSlamBits(const LLInventoryItem& old_item);

	// Put this inventory item onto the current outgoing mesage.
	// Assumes you have already called nextBlock().
	virtual void packMessage(LLMessageSystem* msg) const;

	// Returns TRUE if the inventory item came through the network correctly.
	// Uses a simple crc check which is defeatable, but we want to detect 
	// network mangling somehow.
	virtual BOOL unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);

	//--------------------------------------------------------------------
	// File Support
	//--------------------------------------------------------------------
public:
	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;
	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;

	//--------------------------------------------------------------------
	// Helper Functions
	//--------------------------------------------------------------------
public:
	// Pack all information needed to reconstruct this item into the given binary bucket.
	S32 packBinaryBucket(U8* bin_bucket, LLPermissions* perm_override = NULL) const;
	void unpackBinaryBucket(U8* bin_bucket, S32 bin_bucket_size);
	LLSD asLLSD() const;
	void asLLSD( LLSD& sd ) const;
	bool fromLLSD(const LLSD& sd);

	//--------------------------------------------------------------------
	// Member Variables
	//--------------------------------------------------------------------
protected:
	LLPermissions mPermissions;
	LLUUID mAssetUUID;
	std::string mDescription;
	LLSaleInfo mSaleInfo;
	LLInventoryType::EType mInventoryType;
	U32 mFlags;
	time_t mCreationDate; // seconds from 1/1/1970, UTC
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCategory
//
//   A category/folder of inventory items. Users come with a set of default 
//   categories, and can create new ones as needed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCategory : public LLInventoryObject
{
public:
	typedef LLDynamicArray<LLPointer<LLInventoryCategory> > cat_array_t;

	//--------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------
public:
	LLInventoryCategory(const LLUUID& uuid, const LLUUID& parent_uuid,
						LLFolderType::EType preferred_type,
						const std::string& name);
	LLInventoryCategory();
	LLInventoryCategory(const LLInventoryCategory* other);
	void copyCategory(const LLInventoryCategory* other); // LLRefCount requires custom copy
protected:
	~LLInventoryCategory();

	//--------------------------------------------------------------------
	// Accessors And Mutators
	//--------------------------------------------------------------------
public:
	LLFolderType::EType getPreferredType() const;
	void setPreferredType(LLFolderType::EType type);
	LLSD asLLSD() const;
	bool fromLLSD(const LLSD& sd);

	//--------------------------------------------------------------------
	// Messaging
	//--------------------------------------------------------------------
public:
	virtual void packMessage(LLMessageSystem* msg) const;
	virtual void unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);

	//--------------------------------------------------------------------
	// File Support
	//--------------------------------------------------------------------
public:
	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;
	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;

	//--------------------------------------------------------------------
	// Member Variables
	//--------------------------------------------------------------------
protected:
	LLFolderType::EType	mPreferredType; // Type that this category was "meant" to hold (although it may hold any type).	
};


//-----------------------------------------------------------------------------
// Convertors
//
//   These functions convert between structured data and an inventory
//   item, appropriate for serialization.
//-----------------------------------------------------------------------------
LLSD ll_create_sd_from_inventory_item(LLPointer<LLInventoryItem> item);
LLSD ll_create_sd_from_inventory_category(LLPointer<LLInventoryCategory> cat);
LLPointer<LLInventoryCategory> ll_create_category_from_sd(const LLSD& sd_cat);

#endif // LL_LLINVENTORY_H
