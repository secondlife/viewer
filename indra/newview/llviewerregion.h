/** 
 * @file llviewerregion.h
 * @brief Description of the LLViewerRegion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLVIEWERREGION_H
#define LL_LLVIEWERREGION_H

// A ViewerRegion is a class that contains a bunch of objects and surfaces
// that are in to a particular region.
#include <string>

#include "lldarray.h"
#include "llwind.h"
#include "llcloud.h"
#include "llstat.h"
#include "v3dmath.h"
#include "llhost.h"
#include "llstring.h"
#include "llregionflags.h"
#include "llptrskipmap.h"
#include "lluuid.h"
#include "lldatapacker.h"
#include "llvocache.h"

// Surface id's
#define LAND  1
#define WATER 2
const U32	MAX_OBJECT_CACHE_ENTRIES = 10000;


class LLEventPoll;
class LLVLComposition;
class LLViewerObject;
class LLMessageSystem;
class LLNetMap;
class LLViewerParcelOverlay;
class LLSurface;
class LLVOCache;
class LLVOCacheEntry;

class LLViewerRegion 
{
public:
	LLViewerRegion(const U64 &handle,
				   const LLHost &host,
				   const U32 surface_grid_width,
				   const U32 patch_grid_width,
				   const F32 region_width_meters);
	~LLViewerRegion();

	// Call this after you have the region name and handle.
	void loadCache();

	void saveCache();

	void sendMessage(); // Send the current message to this region's simulator
	void sendReliableMessage(); // Send the current message to this region's simulator

	void setOriginGlobal(const LLVector3d &origin);
	void setAgentOffset(const LLVector3d &offset);

	void setAllowDamage(BOOL b) { setFlags(b, REGION_FLAGS_ALLOW_DAMAGE); }
	void setAllowLandmark(BOOL b) { setFlags(b, REGION_FLAGS_ALLOW_LANDMARK); }
	void setAllowSetHome(BOOL b) { setFlags(b, REGION_FLAGS_ALLOW_SET_HOME); }
	void setResetHomeOnTeleport(BOOL b) { setFlags(b, REGION_FLAGS_RESET_HOME_ON_TELEPORT); }
	void setSunFixed(BOOL b) { setFlags(b, REGION_FLAGS_SUN_FIXED); }
	void setBlockFly(BOOL b) { setFlags(b, REGION_FLAGS_BLOCK_FLY); }
	void setAllowDirectTeleport(BOOL b) { setFlags(b, REGION_FLAGS_ALLOW_DIRECT_TELEPORT); }


	inline BOOL getAllowDamage()			const;
	inline BOOL getAllowLandmark()			const;
	inline BOOL getAllowSetHome()			const;
	inline BOOL getResetHomeOnTeleport()	const;
	inline BOOL getSunFixed()				const;
	inline BOOL getBlockFly()				const;
	inline BOOL getAllowDirectTeleport()	const;
	inline BOOL isPrelude()					const;
	inline BOOL getAllowTerraform() 		const;
	inline BOOL getRestrictPushObject()		const;

	void setWaterHeight(F32 water_level);
	F32 getWaterHeight() const;

	BOOL isVoiceEnabled() const;

	void setBillableFactor(F32 billable_factor) { mBillableFactor = billable_factor; }
	F32 getBillableFactor() 		const 	{ return mBillableFactor; }

	// Maximum number of primitives allowed, regardless of object
	// bonus factor.
	U32 getMaxTasks() const { return mMaxTasks; }
	void setMaxTasks(U32 max_tasks) { mMaxTasks = max_tasks; }

	// Draw lines in the dirt showing ownership. Return number of 
	// vertices drawn.
	S32 renderPropertyLines();

	// Call this whenever you change the height data in the region.
	// (Automatically called by LLSurfacePatch's update routine)
	void dirtyHeights();

	LLViewerParcelOverlay *getParcelOverlay() const
			{ return mParcelOverlay; }

	void setRegionFlags(U32 flags);
	U32 getRegionFlags() const					{ return mRegionFlags; }

	void setTimeDilation(F32 time_dilation);
	F32  getTimeDilation() const				{ return mTimeDilation; }

	// Origin height is at zero.
	const LLVector3d &getOriginGlobal() const	{ return mOriginGlobal; }
	LLVector3 getOriginAgent() const;

	// Center is at the height of the water table.
	const LLVector3d &getCenterGlobal() const	{ return mCenterGlobal; }
	LLVector3 getCenterAgent() const;

	void setRegionNameAndZone(const char* name_and_zone);
	const LLString& getName() const				{ return mName; }
	const LLString& getZoning() const			{ return mZoning; }

	void setOwner(const LLUUID& owner_id) { mOwnerID = owner_id; }
	const LLUUID& getOwner() const { return mOwnerID; }

	// Is the current agent on the estate manager list for this region?
	void setIsEstateManager(BOOL b) { mIsEstateManager = b; }
	BOOL isEstateManager() const { return mIsEstateManager; }
	BOOL canManageEstate() const;

	void setSimAccess(U8 sim_access)			{ mSimAccess = sim_access; }
	U8 getSimAccess() const						{ return mSimAccess; }
	const char* getSimAccessString() const;

	// Returns "Sandbox", "Expensive", etc.
	static std::string regionFlagsToString(U32 flags);

	// Returns "Mature", "PG", etc.
	static const char* accessToString(U8 access);

	static U8 stringToAccess(const char* access_str);

	// Returns "M", "PG", etc.
	static const char* accessToShortString(U8 access);		/* Flawfinder: ignore */

	// helper function which just makes sure all interested parties
	// can process the message.
	static void processRegionInfo(LLMessageSystem* msg, void**);

	void setCacheID(const LLUUID& id)			{ mCacheID = id; }

	F32	getWidth() const						{ return mWidth; }

	BOOL idleUpdate(F32 max_update_time);

	// Like idleUpdate, but forces everything to complete regardless of
	// how long it takes.
	void forceUpdate();

	void connectNeighbor(LLViewerRegion *neighborp, U32 direction);

	void updateNetStats();

	U32	getPacketsLost() const;

	// Get/set named capability URLs for this region.
	void setSeedCapability(const std::string& url);
	void setCapability(const std::string& name, const std::string& url);
	std::string getCapability(const std::string& name) const;
	void logActiveCapabilities() const;

	const LLHost	&getHost() const			{ return mHost; }
	const U64 		&getHandle() const 			{ return mHandle; }

	LLSurface		&getLand() const			{ return *mLandp; }

	// set and get the region id
	const LLUUID& getRegionID() const { return mRegionID; }
	void setRegionID(const LLUUID& region_id) { mRegionID = region_id; }

	BOOL pointInRegionGlobal(const LLVector3d &point_global) const;
	LLVector3	getPosRegionFromGlobal(const LLVector3d &point_global) const;
	LLVector3	getPosRegionFromAgent(const LLVector3 &agent_pos) const;
	LLVector3	getPosAgentFromRegion(const LLVector3 &region_pos) const;
	LLVector3d	getPosGlobalFromRegion(const LLVector3 &offset) const;

	LLVLComposition *getComposition() const		{ return mCompositionp; }
	F32 getCompositionXY(const S32 x, const S32 y) const;

	BOOL isOwnedSelf(const LLVector3& pos);

	// Owned by a group you belong to?  (officer OR member)
	BOOL isOwnedGroup(const LLVector3& pos);

	// deal with map object updates in the world.
	void updateCoarseLocations(LLMessageSystem* msg);

	F32 getLandHeightRegion(const LLVector3& region_pos);

	void getInfo(LLSD& info);

	// handle a full update message
	void cacheFullUpdate(LLViewerObject* objectp, LLDataPackerBinaryBuffer &dp);
	LLDataPacker *getDP(U32 local_id, U32 crc);
	void requestCacheMisses();
	void addCacheMissFull(const U32 local_id);

	void dumpCache();

	void unpackRegionHandshake();

	void calculateCenterGlobal();
	void calculateCameraDistance();

	friend std::ostream& operator<<(std::ostream &s, const LLViewerRegion &region);

	// used by LCD to get details for debug screen
	U32 getNetDetailsForLCD();

