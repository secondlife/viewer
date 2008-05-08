/** 
 * @file llinventory.h
 * @brief LLInventoryItem and LLInventoryCategory class declaration.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLINVENTORY_H
#define LL_LLINVENTORY_H

#include <functional>

#include "llassetstorage.h"
#include "lldarray.h"
#include "llinventorytype.h"
#include "llmemtype.h"
#include "llpermissions.h"
#include "llsaleinfo.h"
#include "llsd.h"
#include "lluuid.h"
#include "llxmlnode.h"

// consts for Key field in the task inventory update message
extern const U8 TASK_INVENTORY_ITEM_KEY;
extern const U8 TASK_INVENTORY_ASSET_KEY;

// anonymous enumeration to specify a max inventory buffer size for
// use in packBinaryBucket()
enum
{
	MAX_INVENTORY_BUFFER_SIZE = 1024
};



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryObject
//
// This is the base class for inventory objects that handles the
// common code between items and categories. The 'mParentUUID' member
// means the parent category since all inventory objects except each
// user's root category are in some category. Each user's root
// category will have mParentUUID==LLUUID::null.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMessageSystem;

class LLInventoryObject : public LLRefCount
{
protected:
	LLUUID mUUID;
	LLUUID mParentUUID;
	LLAssetType::EType mType;
	LLString mName;

protected:
	virtual ~LLInventoryObject( void );
	
public:
	MEM_TYPE_NEW(LLMemType::MTYPE_INVENTORY);
	LLInventoryObject(const LLUUID& uuid, const LLUUID& parent_uuid,
					  LLAssetType::EType type, const LLString& name);
	LLInventoryObject();
	void copyObject(const LLInventoryObject* other); // LLRefCount requires custom copy

	// accessors
	const LLUUID& getUUID() const;
	const LLUUID& getParentUUID() const;
	const LLString& getName() const;
	LLAssetType::EType getType() const;

	// mutators - will not call updateServer();
	void setUUID(const LLUUID& new_uuid);
	void rename(const LLString& new_name);
	void setParent(const LLUUID& new_parent);
	void setType(LLAssetType::EType type);

	// file support - implemented here so that a minimal information
	// set can be transmitted between simulator and viewer.
// 	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;

	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;

	// virtual methods
	virtual void removeFromServer();
	virtual void updateParentOnServer(BOOL) const;
	virtual void updateServer(BOOL) const;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryItem
//
// An inventory item represents something that the current user has in
// their inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryItem : public LLInventoryObject
{
public:
	typedef LLDynamicArray<LLPointer<LLInventoryItem> > item_array_t;
	
protected:
	LLPermissions mPermissions;
	LLUUID mAssetUUID;
	LLString mDescription;
	LLSaleInfo mSaleInfo;
	LLInventoryType::EType mInventoryType;
	U32 mFlags;
	S32 mCreationDate;	// seconds from 1/1/1970, UTC

public:

	/**
	 * Anonymous enumeration for specifying the inventory item flags.
	 */
	enum
	{
		// The shared flags at the top are shared among all inventory
		// types. After that section, all values of flags are type
		// dependent.  The shared flags will start at 2^30 and work
		// down while item type specific flags will start at 2^0 and
		// work up.
		II_FLAGS_NONE = 0,


		//
		// Shared flags
		//
		//

		// This value means that the asset has only one reference in
		// the system. If the inventory item is deleted, or the asset
		// id updated, then we can remove the old reference.
		II_FLAGS_SHARED_SINGLE_REFERENCE = 0x40000000,


		//
		// Landmark flags
		//
		II_FLAGS_LANDMARK_VISITED = 1,

		//
		// Object flags
		//

		// flag to indicate that object permissions should have next
		// owner perm be more restrictive on rez. We bump this into
		// the second byte of the flags since the low byte is used to
		// track attachment points.
		II_FLAGS_OBJECT_SLAM_PERM = 0x100,

		// flag to indicate that the object sale information has been changed.
		II_FLAGS_OBJECT_SLAM_SALE = 0x1000,

		// These flags specify which permissions masks to overwrite
		// upon rez.  Normally, if no permissions slam (above) or
		// overwrite flags are set, the asset's permissions are
		// used and the inventory's permissions are ignored.  If
		// any of these flags are set, the inventory's permissions
		// take precedence.
		II_FLAGS_OBJECT_PERM_OVERWRITE_BASE			= 0x010000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_OWNER		= 0x020000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP		= 0x040000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE		= 0x080000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_NEXT_OWNER	= 0x100000,

 		// flag to indicate whether an object that is returned is composed 
		// of muiltiple items or not.
		II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS			= 0x200000,

		//
		// wearables use the low order byte of flags to store the
		// EWearableType enumeration found in newview/llwearable.h
		//
		II_FLAGS_WEARABLES_MASK = 0xff,
	};

