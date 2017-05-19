/** 
 * @file llinventorymodel.h
 * @brief LLInventoryModel class header file
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

#ifndef LL_LLINVENTORYMODEL_H
#define LL_LLINVENTORYMODEL_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "llassettype.h"
#include "llfoldertype.h"
#include "llframetimer.h"
#include "lluuid.h"
#include "llpermissionsflags.h"
#include "llviewerinventory.h"
#include "llstring.h"
#include "llmd5.h"
#include "httpcommon.h"
#include "httprequest.h"
#include "httpoptions.h"
#include "httpheaders.h"
#include "httphandler.h"
#include "lleventcoro.h"
#include "llcoros.h"

class LLInventoryObserver;
class LLInventoryObject;
class LLInventoryItem;
class LLInventoryCategory;
class LLMessageSystem;
class LLInventoryCollectFunctor;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLInventoryModel
//
// Represents a collection of inventory, and provides efficient ways to access 
// that information.
//   NOTE: This class could in theory be used for any place where you need 
//   inventory, though it optimizes for time efficiency - not space efficiency, 
//   probably making it inappropriate for use on tasks.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryModel
{
	LOG_CLASS(LLInventoryModel);

public:
	enum EHasChildren
	{
		CHILDREN_NO,
		CHILDREN_YES,
		CHILDREN_MAYBE
	};

	typedef std::vector<LLPointer<LLViewerInventoryCategory> > cat_array_t;
	typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
	typedef std::set<LLUUID> changed_items_t;

    // Rider: This is using the old responder patter.  It should be refactored to 
    // take advantage of coroutines.

	// HTTP handler for individual item requests (inventory or library).
	// Background item requests are derived from this in the background
	// inventory system.  All folder requests are also located there
	// but have their own handler derived from HttpHandler.
	class FetchItemHttpHandler : public LLCore::HttpHandler
	{
	public:
		LOG_CLASS(FetchItemHttpHandler);

		FetchItemHttpHandler(const LLSD & request_sd);
		virtual ~FetchItemHttpHandler();

	protected:
		FetchItemHttpHandler(const FetchItemHttpHandler &);				// Not defined
		void operator=(const FetchItemHttpHandler &);					// Not defined

	public:
		virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

	private:
		void processData(LLSD & body, LLCore::HttpResponse * response);
		void processFailure(LLCore::HttpStatus status, LLCore::HttpResponse * response);
		void processFailure(const char * const reason, LLCore::HttpResponse * response);

	private:
		LLSD mRequestSD;
	};

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION/SETUP
 **/

	//--------------------------------------------------------------------
	// Constructors / Destructors
	//--------------------------------------------------------------------
public:
	LLInventoryModel();
	~LLInventoryModel();
	void cleanupInventory();
protected:
	void empty(); // empty the entire contents

	//--------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------
public:
	// The inventory model usage is sensitive to the initial construction of the model
	bool isInventoryUsable() const;
private:
	bool mIsAgentInvUsable; // used to handle an invalid inventory state

	// One-time initialization of HTTP system.
	void initHttpRequest();
	
	//--------------------------------------------------------------------
	// Root Folders
	//--------------------------------------------------------------------
public:
	// The following are set during login with data from the server
	void setRootFolderID(const LLUUID& id);
	void setLibraryOwnerID(const LLUUID& id);
	void setLibraryRootFolderID(const LLUUID& id);

	const LLUUID &getRootFolderID() const;
	const LLUUID &getLibraryOwnerID() const;
	const LLUUID &getLibraryRootFolderID() const;
private:
	LLUUID mRootFolderID;
	LLUUID mLibraryRootFolderID;
	LLUUID mLibraryOwnerID;	
	
	//--------------------------------------------------------------------
	// Structure
	//--------------------------------------------------------------------
