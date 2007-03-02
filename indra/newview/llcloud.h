/** 
 * @file llcloud.h
 * @brief Description of viewer LLCloudLayer class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCLOUD_H
#define LL_LLCLOUD_H

// Some ideas on how clouds should work
//
// Each region has a cloud layer
// Each cloud layer has pre-allocated space for N clouds
// The LLSky class knows the max number of clouds to render M.
// All clouds use the same texture, but the tex-coords can take on 8 configurations 
// (four rotations, front and back)
// 
// The sky's part
// --------------
// The sky knows that A clouds have been assigned to regions and there are B left over. 
// Divide B by number of active regions to get C.
// Ask each region to add C more clouds and return total number D.
// Add up all the D's to get a new A.
//
// The cloud layer's part
// ----------------------
// The cloud layer is a grid of possibility.  Each grid's value represents the probablility
// (0.0 to 1.0) that a cloud placement query will succeed.  
//
// The sky asks the region to add C more clouds.
// The cloud layer tries a total of E times to place clouds and returns total cloud count.
// 
// Clouds move according to local wind velocity.
// If a cloud moves out of region then it's location is sent to neighbor region
// or it is allowed to drift and decay.
//
// The clouds in non-visible regions do not propagate every frame.
// Each frame one non-visible region is allowed to propagate it's clouds 
// (might have to check to see if incoming cloud was already visible or not).
//
// 

#include "llmath.h"
//#include "vmath.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "v4color.h"
#include "llmemory.h"
#include "lldarray.h"

#include "llframetimer.h"

const U32 CLOUD_GRIDS_PER_EDGE 			= 16;

const F32 CLOUD_PUFF_WIDTH	= 64.f;
const F32 CLOUD_PUFF_HEIGHT	= 48.f;

class LLWind;
class LLVOClouds;
class LLViewerRegion;
class LLCloudLayer;
class LLBitPack;
class LLGroupHeader;

const S32 CLOUD_GROUPS_PER_EDGE = 4;

class LLCloudPuff
{
public:
	LLCloudPuff();

	const LLVector3d &getPositionGlobal() const		{ return mPositionGlobal; }
	friend class LLCloudGroup;

	void updatePuffs(const F32 dt);
	void updatePuffOwnership();

	F32 getAlpha() const							{ return mAlpha; }
	U32 getLifeState() const						{ return mLifeState; }
	void setLifeState(const U32 state)				{ mLifeState = state; }
	BOOL isDead() const								{ return mAlpha <= 0.f; }	


	static S32	sPuffCount;
protected:
	F32			mAlpha;
	F32			mRate;
	LLVector3d	mPositionGlobal;

	BOOL		mLifeState;
};

class LLCloudGroup
{
public:
	LLCloudGroup();

	void cleanup();

	void setCloudLayerp(LLCloudLayer *clp)			{ mCloudLayerp = clp; }
	void setCenterRegion(const LLVector3 &center);

	void updatePuffs(const F32 dt);
	void updatePuffOwnership();
	void updatePuffCount();

	BOOL inGroup(const LLCloudPuff &puff) const;

	F32 getDensity() const							{ return mDensity; }
	S32 getNumPuffs() const							{ return (S32) mCloudPuffs.size(); }
	const LLCloudPuff &getPuff(const S32 i)			{ return mCloudPuffs[i]; }
protected:
	LLCloudLayer *mCloudLayerp;
	LLVector3 mCenterRegion;
	F32 mDensity;
	S32 mTargetPuffCount;

	std::vector<LLCloudPuff> mCloudPuffs;
	LLPointer<LLVOClouds> mVOCloudsp;
};


class LLCloudLayer
{
public:
	LLCloudLayer();
	~LLCloudLayer();

	void create(LLViewerRegion *regionp);
	void destroy();

	void reset();						// Clears all active cloud puffs


	void updatePuffs(const F32 dt);
	void updatePuffOwnership();
	void updatePuffCount();

	LLCloudGroup *findCloudGroup(const LLCloudPuff &puff);

	void setRegion(LLViewerRegion *regionp);
	LLViewerRegion* getRegion() const						{ return mRegionp; }
	void setWindPointer(LLWind *windp);
	void setOriginGlobal(const LLVector3d &origin_global)	{ mOriginGlobal = origin_global; }
	void setWidth(F32 width);

	void setBrightness(F32 brightness);
	void setSunColor(const LLColor4 &color);

	F32 getDensityRegion(const LLVector3 &pos_region);		// "position" is in local coordinates

	void renderDensityField();
	void decompress(LLBitPack &bitpack, LLGroupHeader *group_header);

	LLCloudLayer* getNeighbor(const S32 n) const					{ return mNeighbors[n]; }

	void connectNeighbor(LLCloudLayer *cloudp, U32 direction);
	void disconnectNeighbor(U32 direction);
	void disconnectAllNeighbors();

public:
	LLVector3d 	mOriginGlobal;
	F32			mMetersPerEdge;
	F32			mMetersPerGrid;


	F32 mMaxAlpha;						// The max cloud puff _render_ alpha

protected:
	LLCloudLayer		*mNeighbors[4];
	LLWind				*mWindp;
	LLViewerRegion		*mRegionp;
	F32 				*mDensityp;			// the probability density grid
	
	LLCloudGroup		mCloudGroups[CLOUD_GROUPS_PER_EDGE][CLOUD_GROUPS_PER_EDGE];
};


#endif
