/** 
 * @file llviewerregion.h
 * @brief Description of the LLViewerRegion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLVIEWERREGION_H
#define LL_LLVIEWERREGION_H

// A ViewerRegion is a class that contains a bunch of objects and surfaces
// that are in to a particular region.
#include <string>
#include <boost/signals2.hpp>

#include "llwind.h"
#include "v3dmath.h"
#include "llstring.h"
#include "llregionflags.h"
#include "lluuid.h"
#include "llweb.h"
#include "llcapabilityprovider.h"
#include "m4math.h"					// LLMatrix4
#include "llframetimer.h"

// Surface id's
#define LAND  1
#define WATER 2
const U32	MAX_OBJECT_CACHE_ENTRIES = 50000;

// Region handshake flags
const U32 REGION_HANDSHAKE_SUPPORTS_SELF_APPEARANCE = 1U << 2;

class LLEventPoll;
class LLVLComposition;
class LLViewerObject;
class LLMessageSystem;
class LLNetMap;
class LLViewerParcelOverlay;
class LLSurface;
class LLVOCache;
class LLVOCacheEntry;
class LLSpatialPartition;
class LLEventPump;
class LLDataPacker;
class LLDataPackerBinaryBuffer;
class LLHost;
class LLBBox;
class LLSpatialGroup;
class LLDrawable;
class LLViewerRegionImpl;
class LLViewerOctreeGroup;
class LLVOCachePartition;

class LLViewerRegion: public LLCapabilityProvider // implements this interface
{
public:
	//MUST MATCH THE ORDER OF DECLARATION IN CONSTRUCTOR
	typedef enum 
	{
		PARTITION_HUD=0,
		PARTITION_TERRAIN,
		PARTITION_VOIDWATER,
		PARTITION_WATER,
		PARTITION_TREE,
		PARTITION_PARTICLE,
		PARTITION_GRASS,
		PARTITION_VOLUME,
		PARTITION_BRIDGE,
		PARTITION_HUD_PARTICLE,
		PARTITION_VO_CACHE,
		PARTITION_NONE,
		NUM_PARTITIONS
	} eObjectPartitions;

	typedef boost::signals2::signal<void(const LLUUID& region_id)> caps_received_signal_t;

	LLViewerRegion(const U64 &handle,
				   const LLHost &host,
				   const U32 surface_grid_width,
				   const U32 patch_grid_width,
				   const F32 region_width_meters);
	~LLViewerRegion();

	// Call this after you have the region name and handle.
	void loadObjectCache();
	void saveObjectCache();

	void sendMessage(); // Send the current message to this region's simulator
	void sendReliableMessage(); // Send the current message to this region's simulator

	void setOriginGlobal(const LLVector3d &origin);
	//void setAgentOffset(const LLVector3d &offset);
	void updateRenderMatrix();

	void setAllowDamage(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_DAMAGE, b); }
	void setAllowLandmark(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_LANDMARK, b); }
	void setAllowSetHome(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_SET_HOME, b); }
	void setResetHomeOnTeleport(BOOL b) { setRegionFlag(REGION_FLAGS_RESET_HOME_ON_TELEPORT, b); }
	void setSunFixed(BOOL b) { setRegionFlag(REGION_FLAGS_SUN_FIXED, b); }
	//void setBlockFly(BOOL b) { setRegionFlag(REGION_FLAGS_BLOCK_FLY, b); }		Never used
	void setAllowDirectTeleport(BOOL b) { setRegionFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT, b); }


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
	inline BOOL getReleaseNotesRequested()		const;

	bool isAlive(); // can become false if circuit disconnects

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

	inline void setRegionFlag(U64 flag, BOOL on);
	inline BOOL getRegionFlag(U64 flag) const;
	void setRegionFlags(U64 flags);
	U64 getRegionFlags() const					{ return mRegionFlags; }

	inline void setRegionProtocol(U64 protocol, BOOL on);
	BOOL getRegionProtocol(U64 protocol) const;
	void setRegionProtocols(U64 protocols)			{ mRegionProtocols = protocols; }
	U64 getRegionProtocols() const					{ return mRegionProtocols; }

	void setTimeDilation(F32 time_dilation);
	F32  getTimeDilation() const				{ return mTimeDilation; }

	// Origin height is at zero.
	const LLVector3d &getOriginGlobal() const;
	LLVector3 getOriginAgent() const;

	// Center is at the height of the water table.
	const LLVector3d &getCenterGlobal() const;
	LLVector3 getCenterAgent() const;

	void setRegionNameAndZone(const std::string& name_and_zone);
	const std::string& getName() const				{ return mName; }
	const std::string& getZoning() const			{ return mZoning; }

	void setOwner(const LLUUID& owner_id);
	const LLUUID& getOwner() const;

	// Is the current agent on the estate manager list for this region?
	void setIsEstateManager(BOOL b) { mIsEstateManager = b; }
	BOOL isEstateManager() const { return mIsEstateManager; }
	BOOL canManageEstate() const;

	void setSimAccess(U8 sim_access)			{ mSimAccess = sim_access; }
	U8 getSimAccess() const						{ return mSimAccess; }
	const std::string getSimAccessString() const;
	
	// Homestead-related getters; there are no setters as nobody should be
	// setting them other than the individual message handler which is a member
	S32 getSimClassID()                    const { return mClassID; }
	S32 getSimCPURatio()                   const { return mCPURatio; }
	const std::string& getSimColoName()    const { return mColoName; }
	const std::string& getSimProductSKU()  const { return mProductSKU; }
	std::string getLocalizedSimProductName() const;

	// Returns "Sandbox", "Expensive", etc.
	static std::string regionFlagsToString(U64 flags);

	// Returns translated version of "Mature", "PG", "Adult", etc.
	static std::string accessToString(U8 sim_access);

	// Returns "M", "PG", "A" etc.
	static std::string accessToShortString(U8 sim_access);
	static U8          shortStringToAccess(const std::string &sim_access);

	// Return access icon name
	static std::string getAccessIcon(U8 sim_access);
	
	// helper function which just makes sure all interested parties
	// can process the message.
	static void processRegionInfo(LLMessageSystem* msg, void**);

	//check if the viewer camera is static
	static BOOL isViewerCameraStatic();
	static void calcNewObjectCreationThrottle();

	void setCacheID(const LLUUID& id);

	F32	getWidth() const						{ return mWidth; }

	void idleUpdate(F32 max_update_time);
	void lightIdleUpdate();
	bool addVisibleGroup(LLViewerOctreeGroup* group);
	void addVisibleChildCacheEntry(LLVOCacheEntry* parent, LLVOCacheEntry* child);
	void addActiveCacheEntry(LLVOCacheEntry* entry);
	void removeActiveCacheEntry(LLVOCacheEntry* entry, LLDrawable* drawablep);	
	void killCacheEntry(U32 local_id); //physically delete the cache entry	

	// Like idleUpdate, but forces everything to complete regardless of
	// how long it takes.
	void forceUpdate();

	void connectNeighbor(LLViewerRegion *neighborp, U32 direction);

	void updateNetStats();

	U32	getPacketsLost() const;

	S32 getHttpResponderID() const;

	// Get/set named capability URLs for this region.
	void setSeedCapability(const std::string& url);
	S32 getNumSeedCapRetries();
	void setCapability(const std::string& name, const std::string& url);
	void setCapabilityDebug(const std::string& name, const std::string& url);
	bool isCapabilityAvailable(const std::string& name) const;
	// implements LLCapabilityProvider
    virtual std::string getCapability(const std::string& name) const;
    std::string getCapabilityDebug(const std::string& name) const;


	// has region received its final (not seed) capability list?
	bool capabilitiesReceived() const;
	void setCapabilitiesReceived(bool received);
	boost::signals2::connection setCapabilitiesReceivedCallback(const caps_received_signal_t::slot_type& cb);

	static bool isSpecialCapabilityName(const std::string &name);
	void logActiveCapabilities() const;

    /// implements LLCapabilityProvider
	/*virtual*/ const LLHost& getHost() const;
	const U64 		&getHandle() const 			{ return mHandle; }

	LLSurface		&getLand() const;

	// set and get the region id
	const LLUUID& getRegionID() const;
	void setRegionID(const LLUUID& region_id);

	BOOL pointInRegionGlobal(const LLVector3d &point_global) const;
	LLVector3	getPosRegionFromGlobal(const LLVector3d &point_global) const;
	LLVector3	getPosRegionFromAgent(const LLVector3 &agent_pos) const;
	LLVector3	getPosAgentFromRegion(const LLVector3 &region_pos) const;
	LLVector3d	getPosGlobalFromRegion(const LLVector3 &offset) const;

	LLVLComposition *getComposition() const;
	F32 getCompositionXY(const S32 x, const S32 y) const;

	BOOL isOwnedSelf(const LLVector3& pos);

	// Owned by a group you belong to?  (officer OR member)
	BOOL isOwnedGroup(const LLVector3& pos);

	// deal with map object updates in the world.
	void updateCoarseLocations(LLMessageSystem* msg);

	F32 getLandHeightRegion(const LLVector3& region_pos);

	U8 getCentralBakeVersion() { return mCentralBakeVersion; }

	void getInfo(LLSD& info);
	
	bool meshRezEnabled() const;
	bool meshUploadEnabled() const;

	// has region received its simulator features list? Requires an additional query after caps received.
	void setSimulatorFeaturesReceived(bool);
	bool simulatorFeaturesReceived() const;
	boost::signals2::connection setSimulatorFeaturesReceivedCallback(const caps_received_signal_t::slot_type& cb);
	
	void getSimulatorFeatures(LLSD& info) const;	
	void setSimulatorFeatures(const LLSD& info);

	
	bool dynamicPathfindingEnabled() const;

	bool avatarHoverHeightEnabled() const;

	typedef enum
	{
		CACHE_MISS_TYPE_FULL = 0,
		CACHE_MISS_TYPE_CRC,
		CACHE_MISS_TYPE_NONE
	} eCacheMissType;

	typedef enum
	{
		CACHE_UPDATE_DUPE = 0,
		CACHE_UPDATE_CHANGED,
		CACHE_UPDATE_ADDED,
		CACHE_UPDATE_REPLACED
	} eCacheUpdateResult;

	// handle a full update message
	eCacheUpdateResult cacheFullUpdate(LLDataPackerBinaryBuffer &dp, U32 flags);
	eCacheUpdateResult cacheFullUpdate(LLViewerObject* objectp, LLDataPackerBinaryBuffer &dp, U32 flags);	
	LLVOCacheEntry* getCacheEntryForOctree(U32 local_id);
	LLVOCacheEntry* getCacheEntry(U32 local_id, bool valid = true);
	bool probeCache(U32 local_id, U32 crc, U32 flags, U8 &cache_miss_type);
	U64 getRegionCacheHitCount() { return mRegionCacheHitCount; }
	U64 getRegionCacheMissCount() { return mRegionCacheMissCount; }
	void requestCacheMisses();
	void addCacheMissFull(const U32 local_id);
	//update object cache if the object receives a full-update or terse update
	LLViewerObject* updateCacheEntry(U32 local_id, LLViewerObject* objectp, U32 update_type);
	void findOrphans(U32 parent_id);
	void clearCachedVisibleObjects();
	void dumpCache();

	void unpackRegionHandshake();

	void calculateCenterGlobal();
	void calculateCameraDistance();

	friend std::ostream& operator<<(std::ostream &s, const LLViewerRegion &region);
    /// implements LLCapabilityProvider
    virtual std::string getDescription() const;
    std::string getViewerAssetUrl() const { return mViewerAssetUrl; }

	U32 getNumOfVisibleGroups() const;
	U32 getNumOfActiveCachedObjects() const;
	LLSpatialPartition* getSpatialPartition(U32 type);
	LLVOCachePartition* getVOCachePartition();

	bool objectIsReturnable(const LLVector3& pos, const std::vector<LLBBox>& boxes) const;
	bool childrenObjectReturnable( const std::vector<LLBBox>& boxes ) const;
	bool objectsCrossParcel(const std::vector<LLBBox>& boxes) const;

	void getNeighboringRegions( std::vector<LLViewerRegion*>& uniqueRegions );
	void getNeighboringRegionsStatus( std::vector<S32>& regions );
	const LLViewerRegionImpl * getRegionImpl() const { return mImpl; }
	LLViewerRegionImpl * getRegionImplNC() { return mImpl; }

	// implements the materials capability throttle
	bool materialsCapThrottled() const { return !mMaterialsCapThrottleTimer.hasExpired(); }
	void resetMaterialsCapThrottle();
	
	U32 getMaxMaterialsPerTransaction() const;

	void removeFromCreatedList(U32 local_id);
	void addToCreatedList(U32 local_id);	

	BOOL isPaused() const {return mPaused;}
	S32  getLastUpdate() const {return mLastUpdate;}

	static BOOL isNewObjectCreationThrottleDisabled() {return sNewObjectCreationThrottle < 0;}

