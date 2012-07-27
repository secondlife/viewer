/** 
 * @file llviewerobjectlist.h
 * @brief Description of LLViewerObjectList class.
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

#ifndef LL_LLVIEWEROBJECTLIST_H
#define LL_LLVIEWEROBJECTLIST_H

#include <map>
#include <set>

// common includes
#include "llstat.h"
#include "llstring.h"

// project includes
#include "llviewerobject.h"

class LLCamera;
class LLNetMap;
class LLDebugBeacon;

const U32 CLOSE_BIN_SIZE = 10;
const U32 NUM_BINS = 128;

// GL name = position in object list + GL_NAME_INDEX_OFFSET so that
// we can have special numbers like zero.
const U32 GL_NAME_LAND = 0;
const U32 GL_NAME_PARCEL_WALL = 1;

const U32 GL_NAME_INDEX_OFFSET = 10;

class LLViewerObjectList
{
public:
	LLViewerObjectList();
	~LLViewerObjectList();

	void destroy();

	// For internal use only.  Does NOT take a local id, takes an index into
	// an internal dynamic array.
	inline LLViewerObject *getObject(const S32 index);
	
	inline LLViewerObject *findObject(const LLUUID &id);
	LLViewerObject *createObjectViewer(const LLPCode pcode, LLViewerRegion *regionp); // Create a viewer-side object
	LLViewerObject *createObject(const LLPCode pcode, LLViewerRegion *regionp,
								 const LLUUID &uuid, const U32 local_id, const LLHost &sender);

	LLViewerObject *replaceObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp); // TomY: hack to switch VO instances on the fly
	
	BOOL killObject(LLViewerObject *objectp);
	void killObjects(LLViewerRegion *regionp); // Kill all objects owned by a particular region.
	void killAllObjects();
	void removeDrawable(LLDrawable* drawablep);

	void cleanDeadObjects(const BOOL use_timer = TRUE);	// Clean up the dead object list.

	// Simulator and viewer side object updates...
	void processUpdateCore(LLViewerObject* objectp, void** data, U32 block, const EObjectUpdateType update_type, LLDataPacker* dpp, BOOL justCreated);
	void processObjectUpdate(LLMessageSystem *mesgsys, void **user_data, EObjectUpdateType update_type, bool cached=false, bool compressed=false);
	void processCompressedObjectUpdate(LLMessageSystem *mesgsys, void **user_data, EObjectUpdateType update_type);
	void processCachedObjectUpdate(LLMessageSystem *mesgsys, void **user_data, EObjectUpdateType update_type);
	void updateApparentAngles(LLAgent &agent);
	void update(LLAgent &agent, LLWorld &world);

	void fetchObjectCosts();
	void fetchPhysicsFlags();

	void updateObjectCost(LLViewerObject* object);
	void updateObjectCost(const LLUUID& object_id, F32 object_cost, F32 link_cost, F32 physics_cost, F32 link_physics_cost);
	void onObjectCostFetchFailure(const LLUUID& object_id);

	void updatePhysicsFlags(const LLViewerObject* object);
	void onPhysicsFlagsFetchFailure(const LLUUID& object_id);
	void updatePhysicsShapeType(const LLUUID& object_id, S32 type);
	void updatePhysicsProperties(const LLUUID& object_id,
									F32 density,
									F32 friction,
									F32 restitution,
									F32 gravity_multiplier);

	void shiftObjects(const LLVector3 &offset);
	void repartitionObjects();

	bool hasMapObjectInRegion(LLViewerRegion* regionp) ;
	void clearAllMapObjectsInRegion(LLViewerRegion* regionp) ;
	void renderObjectsForMap(LLNetMap &netmap);
	void renderObjectBounds(const LLVector3 &center);

	void addDebugBeacon(const LLVector3 &pos_agent, const std::string &string,
						const LLColor4 &color=LLColor4(1.f, 0.f, 0.f, 0.5f),
						const LLColor4 &text_color=LLColor4(1.f, 1.f, 1.f, 1.f),
						S32 line_width = 1);
	void renderObjectBeacons();
	void resetObjectBeacons();

	void dirtyAllObjectInventory();

	void updateActive(LLViewerObject *objectp);
	void updateAvatarVisibility();

	// Selection related stuff
	void generatePickList(LLCamera &camera);

	LLViewerObject *getSelectedObject(const U32 object_id);

	inline S32 getNumObjects() { return (S32) mObjects.size(); }

	void addToMap(LLViewerObject *objectp);
	void removeFromMap(LLViewerObject *objectp);

	void clearDebugText();

	////////////////////////////////////////////
	//
	// Only accessed by markDead in LLViewerObject
	void cleanupReferences(LLViewerObject *objectp);

	S32 findReferences(LLDrawable *drawablep) const; // Find references to drawable in all objects, and return value.

	S32 getOrphanParentCount() const { return (S32) mOrphanParents.size(); }
	S32 getOrphanCount() const { return mNumOrphans; }
	void orphanize(LLViewerObject *childp, U32 parent_id, U32 ip, U32 port);
	void findOrphans(LLViewerObject* objectp, U32 ip, U32 port);

public:
	// Class for keeping track of orphaned objects
	class OrphanInfo
	{
	public:
		OrphanInfo();
		OrphanInfo(const U64 parent_info, const LLUUID child_info);
		bool operator==(const OrphanInfo &rhs) const;
		bool operator!=(const OrphanInfo &rhs) const;
		U64 mParentInfo;
		LLUUID mChildInfo;
	};


	U32	mCurBin; // Current bin we're working on...

	// Statistics data (see also LLViewerStats)
	S32 mNumNewObjects;
	S32 mNumSizeCulled;
	S32 mNumVisCulled;

	// if we paused in the last frame
	// used to discount stats from this frame
	BOOL mWasPaused;

	static void getUUIDFromLocal(LLUUID &id,
								const U32 local_id,
								const U32 ip,
								const U32 port);
	static void setUUIDAndLocal(const LLUUID &id,
								const U32 local_id,
								const U32 ip,
								const U32 port); // Requires knowledge of message system info!

	static BOOL removeFromLocalIDTable(const LLViewerObject* objectp);
	// Used ONLY by the orphaned object code.
	static U64 getIndex(const U32 local_id, const U32 ip, const U32 port);

	S32 mNumUnknownUpdates;
	S32 mNumDeadObjectUpdates;
	S32 mNumUnknownKills;
	S32 mNumDeadObjects;
protected:
	std::vector<U64>	mOrphanParents;	// LocalID/ip,port of orphaned objects
	std::vector<OrphanInfo> mOrphanChildren;	// UUID's of orphaned objects
	S32 mNumOrphans;

	static LLStat sCacheHitRate;

	typedef std::vector<LLPointer<LLViewerObject> > vobj_list_t;

	vobj_list_t mObjects;
	std::set<LLPointer<LLViewerObject> > mActiveObjects;

	vobj_list_t mMapObjects;

	std::set<LLUUID> mDeadObjects;	

	std::map<LLUUID, LLPointer<LLViewerObject> > mUUIDObjectMap;

	//set of objects that need to update their cost
	std::set<LLUUID> mStaleObjectCost;
	std::set<LLUUID> mPendingObjectCost;

	//set of objects that need to update their physics flags
	std::set<LLUUID> mStalePhysicsFlags;
	std::set<LLUUID> mPendingPhysicsFlags;

	std::vector<LLDebugBeacon> mDebugBeacons;

	S32 mCurLazyUpdateIndex;

	static U32 sSimulatorMachineIndex;
	static std::map<U64, U32> sIPAndPortToIndex;

	static std::map<U64, LLUUID> sIndexAndLocalIDToUUID;

	std::set<LLViewerObject *> mSelectPickList;

	friend class LLViewerObject;
};


class LLDebugBeacon
{
public:
	~LLDebugBeacon()
	{
		if (mHUDObject.notNull())
		{
			mHUDObject->markDead();
		}
	}

	LLVector3 mPositionAgent;
	std::string mString;
	LLColor4 mColor;
	LLColor4 mTextColor;
	S32 mLineWidth;
	LLPointer<LLHUDObject> mHUDObject;
};



// Global object list
extern LLViewerObjectList gObjectList;

// Inlines
/**
 * Note:
 * it will return NULL for offline avatar_id 
 */
inline LLViewerObject *LLViewerObjectList::findObject(const LLUUID &id)
{
	std::map<LLUUID, LLPointer<LLViewerObject> >::iterator iter = mUUIDObjectMap.find(id);
	if(iter != mUUIDObjectMap.end())
	{
		return iter->second;
	}
	else
	{
		return NULL;
	}
}

inline LLViewerObject *LLViewerObjectList::getObject(const S32 index)
{
	LLViewerObject *objectp;
	objectp = mObjects[index];
	if (objectp->isDead())
	{
		//llwarns << "Dead object " << objectp->mID << " in getObject" << llendl;
		return NULL;
	}
	return objectp;
}

inline void LLViewerObjectList::addToMap(LLViewerObject *objectp)
{
	mMapObjects.push_back(objectp);
}

inline void LLViewerObjectList::removeFromMap(LLViewerObject *objectp)
{
	std::vector<LLPointer<LLViewerObject> >::iterator iter = std::find(mMapObjects.begin(), mMapObjects.end(), objectp);
	if (iter != mMapObjects.end())
	{
		mMapObjects.erase(iter);
	}
}


#endif // LL_VIEWER_OBJECT_LIST_H
