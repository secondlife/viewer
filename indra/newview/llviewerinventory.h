/** 
 * @file llviewerinventory.h
 * @brief Declaration of the inventory bits that only used on the viewer.
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

#ifndef LL_LLVIEWERINVENTORY_H
#define LL_LLVIEWERINVENTORY_H

#include "llinventory.h"
#include "llframetimer.h"
#include "llwearable.h"
#include "llinitdestroyclass.h" //for LLDestroyClass
#include "llinventorysettings.h"

#include <boost/signals2.hpp>	// boost::signals2::trackable

class LLInventoryPanel;
class LLFolderView;
class LLFolderBridge;
class LLViewerInventoryCategory;
class LLInventoryCallback;
class LLAvatarName;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLViewerInventoryItem
//
// An inventory item represents something that the current user has in
// their inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLViewerInventoryItem : public LLInventoryItem, public boost::signals2::trackable
{
public:
	typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
	
protected:
	~LLViewerInventoryItem( void ); // ref counted
	bool extractSortFieldAndDisplayName(S32* sortField, std::string* displayName) const { return extractSortFieldAndDisplayName(mName, sortField, displayName); }
	mutable std::string mDisplayName;
	
public:
	virtual LLAssetType::EType getType() const;
	virtual const LLUUID& getAssetUUID() const;
	virtual const LLUUID& getProtectedAssetUUID() const; // returns LLUUID::null if current agent does not have permission to expose this asset's UUID to the user
	virtual const std::string& getName() const;
	virtual S32 getSortField() const;
	//virtual void setSortField(S32 sortField);
	virtual void getSLURL(); //Caches SLURL for landmark. //*TODO: Find a better way to do it and remove this method from here.
	virtual const LLPermissions& getPermissions() const;
	virtual const bool getIsFullPerm() const; // 'fullperm' in the popular sense: modify-ok & copy-ok & transfer-ok, no special god rules applied
	virtual const LLUUID& getCreatorUUID() const;
	virtual const std::string& getDescription() const;
	virtual const LLSaleInfo& getSaleInfo() const;
    virtual const LLUUID& getThumbnailUUID() const;
	virtual LLInventoryType::EType getInventoryType() const;
	virtual bool isWearableType() const;
	virtual LLWearableType::EType getWearableType() const;
    virtual bool isSettingsType() const;
    virtual LLSettingsType::type_e getSettingsType() const;

	virtual U32 getFlags() const;
	virtual time_t getCreationDate() const;
	virtual U32 getCRC32() const; // really more of a checksum.

	static bool extractSortFieldAndDisplayName(const std::string& name, S32* sortField, std::string* displayName);

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
	virtual void updateParentOnServer(bool restamp) const;
	virtual void updateServer(bool is_new) const;
	void fetchFromServer(void) const;

	virtual void packMessage(LLMessageSystem* msg) const;
	virtual bool unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
	virtual bool unpackMessage(const LLSD& item);
	virtual bool importLegacyStream(std::istream& input_stream);

	// new methods
	bool isFinished() const { return mIsComplete; }
	void setComplete(bool complete) { mIsComplete = complete; }
	//void updateAssetOnServer() const;

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
	PermissionMask getPermissionMask() const;

	// callback
	void onCallingCardNameLookup(const LLUUID& id, const LLAvatarName& name);

	// If this is a broken link, try to fix it and any other identical link.
	bool regenerateLink();

public:
	bool mIsComplete;
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
	typedef std::vector<LLPointer<LLViewerInventoryCategory> > cat_array_t;
	
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

	virtual void updateParentOnServer(bool restamp_children) const;
	virtual void updateServer(bool is_new) const;

	virtual void packMessage(LLMessageSystem* msg) const;

	const LLUUID& getOwnerID() const { return mOwnerID; }

	// Version handling
	enum { VERSION_UNKNOWN = -1, VERSION_INITIAL = 1 };
	S32 getVersion() const;
	void setVersion(S32 version);

	// Returns true if a fetch was issued (not nessesary in progress).
	bool fetch();

    typedef enum {
        FETCH_NONE = 0,
        FETCH_NORMAL,
        FETCH_RECURSIVE,
    } EFetchType;
    EFetchType getFetching();
    // marks as fetch being in progress or as done
    void setFetching(EFetchType);

	// used to help make caching more robust - for example, if
	// someone is getting 4 packets but logs out after 3. the viewer
	// may never know the cache is wrong.
	enum { DESCENDENT_COUNT_UNKNOWN = -1 };
	S32 getDescendentCount() const { return mDescendentCount; }
	void setDescendentCount(S32 descendents) { mDescendentCount = descendents; }
	// How many descendents do we currently have information for in the InventoryModel?
	S32 getViewerDescendentCount() const;

	LLSD exportLLSD() const;
	bool importLLSD(const LLSD& cat_data);

	void determineFolderType();
	void changeType(LLFolderType::EType new_folder_type);
	virtual void unpackMessage(LLMessageSystem* msg, const char* block, S32 block_num = 0);
	virtual bool unpackMessage(const LLSD& category);
    
    // returns true if the category object will accept the incoming item
    bool acceptItem(LLInventoryItem* inv_item);

private:
	friend class LLInventoryModel;
	void localizeName(); // intended to be called from the LLInventoryModel

protected:
	LLUUID mOwnerID;
	S32 mVersion;
	S32 mDescendentCount;
    EFetchType mFetching;
	LLFrameTimer mDescendentsRequested;
};

class LLInventoryCallback : public LLRefCount
{
public:
    virtual ~LLInventoryCallback() {}
	virtual void fire(const LLUUID& inv_item) = 0;
};

class LLViewerJointAttachment;

void rez_attachment_cb(const LLUUID& inv_item, LLViewerJointAttachment *attachmentp);

void activate_gesture_cb(const LLUUID& inv_item);

void create_script_cb(const LLUUID& inv_item);
void create_gesture_cb(const LLUUID& inv_item);
void create_notecard_cb(const LLUUID& inv_item);

class AddFavoriteLandmarkCallback : public LLInventoryCallback
{
public:
	AddFavoriteLandmarkCallback() : mTargetLandmarkId(LLUUID::null) {}
	void setTargetLandmarkId(const LLUUID& target_uuid) { mTargetLandmarkId = target_uuid; }
	
private:
	void fire(const LLUUID& inv_item);

	LLUUID mTargetLandmarkId;
};

typedef boost::function<void(const LLUUID&)> inventory_func_type;
typedef boost::function<void(const LLSD&)> llsd_func_type;
typedef boost::function<void()> nullary_func_type;

void no_op_inventory_func(const LLUUID&); // A do-nothing inventory_func
void no_op_llsd_func(const LLSD&); // likewise for LLSD
void no_op(); // A do-nothing nullary func.

// Shim between inventory callback and boost function/callable
class LLBoostFuncInventoryCallback: public LLInventoryCallback
{
public:

	LLBoostFuncInventoryCallback(inventory_func_type fire_func,
								 nullary_func_type destroy_func = no_op):
		mDestroyFunc(destroy_func)
	{
        mFireFuncs.push_back(fire_func);
	}

    LLBoostFuncInventoryCallback()
    {
    }

    void addOnFireFunc(inventory_func_type fire_func)
    {
        mFireFuncs.push_back(fire_func);
    }

	// virtual
	void fire(const LLUUID& item_id)
    {
        for (inventory_func_type &func: mFireFuncs)
        {
            func(item_id);
        }
	}

	// virtual
	~LLBoostFuncInventoryCallback()
{
		mDestroyFunc();
	}
	

private:
	std::list<inventory_func_type> mFireFuncs;
	nullary_func_type mDestroyFunc;
};

// misc functions
//void inventory_reliable_callback(void**, S32 status);

class LLInventoryCallbackManager : public LLDestroyClass<LLInventoryCallbackManager>
{
	friend class LLDestroyClass<LLInventoryCallbackManager>;
public:
	LLInventoryCallbackManager();
	~LLInventoryCallbackManager();

	void fire(U32 callback_id, const LLUUID& item_id);
	U32 registerCB(LLPointer<LLInventoryCallback> cb);
private:
	typedef std::map<U32, LLPointer<LLInventoryCallback> > callback_map_t;
	callback_map_t mMap;
	U32 mLastCallback;
	static LLInventoryCallbackManager *sInstance;
	static void destroyClass();

public:
	static bool is_instantiated() { return sInstance != NULL; }
};
extern LLInventoryCallbackManager gInventoryCallbacks;


const U8 NO_INV_SUBTYPE{ 0 };

// *TODO: Find a home for these
void create_inventory_item(const LLUUID& agent_id, const LLUUID& session_id,
						   const LLUUID& parent, const LLTransactionID& transaction_id,
						   const std::string& name,
						   const std::string& desc, LLAssetType::EType asset_type,
						   LLInventoryType::EType inv_type, U8 subtype,
						   U32 next_owner_perm,
						   LLPointer<LLInventoryCallback> cb);

void create_inventory_wearable(const LLUUID& agent_id, const LLUUID& session_id,
    const LLUUID& parent, const LLTransactionID& transaction_id,
    const std::string& name,
    const std::string& desc, LLAssetType::EType asset_type,
    LLWearableType::EType wtype,
    U32 next_owner_perm,
    LLPointer<LLInventoryCallback> cb);

void create_inventory_settings(const LLUUID& agent_id, const LLUUID& session_id,
    const LLUUID& parent, const LLTransactionID& transaction_id,
    const std::string& name, const std::string& desc, 
    LLSettingsType::type_e settype, 
    U32 next_owner_perm, LLPointer<LLInventoryCallback> cb);


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

// utility functions for inventory linking.
void link_inventory_object(const LLUUID& category,
			 LLConstPointer<LLInventoryObject> baseobj,
			 LLPointer<LLInventoryCallback> cb);
void link_inventory_object(const LLUUID& category,
			 const LLUUID& id,
			 LLPointer<LLInventoryCallback> cb);
void link_inventory_array(const LLUUID& category,
						  LLInventoryObject::const_object_list_t& baseobj_array,
						  LLPointer<LLInventoryCallback> cb);

void move_inventory_item(
	const LLUUID& agent_id,
	const LLUUID& session_id,
	const LLUUID& item_id,
	const LLUUID& parent_id,
	const std::string& new_name,
	LLPointer<LLInventoryCallback> cb);

void update_inventory_item(
	LLViewerInventoryItem *update_item,
	LLPointer<LLInventoryCallback> cb);

void update_inventory_item(
	const LLUUID& item_id,
	const LLSD& updates,
	LLPointer<LLInventoryCallback> cb);

void update_inventory_category(
    const LLUUID& cat_id,
    const LLSD& updates,
    LLPointer<LLInventoryCallback> cb);

void remove_inventory_items(
	LLInventoryObject::object_list_t& items,
	LLPointer<LLInventoryCallback> cb);

void remove_inventory_item(
	LLPointer<LLInventoryObject> obj,
	LLPointer<LLInventoryCallback> cb,
	bool immediate_delete = false);

void remove_inventory_item(
	const LLUUID& item_id,
	LLPointer<LLInventoryCallback> cb,
	bool immediate_delete = false);
	
void remove_inventory_category(
	const LLUUID& cat_id,
	LLPointer<LLInventoryCallback> cb);
	
void remove_inventory_object(
	const LLUUID& object_id,
	LLPointer<LLInventoryCallback> cb);

void purge_descendents_of(
	const LLUUID& cat_id,
	LLPointer<LLInventoryCallback> cb);

const LLUUID get_folder_by_itemtype(const LLInventoryItem *src);

void copy_inventory_from_notecard(const LLUUID& destination_id,
								  const LLUUID& object_id,
								  const LLUUID& notecard_inv_id,
								  const LLInventoryItem *src,
								  U32 callback_id = 0);

void menu_create_inventory_item(LLInventoryPanel* root,
								LLFolderBridge* bridge,
								const LLSD& userdata,
								const LLUUID& default_parent_uuid = LLUUID::null);

void menu_create_inventory_item(LLInventoryPanel* panel, LLUUID dest_id, const LLSD& userdata, const LLUUID& default_parent_uuid = LLUUID::null, std::function<void(const LLUUID&)> folder_created_cb = NULL);

void slam_inventory_folder(const LLUUID& folder_id,
						   const LLSD& contents,
						   LLPointer<LLInventoryCallback> cb);

void remove_folder_contents(const LLUUID& folder_id, bool keep_outfit_links,
							  LLPointer<LLInventoryCallback> cb);

#endif // LL_LLVIEWERINVENTORY_H