private:
	void addToVOCacheTree(LLVOCacheEntry* entry);
	LLViewerObject* addNewObject(LLVOCacheEntry* entry);
	void killObject(LLVOCacheEntry* entry, std::vector<LLDrawable*>& delete_list); //adds entry into list if it is safe to move into cache
	void removeFromVOCacheTree(LLVOCacheEntry* entry);
	void killCacheEntry(LLVOCacheEntry* entry, bool for_rendering = false); //physically delete the cache entry	
	void killInvisibleObjects(F32 max_time);
	void createVisibleObjects(F32 max_time);
	void updateVisibleEntries(F32 max_time); //update visible entries

	void addCacheMiss(U32 id, LLViewerRegion::eCacheMissType miss_type);
	void decodeBoundingInfo(LLVOCacheEntry* entry);
	bool isNonCacheableObjectCreated(U32 local_id);	

public:
	struct CompareDistance
	{
		bool operator()(const LLViewerRegion* const& lhs, const LLViewerRegion* const& rhs)
		{
			return lhs->mCameraDistanceSquared < rhs->mCameraDistanceSquared; 
		}
	};

	void showReleaseNotes();

protected:
	void disconnectAllNeighbors();
	void initStats();

public:
	LLWind  mWind;
	LLViewerParcelOverlay	*mParcelOverlay;

	F32Bits	mBitsReceived;
	F32		mPacketsReceived;

	LLMatrix4 mRenderMatrix;

	// These arrays are maintained in parallel. Ideally they'd be combined into a
	// single array of an aggrigate data type but for compatibility with the old
	// messaging system in which the previous message only sends and parses the 
	// positions stored in the first array so they're maintained separately until 
	// we stop supporting the old CoarseLocationUpdate message.
	std::vector<U32> mMapAvatars;
	std::vector<LLUUID> mMapAvatarIDs;

	static BOOL sVOCacheCullingEnabled; //vo cache culling enabled or not.
	static S32  sLastCameraUpdated;

	LLFrameTimer &	getRenderInfoRequestTimer()	{ return mRenderInfoRequestTimer; };
	LLFrameTimer &	getRenderInfoReportTimer()	{ return mRenderInfoReportTimer; };

	struct CompareRegionByLastUpdate
	{
		bool operator()(const LLViewerRegion* const& lhs, const LLViewerRegion* const& rhs)
		{
			S32 lpa = lhs->getLastUpdate();
			S32 rpa = rhs->getLastUpdate();

			//small mLastUpdate first
			if(lpa < rpa)		
			{
				return true;
			}
			else if(lpa > rpa)
			{
				return false;
			}
			else
			{
				return lhs < rhs;
			}			
		}
	};
	typedef std::set<LLViewerRegion*, CompareRegionByLastUpdate> region_priority_list_t;

