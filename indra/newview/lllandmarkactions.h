/** 
 * @file lllandmarkactions.h
 * @brief LLLandmark class declaration
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLLANDMARKACTIONS_H
#define LL_LLLANDMARKACTIONS_H

#include "llinventorymodel.h"

#include "lllandmarklist.h"

class LLLandmark;

/**
 * @brief Provides helper functions to manage landmarks
 */
class LLLandmarkActions
{
public:
	typedef boost::function<void(std::string& slurl)> slurl_callback_t;
	typedef boost::function<void(std::string& slurl, S32 x, S32 y, S32 z)> region_name_and_coords_callback_t;

	/**
	 * @brief Fetches landmark LLViewerInventoryItems for the given landmark name. 
	 */
	static LLInventoryModel::item_array_t fetchLandmarksByName(std::string& name, BOOL if_use_substring);
	/**
	 * @brief Checks whether landmark exists for current agent position.
	 */
	static bool landmarkAlreadyExists();
	
	/**
	 * @brief Checks whether landmark exists for current agent parcel.
	 */
	static bool hasParcelLandmark();

	/**
	 * @brief Searches landmark for global position.
	 * @return Returns landmark or NULL.
	 * 
	 * *TODO: dzaporozhan: There can be many landmarks for single parcel.
	 */
	static LLViewerInventoryItem* findLandmarkForGlobalPos(const LLVector3d &pos);

	/**
	 * @brief Searches landmark for agent global position.
	 * @return Returns landmark or NULL.
	 * 
	 * *TODO: dzaporozhan: There can be many landmarks for single parcel.
	 */
	static LLViewerInventoryItem* findLandmarkForAgentPos();


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
	 * @brief Creates SLURL for given global position.
	 */
	static void getSLURLfromPosGlobal(const LLVector3d& global_pos, slurl_callback_t cb, bool escaped = true);

	static void getRegionNameAndCoordsFromPosGlobal(const LLVector3d& global_pos, region_name_and_coords_callback_t cb);

    /**
     * @brief Gets landmark global position specified by inventory LLUUID.
     * Found position is placed into "posGlobal" variable.
     *.
     * @return - true if specified item exists in Inventory and an appropriate landmark found.
     * and its position is known, false otherwise.
     */
    // *TODO: mantipov: profide callback for cases, when Landmark is not loaded yet.
    static bool getLandmarkGlobalPos(const LLUUID& landmarkInventoryItemID, LLVector3d& posGlobal);

    /**
     * @brief Retrieve a landmark from gLandmarkList by inventory item's id
     * If a landmark is not currently in the gLandmarkList a callback "cb" is called when it is loaded.
     * 
     * @return pointer to loaded landmark from gLandmarkList or NULL if landmark does not exist or wasn't loaded.
     */
    static LLLandmark* getLandmark(const LLUUID& landmarkInventoryItemID, LLLandmarkList::loaded_callback_t cb = NULL);

    /**
     * @brief  Performs standard action of copying of SLURL from landmark to user's clipboard.
     * 	This action requires additional server request. The user will be notified  by info message, 
     *  when URL is copied .
     */
    static void copySLURLtoClipboard(const LLUUID& landmarkInventoryItemID);

private:
    LLLandmarkActions();
    LLLandmarkActions(const LLLandmarkActions&);

	static void onRegionResponseSLURL(slurl_callback_t cb,
								 const LLVector3d& global_pos,
								 bool escaped,
								 const std::string& url);
	static void onRegionResponseNameAndCoords(region_name_and_coords_callback_t cb,
								 const LLVector3d& global_pos,
								 U64 region_handle);
};

#endif //LL_LLLANDMARKACTIONS_H
