/** 
 * @file llworldmapmessage.h
 * @brief Handling of the messages to the DB made by and for the world map.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_LLWORLDMAPMESSAGE_H
#define LL_LLWORLDMAPMESSAGE_H

// Handling of messages (send and process) as well as SLURL callback if necessary
class LLMessageSystem;

class LLWorldMapMessage : public LLSingleton<LLWorldMapMessage>
{
public:
	typedef boost::function<void(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)>
		url_callback_t;

	LLWorldMapMessage();
	~LLWorldMapMessage();

	// Process incoming answers to map stuff requests
	static void processMapBlockReply(LLMessageSystem*, void**);
	static void processMapItemReply(LLMessageSystem*, void**);

	// Request data for all regions in a rectangular area. Coordinates in grids (i.e. meters / 256).
	void sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent = false);

	// Various methods to request LLSimInfo data to the simulator and asset DB
	void sendNamedRegionRequest(std::string region_name);
	void sendNamedRegionRequest(std::string region_name, 
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport);
	void sendHandleRegionRequest(U64 region_handle, 
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport);

	// Request item data for regions
	// Note: the handle works *only* when requesting agent count (type = MAP_ITEM_AGENT_LOCATIONS). In that case,
	// the request will actually be transitting through the spaceserver (all that is done on the sim).
	// All other values of type do create a global grid request to the asset DB. So no need to try to get, say,
	// the events for one particular region. For such a request, the handle is ignored.
	void sendItemRequest(U32 type, U64 handle = 0);

private:
	// Search for region (by name or handle) for SLURL processing and teleport
	// None of this relies explicitly on the LLWorldMap instance so better handle it here
	std::string		mSLURLRegionName;
	U64				mSLURLRegionHandle;
	std::string		mSLURL;
	url_callback_t	mSLURLCallback;
	bool			mSLURLTeleport;
};

#endif // LL_LLWORLDMAPMESSAGE_H
