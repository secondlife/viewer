/** 
 * @file llappearancemgr.h
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLAPPEARANCEMGR_H
#define LL_LLAPPEARANCEMGR_H

#include "llsingleton.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

class LLWearable;
struct LLWearableHoldingPattern;

class LLAppearanceManager: public LLSingleton<LLAppearanceManager>
{
	friend class LLSingleton<LLAppearanceManager>;
	
public:
	void updateAppearanceFromCOF();
	bool needToSaveCOF();
	void updateCOF(const LLUUID& category, bool append = false);
	void wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append);
	void wearInventoryCategoryOnAvatar(LLInventoryCategory* category, bool append);
	void wearOutfitByName(const std::string& name);
	void changeOutfit(bool proceed, const LLUUID& category, bool append);

	// Add COF link to individual item.
	void wearItem(LLInventoryItem* item, bool do_update = true);

	// Add COF link to ensemble folder.
	void wearEnsemble(LLInventoryCategory* item, bool do_update = true);

	// Copy all items in a category.
	void shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
							 LLPointer<LLInventoryCallback> cb);

	// Find the Current Outfit folder.
	LLUUID getCOF();

	// Remove COF entries
	void removeItemLinks(const LLUUID& item_id, bool do_update = true);

	void updateAgentWearables(LLWearableHoldingPattern* holder, bool append);

	// For debugging - could be moved elsewhere.
	void dumpCat(const LLUUID& cat_id, const std::string& msg);
	void dumpItemArray(const LLInventoryModel::item_array_t& items, const std::string& msg);

	// Attachment link management
	void unregisterAttachment(const LLUUID& item_id);
	void registerAttachment(const LLUUID& item_id);
	void setAttachmentInvLinkEnable(bool val);
	void linkRegisteredAttachments();

protected:
	LLAppearanceManager();
	~LLAppearanceManager();

private:

	void filterWearableItems(LLInventoryModel::item_array_t& items, S32 max_per_type);
	void linkAll(const LLUUID& category,
						LLInventoryModel::item_array_t& items,
						LLPointer<LLInventoryCallback> cb);
	
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

	std::set<LLUUID> mRegisteredAttachments;
	bool mAttachmentInvLinkEnabled;
};

#define SUPPORT_ENSEMBLES 0

#endif
