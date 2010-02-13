/** 
 * @file llviewerobjectlist.h
 * @brief Description of LLViewerObjectList class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLVIEWEROBJECTLIST_H
#define LL_LLVIEWEROBJECTLIST_H

#include <map>
#include <set>

// common includes
#include "llstat.h"
#include "lldarrayptr.h"
#include "llmap.h"			// *TODO: switch to std::map
#include "llstring.h"

// project includes
#include "llviewerobject.h"

class LLCamera;
class LLNetMap;
class LLDebugBeacon;

const U32 CLOSE_BIN_SIZE = 10;
const U32 NUM_BINS = 16;

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

	void shiftObjects(const LLVector3 &offset);

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
	void renderObjectsForSelect(LLCamera &camera, const LLRect& screen_rect, BOOL pick_parcel_wall = FALSE, BOOL render_transparent = TRUE);
	void generatePickList(LLCamera &camera);
	void renderPickList(const LLRect& screen_rect, BOOL pick_parcel_wall, BOOL render_transparent);

	LLViewerObject *getSelectedObject(const U32 object_id);

	inline S32 getNumObjects() { return mObjects.count(); }

	void addToMap(LLViewerObject *objectp);
	void removeFromMap(LLViewerObject *objectp);

	void clearDebugText();

	////////////////////////////////////////////
	//
	// Only accessed by markDead in LLViewerObject
	void cleanupReferences(LLViewerObject *objectp);

	S32 findReferences(LLDrawable *drawablep) const; // Find references to drawable in all objects, and return value.

	S32 getOrphanParentCount() const { return mOrphanParents.count(); }
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

	static BOOL removeFromLocalIDTable(const LLViewerObject &object);
	// Used ONLY by the orphaned object code.
	static U64 getIndex(const U32 local_id, const U32 ip, const U32 port);

	S32 mNumUnknownUpdates;
	S32 mNumDeadObjectUpdates;
	S32 mNumUnknownKills;
	S32 mNumDeadObjects;
protected:
	LLDynamicArray<U64>	mOrphanParents;	// LocalID/ip,port of orphaned objects
	LLDynamicArray<OrphanInfo> mOrphanChildren;	// UUID's of orphaned objects
	S32 mNumOrphans;

	LLDynamicArrayPtr<LLPointer<LLViewerObject>, 256> mObjects;
	std::set<LLPointer<LLViewerObject> > mActiveObjects;

	LLDynamicArrayPtr<LLPointer<LLViewerObject> > mMapObjects;

	typedef std::map<LLUUID, LLPointer<LLViewerObject> > vo_map;
	vo_map mDeadObjects;	// Need to keep multiple entries per UUID

	std::map<LLUUID, LLPointer<LLViewerObject> > mUUIDObjectMap;

	LLDynamicArray<LLDebugBeacon> mDebugBeacons;

	S32 mCurLazyUpdateIndex;

	static U32 sSimulatorMachineIndex;
	static LLMap<U64, U32> sIPAndPortToIndex;

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
	mMapObjects.put(objectp);
}

inline void LLViewerObjectList::removeFromMap(LLViewerObject *objectp)
{
	mMapObjects.removeObj(objectp);
}


#endif // LL_VIEWER_OBJECT_LIST_H