public:
	// Methods to load up inventory skeleton & meat. These are used
	// during authentication. Returns true if everything parsed.
	bool loadSkeleton(const LLSD& options, const LLUUID& owner_id);
	void buildParentChildMap(); // brute force method to rebuild the entire parent-child relations
	void createCommonSystemCategories();

	static std::string getInvCacheAddres(const LLUUID& owner_id);

	// Call on logout to save a terse representation.
	void cache(const LLUUID& parent_folder_id, const LLUUID& agent_id);
private:
	// Information for tracking the actual inventory. We index this
	// information in a lot of different ways so we can access
	// the inventory using several different identifiers.
	// mInventory member data is the 'master' list of inventory, and
	// mCategoryMap and mItemMap store uuid->object mappings. 
	typedef std::map<LLUUID, LLPointer<LLViewerInventoryCategory> > cat_map_t;
	typedef std::map<LLUUID, LLPointer<LLViewerInventoryItem> > item_map_t;
	cat_map_t mCategoryMap;
	item_map_t mItemMap;
	// This last set of indices is used to map parents to children.
	typedef std::map<LLUUID, cat_array_t*> parent_cat_map_t;
	typedef std::map<LLUUID, item_array_t*> parent_item_map_t;
	parent_cat_map_t mParentChildCategoryTree;
	parent_item_map_t mParentChildItemTree;

	// Track links to items and categories. We do not store item or
	// category pointers here, because broken links are also supported.
	typedef std::multimap<LLUUID, LLUUID> backlink_mmap_t;
	backlink_mmap_t mBacklinkMMap; // key = target_id: ID of item, values = link_ids: IDs of item or folder links referencing it.
	// For internal use only
	bool hasBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id) const;
	void addBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id);
	void removeBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id);
	
	//--------------------------------------------------------------------
	// Login
	//--------------------------------------------------------------------
public:
	static BOOL getIsFirstTimeInViewer2();
private:
	static BOOL sFirstTimeInViewer2;
	const static S32 sCurrentInvCacheVersion; // expected inventory cache version

/**                    Initialization/Setup
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    ACCESSORS
 **/

	//--------------------------------------------------------------------
	// Descendants
	//--------------------------------------------------------------------
public:
	// Make sure we have the descendants in the structure.  Returns true
	// if a fetch was performed.
	bool fetchDescendentsOf(const LLUUID& folder_id) const;

	// Return the direct descendants of the id provided.Set passed
	// in values to NULL if the call fails.
	//    NOTE: The array provided points straight into the guts of
	//    this object, and should only be used for read operations, since
	//    modifications may invalidate the internal state of the inventory.
	void getDirectDescendentsOf(const LLUUID& cat_id,
								cat_array_t*& categories,
								item_array_t*& items) const;

	// Compute a hash of direct descendant names (for detecting child name changes)
	LLMD5 hashDirectDescendentNames(const LLUUID& cat_id) const;
	
	// Starting with the object specified, add its descendants to the
	// array provided, but do not add the inventory object specified
	// by id. There is no guaranteed order. 
	//    NOTE: Neither array will be erased before adding objects to it. 
	//    Do not store a copy of the pointers collected - use them, and 
	//    collect them again later if you need to reference the same objects.
	enum { 
		EXCLUDE_TRASH = FALSE, 
		INCLUDE_TRASH = TRUE 
	};
	// Simpler existence test if matches don't actually need to be collected.
	bool hasMatchingDirectDescendent(const LLUUID& cat_id,
									 LLInventoryCollectFunctor& filter);
	void collectDescendents(const LLUUID& id,
							cat_array_t& categories,
							item_array_t& items,
							BOOL include_trash);
	void collectDescendentsIf(const LLUUID& id,
							  cat_array_t& categories,
							  item_array_t& items,
							  BOOL include_trash,
							  LLInventoryCollectFunctor& add);

	// Collect all items in inventory that are linked to item_id.
	// Assumes item_id is itself not a linked item.
	item_array_t collectLinksTo(const LLUUID& item_id);

	// Check if one object has a parent chain up to the category specified by UUID.
	BOOL isObjectDescendentOf(const LLUUID& obj_id, const LLUUID& cat_id) const;

	// Follow parent chain to the top.
	bool getObjectTopmostAncestor(const LLUUID& object_id, LLUUID& result) const;

	//--------------------------------------------------------------------
	// Find
	//--------------------------------------------------------------------
