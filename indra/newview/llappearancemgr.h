/** 
 * @file llappearancemgr.h
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLAPPEARANCEMGR_H
#define LL_LLAPPEARANCEMGR_H

#include "llsingleton.h"

#include "llagentwearables.h"
#include "llcallbacklist.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llviewerinventory.h"
#include "llcorehttputil.h"

class LLWearableHoldingPattern;
class LLInventoryCallback;
class LLOutfitUnLockTimer;

class LLAppearanceMgr: public LLSingleton<LLAppearanceMgr>
{
	LLSINGLETON(LLAppearanceMgr);
	~LLAppearanceMgr();
	LOG_CLASS(LLAppearanceMgr);

	friend class LLOutfitUnLockTimer;
	
public:
	typedef std::vector<LLInventoryModel::item_array_t> wearables_by_type_t;

	void updateAppearanceFromCOF(bool enforce_item_restrictions = true,
								 bool enforce_ordering = true,
								 nullary_func_t post_update_func = no_op);
	void updateCOF(const LLUUID& category, bool append = false);
	void wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append);
	void wearInventoryCategoryOnAvatar(LLInventoryCategory* category, bool append);
	void wearCategoryFinal(LLUUID& cat_id, bool copy_items, bool append);
	void wearOutfitByName(const std::string& name);
	void changeOutfit(bool proceed, const LLUUID& category, bool append);
	void replaceCurrentOutfit(const LLUUID& new_outfit);
	void renameOutfit(const LLUUID& outfit_id);
	void removeOutfitPhoto(const LLUUID& outfit_id);
	void takeOffOutfit(const LLUUID& cat_id);
	void addCategoryToCurrentOutfit(const LLUUID& cat_id);
	S32 findExcessOrDuplicateItems(const LLUUID& cat_id,
								   LLAssetType::EType type,
								   S32 max_items_per_type,
								   S32 max_items_total,
								   LLInventoryObject::object_list_t& items_to_kill);
	void findAllExcessOrDuplicateItems(const LLUUID& cat_id,
									  LLInventoryObject::object_list_t& items_to_kill);
	void enforceCOFItemRestrictions(LLPointer<LLInventoryCallback> cb);

	S32 getActiveCopyOperations() const;

	// Replace category contents with copied links via the slam_inventory_folder
	// command (single inventory operation where supported)
	void slamCategoryLinks(const LLUUID& src_id, const LLUUID& dst_id,
						   bool include_folder_links, LLPointer<LLInventoryCallback> cb);
 
	// Copy all items and the src category itself.
	void shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
							 LLPointer<LLInventoryCallback> cb);

	// Return whether this folder contains minimal contents suitable for making a full outfit.
	BOOL getCanMakeFolderIntoOutfit(const LLUUID& folder_id);

	// Determine whether a given outfit can be removed.
	bool getCanRemoveOutfit(const LLUUID& outfit_cat_id);

	// Determine whether we're wearing any of the outfit contents (excluding body parts).
	static bool getCanRemoveFromCOF(const LLUUID& outfit_cat_id);

	// Determine whether we can add anything (but body parts) from the outfit contents to COF.
	static bool getCanAddToCOF(const LLUUID& outfit_cat_id);

	// Determine whether we can replace current outfit with the given one.
	bool getCanReplaceCOF(const LLUUID& outfit_cat_id);

    // Can we add all referenced items to the avatar?
    bool canAddWearables(const uuid_vec_t& item_ids);
    
	// Copy all items in a category.
	void shallowCopyCategoryContents(const LLUUID& src_id, const LLUUID& dst_id,
									 LLPointer<LLInventoryCallback> cb);

	// Find the Current Outfit folder.
	const LLUUID getCOF() const;
	S32 getCOFVersion() const;

	// Debugging - get truncated LLSD summary of COF contents.
	LLSD dumpCOF() const;

	// Finds the folder link to the currently worn outfit
	const LLViewerInventoryItem *getBaseOutfitLink();
	bool getBaseOutfitName(std::string &name);

	// find the UUID of the currently worn outfit (Base Outfit)
	const LLUUID getBaseOutfitUUID();

    void wearItemsOnAvatar(const uuid_vec_t& item_ids_to_wear,
                           bool do_update,
                           bool replace,
                           LLPointer<LLInventoryCallback> cb = NULL);

	// Wear/attach an item (from a user's inventory) on the agent
	void wearItemOnAvatar(const LLUUID& item_to_wear, bool do_update, bool replace = false,
						  LLPointer<LLInventoryCallback> cb = NULL);

	// Update the displayed outfit name in UI.
	void updatePanelOutfitName(const std::string& name);

	void purgeBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> cb = NULL);
	void createBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> link_waiter);

	void updateAgentWearables(LLWearableHoldingPattern* holder);

	S32 countActiveHoldingPatterns();

	// For debugging - could be moved elsewhere.
	void dumpCat(const LLUUID& cat_id, const std::string& msg);
	void dumpItemArray(const LLInventoryModel::item_array_t& items, const std::string& msg);

	// Attachment link management
	void unregisterAttachment(const LLUUID& item_id);
	void registerAttachment(const LLUUID& item_id);
	void setAttachmentInvLinkEnable(bool val);

	// Add COF link to individual item.
	void addCOFItemLink(const LLUUID& item_id, LLPointer<LLInventoryCallback> cb = NULL, const std::string description = "");
	void addCOFItemLink(const LLInventoryItem *item, LLPointer<LLInventoryCallback> cb = NULL, const std::string description = "");

	// Find COF entries referencing the given item.
	LLInventoryModel::item_array_t findCOFItemLinks(const LLUUID& item_id);
	bool isLinkedInCOF(const LLUUID& item_id);

	// Remove COF entries
	void removeCOFItemLinks(const LLUUID& item_id, LLPointer<LLInventoryCallback> cb = NULL);
	void removeCOFLinksOfType(LLWearableType::EType type, LLPointer<LLInventoryCallback> cb = NULL);
	void removeAllClothesFromAvatar();
	void removeAllAttachmentsFromAvatar();

	// Special handling of temp attachments, which are not in the COF
	bool shouldRemoveTempAttachment(const LLUUID& item_id);

	//has the current outfit changed since it was loaded?
	bool isOutfitDirty() { return mOutfitIsDirty; }

	// set false if you just loaded the outfit, true otherwise
	void setOutfitDirty(bool isDirty) { mOutfitIsDirty = isDirty; }
	
	// manually compare ouftit folder link to COF to see if outfit has changed.
	// should only be necessary to do on initial login.
	void updateIsDirty();

	void setOutfitLocked(bool locked);

	// Called when self avatar is first fully visible.
	void onFirstFullyVisible();

	// Copy initial gestures from library.
	void copyLibraryGestures();
	
	void wearBaseOutfit();

	void setOutfitImage(const LLUUID& image_id) {mCOFImageID = image_id;}
	LLUUID getOutfitImage() {return mCOFImageID;}

	// Overrides the base outfit with the content from COF
	// @return false if there is no base outfit
	bool updateBaseOutfit();

	//Remove clothing or detach an object from the agent (a bodypart cannot be removed)
	void removeItemsFromAvatar(const uuid_vec_t& item_ids);
	void removeItemFromAvatar(const LLUUID& item_id);


	void onOutfitFolderCreated(const LLUUID& folder_id, bool show_panel);
	void onOutfitFolderCreatedAndClothingOrdered(const LLUUID& folder_id, bool show_panel);

	void makeNewOutfitLinks(const std::string& new_folder_name,bool show_panel = true);

	bool moveWearable(LLViewerInventoryItem* item, bool closer_to_body);

	static void sortItemsByActualDescription(LLInventoryModel::item_array_t& items);

	//Divvy items into arrays by wearable type
	static void divvyWearablesByType(const LLInventoryModel::item_array_t& items, wearables_by_type_t& items_by_type);

	typedef std::map<LLUUID,std::string> desc_map_t;

	void getWearableOrderingDescUpdates(LLInventoryModel::item_array_t& wear_items, desc_map_t& desc_map);

	//Check ordering information on wearables stored in links' descriptions and update if it is invalid
	// COF is processed if cat_id is not specified
	bool validateClothingOrderingInfo(LLUUID cat_id = LLUUID::null);
	
	void updateClothingOrderingInfo(LLUUID cat_id = LLUUID::null,
									LLPointer<LLInventoryCallback> cb = NULL);

	bool isOutfitLocked() { return mOutfitLocked; }

	bool isInUpdateAppearanceFromCOF() { return mIsInUpdateAppearanceFromCOF; }

	static void onIdle(void *);
	void requestServerAppearanceUpdate();

	void setAppearanceServiceURL(const std::string& url) { mAppearanceServiceURL = url; }
	std::string getAppearanceServiceURL() const;

	typedef boost::function<void ()> attachments_changed_callback_t;
	typedef boost::signals2::signal<void ()> attachments_changed_signal_t;
	boost::signals2::connection setAttachmentsChangedCallback(attachments_changed_callback_t cb);


private:
    void serverAppearanceUpdateCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter);

    static void debugAppearanceUpdateCOF(const LLSD& content);

	std::string		mAppearanceServiceURL;

private:

	void filterWearableItems(LLInventoryModel::item_array_t& items, S32 max_per_type, S32 max_total);
	
	void getDescendentsOfAssetType(const LLUUID& category, 
										  LLInventoryModel::item_array_t& items,
										  LLAssetType::EType type);

	void getUserDescendents(const LLUUID& category, 
								   LLInventoryModel::item_array_t& wear_items,
								   LLInventoryModel::item_array_t& obj_items,
								   LLInventoryModel::item_array_t& gest_items);

	static void onOutfitRename(const LLSD& notification, const LLSD& response);

	bool mAttachmentInvLinkEnabled;
	bool mOutfitIsDirty;
	bool mIsInUpdateAppearanceFromCOF; // to detect recursive calls.
    bool mOutstandingAppearanceBakeRequest; // A bake request is outstanding.  Do not overlap.
    bool mRerequestAppearanceBake;

	/**
	 * Lock for blocking operations on outfit until server reply or timeout exceed
	 * to avoid unsynchronized outfit state or performing duplicate operations.
	 */
	bool mOutfitLocked;
	LLTimer mInFlightTimer;
	static bool mActive;

	attachments_changed_signal_t		mAttachmentsChangeSignal;
	
	LLUUID mCOFImageID;

	std::auto_ptr<LLOutfitUnLockTimer> mUnlockOutfitTimer;

	// Set of temp attachment UUIDs that should be removed
	typedef std::set<LLUUID> doomed_temp_attachments_t;
	doomed_temp_attachments_t	mDoomedTempAttachmentIDs;

	void addDoomedTempAttachment(const LLUUID& id_to_remove);

	//////////////////////////////////////////////////////////////////////////////////
	// Item-specific convenience functions 
