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

class LLWearable;
class LLWearableHoldingPattern;
class LLInventoryCallback;
class LLOutfitUnLockTimer;

class LLAppearanceMgr: public LLSingleton<LLAppearanceMgr>
{
	LOG_CLASS(LLAppearanceMgr);

	friend class LLSingleton<LLAppearanceMgr>;
	friend class LLOutfitUnLockTimer;
	
public:
	typedef std::vector<LLInventoryModel::item_array_t> wearables_by_type_t;

	void updateAppearanceFromCOF(bool update_base_outfit_ordering = false);
	bool needToSaveCOF();
	void updateCOF(const LLUUID& category, bool append = false);
	void wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append);
	void wearInventoryCategoryOnAvatar(LLInventoryCategory* category, bool append);
	void wearCategoryFinal(LLUUID& cat_id, bool copy_items, bool append);
	void wearOutfitByName(const std::string& name);
	void changeOutfit(bool proceed, const LLUUID& category, bool append);
	void replaceCurrentOutfit(const LLUUID& new_outfit);
	void renameOutfit(const LLUUID& outfit_id);
	void takeOffOutfit(const LLUUID& cat_id);
	void addCategoryToCurrentOutfit(const LLUUID& cat_id);
	S32 findExcessOrDuplicateItems(const LLUUID& cat_id,
								   LLAssetType::EType type,
								   S32 max_items,
								   LLInventoryModel::item_array_t& items_to_kill);
	void enforceItemRestrictions();

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

	// Copy all items in a category.
	void shallowCopyCategoryContents(const LLUUID& src_id, const LLUUID& dst_id,
									 LLPointer<LLInventoryCallback> cb);

	// Find the Current Outfit folder.
	const LLUUID getCOF() const;

	// Finds the folder link to the currently worn outfit
	const LLViewerInventoryItem *getBaseOutfitLink();
	bool getBaseOutfitName(std::string &name);

	// find the UUID of the currently worn outfit (Base Outfit)
	const LLUUID getBaseOutfitUUID();

	// Wear/attach an item (from a user's inventory) on the agent
	bool wearItemOnAvatar(const LLUUID& item_to_wear, bool do_update = true, bool replace = false, LLPointer<LLInventoryCallback> cb = NULL);

	// Update the displayed outfit name in UI.
	void updatePanelOutfitName(const std::string& name);

	void createBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> link_waiter);

	void updateAgentWearables(LLWearableHoldingPattern* holder, bool append);

	// For debugging - could be moved elsewhere.
	void dumpCat(const LLUUID& cat_id, const std::string& msg);
	void dumpItemArray(const LLInventoryModel::item_array_t& items, const std::string& msg);

	// Attachment link management
	void unregisterAttachment(const LLUUID& item_id);
	void registerAttachment(const LLUUID& item_id);
	void setAttachmentInvLinkEnable(bool val);

	// utility function for bulk linking.
	void linkAll(const LLUUID& category,
				 LLInventoryModel::item_array_t& items,
				 LLPointer<LLInventoryCallback> cb);

	// Add COF link to individual item.
	void addCOFItemLink(const LLUUID& item_id, bool do_update = true, LLPointer<LLInventoryCallback> cb = NULL);
	void addCOFItemLink(const LLInventoryItem *item, bool do_update = true, LLPointer<LLInventoryCallback> cb = NULL);

	// Remove COF entries
	void removeCOFItemLinks(const LLUUID& item_id, bool do_update = true);
	void removeCOFLinksOfType(LLWearableType::EType type, bool do_update = true);

	// Add COF link to ensemble folder.
	void addEnsembleLink(LLInventoryCategory* item, bool do_update = true);

	//has the current outfit changed since it was loaded?
	bool isOutfitDirty() { return mOutfitIsDirty; }

	// set false if you just loaded the outfit, true otherwise
	void setOutfitDirty(bool isDirty) { mOutfitIsDirty = isDirty; }
	
	// manually compare ouftit folder link to COF to see if outfit has changed.
	// should only be necessary to do on initial login.
	void updateIsDirty();

	// Called when self avatar is first fully visible.
	void onFirstFullyVisible();

	// Create initial outfits from library.
	void autopopulateOutfits();

	// Copy initial gestures from library.
	void copyLibraryGestures();
	
	void wearBaseOutfit();

	// Overrides the base outfit with the content from COF
	// @return false if there is no base outfit
	bool updateBaseOutfit();

	//Remove clothing or detach an object from the agent (a bodypart cannot be removed)
	void removeItemFromAvatar(const LLUUID& item_id);


	LLUUID makeNewOutfitLinks(const std::string& new_folder_name,bool show_panel = true);

	bool moveWearable(LLViewerInventoryItem* item, bool closer_to_body);

	static void sortItemsByActualDescription(LLInventoryModel::item_array_t& items);

	//Divvy items into arrays by wearable type
	static void divvyWearablesByType(const LLInventoryModel::item_array_t& items, wearables_by_type_t& items_by_type);

	//Check ordering information on wearables stored in links' descriptions and update if it is invalid
	// COF is processed if cat_id is not specified
	void updateClothingOrderingInfo(LLUUID cat_id = LLUUID::null, bool update_base_outfit_ordering = false);

	bool isOutfitLocked() { return mOutfitLocked; }

	bool isInUpdateAppearanceFromCOF() { return mIsInUpdateAppearanceFromCOF; }

	bool useServerTextureBaking();

