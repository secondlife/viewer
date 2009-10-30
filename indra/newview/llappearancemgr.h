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
public:
	static void updateAppearanceFromCOF();
	static bool needToSaveCOF();
	static void changeOutfit(bool proceed, const LLUUID& category, bool append);
	static void updateCOF(const LLUUID& category, bool append = false);
	static void updateCOFFromCategory(const LLUUID& category, bool append);
	static void rebuildCOFFromOutfit(const LLUUID& category);
	static void wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append);
	static void wearInventoryCategoryOnAvatar(LLInventoryCategory* category, bool append);
	static void wearOutfitByName(const std::string& name);
	static void shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
									LLPointer<LLInventoryCallback> cb);

	// Add COF link to individual item.
	static void wearItem(LLInventoryItem* item, bool do_update = true);

	// Add COF link to ensemble folder.
	static void wearEnsemble(LLInventoryCategory* item, bool do_update = true);
	static LLUUID getCOF();

	// Remove COF entries
	static void removeItemLinks(const LLUUID& item_id, bool do_update = true);

	// For debugging - could be moved elsewhere.
	static void dumpCat(const LLUUID& cat_id, const std::string& msg);
	static void dumpItemArray(const LLInventoryModel::item_array_t& items, const std::string& msg);

private:
	static void filterWearableItems(LLInventoryModel::item_array_t& items, S32 max_per_type);
	static void linkAll(const LLUUID& category,
						LLInventoryModel::item_array_t& items,
						LLPointer<LLInventoryCallback> cb);
	
	static void getDescendentsOfAssetType(const LLUUID& category, 
										  LLInventoryModel::item_array_t& items,
										  LLAssetType::EType type,
										  bool follow_folder_links);

	static void getCOFValidDescendents(const LLUUID& category, 
									   LLInventoryModel::item_array_t& items);
									   
	static void getUserDescendents(const LLUUID& category, 
								   LLInventoryModel::item_array_t& wear_items,
								   LLInventoryModel::item_array_t& obj_items,
								   LLInventoryModel::item_array_t& gest_items,
								   bool follow_folder_links);
	static void onWearableAssetFetch(LLWearable* wearable, void* data);
	static void updateAgentWearables(LLWearableHoldingPattern* holder, bool append);
	static bool isMandatoryWearableType(EWearableType type);
	static void checkMandatoryWearableTypes(const LLUUID& category, std::set<EWearableType>& types_found);
	static void purgeCOFBeforeRebuild(const LLUUID& category);
	static void purgeCategory(const LLUUID& category, bool keep_outfit_links);
};

#define SUPPORT_ENSEMBLES 0

#endif