public:
	struct CompareDistance
	{
		bool operator()(const LLViewerRegion* const& lhs, const LLViewerRegion* const& rhs)
		{
			return lhs->mCameraDistanceSquared < rhs->mCameraDistanceSquared; 
		}
	};
	
protected:
	void disconnectAllNeighbors();
	void initStats();
	void setFlags(BOOL b, U32 flags);

public:
	LLWind  mWind;
	LLCloudLayer mCloudLayer;
	LLViewerParcelOverlay	*mParcelOverlay;

	BOOL	mAlive;					// can become false if circuit disconnects

	LLStat	mBitStat;
	LLStat	mPacketsStat;
	LLStat	mPacketsLostStat;

	// These arrays are maintained in parallel. Ideally they'd be combined into a
	// single array of an aggrigate data type but for compatibility with the old
	// messaging system in which the previous message only sends and parses the 
	// positions stored in the first array so they're maintained separately until 
	// we stop supporting the old CoarseLocationUpdate message.
	LLDynamicArray<U32> mMapAvatars;
	LLDynamicArray<LLUUID> mMapAvatarIDs;

protected:
	// The surfaces and other layers
	LLSurface*	mLandp;

	// Region geometry data
	LLVector3d	mOriginGlobal;	// Location of southwest corner of region (meters)
	LLVector3d	mCenterGlobal;	// Location of center in world space (meters)
	F32			mWidth;			// Width of region on a side (meters)

	U64			mHandle;
	LLHost		mHost;

	// The unique ID for this region.
	LLUUID mRegionID;

	F32			mTimeDilation;	// time dilation of physics simulation on simulator

	// simulator name
	LLString mName;
	LLString mZoning;

	// region/estate owner - usually null.
	LLUUID mOwnerID;

	// Is this agent on the estate managers list for this region?
	BOOL mIsEstateManager;

	// Network statistics for the region's circuit...
	LLTimer mLastNetUpdate;
	U32		mPacketsIn;
	U32		mBitsIn;
	U32		mLastBitsIn;
	U32		mLastPacketsIn;
	U32		mPacketsOut;
	U32		mLastPacketsOut;
	S32		mPacketsLost;
	S32		mLastPacketsLost;
	U32		mPingDelay;
	F32		mDeltaTime;				// Time since last measurement of lastPackets, Bits, etc

	// Misc
	LLVLComposition *mCompositionp;		// Composition layer for the surface

	U32		mRegionFlags;			// includes damage flags
	U8		mSimAccess;
	F32 	mBillableFactor;
	U32		mMaxTasks;				// max prim count
	F32		mCameraDistanceSquared;	// updated once per frame
	
	// Maps local ids to cache entries.
	// Regions can have order 10,000 objects, so assume
	// a structure of size 2^14 = 16,000
	BOOL									mCacheLoaded;
	LLPtrSkipMap<U32, LLVOCacheEntry *, 14>	mCacheMap;
	LLVOCacheEntry							mCacheStart;
	LLVOCacheEntry							mCacheEnd;
	U32										mCacheEntriesCount;
	LLDynamicArray<U32>						mCacheMissFull;
	LLDynamicArray<U32>						mCacheMissCRC;
	// time?
	// LRU info?

	// Cache ID is unique per-region, across renames, moving locations,
	// etc.
	LLUUID mCacheID;
	
	typedef std::map<std::string, std::string> CapabilityMap;
	CapabilityMap mCapabilities;
	
	LLEventPoll* mEventPoll;
};

