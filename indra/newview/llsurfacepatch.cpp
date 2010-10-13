/** 
 * @file llsurfacepatch.cpp
 * @brief LLSurfacePatch class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llsurfacepatch.h"
#include "llpatchvertexarray.h"
#include "llviewerobjectlist.h"
#include "llvosurfacepatch.h"
#include "llsurface.h"
#include "pipeline.h"
#include "llagent.h"
#include "timing.h"
#include "llsky.h"
#include "llviewercamera.h"

// For getting composition values
#include "llviewerregion.h"
#include "llvlcomposition.h"
#include "lldrawpool.h"
#include "noise.h"

extern U64 gFrameTime;
extern LLPipeline gPipeline;

LLSurfacePatch::LLSurfacePatch() :
	mHasReceivedData(FALSE),
	mSTexUpdate(FALSE),
	mDirty(FALSE),
	mDirtyZStats(TRUE),
	mHeightsGenerated(FALSE),
	mDataOffset(0),
	mDataZ(NULL),
	mDataNorm(NULL),
	mVObjp(NULL),
	mOriginRegion(0.f, 0.f, 0.f),
	mCenterRegion(0.f, 0.f, 0.f),
	mMinZ(0.f),
	mMaxZ(0.f),
	mMeanZ(0.f),
	mRadius(0.f),
	mMinComposition(0.f),
	mMaxComposition(0.f),
	mMeanComposition(0.f),
	// This flag is used to communicate between adjacent surfaces and is
	// set to non-zero values by higher classes.  
	mConnectedEdge(NO_EDGE),
	mLastUpdateTime(0),
	mSurfacep(NULL)
{	
	S32 i;
	for (i = 0; i < 8; i++)
	{
		setNeighborPatch(i, NULL);
	}
	for (i = 0; i < 9; i++)
	{
		mNormalsInvalid[i] = TRUE;
	}
}


LLSurfacePatch::~LLSurfacePatch()
{
	mVObjp = NULL;
}


void LLSurfacePatch::dirty()
{
	// These are outside of the loop in case we're still waiting for a dirty from the
	// texture being updated...
	if (mVObjp)
	{
		mVObjp->dirtyGeom();
	}
	else
	{
		llwarns << "No viewer object for this surface patch!" << llendl;
	}

	mDirtyZStats = TRUE;
	mHeightsGenerated = FALSE;
	
	if (!mDirty)
	{
		mDirty = TRUE;
		mSurfacep->dirtySurfacePatch(this);
	}
}


void LLSurfacePatch::setSurface(LLSurface *surfacep)
{
	mSurfacep = surfacep;
	if (mVObjp == (LLVOSurfacePatch *)NULL)
	{
		llassert(mSurfacep->mType == 'l');

		mVObjp = (LLVOSurfacePatch *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_SURFACE_PATCH, mSurfacep->getRegion());
		mVObjp->setPatch(this);
		mVObjp->setPositionRegion(mCenterRegion);
		gPipeline.createObject(mVObjp);
	}
} 

void LLSurfacePatch::disconnectNeighbor(LLSurface *surfacep)
{
	U32 i;
	for (i = 0; i < 8; i++)
	{
		if (getNeighborPatch(i))
		{
			if (getNeighborPatch(i)->mSurfacep == surfacep)
			{
				setNeighborPatch(i, NULL);
				mNormalsInvalid[i] = TRUE;
			}
		}
	}

	// Clean up connected edges
	if (getNeighborPatch(EAST))
	{
		if (getNeighborPatch(EAST)->mSurfacep == surfacep)
		{
			mConnectedEdge &= ~EAST_EDGE;
		}
	}
	if (getNeighborPatch(NORTH))
	{
		if (getNeighborPatch(NORTH)->mSurfacep == surfacep)
		{
			mConnectedEdge &= ~NORTH_EDGE;
		}
	}
	if (getNeighborPatch(WEST))
	{
		if (getNeighborPatch(WEST)->mSurfacep == surfacep)
		{
			mConnectedEdge &= ~WEST_EDGE;
		}
	}
	if (getNeighborPatch(SOUTH))
	{
		if (getNeighborPatch(SOUTH)->mSurfacep == surfacep)
		{
			mConnectedEdge &= ~SOUTH_EDGE;
		}
	}
}

LLVector3 LLSurfacePatch::getPointAgent(const U32 x, const U32 y) const
{
	U32 surface_stride = mSurfacep->getGridsPerEdge();
	U32 point_offset = x + y*surface_stride;
	LLVector3 pos;
	pos = getOriginAgent();
	pos.mV[VX] += x	* mSurfacep->getMetersPerGrid();
	pos.mV[VY] += y * mSurfacep->getMetersPerGrid();
	pos.mV[VZ] = *(mDataZ + point_offset);
	return pos;
}

LLVector2 LLSurfacePatch::getTexCoords(const U32 x, const U32 y) const
{
	U32 surface_stride = mSurfacep->getGridsPerEdge();
	U32 point_offset = x + y*surface_stride;
	LLVector3 pos, rel_pos;
	pos = getOriginAgent();
	pos.mV[VX] += x	* mSurfacep->getMetersPerGrid();
	pos.mV[VY] += y * mSurfacep->getMetersPerGrid();
	pos.mV[VZ] = *(mDataZ + point_offset);
	rel_pos = pos - mSurfacep->getOriginAgent();
	rel_pos *= 1.f/surface_stride;
	return LLVector2(rel_pos.mV[VX], rel_pos.mV[VY]);
}


void LLSurfacePatch::eval(const U32 x, const U32 y, const U32 stride, LLVector3 *vertex, LLVector3 *normal,
						  LLVector2 *tex0, LLVector2 *tex1)
{
	if (!mSurfacep || !mSurfacep->getRegion() || !mSurfacep->getGridsPerEdge())
	{
		return; // failsafe
	}
	llassert_always(vertex && normal && tex0 && tex1);
	
	U32 surface_stride = mSurfacep->getGridsPerEdge();
	U32 point_offset = x + y*surface_stride;

	*normal = getNormal(x, y);

	LLVector3 pos_agent = getOriginAgent();
	pos_agent.mV[VX] += x * mSurfacep->getMetersPerGrid();
	pos_agent.mV[VY] += y * mSurfacep->getMetersPerGrid();
	pos_agent.mV[VZ]  = *(mDataZ + point_offset);
	*vertex     = pos_agent;

	LLVector3 rel_pos = pos_agent - mSurfacep->getOriginAgent();
	LLVector3 tex_pos = rel_pos * (1.f/surface_stride);
	tex0->mV[0]  = tex_pos.mV[0];
	tex0->mV[1]  = tex_pos.mV[1];
	tex1->mV[0] = mSurfacep->getRegion()->getCompositionXY(llfloor(mOriginRegion.mV[0])+x, llfloor(mOriginRegion.mV[1])+y);

	const F32 xyScale = 4.9215f*7.f; //0.93284f;
	const F32 xyScaleInv = (1.f / xyScale)*(0.2222222222f);

	F32 vec[3] = {
					fmod((F32)(mOriginGlobal.mdV[0] + x)*xyScaleInv, 256.f),
					fmod((F32)(mOriginGlobal.mdV[1] + y)*xyScaleInv, 256.f),
					0.f
				};
	F32 rand_val = llclamp(noise2(vec)* 0.75f + 0.5f, 0.f, 1.f);
	tex1->mV[1] = rand_val;


}


void LLSurfacePatch::calcNormal(const U32 x, const U32 y, const U32 stride)
{
	U32 patch_width = mSurfacep->mPVArray.mPatchWidth;
	U32 surface_stride = mSurfacep->getGridsPerEdge();

	const F32 mpg = mSurfacep->getMetersPerGrid() * stride;

	S32 poffsets[2][2][2];
	poffsets[0][0][0] = x - stride;
	poffsets[0][0][1] = y - stride;

	poffsets[0][1][0] = x - stride;
	poffsets[0][1][1] = y + stride;

	poffsets[1][0][0] = x + stride;
	poffsets[1][0][1] = y - stride;

	poffsets[1][1][0] = x + stride;
	poffsets[1][1][1] = y + stride;

	const LLSurfacePatch *ppatches[2][2];

	// LLVector3 p1, p2, p3, p4;

	ppatches[0][0] = this;
	ppatches[0][1] = this;
	ppatches[1][0] = this;
	ppatches[1][1] = this;

	U32 i, j;
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			if (poffsets[i][j][0] < 0)
			{
				if (!ppatches[i][j]->getNeighborPatch(WEST))
				{
					poffsets[i][j][0] = 0;
				}
				else
				{
					poffsets[i][j][0] += patch_width;
					ppatches[i][j] = ppatches[i][j]->getNeighborPatch(WEST);
				}
			}
			if (poffsets[i][j][1] < 0)
			{
				if (!ppatches[i][j]->getNeighborPatch(SOUTH))
				{
					poffsets[i][j][1] = 0;
				}
				else
				{
					poffsets[i][j][1] += patch_width;
					ppatches[i][j] = ppatches[i][j]->getNeighborPatch(SOUTH);
				}
			}
			if (poffsets[i][j][0] >= (S32)patch_width)
			{
				if (!ppatches[i][j]->getNeighborPatch(EAST))
				{
					poffsets[i][j][0] = patch_width - 1;
				}
				else
				{
					poffsets[i][j][0] -= patch_width;
					ppatches[i][j] = ppatches[i][j]->getNeighborPatch(EAST);
				}
			}
			if (poffsets[i][j][1] >= (S32)patch_width)
			{
				if (!ppatches[i][j]->getNeighborPatch(NORTH))
				{
					poffsets[i][j][1] = patch_width - 1;
				}
				else
				{
					poffsets[i][j][1] -= patch_width;
					ppatches[i][j] = ppatches[i][j]->getNeighborPatch(NORTH);
				}
			}
		}
	}

	LLVector3 p00(-mpg,-mpg,
				  *(ppatches[0][0]->mDataZ
				  + poffsets[0][0][0]
				  + poffsets[0][0][1]*surface_stride));
	LLVector3 p01(-mpg,+mpg,
				  *(ppatches[0][1]->mDataZ
				  + poffsets[0][1][0]
				  + poffsets[0][1][1]*surface_stride));
	LLVector3 p10(+mpg,-mpg,
				  *(ppatches[1][0]->mDataZ
				  + poffsets[1][0][0]
				  + poffsets[1][0][1]*surface_stride));
	LLVector3 p11(+mpg,+mpg,
				  *(ppatches[1][1]->mDataZ
				  + poffsets[1][1][0]
				  + poffsets[1][1][1]*surface_stride));

	LLVector3 c1 = p11 - p00;
	LLVector3 c2 = p01 - p10;

	LLVector3 normal = c1;
	normal %= c2;
	normal.normVec();

	llassert(mDataNorm);
	*(mDataNorm + surface_stride * y + x) = normal;
}

const LLVector3 &LLSurfacePatch::getNormal(const U32 x, const U32 y) const
{
	U32 surface_stride = mSurfacep->getGridsPerEdge();
	llassert(mDataNorm);
	return *(mDataNorm + surface_stride * y + x);
}


void LLSurfacePatch::updateCameraDistanceRegion(const LLVector3 &pos_region)
{
	if (LLPipeline::sDynamicLOD)
	{
		LLVector3 dv = pos_region;
		dv -= mCenterRegion;
		mVisInfo.mDistance = llmax(0.f, (F32)(dv.magVec() - mRadius))/
			llmax(LLVOSurfacePatch::sLODFactor, 0.1f);
	}
	else
	{
		mVisInfo.mDistance = 0.f;
	}
}

F32 LLSurfacePatch::getDistance() const
{
	return mVisInfo.mDistance;
}


// Called when a patch has changed its height field
// data.
void LLSurfacePatch::updateVerticalStats() 
{
	if (!mDirtyZStats)
	{
		return;
	}

	U32 grids_per_patch_edge = mSurfacep->getGridsPerPatchEdge();
	U32 grids_per_edge = mSurfacep->getGridsPerEdge();
	F32 meters_per_grid = mSurfacep->getMetersPerGrid();

	U32 i, j, k;
	F32 z, total;

	llassert(mDataZ);
	z = *(mDataZ);

	mMinZ = z;
	mMaxZ = z;

	k = 0;
	total = 0.0f;

	// Iterate to +1 because we need to do the edges correctly.
	for (j=0; j<(grids_per_patch_edge+1); j++) 
	{
		for (i=0; i<(grids_per_patch_edge+1); i++) 
		{
			z = *(mDataZ + i + j*grids_per_edge);

			if (z < mMinZ)
			{
				mMinZ = z;
			}
			if (z > mMaxZ)
			{
				mMaxZ = z;
			}
			total += z;
			k++;
		}
	}
	mMeanZ = total / (F32) k;
	mCenterRegion.mV[VZ] = 0.5f * (mMinZ + mMaxZ);

	LLVector3 diam_vec(meters_per_grid*grids_per_patch_edge,
						meters_per_grid*grids_per_patch_edge,
						mMaxZ - mMinZ);
	mRadius = diam_vec.magVec() * 0.5f;

	mSurfacep->mMaxZ = llmax(mMaxZ, mSurfacep->mMaxZ);
	mSurfacep->mMinZ = llmin(mMinZ, mSurfacep->mMinZ);
	mSurfacep->mHasZData = TRUE;
	mSurfacep->getRegion()->calculateCenterGlobal();

	if (mVObjp)
	{
		mVObjp->dirtyPatch();
	}
	mDirtyZStats = FALSE;
}


void LLSurfacePatch::updateNormals() 
{
	if (mSurfacep->mType == 'w')
	{
		return;
	}
	U32 grids_per_patch_edge = mSurfacep->getGridsPerPatchEdge();
	U32 grids_per_edge = mSurfacep->getGridsPerEdge();

	BOOL dirty_patch = FALSE;

	U32 i, j;
	// update the east edge
	if (mNormalsInvalid[EAST] || mNormalsInvalid[NORTHEAST] || mNormalsInvalid[SOUTHEAST])
	{
		for (j = 0; j <= grids_per_patch_edge; j++)
		{
			calcNormal(grids_per_patch_edge, j, 2);
			calcNormal(grids_per_patch_edge - 1, j, 2);
			calcNormal(grids_per_patch_edge - 2, j, 2);
		}

		dirty_patch = TRUE;
	}

	// update the north edge
	if (mNormalsInvalid[NORTHEAST] || mNormalsInvalid[NORTH] || mNormalsInvalid[NORTHWEST])
	{
		for (i = 0; i <= grids_per_patch_edge; i++)
		{
			calcNormal(i, grids_per_patch_edge, 2);
			calcNormal(i, grids_per_patch_edge - 1, 2);
			calcNormal(i, grids_per_patch_edge - 2, 2);
		}

		dirty_patch = TRUE;
	}

	// update the west edge
	if (mNormalsInvalid[NORTHWEST] || mNormalsInvalid[WEST] || mNormalsInvalid[SOUTHWEST])
	{
		for (j = 0; j < grids_per_patch_edge; j++)
		{
			calcNormal(0, j, 2);
			calcNormal(1, j, 2);
		}
		dirty_patch = TRUE;
	}

	// update the south edge
	if (mNormalsInvalid[SOUTHWEST] || mNormalsInvalid[SOUTH] || mNormalsInvalid[SOUTHEAST])
	{
		for (i = 0; i < grids_per_patch_edge; i++)
		{
			calcNormal(i, 0, 2);
			calcNormal(i, 1, 2);
		}
		dirty_patch = TRUE;
	}

	// Invalidating the northeast corner is different, because depending on what the adjacent neighbors are,
	// we'll want to do different things.
	if (mNormalsInvalid[NORTHEAST])
	{
		if (!getNeighborPatch(NORTHEAST))
		{
			if (!getNeighborPatch(NORTH))
			{
				if (!getNeighborPatch(EAST))
				{
					// No north or east neighbors.  Pull from the diagonal in your own patch.
					*(mDataZ + grids_per_patch_edge + grids_per_patch_edge*grids_per_edge) =
						*(mDataZ + grids_per_patch_edge - 1 + (grids_per_patch_edge - 1)*grids_per_edge);
				}
				else
				{
					if (getNeighborPatch(EAST)->getHasReceivedData())
					{
						// East, but not north.  Pull from your east neighbor's northwest point.
						*(mDataZ + grids_per_patch_edge + grids_per_patch_edge*grids_per_edge) =
							*(getNeighborPatch(EAST)->mDataZ + (grids_per_patch_edge - 1)*grids_per_edge);
					}
					else
					{
						*(mDataZ + grids_per_patch_edge + grids_per_patch_edge*grids_per_edge) =
							*(mDataZ + grids_per_patch_edge - 1 + (grids_per_patch_edge - 1)*grids_per_edge);
					}
				}
			}
			else
			{
				// We have a north.
				if (getNeighborPatch(EAST))
				{
					// North and east neighbors, but not northeast.
					// Pull from diagonal in your own patch.
					*(mDataZ + grids_per_patch_edge + grids_per_patch_edge*grids_per_edge) =
						*(mDataZ + grids_per_patch_edge - 1 + (grids_per_patch_edge - 1)*grids_per_edge);
				}
				else
				{
					if (getNeighborPatch(NORTH)->getHasReceivedData())
					{
						// North, but not east.  Pull from your north neighbor's southeast corner.
						*(mDataZ + grids_per_patch_edge + grids_per_patch_edge*grids_per_edge) =
							*(getNeighborPatch(NORTH)->mDataZ + (grids_per_patch_edge - 1));
					}
					else
					{
						*(mDataZ + grids_per_patch_edge + grids_per_patch_edge*grids_per_edge) =
							*(mDataZ + grids_per_patch_edge - 1 + (grids_per_patch_edge - 1)*grids_per_edge);
					}
				}
			}
		}
		else if (getNeighborPatch(NORTHEAST)->mSurfacep != mSurfacep)
		{
			if (
				(!getNeighborPatch(NORTH) || (getNeighborPatch(NORTH)->mSurfacep != mSurfacep))
				&&
				(!getNeighborPatch(EAST) || (getNeighborPatch(EAST)->mSurfacep != mSurfacep)))
			{
				*(mDataZ + grids_per_patch_edge + grids_per_patch_edge*grids_per_edge) =
										*(getNeighborPatch(NORTHEAST)->mDataZ);
			}
		}
		else
		{
			// We've got a northeast patch in the same surface.
			// The z and normals will be handled by that patch.
		}
		calcNormal(grids_per_patch_edge, grids_per_patch_edge, 2);
		calcNormal(grids_per_patch_edge, grids_per_patch_edge - 1, 2);
		calcNormal(grids_per_patch_edge - 1, grids_per_patch_edge, 2);
		calcNormal(grids_per_patch_edge - 1, grids_per_patch_edge - 1, 2);
		dirty_patch = TRUE;
	}

	// update the middle normals
	if (mNormalsInvalid[MIDDLE])
	{
		for (j=2; j < grids_per_patch_edge - 2; j++)
		{
			for (i=2; i < grids_per_patch_edge - 2; i++)
			{
				calcNormal(i, j, 2);
			}
		}
		dirty_patch = TRUE;
	}

	if (dirty_patch)
	{
		mSurfacep->dirtySurfacePatch(this);
	}

	for (i = 0; i < 9; i++)
	{
		mNormalsInvalid[i] = FALSE;
	}
}

void LLSurfacePatch::updateEastEdge()
{
	U32 grids_per_patch_edge = mSurfacep->getGridsPerPatchEdge();
	U32 grids_per_edge = mSurfacep->getGridsPerEdge();

	U32 j, k;
	F32 *west_surface, *east_surface;

	if (!getNeighborPatch(EAST))
	{
		west_surface = mDataZ + grids_per_patch_edge;
		east_surface = mDataZ + grids_per_patch_edge - 1;
	}
	else if (mConnectedEdge & EAST_EDGE)
	{
		west_surface = mDataZ + grids_per_patch_edge;
		east_surface = getNeighborPatch(EAST)->mDataZ;
	}
	else
	{
		return;
	}

	// If patchp is on the east edge of its surface, then we update the east
	// side buffer
	for (j=0; j < grids_per_patch_edge; j++)
	{
		k = j * grids_per_edge;
		*(west_surface + k) = *(east_surface + k);	// update buffer Z
	}
}


void LLSurfacePatch::updateNorthEdge()
{
	U32 grids_per_patch_edge = mSurfacep->getGridsPerPatchEdge();
	U32 grids_per_edge = mSurfacep->getGridsPerEdge();

	U32 i;
	F32 *south_surface, *north_surface;

	if (!getNeighborPatch(NORTH))
	{
		south_surface = mDataZ + grids_per_patch_edge*grids_per_edge;
		north_surface = mDataZ + (grids_per_patch_edge - 1) * grids_per_edge;
	}
	else if (mConnectedEdge & NORTH_EDGE)
	{
		south_surface = mDataZ + grids_per_patch_edge*grids_per_edge;
		north_surface = getNeighborPatch(NORTH)->mDataZ;
	}
	else
	{
		return;
	}

	// Update patchp's north edge ...
	for (i=0; i<grids_per_patch_edge; i++)
	{
		*(south_surface + i) = *(north_surface + i);	// update buffer Z
	}
}


BOOL LLSurfacePatch::updateTexture()
{
	if (mSTexUpdate)		//  Update texture as needed
	{
		F32 meters_per_grid = getSurface()->getMetersPerGrid();
		F32 grids_per_patch_edge = (F32)getSurface()->getGridsPerPatchEdge();

		if ((!getNeighborPatch(EAST) || getNeighborPatch(EAST)->getHasReceivedData())
			&& (!getNeighborPatch(WEST) || getNeighborPatch(WEST)->getHasReceivedData())
			&& (!getNeighborPatch(SOUTH) || getNeighborPatch(SOUTH)->getHasReceivedData())
			&& (!getNeighborPatch(NORTH) || getNeighborPatch(NORTH)->getHasReceivedData()))
		{
			LLViewerRegion *regionp = getSurface()->getRegion();
			LLVector3d origin_region = getOriginGlobal() - getSurface()->getOriginGlobal();

			// Have to figure out a better way to deal with these edge conditions...
			LLVLComposition* comp = regionp->getComposition();
			if (!mHeightsGenerated)
			{
				F32 patch_size = meters_per_grid*(grids_per_patch_edge+1);
				if (comp->generateHeights((F32)origin_region[VX], (F32)origin_region[VY],
										  patch_size, patch_size))
				{
					mHeightsGenerated = TRUE;
				}
				else
				{
					return FALSE;
				}
			}
			
			if (comp->generateComposition())
			{
				if (mVObjp)
				{
					mVObjp->dirtyGeom();
					gPipeline.markGLRebuild(mVObjp);
					return TRUE;
				}
			}
		}
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void LLSurfacePatch::updateGL()
{
	F32 meters_per_grid = getSurface()->getMetersPerGrid();
	F32 grids_per_patch_edge = (F32)getSurface()->getGridsPerPatchEdge();

	LLViewerRegion *regionp = getSurface()->getRegion();
	LLVector3d origin_region = getOriginGlobal() - getSurface()->getOriginGlobal();

	LLVLComposition* comp = regionp->getComposition();
	
	updateCompositionStats();
	F32 tex_patch_size = meters_per_grid*grids_per_patch_edge;
	if (comp->generateTexture((F32)origin_region[VX], (F32)origin_region[VY],
							  tex_patch_size, tex_patch_size))
	{
		mSTexUpdate = FALSE;

		// Also generate the water texture
		mSurfacep->generateWaterTexture((F32)origin_region.mdV[VX], (F32)origin_region.mdV[VY],
										tex_patch_size, tex_patch_size);
	}
}

void LLSurfacePatch::dirtyZ()
{
	mSTexUpdate = TRUE;

	// Invalidate all normals in this patch
	U32 i;
	for (i = 0; i < 9; i++)
	{
		mNormalsInvalid[i] = TRUE;
	}

	// Invalidate normals in this and neighboring patches
	for (i = 0; i < 8; i++)
	{
		if (getNeighborPatch(i))
		{
			getNeighborPatch(i)->mNormalsInvalid[gDirOpposite[i]] = TRUE;
			getNeighborPatch(i)->dirty();
			if (i < 4)
			{
				getNeighborPatch(i)->mNormalsInvalid[gDirAdjacent[gDirOpposite[i]][0]] = TRUE;
				getNeighborPatch(i)->mNormalsInvalid[gDirAdjacent[gDirOpposite[i]][1]] = TRUE;
			}
		}
	}

	dirty();
	mLastUpdateTime = gFrameTime;
}


const U64 &LLSurfacePatch::getLastUpdateTime() const
{
	return mLastUpdateTime;
}

F32 LLSurfacePatch::getMaxZ() const
{
	return mMaxZ;
}

F32 LLSurfacePatch::getMinZ() const
{
	return mMinZ;
}

void LLSurfacePatch::setOriginGlobal(const LLVector3d &origin_global)
{
	mOriginGlobal = origin_global;

	LLVector3 origin_region;
	origin_region.setVec(mOriginGlobal - mSurfacep->getOriginGlobal());

	mOriginRegion = origin_region;
	mCenterRegion.mV[VX] = origin_region.mV[VX] + 0.5f*mSurfacep->getGridsPerPatchEdge()*mSurfacep->getMetersPerGrid();
	mCenterRegion.mV[VY] = origin_region.mV[VY] + 0.5f*mSurfacep->getGridsPerPatchEdge()*mSurfacep->getMetersPerGrid();

	mVisInfo.mbIsVisible = FALSE;
	mVisInfo.mDistance = 512.0f;
	mVisInfo.mRenderLevel = 0;
	mVisInfo.mRenderStride = mSurfacep->getGridsPerPatchEdge();
	
}

void LLSurfacePatch::connectNeighbor(LLSurfacePatch *neighbor_patchp, const U32 direction)
{
	llassert(neighbor_patchp);
	mNormalsInvalid[direction] = TRUE;
	neighbor_patchp->mNormalsInvalid[gDirOpposite[direction]] = TRUE;

	setNeighborPatch(direction, neighbor_patchp);
	neighbor_patchp->setNeighborPatch(gDirOpposite[direction], this);

	if (EAST == direction)
	{
		mConnectedEdge |= EAST_EDGE;
		neighbor_patchp->mConnectedEdge |= WEST_EDGE;
	}
	else if (NORTH == direction)
	{
		mConnectedEdge |= NORTH_EDGE;
		neighbor_patchp->mConnectedEdge |= SOUTH_EDGE;
	}
	else if (WEST == direction)
	{
		mConnectedEdge |= WEST_EDGE;
		neighbor_patchp->mConnectedEdge |= EAST_EDGE;
	}
	else if (SOUTH == direction)
	{
		mConnectedEdge |= SOUTH_EDGE;
		neighbor_patchp->mConnectedEdge |= NORTH_EDGE;
	}
}

void LLSurfacePatch::updateVisibility()
{
	if (mVObjp.isNull())
	{
		return;
	}

	const F32 DEFAULT_DELTA_ANGLE 	= (0.15f);
	U32 old_render_stride, max_render_stride;
	U32 new_render_level;
	F32 stride_per_distance = DEFAULT_DELTA_ANGLE / mSurfacep->getMetersPerGrid();
	U32 grids_per_patch_edge = mSurfacep->getGridsPerPatchEdge();

	LLVector4a center;
	center.load3( (mCenterRegion + mSurfacep->getOriginAgent()).mV);
	LLVector4a radius;
	radius.splat(mRadius);

	// sphere in frustum on global coordinates
	if (LLViewerCamera::getInstance()->AABBInFrustumNoFarClip(center, radius))
	{
		// We now need to calculate the render stride based on patchp's distance 
		// from LLCamera render_stride is governed by a relation something like this...
		//
		//                       delta_angle * patch.distance
		// render_stride <=  ----------------------------------------
		//                           mMetersPerGrid
		//
		// where 'delta_angle' is the desired solid angle of the average polgon on a patch.
		//
		// Any render_stride smaller than the RHS would be 'satisfactory'.  Smaller 
		// strides give more resolution, but efficiency suggests that we use the largest 
		// of the render_strides that obey the relation.  Flexibility is achieved by 
		// modulating 'delta_angle' until we have an acceptable number of triangles.
	
		old_render_stride = mVisInfo.mRenderStride;

		// Calculate the render_stride using information in agent
		max_render_stride = lltrunc(mVisInfo.mDistance * stride_per_distance);
		max_render_stride = llmin(max_render_stride , 2*grids_per_patch_edge);

		// We only use render_strides that are powers of two, so we use look-up tables to figure out
		// the render_level and corresponding render_stride
		new_render_level = mVisInfo.mRenderLevel = mSurfacep->getRenderLevel(max_render_stride);
		mVisInfo.mRenderStride = mSurfacep->getRenderStride(new_render_level);

		if ((mVisInfo.mRenderStride != old_render_stride)) 
			// The reason we check !mbIsVisible is because non-visible patches normals 
			// are not updated when their data is changed.  When this changes we can get 
			// rid of mbIsVisible altogether.
		{
			if (mVObjp)
			{
				mVObjp->dirtyGeom();
				if (getNeighborPatch(WEST))
				{
					getNeighborPatch(WEST)->mVObjp->dirtyGeom();
				}
				if (getNeighborPatch(SOUTH))
				{
					getNeighborPatch(SOUTH)->mVObjp->dirtyGeom();
				}
			}
		}
		mVisInfo.mbIsVisible = TRUE;
	}
	else
	{
		mVisInfo.mbIsVisible = FALSE;
	}
}


const LLVector3d &LLSurfacePatch::getOriginGlobal() const
{
	return mOriginGlobal;
}

LLVector3 LLSurfacePatch::getOriginAgent() const
{
	return gAgent.getPosAgentFromGlobal(mOriginGlobal);
}

BOOL LLSurfacePatch::getVisible() const
{
	return mVisInfo.mbIsVisible;
}

U32 LLSurfacePatch::getRenderStride() const
{
	return mVisInfo.mRenderStride;
}

S32 LLSurfacePatch::getRenderLevel() const
{
	return mVisInfo.mRenderLevel;
}

void LLSurfacePatch::setHasReceivedData()
{
	mHasReceivedData = TRUE;
}

BOOL LLSurfacePatch::getHasReceivedData() const
{
	return mHasReceivedData;
}

const LLVector3 &LLSurfacePatch::getCenterRegion() const
{
	return mCenterRegion;
}


void LLSurfacePatch::updateCompositionStats()
{
	LLViewerLayer *vlp = mSurfacep->getRegion()->getComposition();

	F32 x, y, width, height, mpg, min, mean, max;

	LLVector3 origin = getOriginAgent() - mSurfacep->getOriginAgent();
	mpg = mSurfacep->getMetersPerGrid();
	x = origin.mV[VX];
	y = origin.mV[VY];
	width = mpg*(mSurfacep->getGridsPerPatchEdge()+1);
	height = mpg*(mSurfacep->getGridsPerPatchEdge()+1);

	mean = 0.f;
	min = vlp->getValueScaled(x, y);
	max= min;
	U32 count = 0;
	F32 i, j;
	for (j = 0; j < height; j += mpg)
	{
		for (i = 0; i < width; i += mpg)
		{
			F32 comp = vlp->getValueScaled(x + i, y + j);
			mean += comp;
			min = llmin(min, comp);
			max = llmax(max, comp);
			count++;
		}
	}
	mean /= count;

	mMinComposition = min;
	mMeanComposition = mean;
	mMaxComposition = max;
}

F32 LLSurfacePatch::getMeanComposition() const
{
	return mMeanComposition;
}

F32 LLSurfacePatch::getMinComposition() const
{
	return mMinComposition;
}

F32 LLSurfacePatch::getMaxComposition() const
{
	return mMaxComposition;
}

void LLSurfacePatch::setNeighborPatch(const U32 direction, LLSurfacePatch *neighborp)
{
	mNeighborPatches[direction] = neighborp;
	mNormalsInvalid[direction] = TRUE;
	if (direction < 4)
	{
		mNormalsInvalid[gDirAdjacent[direction][0]] = TRUE;
		mNormalsInvalid[gDirAdjacent[direction][1]] = TRUE;
	}
}

LLSurfacePatch *LLSurfacePatch::getNeighborPatch(const U32 direction) const
{
	return mNeighborPatches[direction];
}

void LLSurfacePatch::clearVObj()
{
	mVObjp = NULL;
}
