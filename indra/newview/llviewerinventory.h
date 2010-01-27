/** 
 * @file llviewerinventory.h
 * @brief Declaration of the inventory bits that only used on the viewer.
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

#ifndef LL_LLVIEWERINVENTORY_H
#define LL_LLVIEWERINVENTORY_H

#include "llinventory.h"
#include "llframetimer.h"
#include "llwearable.h"

#include <boost/signals2.hpp>	// boost::signals2::trackable

class LLFolderView;
class LLFolderBridge;
class LLViewerInventoryCategory;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLViewerInventoryItem
//
// An inventory item represents something that the current user has in
// their inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLViewerInventoryItem : public LLInventoryItem, public boost::signals2::trackable
{
public:
	typedef LLDynamicArray<LLPointer<LLViewerInventoryItem> > item_array_t;
	
protected:
	~LLViewerInventoryItem( void ); // ref counted
	BOOL extractSortFieldAndDisplayName(S32* sortField, std::string* displayName) const { return extractSortFieldAndDisplayName(mName, sortField, displayName); }
	mutable std::string mDisplayName;
	
public:
	virtual LLAssetType::EType getType() const;
	virtual const LLUUID& getAssetUUID() const;
	virtual const std::string& getName() const;
	virtual const std::string& getDisplayName() const;
	static std::string getDisplayName(const std::string& name);
	static char getSeparator() { return '@'; }
	virtual S32 getSortField() const;
	virtual void setSortField(S32 sortField);
	virtual void rename(const std::string& new_name);
	virtual const LLPermissions& getPermissions() const;
	virtual const LLUUID& getCreatorUUID() const;
	virtual const std::string& getDescription() const;
	virtual const LLSaleInfo& getSaleInfo() const;
	virtual LLInventoryType::EType getInventoryType() const;
	virtual bool isWearableType() const;
	virtual EWearableType getWearableType() const;
	virtual U32 getFlags() const;
	virtual time_t getCreationDate() const;
	virtual U32 getCRC32() const; // really more of a checksum.

	static BOOL extractSortFieldAndDisplayName(const std::string& name, S32* sortField, std::string* displayName);

	// construct a complete viewer inventory item
	LLViewerInventoryItem(const LLUUID& uuid, const LLUUID& parent_uuid,
						  const LLPermissions& permissions,
						  const LLUUID& asset_uuid,
						  LLAssetType::EType type,
						  LLInventoryType::EType inv_type,
						  const std::string& name, 
						  const std::string& desc,
						  const LLSaleInfo& sale_info,
						  U32 flags,
						  time_t creation_date_utc);

	// construct a viewer inventory item which has the minimal amount
	// of information to use in the UI.
	LLViewerInventoryItem(
		const LLUUID& item_id,
		const LLUUID& parent_id,
		const std::string& name,
		LLInventoryType::EType inv_type);

	// construct an invalid and incomplete viewer inventory item.
	// usually useful for unpacking or importing or what have you.
	// *NOTE: it is important to call setComplete() if you expect the
	// operations to provide all necessary information.
	LLViewerInventoryItem();
	// Create a copy of an inventory item from a pointer to another item
	// Note: Because InventoryItems are ref counted,
	//       reference copy (a = b) is prohibited
	LLViewerInventoryItem(const LLViewerInventoryItem* other);
	LLViewerInventoryItem(const LLInventoryItem* other);

	void copyViewerItem(const LLViewerInventoryItem* other);
	/*virtual*/ void copyItem(const LLInventoryItem* other);

	// construct a new clone of this item - it creates a new viewer
	// inventory item using the copy constructor, and returns it.
	// It is up to the caller to delete (unref) the item.
	void cloneViewerItem(LLPointer<LLViewerInventoryItem>& newitem) const;

	// virtual methods
	virtual void removeFromServer( void );
	virtual void updateParentOnServer(BOOL restamp) const;
	virtual void updateServer(BOOL is_new) const;
	void fetchFromServer(void) const;

	//virtual void packMessage(LLMessageSystem* msg) const;
	virtual BOOL unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
	virtual BOOL unpackMessage(LLSD item);
	virtual BOOL importFile(LLFILE* fp);
	virtual BOOL importLegacyStream(std::istream& input_stream);

	// file handling on the viewer. These are not meant for anything
	// other than cacheing.
	bool exportFileLocal(LLFILE* fp) const;
	bool importFileLocal(LLFILE* fp);

	// new methods
	BOOL isComplete() const { return mIsComplete; }
	void setComplete(BOOL complete) { mIsComplete = complete; }
	//void updateAssetOnServer() const;

	virtual void packMessage(LLMessageSystem* msg) const;
	virtual void setTransactionID(const LLTransactionID& transaction_id);
	struct comparePointers
	{
		bool operator()(const LLPointer<LLViewerInventoryItem>& a, const LLPointer<LLViewerInventoryItem>& b)
		{
			return a->getName().compare(b->getName()) < 0;
		}
	};
	LLTransactionID getTransactionID() const { return mTransactionID; }
	
	bool getIsBrokenLink() const; // true if the baseitem this points to doesn't exist in memory.
	LLViewerInventoryItem *getLinkedItem() const;
	LLViewerInventoryCategory *getLinkedCategory() const;
	
	// Checks the items permissions (for owner, group, or everyone) and returns true if all mask bits are set.
	bool checkPermissionsSet(PermissionMask mask) const;

	// callback
	void onCallingCardNameLookup(const LLUUID& id, const std::string& first_name, const std::string& last_name);

	// If this is a broken link, try to fix it and any other identical link.
	BOOL regenerateLink();

public:
	BOOL mIsComplete;
	LLTransactionID mTransactionID;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLViewerInventoryCategory
//
// An instance of this class represents a category of inventory
// items. Users come with a set of default categories, and can create
// new ones as needed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLViewerInventoryCategory  : public LLInventoryCategory
{
public:
	typedef LLDynamicArray<LLPointer<LLViewerInventoryCategory> > cat_array_t;
	
protected:
	~LLViewerInventoryCategory();
	
public:
	LLViewerInventoryCategory(const LLUUID& uuid, const LLUUID& parent_uuid,
							  LLFolderType::EType preferred_type,
							  const std::string& name,
							  const LLUUID& owner_id);
	LLViewerInventoryCategory(const LLUUID& owner_id);
	// Create a copy of an inventory category from a pointer to another category
	// Note: Because InventoryCategorys are ref counted, reference copy (a = b)
	// is prohibited
	LLViewerInventoryCategory(const LLViewerInventoryCategory* other);
	void copyViewerCategory(const LLViewerInventoryCategory* other);

	virtual void removeFromServer();
	virtual void updateParentOnServer(BOOL restamp_children) const;
	virtual void updateServer(BOOL is_new) const;

	const LLUUID& getOwnerID() const { return mOwnerID; }

	// Version handling
	enum { VERSION_UNKNOWN = -1, VERSION_INITIAL = 1 };
	S32 getVersion() const { return mVersion; }
	void setVersion(S32 version) { mVersion = version; }

	// Returns true if a fetch was issued.
	bool fetchDescendents();

	// used to help make cacheing more robust - for example, if
	// someone is getting 4 packets but logs out after 3. the viewer
	// may never know the cache is wrong.
	enum { DESCENDENT_COUNT_UNKNOWN = -1 };
	S32 getDescendentCount() const { return mDescendentCount; }
	void setDescendentCount(S32 descendents) { mDescendentCount = descendents; }

	// file handling on the viewer. These are not meant for anything
	// other than cacheing.
	bool exportFileLocal(LLFILE* fp) const;
	bool importFileLocal(LLFILE* fp);
	void determineFolderType();
	void changeType(LLFolderType::EType new_folder_type);
protected:
	LLUUID mOwnerID;
	S32 mVersion;
	S32 mDescendentCount;
	LLFrameTimer mDescendentsRequested;
};

class LLInventoryCallback : public LLRefCount
{
public:
	virtual void fire(const LLUUID& inv_item) = 0;
};

class WearOnAvatarCallback : public LLInventoryCallback
{
	void fire(const LLUUID& inv_item);
};

class ModifiedCOFCallback : public LLInventoryCallback
{
	void fire(const LLUUID& inv_item);
};

class LLViewerJointAttachment;

class RezAttachmentCallback : public LLInventoryCallback
{
public:
	RezAttachmentCallback(LLViewerJointAttachment *attachmentp);
	void fire(const LLUUID& inv_item);

protected:
	~RezAttachmentCallback();

private:
	LLViewerJointAttachment* mAttach;
};

class ActivateGestureCallback : public LLInventoryCallback
{
public:
	void fire(const LLUUID& inv_item);
};

class CreateGestureCallback : public LLInventoryCallback
{
public:
	void fire(const LLUUID& inv_item);
};

class AddFavoriteLandmarkCallback : public LLInventoryCallback
{
public:
	AddFavoriteLandmarkCallback() : mTargetLandmarkId(LLUUID::null) {}
	void setTargetLandmarkId(const LLUUID& target_uuid) { mTargetLandmarkId = target_uuid; }

private:
	void fire(const LLUUID& inv_item);

	LLUUID mTargetLandmarkId;
};

// misc functions
//void inventory_reliable_callback(void**, S32 status);

class LLInventoryCallbackManager
{
public:
	LLInventoryCallbackManager();
	~LLInventoryCallbackManager();

	void fire(U32 callback_id, const LLUUID& item_id);
	U32 registerCB(LLPointer<LLInventoryCallback> cb);
private:
	std::map<U32, LLPointer<LLInventoryCallback> > mMap;
	U32 mLastCallback;
	static LLInventoryCallbackManager *sInstance;
public:
	static bool is_instantiated() { return sInstance != NULL; }
};
extern LLInventoryCallbackManager gInventoryCallbacks;


#define NOT_WEARABLE (EWearableType)0

// *TODO: Find a home for these
void create_inventory_item(const LLUUID& agent_id, const LLUUID& session_id,
						   const LLUUID& parent, const LLTransactionID& transaction_id,
						   const std::string& name,
						   const std::string& desc, LLAssetType::EType asset_type,
						   LLInventoryType::EType inv_type, EWearableType wtype,
						   U32 next_owner_perm,
						   LLPointer<LLInventoryCallback> cb);

void create_inventory_callingcard(const LLUUID& avatar_id, const LLUUID& parent = LLUUID::null, LLPointer<LLInventoryCallback> cb=NULL);

/**
 * @brief Securely create a new inventory item by copying from another.
 */
void copy_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& current_owner,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	LLPointer<LLInventoryCallback> cb);

void link_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	const LLAssetType::EType asset_type,
	LLPointer<LLInventoryCallback> cb);

void move_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& session_id,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	LLPointer<LLInventoryCallback> cb);

void copy_inventory_from_notecard(const LLUUID& object_id,
								  const LLUUID& notecard_inv_id,
								  const LLInventoryItem *src,
								  U32 callback_id = 0);


void menu_create_inventory_item(LLFolderView* folder,
								LLFolderBridge* bridge,
								const LLSD& userdata,
								const LLUUID& default_parent_uuid = LLUUID::null);

#endif // LL_LLVIEWERINVENTORY_H
