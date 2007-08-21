/** 
 * @file llviewerinventory.h
 * @brief Declaration of the inventory bits that only used on the viewer.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERINVENTORY_H
#define LL_LLVIEWERINVENTORY_H

#include "llinventory.h"
#include "llframetimer.h"
#include "llwearable.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLViewerInventoryItem
//
// An inventory item represents something that the current user has in
// their inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLViewerInventoryItem : public LLInventoryItem
{
public:
	typedef LLDynamicArray<LLPointer<LLViewerInventoryItem> > item_array_t;
	
protected:
	~LLViewerInventoryItem( void ); // ref counted
	
public:
	// construct a complete viewer inventory item
	LLViewerInventoryItem(const LLUUID& uuid, const LLUUID& parent_uuid,
						  const LLPermissions& permissions,
						  const LLUUID& asset_uuid,
						  LLAssetType::EType type,
						  LLInventoryType::EType inv_type,
						  const LLString& name, 
						  const LLString& desc,
						  const LLSaleInfo& sale_info,
						  U32 flags,
						  S32 creation_date_utc);

	// construct a viewer inventory item which has the minimal amount
	// of information to use in the UI.
	LLViewerInventoryItem(
		const LLUUID& item_id,
		const LLUUID& parent_id,
		const char* name,
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

	virtual void copy(const LLViewerInventoryItem* other);
	virtual void copy(const LLInventoryItem* other);

	// construct a new clone of this item - it creates a new viewer
	// inventory item using the copy constructor, and returns it.
	// It is up to the caller to delete (unref) the item.
	virtual void clone(LLPointer<LLViewerInventoryItem>& newitem) const;

	// virtual methods
	virtual void removeFromServer( void );
	virtual void updateParentOnServer(BOOL restamp) const;
	virtual void updateServer(BOOL is_new) const;
	void fetchFromServer(void) const;

	//virtual void packMessage(LLMessageSystem* msg) const;
	virtual BOOL unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
	virtual BOOL importFile(FILE* fp);
	virtual BOOL importLegacyStream(std::istream& input_stream);

	// file handling on the viewer. These are not meant for anything
	// other than cacheing.
	bool exportFileLocal(FILE* fp) const;
	bool importFileLocal(FILE* fp);

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
	
protected:
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
							  LLAssetType::EType preferred_type,
							  const LLString& name,
							  const LLUUID& owner_id);
	LLViewerInventoryCategory(const LLUUID& owner_id);
	// Create a copy of an inventory category from a pointer to another category
	// Note: Because InventoryCategorys are ref counted, reference copy (a = b)
	// is prohibited
	LLViewerInventoryCategory(const LLViewerInventoryCategory* other);
	virtual void copy(const LLViewerInventoryCategory* other);

	virtual void removeFromServer();
	virtual void updateParentOnServer(BOOL restamp_children) const;
	virtual void updateServer(BOOL is_new) const;
	//virtual BOOL unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);

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
	bool exportFileLocal(FILE* fp) const;
	bool importFileLocal(FILE* fp);

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

void create_inventory_item(const LLUUID& agent_id, const LLUUID& session_id,
						   const LLUUID& parent, const LLTransactionID& transaction_id,
						   const std::string& name,
						   const std::string& desc, LLAssetType::EType asset_type,
						   LLInventoryType::EType inv_type, EWearableType wtype,
						   U32 next_owner_perm,
						   LLPointer<LLInventoryCallback> cb);

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


#endif // LL_LLVIEWERINVENTORY_H