protected:
	~LLInventoryItem(); // ref counted

public:
	MEM_TYPE_NEW(LLMemType::MTYPE_INVENTORY);
	LLInventoryItem(const LLUUID& uuid,
					const LLUUID& parent_uuid,
					const LLPermissions& permissions,
					const LLUUID& asset_uuid,
					LLAssetType::EType type,
					LLInventoryType::EType inv_type,
					const LLString& name, 
					const LLString& desc,
					const LLSaleInfo& sale_info,
					U32 flags,
					S32 creation_date_utc);
	LLInventoryItem();
	// Create a copy of an inventory item from a pointer to another item
	// Note: Because InventoryItems are ref counted, reference copy (a = b)
	// is prohibited
	LLInventoryItem(const LLInventoryItem* other);
	virtual void copyItem(const LLInventoryItem* other); // LLRefCount requires custom copy

	// As a constructor alternative, the clone() method works like a
	// copy constructor, but gens a new UUID.
	// It is up to the caller to delete (unref) the item.
	virtual void cloneItem(LLPointer<LLInventoryItem>& newitem) const;
	
	// accessors
	const LLPermissions& getPermissions() const;
	const LLUUID& getCreatorUUID() const;
	const LLUUID& getAssetUUID() const;
	const LLString& getDescription() const;
	const LLSaleInfo& getSaleInfo() const;
	LLInventoryType::EType getInventoryType() const;
	U32 getFlags() const;
	S32 getCreationDate() const;
	U32 getCRC32() const; // really more of a checksum.
	
	// mutators - will not call updateServer(), and will never fail
	// (though it may correct to sane values)
	void setAssetUUID(const LLUUID& asset_id);
	void setDescription(const LLString& new_desc);
	void setSaleInfo(const LLSaleInfo& sale_info);
	void setPermissions(const LLPermissions& perm);
	void setInventoryType(LLInventoryType::EType inv_type);
	void setFlags(U32 flags);
	void setCreationDate(S32 creation_date_utc);

	// Put this inventory item onto the current outgoing mesage. It
	// assumes you have already called nextBlock().
	virtual void packMessage(LLMessageSystem* msg) const;

	// unpack returns TRUE if the inventory item came through the
	// network ok. It uses a simple crc check which is defeatable, but
	// we want to detect network mangling somehow.
	virtual BOOL unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
	// file support
	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;

	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;

	virtual LLXMLNode *exportFileXML(BOOL include_asset_key = TRUE) const;
	BOOL importXML(LLXMLNode* node);

	// helper functions

	// pack all information needed to reconstruct this item into the given binary bucket.
	// perm_override is optional
	S32 packBinaryBucket(U8* bin_bucket, LLPermissions* perm_override = NULL) const;
	void unpackBinaryBucket(U8* bin_bucket, S32 bin_bucket_size);
	LLSD asLLSD() const;
	bool fromLLSD(LLSD& sd);

};

BOOL item_dictionary_sort(LLInventoryItem* a,LLInventoryItem* b);
BOOL item_date_sort(LLInventoryItem* a, LLInventoryItem* b);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCategory
//
// An instance of this class represents a category of inventory
// items. Users come with a set of default categories, and can create
// new ones as needed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryCategory : public LLInventoryObject
{
public:
	typedef LLDynamicArray<LLPointer<LLInventoryCategory> > cat_array_t;

protected:
	~LLInventoryCategory();
	
public:
	MEM_TYPE_NEW(LLMemType::MTYPE_INVENTORY);
	LLInventoryCategory(const LLUUID& uuid, const LLUUID& parent_uuid,
						LLAssetType::EType preferred_type,
						const LLString& name);
	LLInventoryCategory();
	LLInventoryCategory(const LLInventoryCategory* other);
	void copyCategory(const LLInventoryCategory* other); // LLRefCount requires custom copy

	// accessors and mutators
	LLAssetType::EType getPreferredType() const;
	void setPreferredType(LLAssetType::EType type);
	// For messaging system support
	virtual void packMessage(LLMessageSystem* msg) const;
	virtual void unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);

	LLSD asLLSD() const;
	bool fromLLSD(LLSD& sd);

	// file support
	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL exportFile(LLFILE* fp, BOOL include_asset_key = TRUE) const;

	virtual BOOL importLegacyStream(std::istream& input_stream);
	virtual BOOL exportLegacyStream(std::ostream& output_stream, BOOL include_asset_key = TRUE) const;

