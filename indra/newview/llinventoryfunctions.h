/** 
 * @file llinventoryfunctions.h
 * @brief Miscellaneous inventory-related functions and classes
 * class definition
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

#ifndef LL_LLINVENTORYFUNCTIONS_H
#define LL_LLINVENTORYFUNCTIONS_H

#include "llinventorymodel.h"
#include "llinventory.h"
#include "llhandle.h"
#include "llwearabletype.h"

// compute_stock_count() return error code
const S32 COMPUTE_STOCK_INFINITE = -1;
const S32 COMPUTE_STOCK_NOT_EVALUATED = -2;

/********************************************************************************
 **                                                                            **
 **                    MISCELLANEOUS GLOBAL FUNCTIONS
 **/

// Is this a parent folder to a worn item
BOOL get_is_parent_to_worn_item(const LLUUID& id);

// Is this item or its baseitem is worn, attached, etc...
BOOL get_is_item_worn(const LLUUID& id);

// Could this item be worn (correct type + not already being worn)
BOOL get_can_item_be_worn(const LLUUID& id);

BOOL get_is_item_removable(const LLInventoryModel* model, const LLUUID& id);

// Performs the appropiate edit action (if one exists) for this item
bool get_is_item_editable(const LLUUID& inv_item_id);
void handle_item_edit(const LLUUID& inv_item_id);

BOOL get_is_category_removable(const LLInventoryModel* model, const LLUUID& id);

BOOL get_is_category_renameable(const LLInventoryModel* model, const LLUUID& id);

void show_item_profile(const LLUUID& item_uuid);
void show_task_item_profile(const LLUUID& item_uuid, const LLUUID& object_id);

void show_item_original(const LLUUID& item_uuid);
void reset_inventory_filter();

// Nudge the listing categories in the inventory to signal that their marketplace status changed
void update_marketplace_category(const LLUUID& cat_id, bool perform_consistency_enforcement = true, bool skip_clear_listing = false);
// Nudge all listing categories to signal that their marketplace status changed
void update_all_marketplace_count();

void rename_category(LLInventoryModel* model, const LLUUID& cat_id, const std::string& new_name);

void copy_inventory_category(LLInventoryModel* model, LLViewerInventoryCategory* cat, const LLUUID& parent_id, const LLUUID& root_copy_id = LLUUID::null, bool move_no_copy_items = false);

void copy_inventory_category_content(const LLUUID& new_cat_uuid, LLInventoryModel* model, LLViewerInventoryCategory* cat, const LLUUID& root_copy_id, bool move_no_copy_items);

// Generates a string containing the path to the item specified by item_id.
void append_path(const LLUUID& id, std::string& path);

typedef boost::function<void(std::string& validation_message, S32 depth, LLError::ELevel log_level)> validation_callback_t;

bool can_move_item_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryItem* inv_item, std::string& tooltip_msg, S32 bundle_size = 1, bool from_paste = false);
bool can_move_folder_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryCategory* inv_cat, std::string& tooltip_msg, S32 bundle_size = 1, bool check_items = true, bool from_paste = false);
bool move_item_to_marketplacelistings(LLInventoryItem* inv_item, LLUUID dest_folder, bool copy = false);
bool move_folder_to_marketplacelistings(LLInventoryCategory* inv_cat, const LLUUID& dest_folder, bool copy = false, bool move_no_copy_items = false);
bool validate_marketplacelistings(LLInventoryCategory* inv_cat, validation_callback_t cb = NULL, bool fix_hierarchy = true, S32 depth = -1, bool notify_observers = true);
S32  depth_nesting_in_marketplace(LLUUID cur_uuid);
LLUUID nested_parent_id(LLUUID cur_uuid, S32 depth);
S32 compute_stock_count(LLUUID cat_uuid, bool force_count = false);

void change_item_parent(const LLUUID& item_id, const LLUUID& new_parent_id);
void move_items_to_new_subfolder(const uuid_vec_t& selected_uuids, const std::string& folder_name);
void move_items_to_folder(const LLUUID& new_cat_uuid, const uuid_vec_t& selected_uuids);
bool is_only_cats_selected(const uuid_vec_t& selected_uuids);
bool is_only_items_selected(const uuid_vec_t& selected_uuids);