public:
	const LLUUID findCategoryUUIDForTypeInRoot(
		LLFolderType::EType preferred_type,
		bool create_folder,
		const LLUUID& root_id);

	// Returns the uuid of the category that specifies 'type' as what it 
	// defaults to containing. The category is not necessarily only for that type. 
	//    NOTE: If create_folder is true, this will create a new inventory category 
	//    on the fly if one does not exist. *NOTE: if find_in_library is true it 
	//    will search in the user's library folder instead of "My Inventory"
	const LLUUID findCategoryUUIDForType(LLFolderType::EType preferred_type, 
										 bool create_folder = true);
	//    will search in the user's library folder instead of "My Inventory"
	const LLUUID findLibraryCategoryUUIDForType(LLFolderType::EType preferred_type, 
												bool create_folder = true);
	// Returns user specified category for uploads, returns default id if there are no
	// user specified one or it does not exist, creates default category if it is missing.
	const LLUUID findUserDefinedCategoryUUIDForType(LLFolderType::EType preferred_type);
	
	// Get whatever special folder this object is a child of, if any.
	const LLViewerInventoryCategory *getFirstNondefaultParent(const LLUUID& obj_id) const;
	
	// Get first descendant of the child object under the specified parent
	const LLViewerInventoryCategory *getFirstDescendantOf(const LLUUID& master_parent_id, const LLUUID& obj_id) const;

	// Get the object by id. Returns NULL if not found.
	//   NOTE: Use the pointer returned for read operations - do
	//   not modify the object values in place or you will break stuff.
	LLInventoryObject* getObject(const LLUUID& id) const;

	// Get the item by id. Returns NULL if not found.
	//    NOTE: Use the pointer for read operations - use the
	//    updateItem() method to actually modify values.
	LLViewerInventoryItem* getItem(const LLUUID& id) const;

	// Get the category by id. Returns NULL if not found.
	//    NOTE: Use the pointer for read operations - use the
	//    updateCategory() method to actually modify values.
	LLViewerInventoryCategory* getCategory(const LLUUID& id) const;

	// Get the inventoryID or item that this item points to, else just return object_id
	const LLUUID& getLinkedItemID(const LLUUID& object_id) const;
	LLViewerInventoryItem* getLinkedItem(const LLUUID& object_id) const;
    
    // Copy content of all folders of type "type" into folder "id" and delete/purge the empty folders
    // Note : This method has been designed for FT_OUTBOX (aka Merchant Outbox) but can be used for other categories
    void consolidateForType(const LLUUID& id, LLFolderType::EType type);
    
private:
	mutable LLPointer<LLViewerInventoryItem> mLastItem; // cache recent lookups	

	//--------------------------------------------------------------------
	// Count
	//--------------------------------------------------------------------
public:
	// Return the number of items or categories
	S32 getItemCount() const;
	S32 getCategoryCount() const;

/**                    Accessors
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    MUTATORS
 **/

