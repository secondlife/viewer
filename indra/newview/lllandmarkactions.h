/** 
 * @file lllandmarkactions.h
 * @brief LLLandmark class declaration
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_LLLANDMARKACTIONS_H
#define LL_LLLANDMARKACTIONS_H

#include "llinventorymodel.h"

/**
 * @brief Provides helper functions to manage landmarks
 */
class LLLandmarkActions
{
public:
	typedef boost::function<void(const LLVector3d& global_pos, std::string& slurl)> slurl_signal_t;

	/**
	 * @brief Fetches landmark LLViewerInventoryItems for the given landmark name. 
	 */
	static LLInventoryModel::item_array_t fetchLandmarksByName(std::string& name);
	/**
	 * @brief Checks whether landmark exists for current parcel.
	 */
	static bool landmarkAlreadyExists();

	/**
	 * @brief Checks whether agent has rights to create landmark for current parcel.
	 */
	static bool canCreateLandmarkHere();

	/**
	 * @brief Creates landmark for current parcel.
	 */
	static void createLandmarkHere();

	/**
	 * @brief Creates landmark for current parcel.
	 */
	static void createLandmarkHere(
		const std::string& name, 
		const std::string& desc, 
		const LLUUID& folder_id);

	/**
	 * @brief Trying to find in inventory a landmark of the  current parcel.
	 * Normally items should contain only one item, 
	 * because we can create the only landmark per parcel according to Navigation spec.
	 */	
	static void collectParcelLandmark(LLInventoryModel::item_array_t& items);
	
	/**
	 * @brief Creates SLURL for given global position.
	 */
	static void getSLURLfromPosGlobal(const LLVector3d& global_pos, slurl_signal_t signal);

    /**
     * @brief Gets landmark global position specified by inventory LLUUID.
     * Found position is placed into "posGlobal" variable.
     *.
     * @return - true if specified item exists in Inventory and an appropriate landmark found.
     * and its position is known, false otherwise.
     */
    // *TODO: mantipov: profide callback for cases, when Landmark is not loaded yet.
    static bool getLandmarkGlobalPos(const LLUUID& landmarkInventoryItemID, LLVector3d& posGlobal);

private:
    LLLandmarkActions();
    LLLandmarkActions(const LLLandmarkActions&);

	static void onRegionResponse(slurl_signal_t signal,
								 const LLVector3d& global_pos,
								 U64 region_handle,
								 const std::string& url,
								 const LLUUID& snapshot_id,
								 bool teleport);
};

#endif //LL_LLLANDMARKACTIONS_H
