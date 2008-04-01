/** 
 * @file llworld.h
 * @brief Initial test structure to organize viewer regions
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

#ifndef LL_LLWORLD_H
#define LL_LLWORLD_H

#include "llpatchvertexarray.h"

#include "llmath.h"
#include "v3math.h"
#include "llmemory.h"
#include "llstring.h"
#include "llviewerpartsim.h"
#include "llviewerimage.h"
#include "llvowater.h"

class LLViewerRegion;
class LLVector3d;
class LLMessageSystem;
class LLNetMap;
class LLHost;

class LLViewerObject;
class LLSurfacePatch;

class LLCloudPuff;
class LLCloudGroup;
class LLVOAvatar;

// LLWorld maintains a stack of unused viewer_regions and an array of pointers to viewer regions
// as simulators are connected to, viewer_regions are popped off the stack and connected as required
// as simulators are removed, they are pushed back onto the stack

class LLWorld : public LLSingleton<LLWorld>
{
public:
	LLWorld();
	void destroyClass();

	LLViewerRegion*	addRegion(const U64 &region_handle, const LLHost &host);
		// safe to call if already present, does the "right thing" if
		// hosts are same, or if hosts are different, etc...
	void			removeRegion(const LLHost &host);

	void	disconnectRegions(); // Send quit messages to all child regions

	LLViewerRegion*			getRegion(const LLHost &host);
	LLViewerRegion*			getRegionFromPosGlobal(const LLVector3d &pos);
	LLViewerRegion*			getRegionFromPosAgent(const LLVector3 &pos);
	LLViewerRegion*			getRegionFromHandle(const U64 &handle);
	BOOL					positionRegionValidGlobal(const LLVector3d& pos);			// true if position is in valid region
	LLVector3d				clipToVisibleRegions(const LLVector3d &start_pos, const LLVector3d &end_pos);

	void					updateAgentOffset(const LLVector3d &offset);

	// All of these should be in the agent coordinate frame
	LLViewerRegion*			resolveRegionGlobal(LLVector3 &localpos, const LLVector3d &position);
	LLViewerRegion*			resolveRegionAgent(LLVector3 &localpos, const LLVector3 &position);
	F32						resolveLandHeightGlobal(const LLVector3d &position);
	F32						resolveLandHeightAgent(const LLVector3 &position);

	// Return the lowest allowed Z point to prevent objects from being moved
	// underground.
	F32 getMinAllowedZ(LLViewerObject* object);

	// takes a line segment defined by point_a and point_b, then
	// determines the closest (to point_a) point of intersection that is
	// on the land surface or on an object of the world.
	// Stores results in "intersection" and "intersection_normal" and
	// returns a scalar value that is the normalized (by length of line segment) 
	// distance along the line from "point_a" to "intersection".
	//
	// Currently assumes point_a and point_b only differ in z-direction,
	// but it may eventually become more general.
	F32 resolveStepHeightGlobal(const LLVOAvatar* avatarp, const LLVector3d &point_a, const LLVector3d &point_b,
							LLVector3d &intersection, LLVector3 &intersection_normal,
							LLViewerObject** viewerObjectPtr=NULL);

	LLSurfacePatch *		resolveLandPatchGlobal(const LLVector3d &position);
	LLVector3				resolveLandNormalGlobal(const LLVector3d &position);		// absolute frame

	U32						getRegionWidthInPoints() const	{ return mWidth; }
	F32						getRegionScale() const			{ return mScale; }

	// region X and Y size in meters
	F32						getRegionWidthInMeters() const	{ return mWidthInMeters; }
	F32						getRegionMinHeight() const		{ return -mWidthInMeters; }
	F32						getRegionMaxHeight() const		{ return 3.f*mWidthInMeters; }

	void					updateRegions(F32 max_update_time);
	void					updateVisibilities();
	void					updateParticles();
	void					updateClouds(const F32 dt);
	LLCloudGroup *			findCloudGroup(const LLCloudPuff &puff);

	void					renderPropertyLines();

	void resetStats();
	void updateNetStats(); // Update network statistics for all the regions...

	void printPacketsLost();
	void requestCacheMisses();

	// deal with map object updates in the world.
	static void processCoarseUpdate(LLMessageSystem* msg, void** user_data);

	F32 getLandFarClip() const;
	void setLandFarClip(const F32 far_clip);

	LLViewerImage *getDefaultWaterTexture();
	void updateWaterObjects();
	void shiftRegions(const LLVector3& offset);

	void setSpaceTimeUSec(const U64 space_time_usec);
	U64 getSpaceTimeUSec() const;

	void getInfo(LLSD& info);

public:
	typedef std::list<LLViewerRegion*> region_list_t;
	
	region_list_t	mActiveRegionList;

	region_list_t& getRegionList() { return mActiveRegionList; }

private:
	region_list_t	mRegionList;
	region_list_t	mVisibleRegionList;
	region_list_t	mCulledRegionList;

	// Number of points on edge
	static const U32 mWidth;

	// meters/point, therefore mWidth * mScale = meters per edge
	static const F32 mScale;

	static const F32 mWidthInMeters;

	F32 mLandFarClip;					// Far clip distance for land.
	LLPatchVertexArray		mLandPatch;
	S32 mLastPacketsIn;
	S32 mLastPacketsOut;
	S32 mLastPacketsLost;

	////////////////////////////
	//
	// Data for "Fake" objects
	//

	// Used to define the "Square" which we need to fill in
	U32 mMinRegionX;
	U32 mMaxRegionX;
	U32 mMinRegionY;
	U32 mMaxRegionY;

	std::list<LLVOWater*> mHoleWaterObjects;
	LLPointer<LLVOWater> mEdgeWaterObjects[8];

	LLPointer<LLViewerImage> mDefaultWaterTexturep;
	U64 mSpaceTimeUSec;
};


void process_enable_simulator(LLMessageSystem *mesgsys, void **user_data);
void process_disable_simulator(LLMessageSystem *mesgsys, void **user_data);

void process_region_handshake(LLMessageSystem* msg, void** user_data);

void send_agent_pause();
void send_agent_resume();

#endif