public:
	// Change an existing item with a matching item_id or add the item
	// to the current inventory. Returns the change mask generated by
	// the update. No notification will be sent to observers. This
	// method will only generate network traffic if the item had to be
	// reparented.
	//    NOTE: In usage, you will want to perform cache accounting
	//    operations in LLInventoryModel::accountForUpdate() or
	//    LLViewerInventoryItem::updateServer() before calling this method.
	U32 updateItem(const LLViewerInventoryItem* item, U32 mask = 0);

	// Change an existing item with the matching id or add
	// the category. No notification will be sent to observers. This
	// method will only generate network traffic if the item had to be
	// reparented.
	//    NOTE: In usage, you will want to perform cache accounting
	//    operations in accountForUpdate() or LLViewerInventoryCategory::
	//    updateServer() before calling this method.
	void updateCategory(const LLViewerInventoryCategory* cat, U32 mask = 0);

	// Move the specified object id to the specified category and
	// update the internal structures. No cache accounting,
	// observer notification, or server update is performed.
	void moveObject(const LLUUID& object_id, const LLUUID& cat_id);

	// Migrated from llinventoryfunctions
	void changeItemParent(LLViewerInventoryItem* item,
						  const LLUUID& new_parent_id,
						  BOOL restamp);

	// Migrated from llinventoryfunctions
	void changeCategoryParent(LLViewerInventoryCategory* cat,
							  const LLUUID& new_parent_id,
							  BOOL restamp);

	//--------------------------------------------------------------------
	// Delete
	//--------------------------------------------------------------------
public:

	// Update model after an AISv3 update received for any operation.
	void onAISUpdateReceived(const std::string& context, const LLSD& update);
		
	// Update model after an item is confirmed as removed from
	// server. Works for categories or items.
	void onObjectDeletedFromServer(const LLUUID& item_id,
								   bool fix_broken_links = true,
								   bool update_parent_version = true,
								   bool do_notify_observers = true);

	// Update model after all descendants removed from server.
	void onDescendentsPurgedFromServer(const LLUUID& object_id, bool fix_broken_links = true);

	// Update model after an existing item gets updated on server.
	void onItemUpdated(const LLUUID& item_id, const LLSD& updates, bool update_parent_version);

	// Update model after an existing category gets updated on server.
	void onCategoryUpdated(const LLUUID& cat_id, const LLSD& updates);

	// Delete a particular inventory object by ID. Will purge one
	// object from the internal data structures, maintaining a
	// consistent internal state. No cache accounting, observer
	// notification, or server update is performed.
	void deleteObject(const LLUUID& id, bool fix_broken_links = true, bool do_notify_observers = true);
	/// move Item item_id to Trash
	void removeItem(const LLUUID& item_id);
	/// move Category category_id to Trash
	void removeCategory(const LLUUID& category_id);
	/// removeItem() or removeCategory(), whichever is appropriate
	void removeObject(const LLUUID& object_id);

	// "TrashIsFull" when trash exceeds maximum capacity
	void checkTrashOverflow();

protected:
	void updateLinkedObjectsFromPurge(const LLUUID& baseobj_id);
	
	//--------------------------------------------------------------------
	// Reorder
	//--------------------------------------------------------------------
public:
	// Changes items order by insertion of the item identified by src_item_id
	// before (or after) the item identified by dest_item_id. Both items must exist in items array.
	// Sorting is stored after method is finished. Only src_item_id is moved before (or after) dest_item_id.
	// The parameter "insert_before" controls on which side of dest_item_id src_item_id gets reinserted.
	static void updateItemsOrder(LLInventoryModel::item_array_t& items, 
								 const LLUUID& src_item_id, 
								 const LLUUID& dest_item_id,
								 bool insert_before = true);
	// Gets an iterator on an item vector knowing only the item UUID.
	// Returns end() of the vector if not found.
	static LLInventoryModel::item_array_t::iterator findItemIterByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id);


	// Rearranges Landmarks inside Favorites folder.
	// Moves source landmark before target one.
	void rearrangeFavoriteLandmarks(const LLUUID& source_item_id, const LLUUID& target_item_id);
	//void saveItemsOrder(const LLInventoryModel::item_array_t& items);

	//--------------------------------------------------------------------
	// Creation
	//--------------------------------------------------------------------
public:
	// Returns the UUID of the new category. If you want to use the default 
	// name based on type, pass in a NULL to the 'name' parameter.
	LLUUID createNewCategory(const LLUUID& parent_id,
							 LLFolderType::EType preferred_type,
							 const std::string& name,
							 inventory_func_type callback = NULL);
