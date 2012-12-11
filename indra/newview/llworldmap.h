/** 
 * @file llworldmap.h
 * @brief Underlying data storage for the map of the entire world.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLWORLDMAP_H
#define LL_LLWORLDMAP_H

#include "llworldmipmap.h"
#include <boost/function.hpp>

#include "v3dmath.h"
#include "lluuid.h"
#include "llpointer.h"
#include "llsingleton.h"
#include "llviewerregion.h"
#include "llviewertexture.h"

// Description of objects like hubs, events, land for sale, people and more (TBD).
// Note: we don't store a "type" in there so we need to store instances of this class in 
// well known objects (i.e. list of objects which type is "well known").
class LLItemInfo
{
public:
	LLItemInfo(F32 global_x, F32 global_y, const std::string& name, LLUUID id);

	// Setters
	void setTooltip(const std::string& tooltip) { mToolTip = tooltip; }
	void setElevation(F64 z) { mPosGlobal.mdV[VZ] = z; }
	void setCount(S32 count) { mCount = count; }
//	void setSelected(bool selected) { mSelected = selected; }
//	void setColor(LLColor4 color) { mColor = color; }

	// Accessors
	const LLVector3d& getGlobalPosition() const { return mPosGlobal; } 
	const std::string& getName() const { return mName; }
	const std::string& getToolTip() const { return mToolTip; }
	const LLUUID& getUUID() const { return mID; }
	S32 getCount() const { return mCount; }

	U64 getRegionHandle() const { return to_region_handle(mPosGlobal); }		// Build the handle on the fly

	bool isName(const std::string& name) const { return (mName == name); }		// True if name same as item's name
//	bool isSelected() const { return mSelected; }

private:
	std::string mName;			// Name of the individual item
	std::string mToolTip;		// Tooltip : typically, something to be displayed to the user when selecting this item
	LLVector3d	mPosGlobal;		// Global world position
	LLUUID		mID;			// UUID of the item
	S32			mCount;			// Number of elements in item (e.g. people count)
	// Currently not used but might prove useful one day so we comment out 
//	bool		mSelected;		// Selected or not: updated by the viewer UI, not the simulator or asset DB
//	LLColor4	mColor;			// Color of the item
};

// Info per region
// Such records are stored in a global map hold by the LLWorldMap and indexed by region handles. 
// To avoid creating too many of them, they are requested in "blocks" corresponding to areas covered by the screen. 
// Unfortunately, when the screen covers the whole world (zoomed out), that can translate in requesting info for 
// every sim on the grid... Not good...
// To avoid this, the code implements a cut-off threshold for overlay graphics and, therefore, all LLSimInfo. 
// In other words, when zooming out too much, we simply stop requesting LLSimInfo and
// LLItemInfo and just display the map tiles. 
// As they are stored in different structures (LLSimInfo and LLWorldMipmap), this strategy is now workable.
class LLSimInfo
{
public:
	LLSimInfo(U64 handle);

	// Convert local region coordinates into world coordinates
	LLVector3d getGlobalPos(const LLVector3& local_pos) const;
	// Get the world coordinates of the SW corner of that region
	LLVector3d getGlobalOrigin() const;
	LLVector3 getLocalPos(LLVector3d global_pos) const;

	void clearImage();					// Clears the reference to the Land for sale image for that region
	void dropImagePriority();			// Drops the boost level of the Land for sale image for that region
	void updateAgentCount(F64 time);	// Send an item request for agent count on that region if time's up

	// Setters
	void setName(std::string& name) { mName = name; }
	void setAccess (U32 accesscode) { mAccess = accesscode; }
	void setRegionFlags (U64 region_flags) { mRegionFlags = region_flags; }
	void setLandForSaleImage (LLUUID image_id);
//	void setWaterHeight (F32 water_height) { mWaterHeight = water_height; }

	// Accessors
	std::string getName() const { return mName; }
	const std::string getFlagsString() const { return LLViewerRegion::regionFlagsToString(mRegionFlags); }
	const std::string getAccessString() const { return LLViewerRegion::accessToString((U8)mAccess); }

	const S32 getAgentCount() const;				// Compute the total agents count
	LLPointer<LLViewerFetchedTexture> getLandForSaleImage();	// Get the overlay image, fetch it if necessary

	bool isName(const std::string& name) const;
	bool isDown() { return (mAccess == SIM_ACCESS_DOWN); }
	bool isPG() { return (mAccess <= SIM_ACCESS_PG); }
	bool isAdult() { return (mAccess == SIM_ACCESS_ADULT); }

	// Debug only
	void dump() const;	// Print the region info to the standard output

	// Items lists handling
	typedef std::vector<LLItemInfo> item_info_list_t;
	void clearItems();

	void insertTeleHub(const LLItemInfo& item) { mTelehubs.push_back(item); }
	void insertInfoHub(const LLItemInfo& item) { mInfohubs.push_back(item); }
	void insertPGEvent(const LLItemInfo& item) { mPGEvents.push_back(item); }
	void insertMatureEvent(const LLItemInfo& item) { mMatureEvents.push_back(item); }
	void insertAdultEvent(const LLItemInfo& item) { mAdultEvents.push_back(item); }
	void insertLandForSale(const LLItemInfo& item) { mLandForSale.push_back(item); }
	void insertLandForSaleAdult(const LLItemInfo& item) { mLandForSaleAdult.push_back(item); }
	void insertAgentLocation(const LLItemInfo& item);

	const LLSimInfo::item_info_list_t& getTeleHub() const { return mTelehubs; }
	const LLSimInfo::item_info_list_t& getInfoHub() const { return mInfohubs; }
	const LLSimInfo::item_info_list_t& getPGEvent() const { return mPGEvents; }
	const LLSimInfo::item_info_list_t& getMatureEvent() const { return mMatureEvents; }
	const LLSimInfo::item_info_list_t& getAdultEvent() const { return mAdultEvents; }
	const LLSimInfo::item_info_list_t& getLandForSale() const { return mLandForSale; }
	const LLSimInfo::item_info_list_t& getLandForSaleAdult() const { return mLandForSaleAdult; }
	const LLSimInfo::item_info_list_t& getAgentLocation() const { return mAgentLocations; }

private:
	U64 mHandle;				// This is a hash of the X and Y world coordinates of the SW corner of the sim
	std::string mName;			// Region name

	F64 mAgentsUpdateTime;		// Time stamp giving the last time the agents information was requested for that region
	bool mFirstAgentRequest;	// Init agent request flag

	U32  mAccess;				// Down/up and maturity rating of the region
	U64 mRegionFlags;			// Tell us if the siminfo has been received (if non 0) and what kind of region it is (Sandbox, allow damage)
	// Currently not used but might prove useful one day so we comment out 
//	F32 mWaterHeight;			// Water height on the region (not actively used)

	// Handling the "land for sale / land for auction" overlay image
	LLUUID mMapImageID;						// Image ID of the overlay image
	LLPointer<LLViewerFetchedTexture> mOverlayImage;	// Reference to the overlay image

	// Items for this region
	// Those are data received through item requests (as opposed to block requests for the rest of the data)
	item_info_list_t mTelehubs;			// List of tele hubs in the region
	item_info_list_t mInfohubs;			// List of info hubs in the region
	item_info_list_t mPGEvents;			// List of PG events in the region
	item_info_list_t mMatureEvents;		// List of Mature events in the region
	item_info_list_t mAdultEvents;		// List of Adult events in the region (AO)
	item_info_list_t mLandForSale;		// List of Land for sales in the region
	item_info_list_t mLandForSaleAdult;	// List of Adult Land for sales in the region (AO)
	item_info_list_t mAgentLocations;	// List of agents in the region
};

// We request region data on the world by "blocks" of (MAP_BLOCK_SIZE x MAP_BLOCK_SIZE) regions
// This is to reduce the number of requests to the asset DB and get things in big "blocks"
const S32 MAP_MAX_SIZE = 2048;
const S32 MAP_BLOCK_SIZE = 4;
const S32 MAP_BLOCK_RES = (MAP_MAX_SIZE / MAP_BLOCK_SIZE);

class LLWorldMap : public LLSingleton<LLWorldMap>
{
public:
	LLWorldMap();
	~LLWorldMap();

	// Clear all: list of region info, tiles, blocks and items
	void reset();

	void clearImageRefs();					// Clears the image references
	void dropImagePriorities();				// Drops the priority of the images being fetched
	void reloadItems(bool force = false);	// Reload the items (people, hub, etc...)

	// Region Map access
	typedef std::map<U64, LLSimInfo*> sim_info_map_t;
	const LLWorldMap::sim_info_map_t& getRegionMap() const { return mSimInfoMap; }
	void updateRegions(S32 x0, S32 y0, S32 x1, S32 y1);		// Requests region info for a rectangle of regions (in grid coordinates)

	// Insert a region and items in the map global instance
	// Note: x_world and y_world in world coordinates (meters)
	static bool insertRegion(U32 x_world, U32 y_world, std::string& name, LLUUID& uuid, U32 accesscode, U64 region_flags);
	static bool insertItem(U32 x_world, U32 y_world, std::string& name, LLUUID& uuid, U32 type, S32 extra, S32 extra2);

	// Get info on sims (region) : note that those methods only search the range of loaded sims (the one that are being browsed)
	// *not* the entire world. So a NULL return does not mean a down or unexisting region, just an out of range region.
	LLSimInfo* simInfoFromHandle(const U64 handle);
	LLSimInfo* simInfoFromPosGlobal(const LLVector3d& pos_global);
	LLSimInfo* simInfoFromName(const std::string& sim_name);

	// Gets simulator name from a global position, returns true if found
	bool simNameFromPosGlobal(const LLVector3d& pos_global, std::string& outSimName );

	// Debug only
	void dump();	// Print the world info to the standard output

	// Track handling
	void cancelTracking() { mIsTrackingLocation = false; mIsTrackingFound = false; mIsInvalidLocation = false; mIsTrackingDoubleClick = false; mIsTrackingCommit = false; }

	void setTracking(const LLVector3d& loc) { mIsTrackingLocation = true; mTrackingLocation = loc; mIsTrackingFound = false; mIsInvalidLocation = false; mIsTrackingDoubleClick = false; mIsTrackingCommit = false;}
	void setTrackingInvalid() { mIsTrackingFound = true; mIsInvalidLocation = true;  }
	void setTrackingValid()   { mIsTrackingFound = true; mIsInvalidLocation = false; }
	void setTrackingDoubleClick() { mIsTrackingDoubleClick = true; }
	void setTrackingCommit() { mIsTrackingCommit = true; }

	bool isTracking() { return mIsTrackingLocation; }
	bool isTrackingValidLocation()   { return mIsTrackingFound && !mIsInvalidLocation; }
	bool isTrackingInvalidLocation() { return mIsTrackingFound &&  mIsInvalidLocation; }
	bool isTrackingDoubleClick() { return mIsTrackingDoubleClick; }
	bool isTrackingCommit() { return mIsTrackingCommit; }
	bool isTrackingInRectangle(F64 x0, F64 y0, F64 x1, F64 y1);

	LLVector3d getTrackedPositionGlobal() const { return mTrackingLocation; }

	// World Mipmap delegation: currently used when drawing the mipmap
	void	equalizeBoostLevels();
	LLPointer<LLViewerFetchedTexture> getObjectsTile(U32 grid_x, U32 grid_y, S32 level, bool load = true) { return mWorldMipmap.getObjectsTile(grid_x, grid_y, level, load); }

private:
	bool clearItems(bool force = false);	// Clears the item lists
	void clearSimFlags();					// Clears the block flags indicating that we've already requested region infos

	// Create a region record corresponding to the handle, insert it in the region map and returns a pointer
	LLSimInfo* createSimInfoFromHandle(const U64 handle);

	// Map from region-handle to region info
	sim_info_map_t	mSimInfoMap;

	// Holds the tiled mipmap of the world. This is the structure that contains the images used for rendering.
	LLWorldMipmap	mWorldMipmap;

	// The World is divided in "blocks" of (MAP_BLOCK_SIZE x MAP_BLOCK_SIZE) regions that get requested at once.
	// This boolean table avoids "blocks" to be requested multiple times. 
	// Issue: Not sure this scheme is foolproof though as I've seen
	// cases where a block is never retrieved and, because of this boolean being set, never re-requested
	bool *			mMapBlockLoaded;		// Telling us if the block of regions has been requested or not

	// Track location data : used while there's nothing tracked yet by LLTracker
	bool			mIsTrackingLocation;	// True when we're tracking a point
	bool			mIsTrackingFound;		// True when the tracking position has been found, valid or not
	bool			mIsInvalidLocation;		// The region is down or the location does not correspond to an existing region
	bool			mIsTrackingDoubleClick;	// User double clicked to set the location (i.e. teleport when found please...)
	bool			mIsTrackingCommit;		// User used the search or landmark fields to set the location
	LLVector3d		mTrackingLocation;		// World global position being tracked

	// General grid items request timing flags (used for events,hubs and land for sale)
	LLTimer			mRequestTimer;
	bool			mFirstRequest;
};

#endif