private:
	static S32  sNewObjectCreationThrottle;
	LLViewerRegionImpl * mImpl;
	LLFrameTimer         mRegionTimer;

	F32			mWidth;			// Width of region on a side (meters)
	U64			mHandle;
	F32			mTimeDilation;	// time dilation of physics simulation on simulator
	S32         mLastUpdate; //last time called idleUpdate()

	// simulator name
	std::string mName;
	std::string mZoning;

	// Is this agent on the estate managers list for this region?
	BOOL mIsEstateManager;

	U32		mPacketsIn;
	U32Bits	mBitsIn,
			mLastBitsIn;
	U32		mLastPacketsIn;
	U32		mPacketsOut;
	U32		mLastPacketsOut;
	S32		mPacketsLost;
	S32		mLastPacketsLost;
	U32Milliseconds		mPingDelay;
	F32		mDeltaTime;				// Time since last measurement of lastPackets, Bits, etc

	U64		mRegionFlags;			// includes damage flags
	U64		mRegionProtocols;		// protocols supported by this region
	U8		mSimAccess;
	F32 	mBillableFactor;
	U32		mMaxTasks;				// max prim count
	F32		mCameraDistanceSquared;	// updated once per frame
	U8		mCentralBakeVersion;
	
	LLVOCacheEntry* mLastVisitedEntry;
	U32				mInvisibilityCheckHistory;	

	// Information for Homestead / CR-53
	S32 mClassID;
	S32 mCPURatio;

	std::string mColoName;
	std::string mProductSKU;
	std::string mProductName;
	std::string mViewerAssetUrl ;
	
	// Maps local ids to cache entries.
	// Regions can have order 10,000 objects, so assume
	// a structure of size 2^14 = 16,000
	BOOL									mCacheLoaded;
	BOOL                                    mCacheDirty;
	BOOL	mAlive;					// can become false if circuit disconnects
	BOOL	mCapabilitiesReceived;
	BOOL	mSimulatorFeaturesReceived;
	BOOL    mReleaseNotesRequested;
	BOOL    mDead;  //if true, this region is in the process of deleting.
	BOOL    mPaused; //pause processing the objects in the region

	typedef std::map<U32, std::vector<U32> > orphan_list_t;
	orphan_list_t mOrphanMap;

	class CacheMissItem
	{
	public:
		CacheMissItem(U32 id, LLViewerRegion::eCacheMissType miss_type) : mID(id), mType(miss_type){}

		U32                            mID;     //local object id
		LLViewerRegion::eCacheMissType mType;   //cache miss type

		typedef std::list<CacheMissItem> cache_miss_list_t;
	};
	CacheMissItem::cache_miss_list_t   mCacheMissList;
	U64 mRegionCacheHitCount;
	U64 mRegionCacheMissCount;

	caps_received_signal_t mCapabilitiesReceivedSignal;		
	caps_received_signal_t mSimulatorFeaturesReceivedSignal;		

	LLSD mSimulatorFeatures;

	// the materials capability throttle
	LLFrameTimer mMaterialsCapThrottleTimer;
	LLFrameTimer mRenderInfoRequestTimer;
	LLFrameTimer mRenderInfoReportTimer;
};

inline BOOL LLViewerRegion::getRegionProtocol(U64 protocol) const
{
	return ((mRegionProtocols & protocol) != 0);
}

inline void LLViewerRegion::setRegionProtocol(U64 protocol, BOOL on)
{
	if (on)
	{
		mRegionProtocols |= protocol;
	}
	else
	{
		mRegionProtocols &= ~protocol;
	}
}

inline BOOL LLViewerRegion::getRegionFlag(U64 flag) const
{
	return ((mRegionFlags & flag) != 0);
}

inline void LLViewerRegion::setRegionFlag(U64 flag, BOOL on)
{
	if (on)
	{
		mRegionFlags |= flag;
	}
	else
	{
		mRegionFlags &= ~flag;
	}
}

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

inline BOOL LLViewerRegion::getReleaseNotesRequested() const
{
	return mReleaseNotesRequested;
}

#endif