public:
	// Is this in the COF?
	BOOL getIsInCOF(const LLUUID& obj_id) const;
	// Is this in the COF and can the user delete it from the COF?
	BOOL getIsProtectedCOFItem(const LLUUID& obj_id) const;

	// Outfits will prioritize textures with such name to use for preview in gallery
	static const std::string sExpectedTextureName;
};

class LLUpdateAppearanceOnDestroy: public LLInventoryCallback
{
public:
	LLUpdateAppearanceOnDestroy(bool enforce_item_restrictions = true,
								bool enforce_ordering = true,
								nullary_func_t post_update_func = no_op);
	virtual ~LLUpdateAppearanceOnDestroy();
	/* virtual */ void fire(const LLUUID& inv_item);

private:
	U32 mFireCount;
	bool mEnforceItemRestrictions;
	bool mEnforceOrdering;
	nullary_func_t mPostUpdateFunc;
};

class LLUpdateAppearanceAndEditWearableOnDestroy: public LLInventoryCallback
{
public:
	LLUpdateAppearanceAndEditWearableOnDestroy(const LLUUID& item_id);

	/* virtual */ void fire(const LLUUID& item_id) {}

	~LLUpdateAppearanceAndEditWearableOnDestroy();
	
private:
	LLUUID mItemID;
};

class LLRequestServerAppearanceUpdateOnDestroy: public LLInventoryCallback
{
public:
	LLRequestServerAppearanceUpdateOnDestroy() {}
	~LLRequestServerAppearanceUpdateOnDestroy();

	/* virtual */ void fire(const LLUUID& item_id) {}
};

LLUUID findDescendentCategoryIDByName(const LLUUID& parent_id,const std::string& name);

// Invoke a given callable after category contents are fully fetched.
void callAfterCategoryFetch(const LLUUID& cat_id, nullary_func_t cb);

// Wear all items in a uuid vector.
void wear_multiple(const uuid_vec_t& ids, bool replace);

#endif