protected:
	LLAppearanceMgr();
	~LLAppearanceMgr();

private:

	void filterWearableItems(LLInventoryModel::item_array_t& items, S32 max_per_type);
	
	void getDescendentsOfAssetType(const LLUUID& category, 
										  LLInventoryModel::item_array_t& items,
										  LLAssetType::EType type,
										  bool follow_folder_links);

	void getUserDescendents(const LLUUID& category, 
								   LLInventoryModel::item_array_t& wear_items,
								   LLInventoryModel::item_array_t& obj_items,
								   LLInventoryModel::item_array_t& gest_items,
								   bool follow_folder_links);

	void purgeCategory(const LLUUID& category, bool keep_outfit_links);
	void purgeBaseOutfitLink(const LLUUID& category);

	static void onOutfitRename(const LLSD& notification, const LLSD& response);

	void setOutfitLocked(bool locked);

	bool mAttachmentInvLinkEnabled;
	bool mOutfitIsDirty;
	bool mIsInUpdateAppearanceFromCOF; // to detect recursive calls.

	/**
	 * Lock for blocking operations on outfit until server reply or timeout exceed
	 * to avoid unsynchronized outfit state or performing duplicate operations.
	 */
	bool mOutfitLocked;

	std::auto_ptr<LLOutfitUnLockTimer> mUnlockOutfitTimer;

	//////////////////////////////////////////////////////////////////////////////////
	// Item-specific convenience functions 
public:
	// Is this in the COF?
	BOOL getIsInCOF(const LLUUID& obj_id) const;
	// Is this in the COF and can the user delete it from the COF?
	BOOL getIsProtectedCOFItem(const LLUUID& obj_id) const;

	/**
	 * Checks if COF contains link to specified object.
	 */
	static bool isLinkInCOF(const LLUUID& obj_id);
};

class LLUpdateAppearanceOnDestroy: public LLInventoryCallback
{
public:
	LLUpdateAppearanceOnDestroy(bool update_base_outfit_ordering = false);
	virtual ~LLUpdateAppearanceOnDestroy();
	/* virtual */ void fire(const LLUUID& inv_item);

private:
	U32 mFireCount;
	bool mUpdateBaseOrder;
};


#define SUPPORT_ENSEMBLES 0

LLUUID findDescendentCategoryIDByName(const LLUUID& parent_id,const std::string& name);

// Invoke a given callable after category contents are fully fetched.
void callAfterCategoryFetch(const LLUUID& cat_id, nullary_func_t cb);

// Wear all items in a uuid vector.
void wear_multiple(const uuid_vec_t& ids, bool replace);

#endif