protected:
	// Internal methods that add inventory and make sure that all of
	// the internal data structures are consistent. These methods
	// should be passed pointers of newly created objects, and the
	// instance will take over the memory management from there.
	void addCategory(LLViewerInventoryCategory* category);
	void addItem(LLViewerInventoryItem* item);

    void createNewCategoryCoro(std::string url, LLSD postData, inventory_func_type callback);
	
/**                    Mutators
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    CATEGORY ACCOUNTING
 **/

public:
	// Represents the number of items added or removed from a category.
	struct LLCategoryUpdate
	{
		LLCategoryUpdate() : mDescendentDelta(0) {}
		LLCategoryUpdate(const LLUUID& category_id, S32 delta) :
			mCategoryID(category_id),
			mDescendentDelta(delta) {}
		LLUUID mCategoryID;
		S32 mDescendentDelta;
	};
	typedef std::vector<LLCategoryUpdate> update_list_t;

	// This exists to make it easier to account for deltas in a map.
	struct LLInitializedS32
	{
		LLInitializedS32() : mValue(0) {}
		LLInitializedS32(S32 value) : mValue(value) {}
		S32 mValue;
		LLInitializedS32& operator++() { ++mValue; return *this; }
		LLInitializedS32& operator--() { --mValue; return *this; }
	};
	typedef std::map<LLUUID, LLInitializedS32> update_map_t;

	// Call when there are category updates.  Call them *before* the 
	// actual update so the method can do descendent accounting correctly.
	void accountForUpdate(const LLCategoryUpdate& update) const;
	void accountForUpdate(const update_list_t& updates);
	void accountForUpdate(const update_map_t& updates);

	// Return (yes/no/maybe) child status of category children.
	EHasChildren categoryHasChildren(const LLUUID& cat_id) const;

	// Returns true if category version is known and theoretical
	// descendents == actual descendents.
	bool isCategoryComplete(const LLUUID& cat_id) const;
	
/**                    Category Accounting
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    NOTIFICATIONS
 **/

public:
	// Called by the idle loop.  Only updates if new state is detected.  Call 
	// notifyObservers() manually to update regardless of whether state change 
	// has been indicated.
	void idleNotifyObservers();

	// Call to explicitly update everyone on a new state.
	void notifyObservers();

	// Allows outsiders to tell the inventory if something has
	// been changed 'under the hood', but outside the control of the
	// inventory. The next notify will include that notification.
	void addChangedMask(U32 mask, const LLUUID& referent);
	
	const changed_items_t& getChangedIDs() const { return mChangedItemIDs; }
	const changed_items_t& getAddedIDs() const { return mAddedItemIDs; }
protected:
	// Updates all linked items pointing to this id.
	void addChangedMaskForLinks(const LLUUID& object_id, U32 mask);
private:
	// Flag set when notifyObservers is being called, to look for bugs
	// where it's called recursively.
	BOOL mIsNotifyObservers;
	// Variables used to track what has changed since the last notify.
	U32 mModifyMask;
	changed_items_t mChangedItemIDs;
	changed_items_t mAddedItemIDs;
	
	
	//--------------------------------------------------------------------
	// Observers
	//--------------------------------------------------------------------
public:
	// If the observer is destroyed, be sure to remove it.
	void addObserver(LLInventoryObserver* observer);
	void removeObserver(LLInventoryObserver* observer);
	BOOL containsObserver(LLInventoryObserver* observer) const;
private:
	typedef std::set<LLInventoryObserver*> observer_list_t;
	observer_list_t mObservers;
	
/**                    Notifications
 **                                                                            **
 *******************************************************************************/


/********************************************************************************
 **                                                                            **
 **                    HTTP Transport
 **/