inline BOOL LLViewerRegion::getAllowDamage() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_DAMAGE) !=0);
}

inline BOOL LLViewerRegion::getAllowLandmark() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_LANDMARK) !=0);
}

inline BOOL LLViewerRegion::getAllowSetHome() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_SET_HOME) != 0);
}

inline BOOL LLViewerRegion::getResetHomeOnTeleport() const
{
	return ((mRegionFlags & REGION_FLAGS_RESET_HOME_ON_TELEPORT) !=0);
}

inline BOOL LLViewerRegion::getSunFixed() const
{
	return ((mRegionFlags & REGION_FLAGS_SUN_FIXED) !=0);
}

inline BOOL LLViewerRegion::getBlockFly() const
{
	return ((mRegionFlags & REGION_FLAGS_BLOCK_FLY) !=0);
}

inline BOOL LLViewerRegion::getAllowDirectTeleport() const
{
	return ((mRegionFlags & REGION_FLAGS_ALLOW_DIRECT_TELEPORT) !=0);
}

inline BOOL LLViewerRegion::isPrelude() const
{
	return is_prelude( mRegionFlags );
}

inline BOOL LLViewerRegion::getAllowTerraform() const
{
	return ((mRegionFlags & REGION_FLAGS_BLOCK_TERRAFORM) == 0);
}

inline BOOL LLViewerRegion::getRestrictPushObject() const
{
	return ((mRegionFlags & REGION_FLAGS_RESTRICT_PUSHOBJECT) != 0);
}

#endif


