/** 
 * @file llworldmap.h
 * @brief Underlying data storage for the map of the entire world.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLWORLDMAP_H
#define LL_LLWORLDMAP_H

#include <map>
#include <string>
#include <vector>

#include "v3math.h"
#include "v3dmath.h"
#include "llframetimer.h"
#include "llmapimagetype.h"
#include "lluuid.h"
#include "llmemory.h"
#include "llviewerimage.h"
#include "lleventinfo.h"
#include "v3color.h"

class LLMessageSystem;


class LLItemInfo
{
public:
	LLItemInfo(F32 global_x, F32 global_y, const std::string& name, LLUUID id, S32 extra = 0, S32 extra2 = 0);

	std::string mName;
	std::string mToolTip;
	LLVector3d	mPosGlobal;
	LLUUID		mID;
	BOOL		mSelected;
	S32			mExtra;
	S32			mExtra2;
	U64			mRegionHandle;
};

#define MAP_SIM_IMAGE_TYPES 3
// 0 - Prim
// 1 - Terrain Only
// 2 - Overlay: Land For Sale

class LLSimInfo
{
public:
	LLSimInfo();

	LLVector3d getGlobalPos(LLVector3 local_pos) const;

public:
	U64 mHandle;
	std::string mName;

	F64 mAgentsUpdateTime;
	BOOL mShowAgentLocations;	// are agents visible?

	U8 mAccess;
	U32 mRegionFlags;
	F32 mWaterHeight;

	F32 mAlpha;

	// Image ID for the current overlay mode.
	LLUUID mMapImageID[MAP_SIM_IMAGE_TYPES];

	// Hold a reference to the currently displayed image.
	LLPointer<LLViewerImage> mCurrentImage;
	LLPointer<LLViewerImage> mOverlayImage;
};

#define MAP_BLOCK_RES 256

struct LLWorldMapLayer
{
	BOOL LayerDefined;
	LLPointer<LLViewerImage> LayerImage;
	LLUUID LayerImageID;
	LLRect LayerExtents;

	LLWorldMapLayer() : LayerDefined(FALSE) { }
};


class LLWorldMap : public LLSingleton<LLWorldMap>
{
public:
	typedef void(*url_callback_t)(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport);

	LLWorldMap();
	~LLWorldMap();

	// clears the list
	void reset();

	// clear the visible items
	void eraseItems();

	// Removes references to cached images
	void clearImageRefs();

	// Clears the flags indicating that we've received sim infos
	// Causes a re-request of the sim info without erasing extisting info
	void clearSimFlags();

	// Returns simulator information, or NULL if out of range
	LLSimInfo* simInfoFromHandle(const U64 handle);

	// Returns simulator information, or NULL if out of range
	LLSimInfo* simInfoFromPosGlobal(const LLVector3d& pos_global);

	// Returns simulator information for named sim, or NULL if non-existent
	LLSimInfo* simInfoFromName(const LLString& sim_name);

	// Gets simulator name for a global position, returns true if it was found
	bool simNameFromPosGlobal(const LLVector3d& pos_global, LLString & outSimName );

	// Sets the current layer
	void setCurrentLayer(S32 layer, bool request_layer = false);

	void sendMapLayerRequest();
	void sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent = false);
	void sendNamedRegionRequest(std::string region_name);
	void sendNamedRegionRequest(std::string region_name, 
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport);
	void sendHandleRegionRequest(U64 region_handle, 
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport);
	void sendItemRequest(U32 type, U64 handle = 0);

	static void processMapLayerReply(LLMessageSystem*, void**);
	static void processMapBlockReply(LLMessageSystem*, void**);
	static void processMapItemReply(LLMessageSystem*, void**);

	void dump();

	// Extend the bounding box of the list of simulators. Returns true
	// if the extents changed.
	BOOL extendAABB(U32 x_min, U32 y_min, U32 x_max, U32 y_max);

	// build coverage maps for telehub region visualization
	void updateTelehubCoverage();
	BOOL coveredByTelehub(LLSimInfo* infop);

	// Bounds of the world, in meters
	U32 getWorldWidth() const;
	U32 getWorldHeight() const;
public:
	// Map from region-handle to simulator info
	typedef std::map<U64, LLSimInfo*> sim_info_map_t;
	sim_info_map_t mSimInfoMap;

	BOOL			mIsTrackingUnknownLocation, mInvalidLocation, mIsTrackingDoubleClick, mIsTrackingCommit;
	LLVector3d		mUnknownLocation;

	bool mRequestLandForSale;

	typedef std::vector<LLItemInfo> item_info_list_t;
	item_info_list_t mTelehubs;
	item_info_list_t mInfohubs;
	item_info_list_t mPGEvents;
	item_info_list_t mMatureEvents;
	item_info_list_t mLandForSale;
	item_info_list_t mClassifieds;

	std::map<U64,S32> mNumAgents;

	typedef std::map<U64, item_info_list_t> agent_list_map_t;
	agent_list_map_t mAgentLocationsMap;
	
	std::vector<LLWorldMapLayer>	mMapLayers[MAP_SIM_IMAGE_TYPES];
	BOOL							mMapLoaded[MAP_SIM_IMAGE_TYPES];
	BOOL *							mMapBlockLoaded[MAP_SIM_IMAGE_TYPES];
	S32								mCurrentMap;

	// AABB of the list of simulators
	U32		mMinX;
	U32		mMaxX;
	U32		mMinY;
	U32		mMaxY;

	U8*		mNeighborMap;
	U8*		mTelehubCoverageMap;
	S32		mNeighborMapWidth;
	S32		mNeighborMapHeight;

private:
	LLTimer	mRequestTimer;

	// search for named region for url processing
	std::string mSLURLRegionName;
	U64 mSLURLRegionHandle;
	std::string mSLURL;
	url_callback_t mSLURLCallback;
	bool mSLURLTeleport;
};

#endif