/**                    Miscellaneous global functions
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    INVENTORY COLLECTOR FUNCTIONS
 **/

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCollectFunctor
//
// Base class for LLInventoryModel::collectDescendentsIf() method
// which accepts an instance of one of these objects to use as the
// function to determine if it should be added. Derive from this class
// and override the () operator to return TRUE if you want to collect
// the category or item passed in.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCollectFunctor
{
public:
	virtual ~LLInventoryCollectFunctor(){};
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item) = 0;

	static bool itemTransferCommonlyAllowed(const LLInventoryItem* item);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLAssetIDMatches
//
// This functor finds inventory items pointing to the specified asset
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLViewerInventoryItem;

class LLAssetIDMatches : public LLInventoryCollectFunctor
{
public:
	LLAssetIDMatches(const LLUUID& asset_id) : mAssetID(asset_id) {}
	virtual ~LLAssetIDMatches() {}
	bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
	
protected:
	LLUUID mAssetID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLLinkedItemIDMatches
//
// This functor finds inventory items linked to the specific inventory id.
// Assumes the inventory id is itself not a linked item.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLLinkedItemIDMatches : public LLInventoryCollectFunctor
{
public:
	LLLinkedItemIDMatches(const LLUUID& item_id) : mBaseItemID(item_id) {}
	virtual ~LLLinkedItemIDMatches() {}
	bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
	
protected:
	LLUUID mBaseItemID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIsType
//
// Implementation of a LLInventoryCollectFunctor which returns TRUE if
// the type is the type passed in during construction.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLIsType : public LLInventoryCollectFunctor
{
public:
	LLIsType(LLAssetType::EType type) : mType(type) {}
	virtual ~LLIsType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLAssetType::EType mType;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIsNotType
//
// Implementation of a LLInventoryCollectFunctor which returns FALSE if the
// type is the type passed in during construction, otherwise false.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLIsNotType : public LLInventoryCollectFunctor
{
public:
	LLIsNotType(LLAssetType::EType type) : mType(type) {}
	virtual ~LLIsNotType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLAssetType::EType mType;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIsOfAssetType
//
// Implementation of a LLInventoryCollectFunctor which returns TRUE if
// the item or category is of asset type passed in during construction.
// Link types are treated as links, not as the types they point to.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLIsOfAssetType : public LLInventoryCollectFunctor
{
public:
	LLIsOfAssetType(LLAssetType::EType type) : mType(type) {}
	virtual ~LLIsOfAssetType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLAssetType::EType mType;
};

class LLIsValidItemLink : public LLInventoryCollectFunctor
{
public:
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
};

class LLIsTypeWithPermissions : public LLInventoryCollectFunctor
{
public:
	LLIsTypeWithPermissions(LLAssetType::EType type, const PermissionBit perms, const LLUUID &agent_id, const LLUUID &group_id) 
		: mType(type), mPerm(perms), mAgentID(agent_id), mGroupID(group_id) {}
	virtual ~LLIsTypeWithPermissions() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLAssetType::EType mType;
	PermissionBit mPerm;
	LLUUID			mAgentID;
	LLUUID			mGroupID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLBuddyCollector
//
// Simple class that collects calling cards that are not null, and not
// the agent. Duplicates are possible.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLBuddyCollector : public LLInventoryCollectFunctor
{
public:
	LLBuddyCollector() {}
	virtual ~LLBuddyCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLUniqueBuddyCollector
//
// Simple class that collects calling cards that are not null, and not
// the agent. Duplicates are discarded.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLUniqueBuddyCollector : public LLInventoryCollectFunctor
{
public:
	LLUniqueBuddyCollector() {}
	virtual ~LLUniqueBuddyCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);

protected:
	std::set<LLUUID> mSeen;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLParticularBuddyCollector
//
// Simple class that collects calling cards that match a particular uuid
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLParticularBuddyCollector : public LLInventoryCollectFunctor
{
public:
	LLParticularBuddyCollector(const LLUUID& id) : mBuddyID(id) {}
	virtual ~LLParticularBuddyCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLUUID mBuddyID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLNameCategoryCollector
//
// Collects categories based on case-insensitive match of prefix
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLNameCategoryCollector : public LLInventoryCollectFunctor
{
public:
	LLNameCategoryCollector(const std::string& name) : mName(name) {}
	virtual ~LLNameCategoryCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	std::string mName;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFindCOFValidItems
//
// Collects items that can be legitimately linked to in the COF.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLFindCOFValidItems : public LLInventoryCollectFunctor
{
public:
	LLFindCOFValidItems() {}
	virtual ~LLFindCOFValidItems() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFindByMask
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLFindByMask : public LLInventoryCollectFunctor
{
public:
	LLFindByMask(U64 mask)
		: mFilterMask(mask)
	{}

	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		//converting an inventory type to a bitmap filter mask
		if(item && (mFilterMask & (1LL << item->getInventoryType())) )
		{
			return true;
		}

		return false;
	}

private:
	U64 mFilterMask;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFindNonLinksByMask
//
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLFindNonLinksByMask : public LLInventoryCollectFunctor
{
public:
	LLFindNonLinksByMask(U64 mask)
		: mFilterMask(mask)
	{}

	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		if(item && !item->getIsLinkType() && (mFilterMask & (1LL << item->getInventoryType())) )
		{
			return true;
		}

		return false;
	}

	void setFilterMask(U64 mask)
	{
		mFilterMask = mask;
	}

private:
	U64 mFilterMask;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFindWearables
//
// Collects wearables based on item type.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLFindWearables : public LLInventoryCollectFunctor
{
public:
	LLFindWearables() {}
	virtual ~LLFindWearables() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFindWearablesEx
//
// Collects wearables based on given criteria.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLFindWearablesEx : public LLInventoryCollectFunctor
{
public:
	LLFindWearablesEx(bool is_worn, bool include_body_parts = true);
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
private:
	bool mIncludeBodyParts;
	bool mIsWorn;
};

//Inventory collect functor collecting wearables of a specific wearable type
class LLFindWearablesOfType : public LLInventoryCollectFunctor
{
public:
	LLFindWearablesOfType(LLWearableType::EType type) : mWearableType(type) {}
	virtual ~LLFindWearablesOfType() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
	void setType(LLWearableType::EType type);

private:
	LLWearableType::EType mWearableType;
};

/** Filter out wearables-links */
class LLFindActualWearablesOfType : public LLFindWearablesOfType
{
public:
	LLFindActualWearablesOfType(LLWearableType::EType type) : LLFindWearablesOfType(type) {}
	virtual ~LLFindActualWearablesOfType() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		if (item && item->getIsLinkType()) return false;
		return LLFindWearablesOfType::operator()(cat, item);
	}
};

/* Filters out items of a particular asset type */
class LLIsTypeActual : public LLIsType
{
public:
	LLIsTypeActual(LLAssetType::EType type) : LLIsType(type) {}
	virtual ~LLIsTypeActual() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		if (item && item->getIsLinkType()) return false;
		return LLIsType::operator()(cat, item);
	}
};

// Collect non-removable folders and items.
class LLFindNonRemovableObjects : public LLInventoryCollectFunctor
{
public:
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
};

/**                    Inventory Collector Functions
 **                                                                            **
 *******************************************************************************/
class LLFolderViewItem;
class LLFolderViewFolder;
class LLInventoryModel;
class LLFolderView;

class LLInventoryState
{
public:
	// HACK: Until we can route this info through the instant message hierarchy
	static BOOL sWearNewClothing;
	static LLUUID sWearNewClothingTransactionID;	// wear all clothing in this transaction	
};

struct LLInventoryAction
{
	static void doToSelected(LLInventoryModel* model, LLFolderView* root, const std::string& action, BOOL user_confirm = TRUE);
	static void callback_doToSelected(const LLSD& notification, const LLSD& response, class LLInventoryModel* model, class LLFolderView* root, const std::string& action);
	static void callback_copySelected(const LLSD& notification, const LLSD& response, class LLInventoryModel* model, class LLFolderView* root, const std::string& action);
	static void onItemsRemovalConfirmation(const LLSD& notification, const LLSD& response, LLHandle<LLFolderView> root);
	static void removeItemFromDND(LLFolderView* root);

    static void saveMultipleTextures(const std::vector<std::string>& filenames, std::set<LLFolderViewItem*> selected_items, LLInventoryModel* model);

	static const int sConfirmOnDeleteItemsNumber;

private:
	static void buildMarketplaceFolders(LLFolderView* root);
	static void updateMarketplaceFolders();
	static std::list<LLUUID> sMarketplaceFolders; // Marketplace folders that will need update once the action is completed
};


#endif // LL_LLINVENTORYFUNCTIONS_H