public:
	// Invoke handler completion method (onCompleted) for all
	// requests that are ready.
	void handleResponses(bool foreground);

	// Request an inventory HTTP operation to either the
	// foreground or background processor.  These are actually
	// the same service queue but the background requests are
	// seviced more slowly effectively de-prioritizing new
	// requests.
	LLCore::HttpHandle requestPost(bool foreground,
								   const std::string & url,
								   const LLSD & body,
								   const LLCore::HttpHandler::ptr_t &handler,
								   const char * const message);
	
private:
	// Usual plumbing for LLCore:: HTTP operations.
	LLCore::HttpRequest *				mHttpRequestFG;
	LLCore::HttpRequest *				mHttpRequestBG;
	LLCore::HttpOptions::ptr_t			mHttpOptions;
	LLCore::HttpHeaders::ptr_t			mHttpHeaders;
	LLCore::HttpRequest::policy_t		mHttpPolicyClass;
	LLCore::HttpRequest::priority_t		mHttpPriorityFG;
	LLCore::HttpRequest::priority_t		mHttpPriorityBG;
	
/**                    HTTP Transport
 **                                                                            **
 *******************************************************************************/


/********************************************************************************
 **                                                                            **
 **                    MISCELLANEOUS
 **/

	//--------------------------------------------------------------------
	// Callbacks
	//--------------------------------------------------------------------
public:
	// Trigger a notification and empty the folder type (FT_TRASH or FT_LOST_AND_FOUND) if confirmed
	void emptyFolderType(const std::string notification, LLFolderType::EType folder_type);
	bool callbackEmptyFolderType(const LLSD& notification, const LLSD& response, LLFolderType::EType preferred_type);
	static void registerCallbacks(LLMessageSystem* msg);

	//--------------------------------------------------------------------
	// File I/O
	//--------------------------------------------------------------------
protected:
	static bool loadFromFile(const std::string& filename,
							 cat_array_t& categories,
							 item_array_t& items,
							 bool& is_cache_obsolete); 
	static bool saveToFile(const std::string& filename,
						   const cat_array_t& categories,
						   const item_array_t& items); 

	//--------------------------------------------------------------------
	// Message handling functionality
	//--------------------------------------------------------------------
public:
	static void processUpdateCreateInventoryItem(LLMessageSystem* msg, void**);
	static void removeInventoryItem(LLUUID agent_id, LLMessageSystem* msg, const char* msg_label);
	static void processRemoveInventoryItem(LLMessageSystem* msg, void**);
	static void processUpdateInventoryFolder(LLMessageSystem* msg, void**);
	static void removeInventoryFolder(LLUUID agent_id, LLMessageSystem* msg);
	static void processRemoveInventoryFolder(LLMessageSystem* msg, void**);
	static void processRemoveInventoryObjects(LLMessageSystem* msg, void**);
	static void processSaveAssetIntoInventory(LLMessageSystem* msg, void**);
	static void processBulkUpdateInventory(LLMessageSystem* msg, void**);
	static void processMoveInventoryItem(LLMessageSystem* msg, void**);
protected:
	bool messageUpdateCore(LLMessageSystem* msg, bool do_accounting, U32 mask = 0x0);

	//--------------------------------------------------------------------
	// Locks
	//--------------------------------------------------------------------
public:
	void lockDirectDescendentArrays(const LLUUID& cat_id,
									cat_array_t*& categories,
									item_array_t*& items);
	void unlockDirectDescendentArrays(const LLUUID& cat_id);
protected:
	cat_array_t* getUnlockedCatArray(const LLUUID& id);
	item_array_t* getUnlockedItemArray(const LLUUID& id);
private:
	std::map<LLUUID, bool> mCategoryLock;
	std::map<LLUUID, bool> mItemLock;
	
	//--------------------------------------------------------------------
	// Debugging
	//--------------------------------------------------------------------
public:
	void dumpInventory() const;
	bool validate() const;

/**                    Miscellaneous
 **                                                                            **
 *******************************************************************************/
};

// a special inventory model for the agent
extern LLInventoryModel gInventory;

#endif // LL_LLINVENTORYMODEL_H