protected:
	// The type of asset that this category was "meant" to hold
	// (although it may in fact hold any type).
	LLAssetType::EType	mPreferredType;		

};


//-----------------------------------------------------------------------------
// Useful bits
//-----------------------------------------------------------------------------

// This functor tests if an item is transferrable and returns true if
// it is. Derived from unary_function<> so that the object can be used
// in stl-compliant adaptable predicates (eg, not1<>). You might want
// to use this in std::partition() or similar logic.
struct IsItemTransferable : public std::unary_function<LLInventoryItem*, bool>
{
	LLUUID mDestID;
	IsItemTransferable(const LLUUID& dest_id) : mDestID(dest_id) {}
	bool operator()(const LLInventoryItem* item) const
	{
		return (item->getPermissions().allowTransferTo(mDestID)) ? true:false;
	}
};

// This functor is used to set the owner and group of inventory items,
// for example, in a simple std::for_each() loop. Note that the call
// to setOwnerAndGroup can fail if authority_id != LLUUID::null.
struct SetItemOwnerAndGroup
{
	LLUUID mAuthorityID;
	LLUUID mOwnerID;
	LLUUID mGroupID;
	SetItemOwnerAndGroup(const LLUUID& authority_id,
						 const LLUUID& owner_id,
						 const LLUUID& group_id) :
		mAuthorityID(authority_id), mOwnerID(owner_id), mGroupID(group_id) {}
	void operator()(LLInventoryItem* item) const
	{
		LLPermissions perm = item->getPermissions();
		bool is_atomic = (LLAssetType::AT_OBJECT == item->getType()) ? false : true;
		perm.setOwnerAndGroup(mAuthorityID, mOwnerID, mGroupID, is_atomic);
		// If no owner id is set, this is equivalent to a deed action.
		// Clear 'share with group'.
		if (mOwnerID.isNull())
		{
			perm.setMaskGroup(PERM_NONE);
		}
		item->setPermissions(perm);
	}
};

// This functor is used to unset the share with group, everyone perms, and
// for sale info for objects being sold through contents.
struct SetNotForSale
{
	LLUUID mAgentID;
	LLUUID mGroupID;
	SetNotForSale(const LLUUID& agent_id,
				  const LLUUID& group_id) :
			mAgentID(agent_id), mGroupID(group_id) {}
	void operator()(LLInventoryItem* item) const
	{
		// Clear group & everyone permissions.
		LLPermissions perm = item->getPermissions();
		perm.setGroupBits(mAgentID, mGroupID, FALSE, PERM_MODIFY | PERM_MOVE | PERM_COPY);
		perm.setEveryoneBits(mAgentID, mGroupID, FALSE, PERM_MOVE | PERM_COPY);
		item->setPermissions(perm);

		// Mark group & everyone permissions for overwrite on the next
		// rez if it is an object.
		if(LLAssetType::AT_OBJECT == item->getType())
		{
			U32 flags = item->getFlags();
			flags |= LLInventoryItem::II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
			flags |= LLInventoryItem::II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
			item->setFlags(flags);
		}

		// Clear for sale info.
		item->setSaleInfo(LLSaleInfo::DEFAULT);
	}
};

typedef std::list<LLPointer<LLInventoryObject> > InventoryObjectList;

// These functions convert between structured data and an inventroy
// item, appropriate for serialization.
LLSD ll_create_sd_from_inventory_item(LLPointer<LLInventoryItem> item);
LLPointer<LLInventoryItem> ll_create_item_from_sd(const LLSD& sd_item);
LLSD ll_create_sd_from_inventory_category(LLPointer<LLInventoryCategory> cat);
LLPointer<LLInventoryCategory> ll_create_category_from_sd(const LLSD& sd_cat);

#endif // LL_LLINVENTORY_H
