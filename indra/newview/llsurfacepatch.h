/** 
 * @file llsurfacepatch.h
 * @brief LLSurfacePatch class definition
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

#ifndef LL_LLSURFACEPATCH_H
#define LL_LLSURFACEPATCH_H

#include "v3math.h"
#include "v3dmath.h"
#include "llpointer.h"

class LLSurface;
class LLVOSurfacePatch;
class LLVector2;
class LLColor4U;
class LLAgent;

// A patch shouldn't know about its visibility since that really depends on the 
// camera that is looking (or not looking) at it.  So, anything about a patch
// that is specific to a camera should be in the class below.
class LLPatchVisibilityInfo
{
public:
	LLPatchVisibilityInfo() :
		mbIsVisible(FALSE),
		mDistance(0.f),
		mRenderLevel(0),
		mRenderStride(0) { };
	~LLPatchVisibilityInfo() { };

	BOOL mbIsVisible;
	F32 mDistance;			// Distance from camera
	S32 mRenderLevel;
	U32 mRenderStride;
};



class LLSurfacePatch 
{
public:
	LLSurfacePatch();
	~LLSurfacePatch();

	void reset(const U32 id);
	void connectNeighbor(LLSurfacePatch *neighborp, const U32 direction);
	void disconnectNeighbor(LLSurface *surfacep);

	void setNeighborPatch(const U32 direction, LLSurfacePatch *neighborp);
	LLSurfacePatch *getNeighborPatch(const U32 direction) const;

	void colorPatch(const U8 r, const U8 g, const U8 b);

	BOOL updateTexture();

	void updateVerticalStats();
	void updateCompositionStats();
	void updateNormals();

	void updateEastEdge();
	void updateNorthEdge();

	void updateCameraDistanceRegion( const LLVector3 &pos_region);
	void updateVisibility();
	void updateGL();

	void dirtyZ(); // Dirty the z values of this patch
	void setHasReceivedData();
	BOOL getHasReceivedData() const;

	F32 getDistance() const;
	F32 getMaxZ() const;
	F32 getMinZ() const;
	F32 getMeanComposition() const;
	F32 getMinComposition() const;
	F32 getMaxComposition() const;
	const LLVector3 &getCenterRegion() const;
	const U64 &getLastUpdateTime() const;
	LLSurface *getSurface() const { return mSurfacep; }
	LLVector3 getPointAgent(const U32 x, const U32 y) const; // get the point at the offset.
	LLVector2 getTexCoords(const U32 x, const U32 y) const;

	void calcNormal(const U32 x, const U32 y, const U32 stride);
	const LLVector3 &getNormal(const U32 x, const U32 y) const;

	void eval(const U32 x, const U32 y, const U32 stride,
				LLVector3 *vertex, LLVector3 *normal, LLVector2 *tex0, LLVector2 *tex1);
	
	

	LLVector3 getOriginAgent() const;
	const LLVector3d &getOriginGlobal() const;
	void setOriginGlobal(const LLVector3d &origin_global);
	
	// connectivity -- each LLPatch points at 5 neighbors (or NULL)
	// +---+---+---+
	// |   | 2 | 5 |
	// +---+---+---+
	// | 3 | 0 | 1 |
	// +---+---+---+
	// | 6 | 4 |   |
	// +---+---+---+


	BOOL getVisible() const;
	U32 getRenderStride() const;
	S32 getRenderLevel() const;

	void setSurface(LLSurface *surfacep);
	void setDataZ(F32 *data_z)					{ mDataZ = data_z; }
	void setDataNorm(LLVector3 *data_norm)		{ mDataNorm = data_norm; }
	F32 *getDataZ() const						{ return mDataZ; }

	void dirty();			// Mark this surface patch as dirty...
	void clearDirty()							{ mDirty = FALSE; }

	void clearVObj();

public:
	BOOL mHasReceivedData;	// has the patch EVER received height data?
	BOOL mSTexUpdate;		// Does the surface texture need to be updated?

protected:
	LLSurfacePatch *mNeighborPatches[8]; // Adjacent patches
	BOOL mNormalsInvalid[9];  // Which normals are invalid

	BOOL mDirty;
	BOOL mDirtyZStats;
	BOOL mHeightsGenerated;

	U32 mDataOffset;
	F32 *mDataZ;
	LLVector3 *mDataNorm;

	// Pointer to the LLVOSurfacePatch object which is used in the new renderer.
	LLPointer<LLVOSurfacePatch> mVObjp;

	// All of the camera-dependent stuff should be in its own class...
	LLPatchVisibilityInfo mVisInfo;

	// pointers to beginnings of patch data fields
	LLVector3d mOriginGlobal;
	LLVector3 mOriginRegion;


	// height field stats
	LLVector3 mCenterRegion; // Center in region-local coords
	F32 mMinZ, mMaxZ, mMeanZ;
	F32 mRadius;

	F32 mMinComposition;
	F32 mMaxComposition;
	F32 mMeanComposition;

	U8 mConnectedEdge;		// This flag is non-zero iff patch is on at least one edge 
							// of LLSurface that is "connected" to another LLSurface
	U64 mLastUpdateTime;	// Time patch was last updated

	LLSurface *mSurfacep; // Pointer to "parent" surface
};


#endif // LL_LLSURFACEPATCH_H
