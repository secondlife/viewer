/** 
 * @file llsurface.h
 * @brief Description of LLSurface class
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLSURFACE_H
#define LL_LLSURFACE_H

//#include "vmath.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "llquaternion.h"

#include "v4coloru.h"
#include "v4color.h"

#include "llvowater.h"
#include "llpatchvertexarray.h"
#include "llviewertexture.h"

class LLTimer;
class LLUUID;
class LLAgent;
class LLStat;

static const U8 NO_EDGE    = 0x00;
static const U8 EAST_EDGE  = 0x01;
static const U8 NORTH_EDGE = 0x02;
static const U8 WEST_EDGE  = 0x04;
static const U8 SOUTH_EDGE = 0x08;

static const S32 ONE_MORE_THAN_NEIGHBOR	= 1;
static const S32 EQUAL_TO_NEIGHBOR 		= 0;
static const S32 ONE_LESS_THAN_NEIGHBOR	= -1;

const S32 ABOVE_WATERLINE_ALPHA = 32;  // The alpha of water when the land elevation is above the waterline.

class LLViewerRegion;
class LLSurfacePatch;
class LLBitPack;
class LLGroupHeader;

class LLSurface 
{
public:
	LLSurface(U32 type, LLViewerRegion *regionp = NULL);
	virtual ~LLSurface();

	static void initClasses(); // Do class initialization for LLSurface and its child classes.

	void create(const S32 surface_grid_width,
				const S32 surface_patch_width,
				const LLVector3d &origin_global,
				const F32 width);	// Allocates and initializes surface

	void setRegion(LLViewerRegion *regionp);

	void setOriginGlobal(const LLVector3d &origin_global);

	void connectNeighbor(LLSurface *neighborp, U32 direction);
	void disconnectNeighbor(LLSurface *neighborp);
	void disconnectAllNeighbors();

	virtual void decompressDCTPatch(LLBitPack &bitpack, LLGroupHeader *gopp, BOOL b_large_patch);
	virtual void updatePatchVisibilities(LLAgent &agent);

	inline F32 getZ(const U32 k) const				{ return mSurfaceZ[k]; }
	inline F32 getZ(const S32 i, const S32 j) const	{ return mSurfaceZ[i + j*mGridsPerEdge]; }

	LLVector3 getOriginAgent() const;
	const LLVector3d &getOriginGlobal() const;
	F32 getMetersPerGrid() const;
	S32 getGridsPerEdge() const; 
	S32 getPatchesPerEdge() const;
	S32 getGridsPerPatchEdge() const;
	U32 getRenderStride(const U32 render_level) const;
	U32 getRenderLevel(const U32 render_stride) const;

	// Returns the height of the surface immediately above (or below) location,
	// or if location is not above surface returns zero.
	F32 resolveHeightRegion(const F32 x, const F32 y) const;
	F32 resolveHeightRegion(const LLVector3 &location) const
			{ return resolveHeightRegion( location.mV[VX], location.mV[VY] ); }
	F32 resolveHeightGlobal(const LLVector3d &position_global) const;
	LLVector3 resolveNormalGlobal(const LLVector3d& v) const;				//  Returns normal to surface

	LLSurfacePatch *resolvePatchRegion(const F32 x, const F32 y) const;
	LLSurfacePatch *resolvePatchRegion(const LLVector3 &position_region) const;
	LLSurfacePatch *resolvePatchGlobal(const LLVector3d &position_global) const;

	// Update methods (called during idle, normally)
	BOOL idleUpdate(F32 max_update_time);

	BOOL containsPosition(const LLVector3 &position);

	void moveZ(const S32 x, const S32 y, const F32 delta);	

	LLViewerRegion *getRegion() const				{ return mRegionp; }

	F32 getMinZ() const								{ return mMinZ; }
	F32 getMaxZ() const								{ return mMaxZ; }

	void setWaterHeight(F32 height);
	F32 getWaterHeight() const;

	LLViewerTexture *getSTexture();
	LLViewerTexture *getWaterTexture();
	BOOL hasZData() const							{ return mHasZData; }

	void dirtyAllPatches();	// Use this to dirty all patches when changing terrain parameters

	void dirtySurfacePatch(LLSurfacePatch *patchp);
	LLVOWater *getWaterObj()						{ return mWaterObjp; }

	static void setTextureSize(const S32 texture_size);

	friend class LLSurfacePatch;
	friend std::ostream& operator<<(std::ostream &s, const LLSurface &S);
	
	void getNeighboringRegions( std::vector<LLViewerRegion*>& uniqueRegions );
	void getNeighboringRegionsStatus( std::vector<S32>& regions );
	
public:
	// Number of grid points on one side of a region, including +1 buffer for
	// north and east edge.
	S32 mGridsPerEdge;

	F32 mOOGridsPerEdge;			// Inverse of grids per edge

	S32 mPatchesPerEdge;			// Number of patches on one side of a region
	S32 mNumberOfPatches;			// Total number of patches


	// Each surface points at 8 neighbors (or NULL)
	// +---+---+---+
	// |NW | N | NE|
	// +---+---+---+
	// | W | 0 | E |
	// +---+---+---+
	// |SW | S | SE|
	// +---+---+---+
	LLSurface *mNeighbors[8]; // Adjacent patches

	U32 mType;				// Useful for identifying derived classes
	
	F32 mDetailTextureScale;	//  Number of times to repeat detail texture across this surface 

	static F32 sTextureUpdateTime;
	static S32 sTexelsUpdated;

protected:
	void createSTexture();
	void createWaterTexture();
	void initTextures();
	void initWater();


	void createPatchData();		// Allocates memory for patches.
	void destroyPatchData();    // Deallocates memory for patches.

	BOOL generateWaterTexture(const F32 x, const F32 y,
						const F32 width, const F32 height);		// Generate texture from composition values.

	//F32 updateTexture(LLSurfacePatch *ppatch);
	
	LLSurfacePatch *getPatch(const S32 x, const S32 y) const;

protected:
	LLVector3d	mOriginGlobal;		// In absolute frame
	LLSurfacePatch *mPatchList;		// Array of all patches

	// Array of grid data, mGridsPerEdge * mGridsPerEdge
	F32 *mSurfaceZ;

	// Array of grid normals, mGridsPerEdge * mGridsPerEdge
	LLVector3 *mNorm;

	std::set<LLSurfacePatch *> mDirtyPatchList;


	// The textures should never be directly initialized - use the setter methods!
	LLPointer<LLViewerTexture> mSTexturep;		// Texture for surface
	LLPointer<LLViewerTexture> mWaterTexturep;	// Water texture

	LLPointer<LLVOWater>	mWaterObjp;

	// When we want multiple cameras we'll need one of each these for each camera
	S32 mVisiblePatchCount;

	U32			mGridsPerPatchEdge;			// Number of grid points on a side of a patch
	F32			mMetersPerGrid;				// Converts (i,j) indecies to distance
	F32			mMetersPerEdge;				// = mMetersPerGrid * (mGridsPerEdge-1)

	LLPatchVertexArray mPVArray;

	BOOL		mHasZData;				// We've received any patch data for this surface.
	F32			mMinZ;					// min z for this region (during the session)
	F32			mMaxZ;					// max z for this region (during the session)

	S32			mSurfacePatchUpdateCount;					// Number of frames since last update.

private:
	LLViewerRegion *mRegionp; // Patch whose coordinate system this surface is using.
	static S32	sTextureSize;				// Size of the surface texture
};



//        .   __.
//     Z /|\   /| Y                                 North
//        |   / 
//        |  /             |<----------------- mGridsPerSurfaceEdge --------------->|
//        | /              __________________________________________________________
//        |/______\ X     /_______________________________________________________  /
//                /      /      /      /      /      /      /      /M*M-2 /M*M-1 / /  
//                      /______/______/______/______/______/______/______/______/ /  
//                     /      /      /      /      /      /      /      /      / /  
//                    /______/______/______/______/______/______/______/______/ /  
//                   /      /      /      /      /      /      /      /      / /  
//                  /______/______/______/______/______/______/______/______/ /  
//      West       /      /      /      /      /      /      /      /      / /  
//                /______/______/______/______/______/______/______/______/ /     East
//               /...   /      /      /      /      /      /      /      / /  
//              /______/______/______/______/______/______/______/______/ /  
//       _.    / 2M   /      /      /      /      /      /      /      / /  
//       /|   /______/______/______/______/______/______/______/______/ /  
//      /    / M    / M+1  / M+2  / ...  /      /      /      / 2M-1 / /   
//     j    /______/______/______/______/______/______/______/______/ /   
//         / 0    / 1    / 2    / ...  /      /      /      / M-1  / /   
//        /______/______/______/______/______/______/______/______/_/   
//                                South             |<-L->|
//             i -->
//
// where M = mSurfPatchWidth
// and L = mPatchGridWidth
// 
// Notice that mGridsPerSurfaceEdge = a power of two + 1
// This provides a buffer on the east and north edges that will allow us to 
// fill the cracks between adjacent surfaces when rendering.
#endif
