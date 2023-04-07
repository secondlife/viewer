/** 
 * @file llvolume.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "linden_common.h"
#include "llmemory.h"
#include "llmath.h"

#include <set>
#if !LL_WINDOWS
#include <stdint.h>
#endif
#include <cmath>

#include "llerror.h"

#include "llvolumemgr.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llmatrix3a.h"
#include "lloctree.h"
#include "llvolume.h"
#include "llvolumeoctree.h"
#include "llstl.h"
#include "llsdserialize.h"
#include "llvector4a.h"
#include "llmatrix4a.h"
#include "llmeshoptimizer.h"
#include "lltimer.h"

#define DEBUG_SILHOUETTE_BINORMALS 0
#define DEBUG_SILHOUETTE_NORMALS 0 // TomY: Use this to display normals using the silhouette
#define DEBUG_SILHOUETTE_EDGE_MAP 0 // DaveP: Use this to display edge map using the silhouette

const F32 MIN_CUT_DELTA = 0.02f;

const F32 HOLLOW_MIN = 0.f;
const F32 HOLLOW_MAX = 0.95f;
const F32 HOLLOW_MAX_SQUARE	= 0.7f;

const F32 TWIST_MIN = -1.f;
const F32 TWIST_MAX =  1.f;

const F32 RATIO_MIN = 0.f;
const F32 RATIO_MAX = 2.f; // Tom Y: Inverted sense here: 0 = top taper, 2 = bottom taper

const F32 HOLE_X_MIN= 0.05f;
const F32 HOLE_X_MAX= 1.0f;

const F32 HOLE_Y_MIN= 0.05f;
const F32 HOLE_Y_MAX= 0.5f;

const F32 SHEAR_MIN = -0.5f;
const F32 SHEAR_MAX =  0.5f;

const F32 REV_MIN = 1.f;
const F32 REV_MAX = 4.f;

const F32 TAPER_MIN = -1.f;
const F32 TAPER_MAX =  1.f;

const F32 SKEW_MIN	= -0.95f;
const F32 SKEW_MAX	=  0.95f;

const F32 SCULPT_MIN_AREA = 0.002f;
const S32 SCULPT_MIN_AREA_DETAIL = 1;

BOOL gDebugGL = FALSE; // See settings.xml "RenderDebugGL"

BOOL check_same_clock_dir( const LLVector3& pt1, const LLVector3& pt2, const LLVector3& pt3, const LLVector3& norm)
{    
	LLVector3 test = (pt2-pt1)%(pt3-pt2);

	//answer
	if(test * norm < 0) 
	{
		return FALSE;
	}
	else 
	{
		return TRUE;
	}
} 

BOOL LLLineSegmentBoxIntersect(const LLVector3& start, const LLVector3& end, const LLVector3& center, const LLVector3& size)
{
	return LLLineSegmentBoxIntersect(start.mV, end.mV, center.mV, size.mV);
}

BOOL LLLineSegmentBoxIntersect(const F32* start, const F32* end, const F32* center, const F32* size)
{
	F32 fAWdU[3];
	F32 dir[3];
	F32 diff[3];

	for (U32 i = 0; i < 3; i++)
	{
		dir[i] = 0.5f * (end[i] - start[i]);
		diff[i] = (0.5f * (end[i] + start[i])) - center[i];
		fAWdU[i] = fabsf(dir[i]);
		if(fabsf(diff[i])>size[i] + fAWdU[i]) return false;
	}

	float f;
	f = dir[1] * diff[2] - dir[2] * diff[1];    if(fabsf(f)>size[1]*fAWdU[2] + size[2]*fAWdU[1])  return false;
	f = dir[2] * diff[0] - dir[0] * diff[2];    if(fabsf(f)>size[0]*fAWdU[2] + size[2]*fAWdU[0])  return false;
	f = dir[0] * diff[1] - dir[1] * diff[0];    if(fabsf(f)>size[0]*fAWdU[1] + size[1]*fAWdU[0])  return false;
	
	return true;
}

// Finds tangent vec based on three vertices with texture coordinates.
// Fills in dummy values if the triangle has degenerate texture coordinates.
void calc_tangent_from_triangle(
	LLVector4a&			normal,
	LLVector4a&			tangent_out,
	const LLVector4a& v1,
	const LLVector2&  w1,
	const LLVector4a& v2,
	const LLVector2&  w2,
	const LLVector4a& v3,
	const LLVector2&  w3)
{
	const F32* v1ptr = v1.getF32ptr();
	const F32* v2ptr = v2.getF32ptr();
	const F32* v3ptr = v3.getF32ptr();

	float x1 = v2ptr[0] - v1ptr[0];
	float x2 = v3ptr[0] - v1ptr[0];
	float y1 = v2ptr[1] - v1ptr[1];
	float y2 = v3ptr[1] - v1ptr[1];
	float z1 = v2ptr[2] - v1ptr[2];
	float z2 = v3ptr[2] - v1ptr[2];

	float s1 = w2.mV[0] - w1.mV[0];
	float s2 = w3.mV[0] - w1.mV[0];
	float t1 = w2.mV[1] - w1.mV[1];
	float t2 = w3.mV[1] - w1.mV[1];

	F32 rd = s1*t2-s2*t1;

	float r = ((rd*rd) > FLT_EPSILON) ? (1.0f / rd)
											    : ((rd > 0.0f) ? 1024.f : -1024.f); //some made up large ratio for division by zero

	llassert(llfinite(r));
	llassert(!llisnan(r));

	LLVector4a sdir(
		(t2 * x1 - t1 * x2) * r,
		(t2 * y1 - t1 * y2) * r,
		(t2 * z1 - t1 * z2) * r);

	LLVector4a tdir(
		(s1 * x2 - s2 * x1) * r,
		(s1 * y2 - s2 * y1) * r,
		(s1 * z2 - s2 * z1) * r);

	LLVector4a	n = normal;
	LLVector4a	t = sdir;

	LLVector4a ncrosst;
	ncrosst.setCross3(n,t);

	// Gram-Schmidt orthogonalize
	n.mul(n.dot3(t).getF32());

	LLVector4a tsubn;
	tsubn.setSub(t,n);

	if (tsubn.dot3(tsubn).getF32() > F_APPROXIMATELY_ZERO)
	{
		tsubn.normalize3fast_checked();

		// Calculate handedness
		F32 handedness = ncrosst.dot3(tdir).getF32() < 0.f ? -1.f : 1.f;

		tsubn.getF32ptr()[3] = handedness;

		tangent_out = tsubn;
	}
	else
	{
		// degenerate, make up a value
		//
		tangent_out.set(0,0,1,1);
	}

}


// intersect test between triangle vert0, vert1, vert2 and a ray from orig in direction dir.
// returns TRUE if intersecting and returns barycentric coordinates in intersection_a, intersection_b,
// and returns the intersection point along dir in intersection_t.

// Moller-Trumbore algorithm
BOOL LLTriangleRayIntersect(const LLVector4a& vert0, const LLVector4a& vert1, const LLVector4a& vert2, const LLVector4a& orig, const LLVector4a& dir,
							F32& intersection_a, F32& intersection_b, F32& intersection_t)
{
	
	/* find vectors for two edges sharing vert0 */
	LLVector4a edge1;
	edge1.setSub(vert1, vert0);
	
	LLVector4a edge2;
	edge2.setSub(vert2, vert0);

	/* begin calculating determinant - also used to calculate U parameter */
	LLVector4a pvec;
	pvec.setCross3(dir, edge2);

	/* if determinant is near zero, ray lies in plane of triangle */
	LLVector4a det;
	det.setAllDot3(edge1, pvec);
	
	if (det.greaterEqual(LLVector4a::getEpsilon()).getGatheredBits() & 0x7)
	{
		/* calculate distance from vert0 to ray origin */
		LLVector4a tvec;
		tvec.setSub(orig, vert0);

		/* calculate U parameter and test bounds */
		LLVector4a u;
		u.setAllDot3(tvec,pvec);

		if ((u.greaterEqual(LLVector4a::getZero()).getGatheredBits() & 0x7) &&
			(u.lessEqual(det).getGatheredBits() & 0x7))
		{
			/* prepare to test V parameter */
			LLVector4a qvec;
			qvec.setCross3(tvec, edge1);
			
			/* calculate V parameter and test bounds */
			LLVector4a v;
			v.setAllDot3(dir, qvec);

			
			//if (!(v < 0.f || u + v > det))

			LLVector4a sum_uv;
			sum_uv.setAdd(u, v);

			S32 v_gequal = v.greaterEqual(LLVector4a::getZero()).getGatheredBits() & 0x7;
			S32 sum_lequal = sum_uv.lessEqual(det).getGatheredBits() & 0x7;

			if (v_gequal  && sum_lequal)
			{
				/* calculate t, scale parameters, ray intersects triangle */
				LLVector4a t;
				t.setAllDot3(edge2,qvec);

				t.div(det);
				u.div(det);
				v.div(det);
				
				intersection_a = u[0];
				intersection_b = v[0];
				intersection_t = t[0];
				return TRUE;
			}
		}
	}
		
	return FALSE;
} 

BOOL LLTriangleRayIntersectTwoSided(const LLVector4a& vert0, const LLVector4a& vert1, const LLVector4a& vert2, const LLVector4a& orig, const LLVector4a& dir,
							F32& intersection_a, F32& intersection_b, F32& intersection_t)
{
	F32 u, v, t;
	
	/* find vectors for two edges sharing vert0 */
	LLVector4a edge1;
	edge1.setSub(vert1, vert0);
	
	
	LLVector4a edge2;
	edge2.setSub(vert2, vert0);

	/* begin calculating determinant - also used to calculate U parameter */
	LLVector4a pvec;
	pvec.setCross3(dir, edge2);

	/* if determinant is near zero, ray lies in plane of triangle */
	F32 det = edge1.dot3(pvec).getF32();

	
	if (det > -F_APPROXIMATELY_ZERO && det < F_APPROXIMATELY_ZERO)
	{
		return FALSE;
	}

	F32 inv_det = 1.f / det;

	/* calculate distance from vert0 to ray origin */
	LLVector4a tvec;
	tvec.setSub(orig, vert0);
	
	/* calculate U parameter and test bounds */
	u = (tvec.dot3(pvec).getF32()) * inv_det;
	if (u < 0.f || u > 1.f)
	{
		return FALSE;
	}

	/* prepare to test V parameter */
	tvec.sub(edge1);
		
	/* calculate V parameter and test bounds */
	v = (dir.dot3(tvec).getF32()) * inv_det;
	
	if (v < 0.f || u + v > 1.f)
	{
		return FALSE;
	}

	/* calculate t, ray intersects triangle */
	t = (edge2.dot3(tvec).getF32()) * inv_det;
	
	intersection_a = u;
	intersection_b = v;
	intersection_t = t;
	
	
	return TRUE;
} 

//helper for non-aligned vectors
BOOL LLTriangleRayIntersect(const LLVector3& vert0, const LLVector3& vert1, const LLVector3& vert2, const LLVector3& orig, const LLVector3& dir,
							F32& intersection_a, F32& intersection_b, F32& intersection_t, BOOL two_sided)
{
	LLVector4a vert0a, vert1a, vert2a, origa, dira;
	vert0a.load3(vert0.mV);
	vert1a.load3(vert1.mV);
	vert2a.load3(vert2.mV);
	origa.load3(orig.mV);
	dira.load3(dir.mV);

	if (two_sided)
	{
		return LLTriangleRayIntersectTwoSided(vert0a, vert1a, vert2a, origa, dira, 
				intersection_a, intersection_b, intersection_t);
	}
	else
	{
		return LLTriangleRayIntersect(vert0a, vert1a, vert2a, origa, dira, 
				intersection_a, intersection_b, intersection_t);
	}
}

class LLVolumeOctreeRebound : public LLOctreeTravelerDepthFirst<LLVolumeTriangle, LLVolumeTriangle*>
{
public:
	const LLVolumeFace* mFace;

	LLVolumeOctreeRebound(const LLVolumeFace* face)
	{
		mFace = face;
	}

    virtual void visit(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* branch)
	{ //this is a depth first traversal, so it's safe to assum all children have complete
		//bounding data
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

		LLVolumeOctreeListener* node = (LLVolumeOctreeListener*) branch->getListener(0);

		LLVector4a& min = node->mExtents[0];
		LLVector4a& max = node->mExtents[1];

		if (!branch->isEmpty())
		{ //node has data, find AABB that binds data set
			const LLVolumeTriangle* tri = *(branch->getDataBegin());
			
			//initialize min/max to first available vertex
			min = *(tri->mV[0]);
			max = *(tri->mV[0]);
			
            for (LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>::const_element_iter iter = branch->getDataBegin(); iter != branch->getDataEnd(); ++iter)
			{ //for each triangle in node

				//stretch by triangles in node
				tri = *iter;
				
				min.setMin(min, *tri->mV[0]);
				min.setMin(min, *tri->mV[1]);
				min.setMin(min, *tri->mV[2]);

				max.setMax(max, *tri->mV[0]);
				max.setMax(max, *tri->mV[1]);
				max.setMax(max, *tri->mV[2]);
			}
		}
		else if (branch->getChildCount() > 0)
		{ //no data, but child nodes exist
			LLVolumeOctreeListener* child = (LLVolumeOctreeListener*) branch->getChild(0)->getListener(0);

			//initialize min/max to extents of first child
			min = child->mExtents[0];
			max = child->mExtents[1];
		}
		else
		{
            llassert(!branch->isLeaf()); // Empty leaf
		}

		for (S32 i = 0; i < branch->getChildCount(); ++i)
		{  //stretch by child extents
			LLVolumeOctreeListener* child = (LLVolumeOctreeListener*) branch->getChild(i)->getListener(0);
			min.setMin(min, child->mExtents[0]);
			max.setMax(max, child->mExtents[1]);
		}

		node->mBounds[0].setAdd(min, max);
		node->mBounds[0].mul(0.5f);

		node->mBounds[1].setSub(max,min);
		node->mBounds[1].mul(0.5f);
	}
};

//-------------------------------------------------------------------
// statics
//-------------------------------------------------------------------


//----------------------------------------------------

LLProfile::Face* LLProfile::addCap(S16 faceID)
{
	Face *face   = vector_append(mFaces, 1);
	
	face->mIndex = 0;
	face->mCount = mTotal;
	face->mScaleU= 1.0f;
	face->mCap   = TRUE;
	face->mFaceID = faceID;
	return face;
}

LLProfile::Face* LLProfile::addFace(S32 i, S32 count, F32 scaleU, S16 faceID, BOOL flat)
{
	Face *face   = vector_append(mFaces, 1);
	
	face->mIndex = i;
	face->mCount = count;
	face->mScaleU= scaleU;

	face->mFlat = flat;
	face->mCap   = FALSE;
	face->mFaceID = faceID;
	return face;
}

//static
S32 LLProfile::getNumNGonPoints(const LLProfileParams& params, S32 sides, F32 offset, F32 bevel, F32 ang_scale, S32 split)
{ // this is basically LLProfile::genNGon stripped down to only the operations that influence the number of points
	S32 np = 0;

	// Generate an n-sided "circular" path.
	// 0 is (1,0), and we go counter-clockwise along a circular path from there.
	F32 t, t_step, t_first, t_fraction;
	
	F32 begin  = params.getBegin();
	F32 end    = params.getEnd();

	t_step = 1.0f / sides;
	
	t_first = floor(begin * sides) / (F32)sides;

	// pt1 is the first point on the fractional face.
	// Starting t and ang values for the first face
	t = t_first;
	
	// Increment to the next point.
	// pt2 is the end point on the fractional face
	t += t_step;
	
	t_fraction = (begin - t_first)*sides;

	// Only use if it's not almost exactly on an edge.
	if (t_fraction < 0.9999f)
	{
		np++;
	}

	// There's lots of potential here for floating point error to generate unneeded extra points - DJS 04/05/02
	while (t < end)
	{
		// Iterate through all the integer steps of t.
		np++;

		t += t_step;
	}

	t_fraction = (end - (t - t_step))*sides;

	// Find the fraction that we need to add to the end point.
	t_fraction = (end - (t - t_step))*sides;
	if (t_fraction > 0.0001f)
	{
		np++;
	}

	// If we're sliced, the profile is open.
	if ((end - begin)*ang_scale < 0.99f)
	{
		if (params.getHollow() <= 0)
		{
			// put center point if not hollow.
			np++;
		}
	}
	
	return np;
}

// What is the bevel parameter used for? - DJS 04/05/02
// Bevel parameter is currently unused but presumedly would support
// filleted and chamfered corners
void LLProfile::genNGon(const LLProfileParams& params, S32 sides, F32 offset, F32 bevel, F32 ang_scale, S32 split)
{
	// Generate an n-sided "circular" path.
	// 0 is (1,0), and we go counter-clockwise along a circular path from there.
	static const F32 tableScale[] = { 1, 1, 1, 0.5f, 0.707107f, 0.53f, 0.525f, 0.5f };
	F32 scale = 0.5f;
	F32 t, t_step, t_first, t_fraction, ang, ang_step;
	LLVector4a pt1,pt2;

	F32 begin  = params.getBegin();
	F32 end    = params.getEnd();

	t_step = 1.0f / sides;
	ang_step = 2.0f*F_PI*t_step*ang_scale;

	// Scale to have size "match" scale.  Compensates to get object to generally fill bounding box.

	S32 total_sides = ll_round(sides / ang_scale);	// Total number of sides all around

	if (total_sides < 8)
	{
		scale = tableScale[total_sides];
	}

	t_first = floor(begin * sides) / (F32)sides;

	// pt1 is the first point on the fractional face.
	// Starting t and ang values for the first face
	t = t_first;
	ang = 2.0f*F_PI*(t*ang_scale + offset);
	pt1.set(cos(ang)*scale,sin(ang)*scale, t);

	// Increment to the next point.
	// pt2 is the end point on the fractional face
	t += t_step;
	ang += ang_step;
	pt2.set(cos(ang)*scale,sin(ang)*scale,t);

	t_fraction = (begin - t_first)*sides;

	// Only use if it's not almost exactly on an edge.
	if (t_fraction < 0.9999f)
	{
		LLVector4a new_pt;
		new_pt.setLerp(pt1, pt2, t_fraction);
		mProfile.push_back(new_pt);
	}

	// There's lots of potential here for floating point error to generate unneeded extra points - DJS 04/05/02
	while (t < end)
	{
		// Iterate through all the integer steps of t.
		pt1.set(cos(ang)*scale,sin(ang)*scale,t);

		if (mProfile.size() > 0) {
			LLVector4a p = mProfile[mProfile.size()-1];
			for (S32 i = 0; i < split && mProfile.size() > 0; i++) {
				//mProfile.push_back(p+(pt1-p) * 1.0f/(float)(split+1) * (float)(i+1));
				LLVector4a new_pt;
				new_pt.setSub(pt1, p);
				new_pt.mul(1.0f/(float)(split+1) * (float)(i+1));
				new_pt.add(p);
				mProfile.push_back(new_pt);
			}
		}
		mProfile.push_back(pt1);

		t += t_step;
		ang += ang_step;
	}

	t_fraction = (end - (t - t_step))*sides;

	// pt1 is the first point on the fractional face
	// pt2 is the end point on the fractional face
	pt2.set(cos(ang)*scale,sin(ang)*scale,t);

	// Find the fraction that we need to add to the end point.
	t_fraction = (end - (t - t_step))*sides;
	if (t_fraction > 0.0001f)
	{
		LLVector4a new_pt;
		new_pt.setLerp(pt1, pt2, t_fraction);
		
		if (mProfile.size() > 0) {
			LLVector4a p = mProfile[mProfile.size()-1];
			for (S32 i = 0; i < split && mProfile.size() > 0; i++) {
				//mProfile.push_back(p+(new_pt-p) * 1.0f/(float)(split+1) * (float)(i+1));

				LLVector4a pt1;
				pt1.setSub(new_pt, p);
				pt1.mul(1.0f/(float)(split+1) * (float)(i+1));
				pt1.add(p);
				mProfile.push_back(pt1);
			}
		}
		mProfile.push_back(new_pt);
	}

	// If we're sliced, the profile is open.
	if ((end - begin)*ang_scale < 0.99f)
	{
		if ((end - begin)*ang_scale > 0.5f)
		{
			mConcave = TRUE;
		}
		else
		{
			mConcave = FALSE;
		}
		mOpen = TRUE;
		if (params.getHollow() <= 0)
		{
			// put center point if not hollow.
			mProfile.push_back(LLVector4a(0,0,0));
		}
	}
	else
	{
		// The profile isn't open.
		mOpen = FALSE;
		mConcave = FALSE;
	}

	mTotal = mProfile.size();
}

// Hollow is percent of the original bounding box, not of this particular
// profile's geometry.  Thus, a swept triangle needs lower hollow values than
// a swept square.
LLProfile::Face* LLProfile::addHole(const LLProfileParams& params, BOOL flat, F32 sides, F32 offset, F32 box_hollow, F32 ang_scale, S32 split)
{
	// Note that addHole will NOT work for non-"circular" profiles, if we ever decide to use them.

	// Total add has number of vertices on outside.
	mTotalOut = mTotal;

	// Why is the "bevel" parameter -1? DJS 04/05/02
	genNGon(params, llfloor(sides),offset,-1, ang_scale, split);

	Face *face = addFace(mTotalOut, mTotal-mTotalOut,0,LL_FACE_INNER_SIDE, flat);

	static thread_local LLAlignedArray<LLVector4a,64> pt;
	pt.resize(mTotal) ;

	for (S32 i=mTotalOut;i<mTotal;i++)
	{
		pt[i] = mProfile[i];
		pt[i].mul(box_hollow);
	}

	S32 j=mTotal-1;
	for (S32 i=mTotalOut;i<mTotal;i++)
	{
		mProfile[i] = pt[j--];
	}

	for (S32 i=0;i<(S32)mFaces.size();i++) 
	{
		if (mFaces[i].mCap)
		{
			mFaces[i].mCount *= 2;
		}
	}

	return face;
}

//static
S32 LLProfile::getNumPoints(const LLProfileParams& params, BOOL path_open,F32 detail, S32 split,
						 BOOL is_sculpted, S32 sculpt_size)
{ // this is basically LLProfile::generate stripped down to only operations that influence the number of points
	if (detail < MIN_LOD)
	{
		detail = MIN_LOD;
	}

	// Generate the face data
	F32 hollow = params.getHollow();

	S32 np = 0;

	switch (params.getCurveType() & LL_PCODE_PROFILE_MASK)
	{
	case LL_PCODE_PROFILE_SQUARE:
		{
			np = getNumNGonPoints(params, 4,-0.375, 0, 1, split);
		
			if (hollow)
			{
				np *= 2;
			}
		}
		break;
	case  LL_PCODE_PROFILE_ISOTRI:
	case  LL_PCODE_PROFILE_RIGHTTRI:
	case  LL_PCODE_PROFILE_EQUALTRI:
		{
			np = getNumNGonPoints(params, 3,0, 0, 1, split);
						
			if (hollow)
			{
				np *= 2;
			}
		}
		break;
	case LL_PCODE_PROFILE_CIRCLE:
		{
			// If this has a square hollow, we should adjust the
			// number of faces a bit so that the geometry lines up.
			U8 hole_type=0;
			F32 circle_detail = MIN_DETAIL_FACES * detail;
			if (hollow)
			{
				hole_type = params.getCurveType() & LL_PCODE_HOLE_MASK;
				if (hole_type == LL_PCODE_HOLE_SQUARE)
				{
					// Snap to the next multiple of four sides,
					// so that corners line up.
					circle_detail = llceil(circle_detail / 4.0f) * 4.0f;
				}
			}

			S32 sides = (S32)circle_detail;

			if (is_sculpted)
				sides = sculpt_size;
			
			np = getNumNGonPoints(params, sides);
			
			if (hollow)
			{
				np *= 2;
			}
		}
		break;
	case LL_PCODE_PROFILE_CIRCLE_HALF:
		{
			// If this has a square hollow, we should adjust the
			// number of faces a bit so that the geometry lines up.
			U8 hole_type=0;
			// Number of faces is cut in half because it's only a half-circle.
			F32 circle_detail = MIN_DETAIL_FACES * detail * 0.5f;
			if (hollow)
			{
				hole_type = params.getCurveType() & LL_PCODE_HOLE_MASK;
				if (hole_type == LL_PCODE_HOLE_SQUARE)
				{
					// Snap to the next multiple of four sides (div 2),
					// so that corners line up.
					circle_detail = llceil(circle_detail / 2.0f) * 2.0f;
				}
			}
			np = getNumNGonPoints(params, llfloor(circle_detail), 0.5f, 0.f, 0.5f);
			
			if (hollow)
			{
				np *= 2;
			}

			// Special case for openness of sphere
			if ((params.getEnd() - params.getBegin()) < 1.f)
			{
			}
			else if (!hollow)
			{
				np++;
			}
		}
		break;
	default:
	   break;
	};

	
	return np;
}


BOOL LLProfile::generate(const LLProfileParams& params, BOOL path_open,F32 detail, S32 split,
						 BOOL is_sculpted, S32 sculpt_size)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	if ((!mDirty) && (!is_sculpted))
	{
		return FALSE;
	}
	mDirty = FALSE;

	if (detail < MIN_LOD)
	{
		LL_INFOS() << "Generating profile with LOD < MIN_LOD.  CLAMPING" << LL_ENDL;
		detail = MIN_LOD;
	}

	mProfile.resize(0);
	mFaces.resize(0);

	// Generate the face data
	S32 i;
	F32 begin = params.getBegin();
	F32 end = params.getEnd();
	F32 hollow = params.getHollow();

	// Quick validation to eliminate some server crashes.
	if (begin > end - 0.01f)
	{
		LL_WARNS() << "LLProfile::generate() assertion failed (begin >= end)" << LL_ENDL;
		return FALSE;
	}

	S32 face_num = 0;

	switch (params.getCurveType() & LL_PCODE_PROFILE_MASK)
	{
	case LL_PCODE_PROFILE_SQUARE:
		{
			genNGon(params, 4,-0.375, 0, 1, split);
			if (path_open)
			{
				addCap (LL_FACE_PATH_BEGIN);
			}

			for (i = llfloor(begin * 4.f); i < llfloor(end * 4.f + .999f); i++)
			{
				addFace((face_num++) * (split +1), split+2, 1, LL_FACE_OUTER_SIDE_0 << i, TRUE);
			}

			LLVector4a scale(1,1,4,1);

			for (i = 0; i <(S32) mProfile.size(); i++)
			{
				// Scale by 4 to generate proper tex coords.
				mProfile[i].mul(scale);
				llassert(mProfile[i].isFinite3());
			}

			if (hollow)
			{
				switch (params.getCurveType() & LL_PCODE_HOLE_MASK)
				{
				case LL_PCODE_HOLE_TRIANGLE:
					// This offset is not correct, but we can't change it now... DK 11/17/04
				  	addHole(params, TRUE, 3, -0.375f, hollow, 1.f, split);
					break;
				case LL_PCODE_HOLE_CIRCLE:
					// TODO: Compute actual detail levels for cubes
				  	addHole(params, FALSE, MIN_DETAIL_FACES * detail, -0.375f, hollow, 1.f);
					break;
				case LL_PCODE_HOLE_SAME:
				case LL_PCODE_HOLE_SQUARE:
				default:
					addHole(params, TRUE, 4, -0.375f, hollow, 1.f, split);
					break;
				}
			}
			
			if (path_open) {
				mFaces[0].mCount = mTotal;
			}
		}
		break;
	case  LL_PCODE_PROFILE_ISOTRI:
	case  LL_PCODE_PROFILE_RIGHTTRI:
	case  LL_PCODE_PROFILE_EQUALTRI:
		{
			genNGon(params, 3,0, 0, 1, split);
			LLVector4a scale(1,1,3,1);
			for (i = 0; i <(S32) mProfile.size(); i++)
			{
				// Scale by 3 to generate proper tex coords.
				mProfile[i].mul(scale);
				llassert(mProfile[i].isFinite3());
			}

			if (path_open)
			{
				addCap(LL_FACE_PATH_BEGIN);
			}

			for (i = llfloor(begin * 3.f); i < llfloor(end * 3.f + .999f); i++)
			{
				addFace((face_num++) * (split +1), split+2, 1, LL_FACE_OUTER_SIDE_0 << i, TRUE);
			}
			if (hollow)
			{
				// Swept triangles need smaller hollowness values,
				// because the triangle doesn't fill the bounding box.
				F32 triangle_hollow = hollow / 2.f;

				switch (params.getCurveType() & LL_PCODE_HOLE_MASK)
				{
				case LL_PCODE_HOLE_CIRCLE:
					// TODO: Actually generate level of detail for triangles
					addHole(params, FALSE, MIN_DETAIL_FACES * detail, 0, triangle_hollow, 1.f);
					break;
				case LL_PCODE_HOLE_SQUARE:
					addHole(params, TRUE, 4, 0, triangle_hollow, 1.f, split);
					break;
				case LL_PCODE_HOLE_SAME:
				case LL_PCODE_HOLE_TRIANGLE:
				default:
					addHole(params, TRUE, 3, 0, triangle_hollow, 1.f, split);
					break;
				}
			}
		}
		break;
	case LL_PCODE_PROFILE_CIRCLE:
		{
			// If this has a square hollow, we should adjust the
			// number of faces a bit so that the geometry lines up.
			U8 hole_type=0;
			F32 circle_detail = MIN_DETAIL_FACES * detail;
			if (hollow)
			{
				hole_type = params.getCurveType() & LL_PCODE_HOLE_MASK;
				if (hole_type == LL_PCODE_HOLE_SQUARE)
				{
					// Snap to the next multiple of four sides,
					// so that corners line up.
					circle_detail = llceil(circle_detail / 4.0f) * 4.0f;
				}
			}

			S32 sides = (S32)circle_detail;

			if (is_sculpted)
				sides = sculpt_size;
			
			genNGon(params, sides);
			
			if (path_open)
			{
				addCap (LL_FACE_PATH_BEGIN);
			}

			if (mOpen && !hollow)
			{
				addFace(0,mTotal-1,0,LL_FACE_OUTER_SIDE_0, FALSE);
			}
			else
			{
				addFace(0,mTotal,0,LL_FACE_OUTER_SIDE_0, FALSE);
			}

			if (hollow)
			{
				switch (hole_type)
				{
				case LL_PCODE_HOLE_SQUARE:
					addHole(params, TRUE, 4, 0, hollow, 1.f, split);
					break;
				case LL_PCODE_HOLE_TRIANGLE:
					addHole(params, TRUE, 3, 0, hollow, 1.f, split);
					break;
				case LL_PCODE_HOLE_CIRCLE:
				case LL_PCODE_HOLE_SAME:
				default:
					addHole(params, FALSE, circle_detail, 0, hollow, 1.f);
					break;
				}
			}
		}
		break;
	case LL_PCODE_PROFILE_CIRCLE_HALF:
		{
			// If this has a square hollow, we should adjust the
			// number of faces a bit so that the geometry lines up.
			U8 hole_type=0;
			// Number of faces is cut in half because it's only a half-circle.
			F32 circle_detail = MIN_DETAIL_FACES * detail * 0.5f;
			if (hollow)
			{
				hole_type = params.getCurveType() & LL_PCODE_HOLE_MASK;
				if (hole_type == LL_PCODE_HOLE_SQUARE)
				{
					// Snap to the next multiple of four sides (div 2),
					// so that corners line up.
					circle_detail = llceil(circle_detail / 2.0f) * 2.0f;
				}
			}
			genNGon(params, llfloor(circle_detail), 0.5f, 0.f, 0.5f);
			if (path_open)
			{
				addCap(LL_FACE_PATH_BEGIN);
			}
			if (mOpen && !params.getHollow())
			{
				addFace(0,mTotal-1,0,LL_FACE_OUTER_SIDE_0, FALSE);
			}
			else
			{
				addFace(0,mTotal,0,LL_FACE_OUTER_SIDE_0, FALSE);
			}

			if (hollow)
			{
				switch (hole_type)
				{
				case LL_PCODE_HOLE_SQUARE:
					addHole(params, TRUE, 2, 0.5f, hollow, 0.5f, split);
					break;
				case LL_PCODE_HOLE_TRIANGLE:
					addHole(params, TRUE, 3,  0.5f, hollow, 0.5f, split);
					break;
				case LL_PCODE_HOLE_CIRCLE:
				case LL_PCODE_HOLE_SAME:
				default:
					addHole(params, FALSE, circle_detail,  0.5f, hollow, 0.5f);
					break;
				}
			}

			// Special case for openness of sphere
			if ((params.getEnd() - params.getBegin()) < 1.f)
			{
				mOpen = TRUE;
			}
			else if (!hollow)
			{
				mOpen = FALSE;
				mProfile.push_back(mProfile[0]);
				mTotal++;
			}
		}
		break;
	default:
	    LL_ERRS() << "Unknown profile: getCurveType()=" << params.getCurveType() << LL_ENDL;
		break;
	};

	if (path_open)
	{
		addCap(LL_FACE_PATH_END); // bottom
	}
	
	if ( mOpen) // interior edge caps
	{
		addFace(mTotal-1, 2,0.5,LL_FACE_PROFILE_BEGIN, TRUE); 

		if (hollow)
		{
			addFace(mTotalOut-1, 2,0.5,LL_FACE_PROFILE_END, TRUE);
		}
		else
		{
			addFace(mTotal-2, 2,0.5,LL_FACE_PROFILE_END, TRUE);
		}
	}
	
	return TRUE;
}



BOOL LLProfileParams::importFile(LLFILE *fp)
{
	const S32 BUFSIZE = 16384;
	char buffer[BUFSIZE];	/* Flawfinder: ignore */
	// *NOTE: changing the size or type of these buffers will require
	// changing the sscanf below.
	char keyword[256];	/* Flawfinder: ignore */
	char valuestr[256];	/* Flawfinder: ignore */
	keyword[0] = 0;
	valuestr[0] = 0;
	F32 tempF32;
	U32 tempU32;

	while (!feof(fp))
	{
		if (fgets(buffer, BUFSIZE, fp) == NULL)
		{
			buffer[0] = '\0';
		}
		
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %255s %255s",
			keyword, valuestr);
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}",keyword))
		{
			break;
		}
		else if (!strcmp("curve", keyword))
		{
			sscanf(valuestr,"%d",&tempU32);
			setCurveType((U8) tempU32);
		}
		else if (!strcmp("begin",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setBegin(tempF32);
		}
		else if (!strcmp("end",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setEnd(tempF32);
		}
		else if (!strcmp("hollow",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setHollow(tempF32);
		}
		else
		{
			LL_WARNS() << "unknown keyword " << keyword << " in profile import" << LL_ENDL;
		}
	}

	return TRUE;
}


BOOL LLProfileParams::exportFile(LLFILE *fp) const
{
	fprintf(fp,"\t\tprofile 0\n");
	fprintf(fp,"\t\t{\n");
	fprintf(fp,"\t\t\tcurve\t%d\n", getCurveType());
	fprintf(fp,"\t\t\tbegin\t%g\n", getBegin());
	fprintf(fp,"\t\t\tend\t%g\n", getEnd());
	fprintf(fp,"\t\t\thollow\t%g\n", getHollow());
	fprintf(fp, "\t\t}\n");
	return TRUE;
}


BOOL LLProfileParams::importLegacyStream(std::istream& input_stream)
{
	const S32 BUFSIZE = 16384;
	char buffer[BUFSIZE];	/* Flawfinder: ignore */
	// *NOTE: changing the size or type of these buffers will require
	// changing the sscanf below.
	char keyword[256];	/* Flawfinder: ignore */
	char valuestr[256];	/* Flawfinder: ignore */
	keyword[0] = 0;
	valuestr[0] = 0;
	F32 tempF32;
	U32 tempU32;

	while (input_stream.good())
	{
		input_stream.getline(buffer, BUFSIZE);
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %255s %255s",
			keyword,
			valuestr);
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}",keyword))
		{
			break;
		}
		else if (!strcmp("curve", keyword))
		{
			sscanf(valuestr,"%d",&tempU32);
			setCurveType((U8) tempU32);
		}
		else if (!strcmp("begin",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setBegin(tempF32);
		}
		else if (!strcmp("end",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setEnd(tempF32);
		}
		else if (!strcmp("hollow",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setHollow(tempF32);
		}
		else
		{
 		LL_WARNS() << "unknown keyword " << keyword << " in profile import" << LL_ENDL;
		}
	}

	return TRUE;
}


BOOL LLProfileParams::exportLegacyStream(std::ostream& output_stream) const
{
	output_stream <<"\t\tprofile 0\n";
	output_stream <<"\t\t{\n";
	output_stream <<"\t\t\tcurve\t" << (S32) getCurveType() << "\n";
	output_stream <<"\t\t\tbegin\t" << getBegin() << "\n";
	output_stream <<"\t\t\tend\t" << getEnd() << "\n";
	output_stream <<"\t\t\thollow\t" << getHollow() << "\n";
	output_stream << "\t\t}\n";
	return TRUE;
}

LLSD LLProfileParams::asLLSD() const
{
	LLSD sd;

	sd["curve"] = getCurveType();
	sd["begin"] = getBegin();
	sd["end"] = getEnd();
	sd["hollow"] = getHollow();
	return sd;
}

bool LLProfileParams::fromLLSD(LLSD& sd)
{
	setCurveType(sd["curve"].asInteger());
	setBegin((F32)sd["begin"].asReal());
	setEnd((F32)sd["end"].asReal());
	setHollow((F32)sd["hollow"].asReal());
	return true;
}

void LLProfileParams::copyParams(const LLProfileParams &params)
{
	setCurveType(params.getCurveType());
	setBegin(params.getBegin());
	setEnd(params.getEnd());
	setHollow(params.getHollow());
}


LLPath::~LLPath()
{
}

S32 LLPath::getNumNGonPoints(const LLPathParams& params, S32 sides, F32 startOff, F32 end_scale, F32 twist_scale)
{ //this is basically LLPath::genNGon stripped down to only operations that influence the number of points added
	S32 ret = 0;

	F32 step= 1.0f / sides;
	F32 t	= params.getBegin();
	ret = 1;
	
	t+=step;

	// Snap to a quantized parameter, so that cut does not
	// affect most sample points.
	t = ((S32)(t * sides)) / (F32)sides;

	// Run through the non-cut dependent points.
	while (t < params.getEnd())
	{
		ret++;
		t+=step;
	}

	ret++;

	return ret;
}

void LLPath::genNGon(const LLPathParams& params, S32 sides, F32 startOff, F32 end_scale, F32 twist_scale)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	// Generates a circular path, starting at (1, 0, 0), counterclockwise along the xz plane.
	static const F32 tableScale[] = { 1, 1, 1, 0.5f, 0.707107f, 0.53f, 0.525f, 0.5f };

	F32 revolutions = params.getRevolutions();
	F32 skew		= params.getSkew();
	F32 skew_mag	= fabs(skew);
	F32 hole_x		= params.getScaleX() * (1.0f - skew_mag);
	F32 hole_y		= params.getScaleY();

	// Calculate taper begin/end for x,y (Negative means taper the beginning)
	F32 taper_x_begin	= 1.0f;
	F32 taper_x_end		= 1.0f - params.getTaperX();
	F32	taper_y_begin	= 1.0f;
	F32	taper_y_end		= 1.0f - params.getTaperY();

	if ( taper_x_end > 1.0f )
	{
		// Flip tapering.
		taper_x_begin	= 2.0f - taper_x_end;
		taper_x_end		= 1.0f;
	}
	if ( taper_y_end > 1.0f )
	{
		// Flip tapering.
		taper_y_begin	= 2.0f - taper_y_end;
		taper_y_end		= 1.0f;
	}

	// For spheres, the radius is usually zero.
	F32 radius_start = 0.5f;
	if (sides < 8)
	{
		radius_start = tableScale[sides];
	}

	// Scale the radius to take the hole size into account.
	radius_start *= 1.0f - hole_y;
	
	// Now check the radius offset to calculate the start,end radius.  (Negative means
	// decrease the start radius instead).
	F32 radius_end    = radius_start;
	F32 radius_offset = params.getRadiusOffset();
	if (radius_offset < 0.f)
	{
		radius_start *= 1.f + radius_offset;
	}
	else
	{
		radius_end   *= 1.f - radius_offset;
	}	

	// Is the path NOT a closed loop?
	mOpen = ( (params.getEnd()*end_scale - params.getBegin() < 1.0f) ||
		      (skew_mag > 0.001f) ||
			  (fabs(taper_x_end - taper_x_begin) > 0.001f) ||
			  (fabs(taper_y_end - taper_y_begin) > 0.001f) ||
			  (fabs(radius_end - radius_start) > 0.001f) );

	F32 ang, c, s;
	LLQuaternion twist, qang;
	PathPt *pt;
	LLVector3 path_axis (1.f, 0.f, 0.f);
	//LLVector3 twist_axis(0.f, 0.f, 1.f);
	F32 twist_begin = params.getTwistBegin() * twist_scale;
	F32 twist_end	= params.getTwist() * twist_scale;

	// We run through this once before the main loop, to make sure
	// the path begins at the correct cut.
	F32 step= 1.0f / sides;
	F32 t	= params.getBegin();
	pt		= mPath.append(1);
	ang		= 2.0f*F_PI*revolutions * t;
	s		= sin(ang)*lerp(radius_start, radius_end, t);	
	c		= cos(ang)*lerp(radius_start, radius_end, t);


	pt->mPos.set(0 + lerp(0,params.getShear().mV[0],s)
					  + lerp(-skew ,skew, t) * 0.5f,
					c + lerp(0,params.getShear().mV[1],s), 
					s);
	pt->mScale.set(hole_x * lerp(taper_x_begin, taper_x_end, t),
		hole_y * lerp(taper_y_begin, taper_y_end, t),
		0,1);
	pt->mTexT  = t;

	// Twist rotates the path along the x,y plane (I think) - DJS 04/05/02
	twist.setQuat  (lerp(twist_begin,twist_end,t) * 2.f * F_PI - F_PI,0,0,1);
	// Rotate the point around the circle's center.
	qang.setQuat   (ang,path_axis);

	LLMatrix3 rot(twist * qang);

	pt->mRot.loadu(rot);

	t+=step;

	// Snap to a quantized parameter, so that cut does not
	// affect most sample points.
	t = ((S32)(t * sides)) / (F32)sides;

	// Run through the non-cut dependent points.
	while (t < params.getEnd())
	{
		pt		= mPath.append(1);

		ang = 2.0f*F_PI*revolutions * t;
		c   = cos(ang)*lerp(radius_start, radius_end, t);
		s   = sin(ang)*lerp(radius_start, radius_end, t);

		pt->mPos.set(0 + lerp(0,params.getShear().mV[0],s)
					      + lerp(-skew ,skew, t) * 0.5f,
						c + lerp(0,params.getShear().mV[1],s), 
						s);

		pt->mScale.set(hole_x * lerp(taper_x_begin, taper_x_end, t),
					hole_y * lerp(taper_y_begin, taper_y_end, t),
					0,1);
		pt->mTexT  = t;

		// Twist rotates the path along the x,y plane (I think) - DJS 04/05/02
		twist.setQuat  (lerp(twist_begin,twist_end,t) * 2.f * F_PI - F_PI,0,0,1);
		// Rotate the point around the circle's center.
		qang.setQuat   (ang,path_axis);
		LLMatrix3 tmp(twist*qang);
		pt->mRot.loadu(tmp);

		t+=step;
	}

	// Make one final pass for the end cut.
	t = params.getEnd();
	pt		= mPath.append(1);
	ang = 2.0f*F_PI*revolutions * t;
	c   = cos(ang)*lerp(radius_start, radius_end, t);
	s   = sin(ang)*lerp(radius_start, radius_end, t);

	pt->mPos.set(0 + lerp(0,params.getShear().mV[0],s)
					  + lerp(-skew ,skew, t) * 0.5f,
					c + lerp(0,params.getShear().mV[1],s), 
					s);
	pt->mScale.set(hole_x * lerp(taper_x_begin, taper_x_end, t),
				   hole_y * lerp(taper_y_begin, taper_y_end, t),
				   0,1);
	pt->mTexT  = t;

	// Twist rotates the path along the x,y plane (I think) - DJS 04/05/02
	twist.setQuat  (lerp(twist_begin,twist_end,t) * 2.f * F_PI - F_PI,0,0,1);
	// Rotate the point around the circle's center.
	qang.setQuat   (ang,path_axis);
	LLMatrix3 tmp(twist*qang);
	pt->mRot.loadu(tmp);

	mTotal = mPath.size();
}

const LLVector2 LLPathParams::getBeginScale() const
{
	LLVector2 begin_scale(1.f, 1.f);
	if (getScaleX() > 1)
	{
		begin_scale.mV[0] = 2-getScaleX();
	}
	if (getScaleY() > 1)
	{
		begin_scale.mV[1] = 2-getScaleY();
	}
	return begin_scale;
}

const LLVector2 LLPathParams::getEndScale() const
{
	LLVector2 end_scale(1.f, 1.f);
	if (getScaleX() < 1)
	{
		end_scale.mV[0] = getScaleX();
	}
	if (getScaleY() < 1)
	{
		end_scale.mV[1] = getScaleY();
	}
	return end_scale;
}

S32 LLPath::getNumPoints(const LLPathParams& params, F32 detail)
{ // this is basically LLPath::generate stripped down to only the operations that influence the number of points
	if (detail < MIN_LOD)
	{
		detail = MIN_LOD;
	}

	S32 np = 2; // hardcode for line

	// Is this 0xf0 mask really necessary?  DK 03/02/05

	switch (params.getCurveType() & 0xf0)
	{
	default:
	case LL_PCODE_PATH_LINE:
		{
			// Take the begin/end twist into account for detail.
			np    = llfloor(fabs(params.getTwistBegin() - params.getTwist()) * 3.5f * (detail-0.5f)) + 2;
		}
		break;

	case LL_PCODE_PATH_CIRCLE:
		{
			// Increase the detail as the revolutions and twist increase.
			F32 twist_mag = fabs(params.getTwistBegin() - params.getTwist());

			S32 sides = (S32)llfloor(llfloor((MIN_DETAIL_FACES * detail + twist_mag * 3.5f * (detail-0.5f))) * params.getRevolutions());

			np = sides;
		}
		break;

	case LL_PCODE_PATH_CIRCLE2:
		{
			//genNGon(params, llfloor(MIN_DETAIL_FACES * detail), 4.f, 0.f);
			np = getNumNGonPoints(params, llfloor(MIN_DETAIL_FACES * detail));
		}
		break;

	case LL_PCODE_PATH_TEST:

		np     = 5;
		break;
	};

	return np;
}

BOOL LLPath::generate(const LLPathParams& params, F32 detail, S32 split,
					  BOOL is_sculpted, S32 sculpt_size)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	if ((!mDirty) && (!is_sculpted))
	{
		return FALSE;
	}

	if (detail < MIN_LOD)
	{
		LL_INFOS() << "Generating path with LOD < MIN!  Clamping to 1" << LL_ENDL;
		detail = MIN_LOD;
	}

	mDirty = FALSE;
	S32 np = 2; // hardcode for line

	mPath.resize(0);
	mOpen = TRUE;

	// Is this 0xf0 mask really necessary?  DK 03/02/05
	switch (params.getCurveType() & 0xf0)
	{
	default:
	case LL_PCODE_PATH_LINE:
		{
			// Take the begin/end twist into account for detail.
			np    = llfloor(fabs(params.getTwistBegin() - params.getTwist()) * 3.5f * (detail-0.5f)) + 2;
			if (np < split+2)
			{
				np = split+2;
			}

			mStep = 1.0f / (np-1);
			
			mPath.resize(np);

			LLVector2 start_scale = params.getBeginScale();
			LLVector2 end_scale = params.getEndScale();

			for (S32 i=0;i<np;i++)
			{
				F32 t = lerp(params.getBegin(),params.getEnd(),(F32)i * mStep);
				mPath[i].mPos.set(lerp(0,params.getShear().mV[0],t),
									 lerp(0,params.getShear().mV[1],t),
									 t - 0.5f);
				LLQuaternion quat;
				quat.setQuat(lerp(F_PI * params.getTwistBegin(),F_PI * params.getTwist(),t),0,0,1);
				LLMatrix3 tmp(quat);
				mPath[i].mRot.loadu(tmp);
				mPath[i].mScale.set(lerp(start_scale.mV[0],end_scale.mV[0],t),
									lerp(start_scale.mV[1],end_scale.mV[1],t),
									0,1);
				mPath[i].mTexT        = t;
			}
		}
		break;

	case LL_PCODE_PATH_CIRCLE:
		{
			// Increase the detail as the revolutions and twist increase.
			F32 twist_mag = fabs(params.getTwistBegin() - params.getTwist());

			S32 sides = (S32)llfloor(llfloor((MIN_DETAIL_FACES * detail + twist_mag * 3.5f * (detail-0.5f))) * params.getRevolutions());

			if (is_sculpted)
				sides = llmax(sculpt_size, 1);
			
			if (0 < sides)
				genNGon(params, sides);
		}
		break;

	case LL_PCODE_PATH_CIRCLE2:
		{
			if (params.getEnd() - params.getBegin() >= 0.99f &&
				params.getScaleX() >= .99f)
			{
				mOpen = FALSE;
			}

			//genNGon(params, llfloor(MIN_DETAIL_FACES * detail), 4.f, 0.f);
			genNGon(params, llfloor(MIN_DETAIL_FACES * detail));

			F32 toggle = 0.5f;
			for (S32 i=0;i<(S32)mPath.size();i++)
			{
				mPath[i].mPos.getF32ptr()[0] = toggle;
				if (toggle == 0.5f)
					toggle = -0.5f;
				else
					toggle = 0.5f;
			}
		}

		break;

	case LL_PCODE_PATH_TEST:

		np     = 5;
		mStep = 1.0f / (np-1);
		
		mPath.resize(np);

		for (S32 i=0;i<np;i++)
		{
			F32 t = (F32)i * mStep;
			mPath[i].mPos.set(0,
								lerp(0,   -sin(F_PI*params.getTwist()*t)*0.5f,t),
								lerp(-0.5f, cos(F_PI*params.getTwist()*t)*0.5f,t));
			mPath[i].mScale.set(lerp(1,params.getScale().mV[0],t),
								lerp(1,params.getScale().mV[1],t), 0,1);
			mPath[i].mTexT  = t;
			LLQuaternion quat;
			quat.setQuat(F_PI * params.getTwist() * t,1,0,0);
			LLMatrix3 tmp(quat);
			mPath[i].mRot.loadu(tmp);
		}

		break;
	};

	if (params.getTwist() != params.getTwistBegin()) mOpen = TRUE;

	//if ((int(fabsf(params.getTwist() - params.getTwistBegin())*100))%100 != 0) {
	//	mOpen = TRUE;
	//}
	
	return TRUE;
}

BOOL LLDynamicPath::generate(const LLPathParams& params, F32 detail, S32 split,
							 BOOL is_sculpted, S32 sculpt_size)
{
	mOpen = TRUE; // Draw end caps
	if (getPathLength() == 0)
	{
		// Path hasn't been generated yet.
		// Some algorithms later assume at least TWO path points.
		resizePath(2);
		LLQuaternion quat;
		quat.setQuat(0,0,0);
		LLMatrix3 tmp(quat);

		for (U32 i = 0; i < 2; i++)
		{
			mPath[i].mPos.set(0, 0, 0);
			mPath[i].mRot.loadu(tmp);
			mPath[i].mScale.set(1, 1, 0, 1);
			mPath[i].mTexT = 0;
		}
	}

	return TRUE;
}


BOOL LLPathParams::importFile(LLFILE *fp)
{
	const S32 BUFSIZE = 16384;
	char buffer[BUFSIZE];	/* Flawfinder: ignore */
	// *NOTE: changing the size or type of these buffers will require
	// changing the sscanf below.
	char keyword[256];	/* Flawfinder: ignore */
	char valuestr[256];	/* Flawfinder: ignore */
	keyword[0] = 0;
	valuestr[0] = 0;

	F32 tempF32;
	F32 x, y;
	U32 tempU32;

	while (!feof(fp))
	{
		if (fgets(buffer, BUFSIZE, fp) == NULL)
		{
			buffer[0] = '\0';
		}
		
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %255s %255s",
			keyword, valuestr);
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}",keyword))
		{
			break;
		}
		else if (!strcmp("curve", keyword))
		{
			sscanf(valuestr,"%d",&tempU32);
			setCurveType((U8) tempU32);
		}
		else if (!strcmp("begin",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setBegin(tempF32);
		}
		else if (!strcmp("end",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setEnd(tempF32);
		}
		else if (!strcmp("scale",keyword))
		{
			// Legacy for one dimensional scale per path
			sscanf(valuestr,"%g",&tempF32);
			setScale(tempF32, tempF32);
		}
		else if (!strcmp("scale_x", keyword))
		{
			sscanf(valuestr, "%g", &x);
			setScaleX(x);
		}
		else if (!strcmp("scale_y", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setScaleY(y);
		}
		else if (!strcmp("shear_x", keyword))
		{
			sscanf(valuestr, "%g", &x);
			setShearX(x);
		}
		else if (!strcmp("shear_y", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setShearY(y);
		}
		else if (!strcmp("twist",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setTwist(tempF32);
		}
		else if (!strcmp("twist_begin", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setTwistBegin(y);
		}
		else if (!strcmp("radius_offset", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setRadiusOffset(y);
		}
		else if (!strcmp("taper_x", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setTaperX(y);
		}
		else if (!strcmp("taper_y", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setTaperY(y);
		}
		else if (!strcmp("revolutions", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setRevolutions(y);
		}
		else if (!strcmp("skew", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setSkew(y);
		}
		else
		{
			LL_WARNS() << "unknown keyword " << " in path import" << LL_ENDL;
		}
	}
	return TRUE;
}


BOOL LLPathParams::exportFile(LLFILE *fp) const
{
	fprintf(fp, "\t\tpath 0\n");
	fprintf(fp, "\t\t{\n");
	fprintf(fp, "\t\t\tcurve\t%d\n", getCurveType());
	fprintf(fp, "\t\t\tbegin\t%g\n", getBegin());
	fprintf(fp, "\t\t\tend\t%g\n", getEnd());
	fprintf(fp, "\t\t\tscale_x\t%g\n", getScaleX() );
	fprintf(fp, "\t\t\tscale_y\t%g\n", getScaleY() );
	fprintf(fp, "\t\t\tshear_x\t%g\n", getShearX() );
	fprintf(fp, "\t\t\tshear_y\t%g\n", getShearY() );
	fprintf(fp,"\t\t\ttwist\t%g\n", getTwist());
	
	fprintf(fp,"\t\t\ttwist_begin\t%g\n", getTwistBegin());
	fprintf(fp,"\t\t\tradius_offset\t%g\n", getRadiusOffset());
	fprintf(fp,"\t\t\ttaper_x\t%g\n", getTaperX());
	fprintf(fp,"\t\t\ttaper_y\t%g\n", getTaperY());
	fprintf(fp,"\t\t\trevolutions\t%g\n", getRevolutions());
	fprintf(fp,"\t\t\tskew\t%g\n", getSkew());

	fprintf(fp, "\t\t}\n");
	return TRUE;
}


BOOL LLPathParams::importLegacyStream(std::istream& input_stream)
{
	const S32 BUFSIZE = 16384;
	char buffer[BUFSIZE];	/* Flawfinder: ignore */
	// *NOTE: changing the size or type of these buffers will require
	// changing the sscanf below.
	char keyword[256];	/* Flawfinder: ignore */
	char valuestr[256];	/* Flawfinder: ignore */
	keyword[0] = 0;
	valuestr[0] = 0;

	F32 tempF32;
	F32 x, y;
	U32 tempU32;

	while (input_stream.good())
	{
		input_stream.getline(buffer, BUFSIZE);
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %255s %255s",
			keyword, valuestr);
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}",keyword))
		{
			break;
		}
		else if (!strcmp("curve", keyword))
		{
			sscanf(valuestr,"%d",&tempU32);
			setCurveType((U8) tempU32);
		}
		else if (!strcmp("begin",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setBegin(tempF32);
		}
		else if (!strcmp("end",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setEnd(tempF32);
		}
		else if (!strcmp("scale",keyword))
		{
			// Legacy for one dimensional scale per path
			sscanf(valuestr,"%g",&tempF32);
			setScale(tempF32, tempF32);
		}
		else if (!strcmp("scale_x", keyword))
		{
			sscanf(valuestr, "%g", &x);
			setScaleX(x);
		}
		else if (!strcmp("scale_y", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setScaleY(y);
		}
		else if (!strcmp("shear_x", keyword))
		{
			sscanf(valuestr, "%g", &x);
			setShearX(x);
		}
		else if (!strcmp("shear_y", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setShearY(y);
		}
		else if (!strcmp("twist",keyword))
		{
			sscanf(valuestr,"%g",&tempF32);
			setTwist(tempF32);
		}
		else if (!strcmp("twist_begin", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setTwistBegin(y);
		}
		else if (!strcmp("radius_offset", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setRadiusOffset(y);
		}
		else if (!strcmp("taper_x", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setTaperX(y);
		}
		else if (!strcmp("taper_y", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setTaperY(y);
		}
		else if (!strcmp("revolutions", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setRevolutions(y);
		}
		else if (!strcmp("skew", keyword))
		{
			sscanf(valuestr, "%g", &y);
			setSkew(y);
		}
		else
		{
			LL_WARNS() << "unknown keyword " << " in path import" << LL_ENDL;
		}
	}
	return TRUE;
}


BOOL LLPathParams::exportLegacyStream(std::ostream& output_stream) const
{
	output_stream << "\t\tpath 0\n";
	output_stream << "\t\t{\n";
	output_stream << "\t\t\tcurve\t" << (S32) getCurveType() << "\n";
	output_stream << "\t\t\tbegin\t" << getBegin() << "\n";
	output_stream << "\t\t\tend\t" << getEnd() << "\n";
	output_stream << "\t\t\tscale_x\t" << getScaleX()  << "\n";
	output_stream << "\t\t\tscale_y\t" << getScaleY()  << "\n";
	output_stream << "\t\t\tshear_x\t" << getShearX()  << "\n";
	output_stream << "\t\t\tshear_y\t" << getShearY()  << "\n";
	output_stream <<"\t\t\ttwist\t" << getTwist() << "\n";
	
	output_stream <<"\t\t\ttwist_begin\t" << getTwistBegin() << "\n";
	output_stream <<"\t\t\tradius_offset\t" << getRadiusOffset() << "\n";
	output_stream <<"\t\t\ttaper_x\t" << getTaperX() << "\n";
	output_stream <<"\t\t\ttaper_y\t" << getTaperY() << "\n";
	output_stream <<"\t\t\trevolutions\t" << getRevolutions() << "\n";
	output_stream <<"\t\t\tskew\t" << getSkew() << "\n";

	output_stream << "\t\t}\n";
	return TRUE;
}

LLSD LLPathParams::asLLSD() const
{
	LLSD sd = LLSD();
	sd["curve"] = getCurveType();
	sd["begin"] = getBegin();
	sd["end"] = getEnd();
	sd["scale_x"] = getScaleX();
	sd["scale_y"] = getScaleY();
	sd["shear_x"] = getShearX();
	sd["shear_y"] = getShearY();
	sd["twist"] = getTwist();
	sd["twist_begin"] = getTwistBegin();
	sd["radius_offset"] = getRadiusOffset();
	sd["taper_x"] = getTaperX();
	sd["taper_y"] = getTaperY();
	sd["revolutions"] = getRevolutions();
	sd["skew"] = getSkew();

	return sd;
}

bool LLPathParams::fromLLSD(LLSD& sd)
{
	setCurveType(sd["curve"].asInteger());
	setBegin((F32)sd["begin"].asReal());
	setEnd((F32)sd["end"].asReal());
	setScaleX((F32)sd["scale_x"].asReal());
	setScaleY((F32)sd["scale_y"].asReal());
	setShearX((F32)sd["shear_x"].asReal());
	setShearY((F32)sd["shear_y"].asReal());
	setTwist((F32)sd["twist"].asReal());
	setTwistBegin((F32)sd["twist_begin"].asReal());
	setRadiusOffset((F32)sd["radius_offset"].asReal());
	setTaperX((F32)sd["taper_x"].asReal());
	setTaperY((F32)sd["taper_y"].asReal());
	setRevolutions((F32)sd["revolutions"].asReal());
	setSkew((F32)sd["skew"].asReal());
	return true;
}

void LLPathParams::copyParams(const LLPathParams &params)
{
	setCurveType(params.getCurveType());
	setBegin(params.getBegin());
	setEnd(params.getEnd());
	setScale(params.getScaleX(), params.getScaleY() );
	setShear(params.getShearX(), params.getShearY() );
	setTwist(params.getTwist());
	setTwistBegin(params.getTwistBegin());
	setRadiusOffset(params.getRadiusOffset());
	setTaper( params.getTaperX(), params.getTaperY() );
	setRevolutions(params.getRevolutions());
	setSkew(params.getSkew());
}

LLProfile::~LLProfile()
{
}


S32 LLVolume::sNumMeshPoints = 0;

LLVolume::LLVolume(const LLVolumeParams &params, const F32 detail, const BOOL generate_single_face, const BOOL is_unique)
	: mParams(params)
{
	mUnique = is_unique;
	mFaceMask = 0x0;
	mDetail = detail;
	mSculptLevel = -2;
	mSurfaceArea = 1.f; //only calculated for sculpts, defaults to 1 for all other prims
	mIsMeshAssetLoaded = FALSE;
	mLODScaleBias.setVec(1,1,1);
	mHullPoints = NULL;
	mHullIndices = NULL;
	mNumHullPoints = 0;
	mNumHullIndices = 0;

	// set defaults
	if (mParams.getPathParams().getCurveType() == LL_PCODE_PATH_FLEXIBLE)
	{
		mPathp = new LLDynamicPath();
	}
	else
	{
		mPathp = new LLPath();
	}
	mProfilep = new LLProfile();

	mGenerateSingleFace = generate_single_face;

	generate();
	
	if ((mParams.getSculptID().isNull() && mParams.getSculptType() == LL_SCULPT_TYPE_NONE) || mParams.getSculptType() == LL_SCULPT_TYPE_MESH)
	{
		createVolumeFaces();
	}
}

void LLVolume::resizePath(S32 length)
{
	mPathp->resizePath(length);
	mVolumeFaces.clear();
	setDirty();
}

void LLVolume::regen()
{
	generate();
	createVolumeFaces();
}

void LLVolume::genTangents(S32 face)
{
	mVolumeFaces[face].createTangents();
}

LLVolume::~LLVolume()
{
	sNumMeshPoints -= mMesh.size();
	delete mPathp;

	delete mProfilep;

	mPathp = NULL;
	mProfilep = NULL;
	mVolumeFaces.clear();

	ll_aligned_free_16(mHullPoints);
	mHullPoints = NULL;
	ll_aligned_free_16(mHullIndices);
	mHullIndices = NULL;
}

BOOL LLVolume::generate()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	LL_CHECK_MEMORY
	llassert_always(mProfilep);
	
	//Added 10.03.05 Dave Parks
	// Split is a parameter to LLProfile::generate that tesselates edges on the profile 
	// to prevent lighting and texture interpolation errors on triangles that are 
	// stretched due to twisting or scaling on the path.  
	S32 split = (S32) ((mDetail)*0.66f);
	
	if (mParams.getPathParams().getCurveType() == LL_PCODE_PATH_LINE &&
		(mParams.getPathParams().getScale().mV[0] != 1.0f ||
		 mParams.getPathParams().getScale().mV[1] != 1.0f) &&
		(mParams.getProfileParams().getCurveType() == LL_PCODE_PROFILE_SQUARE ||
		 mParams.getProfileParams().getCurveType() == LL_PCODE_PROFILE_ISOTRI ||
		 mParams.getProfileParams().getCurveType() == LL_PCODE_PROFILE_EQUALTRI ||
		 mParams.getProfileParams().getCurveType() == LL_PCODE_PROFILE_RIGHTTRI))
	{
		split = 0;
	}
		 
	mLODScaleBias.setVec(0.5f, 0.5f, 0.5f);
	
	F32 profile_detail = mDetail;
	F32 path_detail = mDetail;

	if ((mParams.getSculptType() & LL_SCULPT_TYPE_MASK) != LL_SCULPT_TYPE_MESH)
	{
		U8 path_type = mParams.getPathParams().getCurveType();
		U8 profile_type = mParams.getProfileParams().getCurveType();
		if (path_type == LL_PCODE_PATH_LINE && profile_type == LL_PCODE_PROFILE_CIRCLE)
		{
			//cylinders don't care about Z-Axis
			mLODScaleBias.setVec(0.6f, 0.6f, 0.0f);
		}
		else if (path_type == LL_PCODE_PATH_CIRCLE)
		{
			mLODScaleBias.setVec(0.6f, 0.6f, 0.6f);
		}
	}

	BOOL regenPath = mPathp->generate(mParams.getPathParams(), path_detail, split);
	BOOL regenProf = mProfilep->generate(mParams.getProfileParams(), mPathp->isOpen(),profile_detail, split);

	if (regenPath || regenProf ) 
	{
		S32 sizeS = mPathp->mPath.size();
		S32 sizeT = mProfilep->mProfile.size();

		sNumMeshPoints -= mMesh.size();
		mMesh.resize(sizeT * sizeS);
		sNumMeshPoints += mMesh.size();		

		//generate vertex positions

		// Run along the path.
		LLVector4a* dst = mMesh.mArray;

		for (S32 s = 0; s < sizeS; ++s)
		{
			F32* scale = mPathp->mPath[s].mScale.getF32ptr();
			
			F32 sc [] = 
			{ scale[0], 0, 0, 0,
				0, scale[1], 0, 0,
				0, 0, scale[2], 0,
					0, 0, 0, 1 };
			
			LLMatrix4 rot((F32*) mPathp->mPath[s].mRot.mMatrix);
			LLMatrix4 scale_mat(sc);
			
			scale_mat *= rot;
			
			LLMatrix4a rot_mat;
			rot_mat.loadu(scale_mat);
			
			LLVector4a* profile = mProfilep->mProfile.mArray;
			LLVector4a* end_profile = profile+sizeT;
			LLVector4a offset = mPathp->mPath[s].mPos;

            // hack to work around MAINT-5660 for debug until we can suss out
            // what is wrong with the path generated that inserts NaNs...
            if (!offset.isFinite3())
            {
                offset.clear();
            }

			LLVector4a tmp;

			// Run along the profile.
			while (profile < end_profile)
			{
				rot_mat.rotate(*profile++, tmp);
				dst->setAdd(tmp,offset);
				++dst;
			}
		}

		for (std::vector<LLProfile::Face>::iterator iter = mProfilep->mFaces.begin();
			 iter != mProfilep->mFaces.end(); ++iter)
		{
			LLFaceID id = iter->mFaceID;
			mFaceMask |= id;
		}
		LL_CHECK_MEMORY
		return TRUE;
	}

	LL_CHECK_MEMORY
	return FALSE;
}

void LLVolumeFace::VertexData::init()
{
	if (!mData)
	{
		mData = (LLVector4a*) ll_aligned_malloc_16(sizeof(LLVector4a)*2);
	}
}

LLVolumeFace::VertexData::VertexData()
{
	mData = NULL;
	init();
}
	
LLVolumeFace::VertexData::VertexData(const VertexData& rhs)
{
	mData = NULL;
	*this = rhs;
}

const LLVolumeFace::VertexData& LLVolumeFace::VertexData::operator=(const LLVolumeFace::VertexData& rhs)
{
	if (this != &rhs)
	{
		init();
		LLVector4a::memcpyNonAliased16((F32*) mData, (F32*) rhs.mData, 2*sizeof(LLVector4a));
		mTexCoord = rhs.mTexCoord;
	}
	return *this;
}

LLVolumeFace::VertexData::~VertexData()
{
	ll_aligned_free_16(mData);
	mData = NULL;
}

LLVector4a& LLVolumeFace::VertexData::getPosition()
{
	return mData[POSITION];
}

LLVector4a& LLVolumeFace::VertexData::getNormal()
{
	return mData[NORMAL];
}

const LLVector4a& LLVolumeFace::VertexData::getPosition() const
{
	return mData[POSITION];
}

const LLVector4a& LLVolumeFace::VertexData::getNormal() const
{
	return mData[NORMAL];
}


void LLVolumeFace::VertexData::setPosition(const LLVector4a& pos)
{
	mData[POSITION] = pos;
}

void LLVolumeFace::VertexData::setNormal(const LLVector4a& norm)
{
	mData[NORMAL] = norm;
}

bool LLVolumeFace::VertexData::operator<(const LLVolumeFace::VertexData& rhs)const
{
	const F32* lp = this->getPosition().getF32ptr();
	const F32* rp = rhs.getPosition().getF32ptr();

	if (lp[0] != rp[0])
	{
		return lp[0] < rp[0];
	}

	if (rp[1] != lp[1])
	{
		return lp[1] < rp[1];
	}

	if (rp[2] != lp[2])
	{
		return lp[2] < rp[2];
	}

	lp = getNormal().getF32ptr();
	rp = rhs.getNormal().getF32ptr();

	if (lp[0] != rp[0])
	{
		return lp[0] < rp[0];
	}

	if (rp[1] != lp[1])
	{
		return lp[1] < rp[1];
	}

	if (rp[2] != lp[2])
	{
		return lp[2] < rp[2];
	}

	if (mTexCoord.mV[0] != rhs.mTexCoord.mV[0])
	{
		return mTexCoord.mV[0] < rhs.mTexCoord.mV[0];
	}

	return mTexCoord.mV[1] < rhs.mTexCoord.mV[1];
}

bool LLVolumeFace::VertexData::operator==(const LLVolumeFace::VertexData& rhs)const
{
	return mData[POSITION].equals3(rhs.getPosition()) &&
			mData[NORMAL].equals3(rhs.getNormal()) &&
			mTexCoord == rhs.mTexCoord;
}

bool LLVolumeFace::VertexData::compareNormal(const LLVolumeFace::VertexData& rhs, F32 angle_cutoff) const
{
	bool retval = false;

	const F32 epsilon = 0.00001f;

	if (rhs.mData[POSITION].equals3(mData[POSITION], epsilon) && 
		fabs(rhs.mTexCoord[0]-mTexCoord[0]) < epsilon &&
		fabs(rhs.mTexCoord[1]-mTexCoord[1]) < epsilon)
	{
		if (angle_cutoff > 1.f)
		{
			retval = (mData[NORMAL].equals3(rhs.mData[NORMAL], epsilon));
		}
		else
		{
			F32 cur_angle = rhs.mData[NORMAL].dot3(mData[NORMAL]).getF32();
			retval = cur_angle > angle_cutoff;
		}
	}

	return retval;
}

bool LLVolume::unpackVolumeFaces(std::istream& is, S32 size)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	//input stream is now pointing at a zlib compressed block of LLSD
	//decompress block
	LLSD mdl;
	U32 uzip_result = LLUZipHelper::unzip_llsd(mdl, is, size);
	if (uzip_result != LLUZipHelper::ZR_OK)
	{
		LL_DEBUGS("MeshStreaming") << "Failed to unzip LLSD blob for LoD with code " << uzip_result << " , will probably fetch from sim again." << LL_ENDL;
		return false;
	}
	return unpackVolumeFacesInternal(mdl);
}

bool LLVolume::unpackVolumeFaces(U8* in_data, S32 size)
{
	//input data is now pointing at a zlib compressed block of LLSD
	//decompress block
	LLSD mdl;
	U32 uzip_result = LLUZipHelper::unzip_llsd(mdl, in_data, size);
	if (uzip_result != LLUZipHelper::ZR_OK)
	{
		LL_DEBUGS("MeshStreaming") << "Failed to unzip LLSD blob for LoD with code " << uzip_result << " , will probably fetch from sim again." << LL_ENDL;
		return false;
	}
	return unpackVolumeFacesInternal(mdl);
}

bool LLVolume::unpackVolumeFacesInternal(const LLSD& mdl)
{
	{
		U32 face_count = mdl.size();

		if (face_count == 0)
		{ //no faces unpacked, treat as failed decode
			LL_WARNS() << "found no faces!" << LL_ENDL;
			return false;
		}

		mVolumeFaces.resize(face_count);

		for (size_t i = 0; i < face_count; ++i)
		{
			LLVolumeFace& face = mVolumeFaces[i];

			if (mdl[i].has("NoGeometry"))
			{ //face has no geometry, continue
				face.resizeIndices(3);
				face.resizeVertices(1);
				face.mPositions->clear();
				face.mNormals->clear();
				face.mTexCoords->setZero();
				memset(face.mIndices, 0, sizeof(U16)*3);
				continue;
			}

			LLSD::Binary pos = mdl[i]["Position"];
			LLSD::Binary norm = mdl[i]["Normal"];
			LLSD::Binary tc = mdl[i]["TexCoord0"];
			LLSD::Binary idx = mdl[i]["TriangleList"];

			

			//copy out indices
            S32 num_indices = idx.size() / 2;
            const S32 indices_to_discard = num_indices % 3;
            if (indices_to_discard > 0)
            {
                // Invalid number of triangle indices
                LL_WARNS() << "Incomplete triangle discarded from face! Indices count " << num_indices << " was not divisible by 3. face index: " << i << " Total: " << face_count << LL_ENDL;
                num_indices -= indices_to_discard;
            }
            face.resizeIndices(num_indices);

            if (num_indices > 2 && !face.mIndices)
            {
                LL_WARNS() << "Failed to allocate " << num_indices << " indices for face index: " << i << " Total: " << face_count << LL_ENDL;
                continue;
            }
			
			if (idx.empty() || face.mNumIndices < 3)
			{ //why is there an empty index list?
				LL_WARNS() << "Empty face present! Face index: " << i << " Total: " << face_count << LL_ENDL;
				continue;
			}

			U16* indices = (U16*) &(idx[0]);
            for (U32 j = 0; j < num_indices; ++j)
			{
				face.mIndices[j] = indices[j];
			}

			//copy out vertices
			U32 num_verts = pos.size()/(3*2);
			face.resizeVertices(num_verts);

            if (num_verts > 0 && !face.mPositions)
            {
                LL_WARNS() << "Failed to allocate " << num_verts << " vertices for face index: " << i << " Total: " << face_count << LL_ENDL;
                face.resizeIndices(0);
                continue;
            }

			LLVector3 minp;
			LLVector3 maxp;
			LLVector2 min_tc; 
			LLVector2 max_tc; 
		
			minp.setValue(mdl[i]["PositionDomain"]["Min"]);
			maxp.setValue(mdl[i]["PositionDomain"]["Max"]);
			LLVector4a min_pos, max_pos;
			min_pos.load3(minp.mV);
			max_pos.load3(maxp.mV);

			min_tc.setValue(mdl[i]["TexCoord0Domain"]["Min"]);
			max_tc.setValue(mdl[i]["TexCoord0Domain"]["Max"]);

			LLVector4a pos_range;
			pos_range.setSub(max_pos, min_pos);
			LLVector2 tc_range2 = max_tc - min_tc;

			LLVector4a tc_range;
			tc_range.set(tc_range2[0], tc_range2[1], tc_range2[0], tc_range2[1]);
			LLVector4a min_tc4(min_tc[0], min_tc[1], min_tc[0], min_tc[1]);

			LLVector4a* pos_out = face.mPositions;
			LLVector4a* norm_out = face.mNormals;
			LLVector4a* tc_out = (LLVector4a*) face.mTexCoords;

			{
				U16* v = (U16*) &(pos[0]);
				for (U32 j = 0; j < num_verts; ++j)
				{
					pos_out->set((F32) v[0], (F32) v[1], (F32) v[2]);
					pos_out->div(65535.f);
					pos_out->mul(pos_range);
					pos_out->add(min_pos);
					pos_out++;
					v += 3;
				}

			}

			{
				if (!norm.empty())
				{
					U16* n = (U16*) &(norm[0]);
					for (U32 j = 0; j < num_verts; ++j)
					{
						norm_out->set((F32) n[0], (F32) n[1], (F32) n[2]);
						norm_out->div(65535.f);
						norm_out->mul(2.f);
						norm_out->sub(1.f);
						norm_out++;
						n += 3;
					}
				}
				else
				{
					for (U32 j = 0; j < num_verts; ++j)
					{
						norm_out->clear();
						norm_out++; // or just norm_out[j].clear();
					}
				}
			}

			{
				if (!tc.empty())
				{
					U16* t = (U16*) &(tc[0]);
					for (U32 j = 0; j < num_verts; j+=2)
					{
						if (j < num_verts-1)
						{
							tc_out->set((F32) t[0], (F32) t[1], (F32) t[2], (F32) t[3]);
						}
						else
						{
							tc_out->set((F32) t[0], (F32) t[1], 0.f, 0.f);
						}

						t += 4;

						tc_out->div(65535.f);
						tc_out->mul(tc_range);
						tc_out->add(min_tc4);

						tc_out++;
					}
				}
				else
				{
					for (U32 j = 0; j < num_verts; j += 2)
					{
						tc_out->clear();
						tc_out++;
					}
				}
			}

			if (mdl[i].has("Weights"))
			{
				face.allocateWeights(num_verts);
                if (!face.mWeights && num_verts)
                {
                    LL_WARNS() << "Failed to allocate " << num_verts << " weights for face index: " << i << " Total: " << face_count << LL_ENDL;
                    face.resizeIndices(0);
                    face.resizeVertices(0);
                    continue;
                }

				LLSD::Binary weights = mdl[i]["Weights"];

				U32 idx = 0;

				U32 cur_vertex = 0;
				while (idx < weights.size() && cur_vertex < num_verts)
				{
					const U8 END_INFLUENCES = 0xFF;
					U8 joint = weights[idx++];

					U32 cur_influence = 0;
					LLVector4 wght(0,0,0,0);
                    U32 joints[4] = {0,0,0,0};
					LLVector4 joints_with_weights(0,0,0,0);

					while (joint != END_INFLUENCES && idx < weights.size())
					{
						U16 influence = weights[idx++];
						influence |= ((U16) weights[idx++] << 8);

						F32 w = llclamp((F32) influence / 65535.f, 0.001f, 0.999f);
						wght.mV[cur_influence] = w;
						joints[cur_influence] = joint;
						cur_influence++;

						if (cur_influence >= 4)
						{
							joint = END_INFLUENCES;
						}
						else
						{
							joint = weights[idx++];
						}
					}
                    F32 wsum = wght.mV[VX] + wght.mV[VY] + wght.mV[VZ] + wght.mV[VW];
                    if (wsum <= 0.f)
                    {
                        wght = LLVector4(0.999f,0.f,0.f,0.f);
                    }
                    for (U32 k=0; k<4; k++)
                    {
                        F32 f_combined = (F32) joints[k] + wght[k];
                        joints_with_weights[k] = f_combined;
                        // Any weights we added above should wind up non-zero and applied to a specific bone.
                        // A failure here would indicate a floating point precision error in the math.
                        llassert((k >= cur_influence) || (f_combined - S32(f_combined) > 0.0f));
                    }
					face.mWeights[cur_vertex].loadua(joints_with_weights.mV);

					cur_vertex++;
				}

				if (cur_vertex != num_verts || idx != weights.size())
				{
					LL_WARNS() << "Vertex weight count does not match vertex count!" << LL_ENDL;
				}
					
			}

			// modifier flags?
			bool do_mirror = (mParams.getSculptType() & LL_SCULPT_FLAG_MIRROR);
			bool do_invert = (mParams.getSculptType() &LL_SCULPT_FLAG_INVERT);
			
			
			// translate to actions:
			bool do_reflect_x = false;
			bool do_reverse_triangles = false;
			bool do_invert_normals = false;
			
			if (do_mirror)
			{
				do_reflect_x = true;
				do_reverse_triangles = !do_reverse_triangles;
			}
			
			if (do_invert)
			{
				do_invert_normals = true;
				do_reverse_triangles = !do_reverse_triangles;
			}
			
			// now do the work

			if (do_reflect_x)
			{
				LLVector4a* p = (LLVector4a*) face.mPositions;
				LLVector4a* n = (LLVector4a*) face.mNormals;
				
				for (S32 i = 0; i < face.mNumVertices; i++)
				{
					p[i].mul(-1.0f);
					n[i].mul(-1.0f);
				}
			}

			if (do_invert_normals)
			{
				LLVector4a* n = (LLVector4a*) face.mNormals;
				
				for (S32 i = 0; i < face.mNumVertices; i++)
				{
					n[i].mul(-1.0f);
				}
			}

			if (do_reverse_triangles)
			{
				for (U32 j = 0; j < face.mNumIndices; j += 3)
				{
					// swap the 2nd and 3rd index
					S32 swap = face.mIndices[j+1];
					face.mIndices[j+1] = face.mIndices[j+2];
					face.mIndices[j+2] = swap;
				}
			}

			//calculate bounding box
			// VFExtents change
			LLVector4a& min = face.mExtents[0];
			LLVector4a& max = face.mExtents[1];

			if (face.mNumVertices < 3)
			{ //empty face, use a dummy 1cm (at 1m scale) bounding box
				min.splat(-0.005f);
				max.splat(0.005f);
			}
			else
			{
				min = max = face.mPositions[0];

				for (S32 i = 1; i < face.mNumVertices; ++i)
				{
					min.setMin(min, face.mPositions[i]);
					max.setMax(max, face.mPositions[i]);
				}

				if (face.mTexCoords)
				{
					LLVector2& min_tc = face.mTexCoordExtents[0];
					LLVector2& max_tc = face.mTexCoordExtents[1];

					min_tc = face.mTexCoords[0];
					max_tc = face.mTexCoords[0];

					for (U32 j = 1; j < face.mNumVertices; ++j)
					{
						update_min_max(min_tc, max_tc, face.mTexCoords[j]);
					}
				}
				else
				{
					face.mTexCoordExtents[0].set(0,0);
					face.mTexCoordExtents[1].set(1,1);
				}
			}
		}
	}

	if (!cacheOptimize())
	{
		// Out of memory?
		LL_WARNS() << "Failed to optimize!" << LL_ENDL;
		mVolumeFaces.clear();
		return false;
	}
	
	mSculptLevel = 0;  // success!

	return true;
}


BOOL LLVolume::isMeshAssetLoaded()
{
	return mIsMeshAssetLoaded;
}

void LLVolume::setMeshAssetLoaded(BOOL loaded)
{
	mIsMeshAssetLoaded = loaded;
}

void LLVolume::copyFacesTo(std::vector<LLVolumeFace> &faces) const 
{
	faces = mVolumeFaces;
}

void LLVolume::copyFacesFrom(const std::vector<LLVolumeFace> &faces)
{
	mVolumeFaces = faces;
	mSculptLevel = 0;
}

void LLVolume::copyVolumeFaces(const LLVolume* volume)
{
	mVolumeFaces = volume->mVolumeFaces;
	mSculptLevel = 0;
}

bool LLVolume::cacheOptimize()
{
	for (S32 i = 0; i < mVolumeFaces.size(); ++i)
	{
		if (!mVolumeFaces[i].cacheOptimize())
		{
			return false;
		}
	}
	return true;
}


S32	LLVolume::getNumFaces() const
{
	return mIsMeshAssetLoaded ? getNumVolumeFaces() : (S32)mProfilep->mFaces.size();
}


void LLVolume::createVolumeFaces()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	if (mGenerateSingleFace)
	{
		// do nothing
	}
	else
	{
		S32 num_faces = getNumFaces();
		BOOL partial_build = TRUE;
		if (num_faces != mVolumeFaces.size())
		{
			partial_build = FALSE;
			mVolumeFaces.resize(num_faces);
		}
		// Initialize volume faces with parameter data
		for (S32 i = 0; i < (S32)mVolumeFaces.size(); i++)
		{
			LLVolumeFace& vf = mVolumeFaces[i];
			LLProfile::Face& face = mProfilep->mFaces[i];
			vf.mBeginS = face.mIndex;
			vf.mNumS = face.mCount;
			if (vf.mNumS < 0)
			{
				LL_ERRS() << "Volume face corruption detected." << LL_ENDL;
			}

			vf.mBeginT = 0;
			vf.mNumT= getPath().mPath.size();
			vf.mID = i;

			// Set the type mask bits correctly
			if (mParams.getProfileParams().getHollow() > 0)
			{
				vf.mTypeMask |= LLVolumeFace::HOLLOW_MASK;
			}
			if (mProfilep->isOpen())
			{
				vf.mTypeMask |= LLVolumeFace::OPEN_MASK;
			}
			if (face.mCap)
			{
				vf.mTypeMask |= LLVolumeFace::CAP_MASK;
				if (face.mFaceID == LL_FACE_PATH_BEGIN)
				{
					vf.mTypeMask |= LLVolumeFace::TOP_MASK;
				}
				else
				{
					llassert(face.mFaceID == LL_FACE_PATH_END);
					vf.mTypeMask |= LLVolumeFace::BOTTOM_MASK;
				}
			}
			else if (face.mFaceID & (LL_FACE_PROFILE_BEGIN | LL_FACE_PROFILE_END))
			{
				vf.mTypeMask |= LLVolumeFace::FLAT_MASK | LLVolumeFace::END_MASK;
			}
			else
			{
				vf.mTypeMask |= LLVolumeFace::SIDE_MASK;
				if (face.mFlat)
				{
					vf.mTypeMask |= LLVolumeFace::FLAT_MASK;
				}
				if (face.mFaceID & LL_FACE_INNER_SIDE)
				{
					vf.mTypeMask |= LLVolumeFace::INNER_MASK;
					if (face.mFlat && vf.mNumS > 2)
					{ //flat inner faces have to copy vert normals
						vf.mNumS = vf.mNumS*2;
						if (vf.mNumS < 0)
						{
							LL_ERRS() << "Volume face corruption detected." << LL_ENDL;
						}
					}
				}
				else
				{
					vf.mTypeMask |= LLVolumeFace::OUTER_MASK;
				}
			}
		}

		for (face_list_t::iterator iter = mVolumeFaces.begin();
			 iter != mVolumeFaces.end(); ++iter)
		{
			(*iter).create(this, partial_build);
		}
	}
}


inline LLVector4a sculpt_rgb_to_vector(U8 r, U8 g, U8 b)
{
	// maps RGB values to vector values [0..255] -> [-0.5..0.5]
	LLVector4a value;
	LLVector4a sub(0.5f, 0.5f, 0.5f);

	value.set(r,g,b);
	value.mul(1.f/255.f);
	value.sub(sub);

	return value;
}

inline U32 sculpt_xy_to_index(U32 x, U32 y, U16 sculpt_width, U16 sculpt_height, S8 sculpt_components)
{
	U32 index = (x + y * sculpt_width) * sculpt_components;
	return index;
}


inline U32 sculpt_st_to_index(S32 s, S32 t, S32 size_s, S32 size_t, U16 sculpt_width, U16 sculpt_height, S8 sculpt_components)
{
	U32 x = (U32) ((F32)s/(size_s) * (F32) sculpt_width);
	U32 y = (U32) ((F32)t/(size_t) * (F32) sculpt_height);

	return sculpt_xy_to_index(x, y, sculpt_width, sculpt_height, sculpt_components);
}


inline LLVector4a sculpt_index_to_vector(U32 index, const U8* sculpt_data)
{
	LLVector4a v = sculpt_rgb_to_vector(sculpt_data[index], sculpt_data[index+1], sculpt_data[index+2]);

	return v;
}

inline LLVector4a sculpt_st_to_vector(S32 s, S32 t, S32 size_s, S32 size_t, U16 sculpt_width, U16 sculpt_height, S8 sculpt_components, const U8* sculpt_data)
{
	U32 index = sculpt_st_to_index(s, t, size_s, size_t, sculpt_width, sculpt_height, sculpt_components);

	return sculpt_index_to_vector(index, sculpt_data);
}

inline LLVector4a sculpt_xy_to_vector(U32 x, U32 y, U16 sculpt_width, U16 sculpt_height, S8 sculpt_components, const U8* sculpt_data)
{
	U32 index = sculpt_xy_to_index(x, y, sculpt_width, sculpt_height, sculpt_components);

	return sculpt_index_to_vector(index, sculpt_data);
}


F32 LLVolume::sculptGetSurfaceArea()
{
	// test to see if image has enough variation to create non-degenerate geometry

	F32 area = 0;

	S32 sizeS = mPathp->mPath.size();
	S32 sizeT = mProfilep->mProfile.size();
			
	for (S32 s = 0; s < sizeS-1; s++)
	{
		for (S32 t = 0; t < sizeT-1; t++)
		{
			// get four corners of quad
			LLVector4a& p1 = mMesh[(s  )*sizeT + (t  )];
			LLVector4a& p2 = mMesh[(s+1)*sizeT + (t  )];
			LLVector4a& p3 = mMesh[(s  )*sizeT + (t+1)];
			LLVector4a& p4 = mMesh[(s+1)*sizeT + (t+1)];

			// compute the area of the quad by taking the length of the cross product of the two triangles
			LLVector4a v0,v1,v2,v3;
			v0.setSub(p1,p2);
			v1.setSub(p1,p3);
			v2.setSub(p4,p2);
			v3.setSub(p4,p3);

			LLVector4a cross1, cross2;
			cross1.setCross3(v0,v1);
			cross2.setCross3(v2,v3);

			//LLVector3 cross1 = (p1 - p2) % (p1 - p3);
			//LLVector3 cross2 = (p4 - p2) % (p4 - p3);
			
			area += (cross1.getLength3() + cross2.getLength3()).getF32() / 2.f;
		}
	}

	return area;
}

// create empty placeholder shape
void LLVolume::sculptGenerateEmptyPlaceholder()
{
	S32 sizeS = mPathp->mPath.size();
	S32 sizeT = mProfilep->mProfile.size();

	S32 line = 0;

	for (S32 s = 0; s < sizeS; s++)
	{
		for (S32 t = 0; t < sizeT; t++)
		{
			S32 i = t + line;
			LLVector4a& pt = mMesh[i];
					
			F32* p = pt.getF32ptr();

			p[0] = 0;
			p[1] = 0;
			p[2] = 0;

			llassert(pt.isFinite3());
		}
		line += sizeT;
	}
}

// create sphere placeholder shape
void LLVolume::sculptGenerateSpherePlaceholder()
{
	S32 sizeS = mPathp->mPath.size();
	S32 sizeT = mProfilep->mProfile.size();

	S32 line = 0;

	for (S32 s = 0; s < sizeS; s++)
	{
		for (S32 t = 0; t < sizeT; t++)
		{
			S32 i = t + line;
			LLVector4a& pt = mMesh[i];


			F32 u = (F32)s / (sizeS - 1);
			F32 v = (F32)t / (sizeT - 1);

			const F32 RADIUS = (F32) 0.3;

			F32* p = pt.getF32ptr();

			p[0] = (F32)(sin(F_PI * v) * cos(2.0 * F_PI * u) * RADIUS);
			p[1] = (F32)(sin(F_PI * v) * sin(2.0 * F_PI * u) * RADIUS);
			p[2] = (F32)(cos(F_PI * v) * RADIUS);

			llassert(pt.isFinite3());
		}
		line += sizeT;
	}
}

// create the vertices from the map
void LLVolume::sculptGenerateMapVertices(U16 sculpt_width, U16 sculpt_height, S8 sculpt_components, const U8* sculpt_data, U8 sculpt_type)
{
	U8 sculpt_stitching = sculpt_type & LL_SCULPT_TYPE_MASK;
	BOOL sculpt_invert = sculpt_type & LL_SCULPT_FLAG_INVERT;
	BOOL sculpt_mirror = sculpt_type & LL_SCULPT_FLAG_MIRROR;
	BOOL reverse_horizontal = (sculpt_invert ? !sculpt_mirror : sculpt_mirror);  // XOR
	
	S32 sizeS = mPathp->mPath.size();
	S32 sizeT = mProfilep->mProfile.size();
	
	S32 line = 0;
	for (S32 s = 0; s < sizeS; s++)
	{
		// Run along the profile.
		for (S32 t = 0; t < sizeT; t++)
		{
			S32 i = t + line;
			LLVector4a& pt = mMesh[i];

			S32 reversed_t = t;

			if (reverse_horizontal)
			{
				reversed_t = sizeT - t - 1;
			}
			
			U32 x = (U32) ((F32)reversed_t/(sizeT-1) * (F32) sculpt_width);
			U32 y = (U32) ((F32)s/(sizeS-1) * (F32) sculpt_height);

			
			if (y == 0)  // top row stitching
			{
				// pinch?
				if (sculpt_stitching == LL_SCULPT_TYPE_SPHERE)
				{
					x = sculpt_width / 2;
				}
			}

			if (y == sculpt_height)  // bottom row stitching
			{
				// wrap?
				if (sculpt_stitching == LL_SCULPT_TYPE_TORUS)
				{
					y = 0;
				}
				else
				{
					y = sculpt_height - 1;
				}

				// pinch?
				if (sculpt_stitching == LL_SCULPT_TYPE_SPHERE)
				{
					x = sculpt_width / 2;
				}
			}

			if (x == sculpt_width)   // side stitching
			{
				// wrap?
				if ((sculpt_stitching == LL_SCULPT_TYPE_SPHERE) ||
					(sculpt_stitching == LL_SCULPT_TYPE_TORUS) ||
					(sculpt_stitching == LL_SCULPT_TYPE_CYLINDER))
				{
					x = 0;
				}
					
				else
				{
					x = sculpt_width - 1;
				}
			}

			pt = sculpt_xy_to_vector(x, y, sculpt_width, sculpt_height, sculpt_components, sculpt_data);

			if (sculpt_mirror)
			{
				LLVector4a scale(-1.f,1,1,1);
				pt.mul(scale);
			}

			llassert(pt.isFinite3());
		}
		
		line += sizeT;
	}
}


const S32 SCULPT_REZ_1 = 6;  // changed from 4 to 6 - 6 looks round whereas 4 looks square
const S32 SCULPT_REZ_2 = 8;
const S32 SCULPT_REZ_3 = 16;
const S32 SCULPT_REZ_4 = 32;

S32 sculpt_sides(F32 detail)
{

	// detail is usually one of: 1, 1.5, 2.5, 4.0.
	
	if (detail <= 1.0)
	{
		return SCULPT_REZ_1;
	}
	if (detail <= 2.0)
	{
		return SCULPT_REZ_2;
	}
	if (detail <= 3.0)
	{
		return SCULPT_REZ_3;
	}
	else
	{
		return SCULPT_REZ_4;
	}
}



// determine the number of vertices in both s and t direction for this sculpt
void sculpt_calc_mesh_resolution(U16 width, U16 height, U8 type, F32 detail, S32& s, S32& t)
{
	// this code has the following properties:
	// 1) the aspect ratio of the mesh is as close as possible to the ratio of the map
	//    while still using all available verts
	// 2) the mesh cannot have more verts than is allowed by LOD
	// 3) the mesh cannot have more verts than is allowed by the map
	
	S32 max_vertices_lod = (S32)pow((double)sculpt_sides(detail), 2.0);
	S32 max_vertices_map = width * height / 4;
	
	S32 vertices;
	if (max_vertices_map > 0)
		vertices = llmin(max_vertices_lod, max_vertices_map);
	else
		vertices = max_vertices_lod;
	

	F32 ratio;
	if ((width == 0) || (height == 0))
		ratio = 1.f;
	else
		ratio = (F32) width / (F32) height;

	
	s = (S32)(F32) sqrt(((F32)vertices / ratio));

	s = llmax(s, 4);              // no degenerate sizes, please
	t = vertices / s;

	t = llmax(t, 4);              // no degenerate sizes, please
	s = vertices / t;
}

// sculpt replaces generate() for sculpted surfaces
void LLVolume::sculpt(U16 sculpt_width, U16 sculpt_height, S8 sculpt_components, const U8* sculpt_data, S32 sculpt_level, bool visible_placeholder)
{
	U8 sculpt_type = mParams.getSculptType();

	BOOL data_is_empty = FALSE;

	if (sculpt_width == 0 || sculpt_height == 0 || sculpt_components < 3 || sculpt_data == NULL)
	{
		sculpt_level = -1;
		data_is_empty = TRUE;
	}

	S32 requested_sizeS = 0;
	S32 requested_sizeT = 0;

	sculpt_calc_mesh_resolution(sculpt_width, sculpt_height, sculpt_type, mDetail, requested_sizeS, requested_sizeT);

	mPathp->generate(mParams.getPathParams(), mDetail, 0, TRUE, requested_sizeS);
	mProfilep->generate(mParams.getProfileParams(), mPathp->isOpen(), mDetail, 0, TRUE, requested_sizeT);

	S32 sizeS = mPathp->mPath.size();         // we requested a specific size, now see what we really got
	S32 sizeT = mProfilep->mProfile.size();   // we requested a specific size, now see what we really got

	// weird crash bug - DEV-11158 - trying to collect more data:
	if ((sizeS == 0) || (sizeT == 0))
	{
		LL_WARNS() << "sculpt bad mesh size " << sizeS << " " << sizeT << LL_ENDL;
	}
	
	sNumMeshPoints -= mMesh.size();
	mMesh.resize(sizeS * sizeT);
	sNumMeshPoints += mMesh.size();

	//generate vertex positions
	if (!data_is_empty)
	{
		sculptGenerateMapVertices(sculpt_width, sculpt_height, sculpt_components, sculpt_data, sculpt_type);

		// don't test lowest LOD to support legacy content DEV-33670
		if (mDetail > SCULPT_MIN_AREA_DETAIL)
		{
			F32 area = sculptGetSurfaceArea();

			mSurfaceArea = area;

			const F32 SCULPT_MAX_AREA = 384.f;

			if (area < SCULPT_MIN_AREA || area > SCULPT_MAX_AREA)
			{
				data_is_empty = TRUE;
				visible_placeholder = true;
			}
		}
	}

	if (data_is_empty)
	{
		if (visible_placeholder)
		{
			// Object should be visible since there will be nothing else to display
			sculptGenerateSpherePlaceholder();
		}
		else
		{
			sculptGenerateEmptyPlaceholder();
		}
	}


	
	for (S32 i = 0; i < (S32)mProfilep->mFaces.size(); i++)
	{
		mFaceMask |= mProfilep->mFaces[i].mFaceID;
	}

	mSculptLevel = sculpt_level;

	// Delete any existing faces so that they get regenerated
	mVolumeFaces.clear();
	
	createVolumeFaces();
}




BOOL LLVolume::isCap(S32 face)
{
	return mProfilep->mFaces[face].mCap; 
}

BOOL LLVolume::isFlat(S32 face)
{
	return mProfilep->mFaces[face].mFlat;
}


bool LLVolumeParams::isSculpt() const
{
	return mSculptID.notNull();
}

bool LLVolumeParams::isMeshSculpt() const
{
	return isSculpt() && ((mSculptType & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH);
}

bool LLVolumeParams::operator==(const LLVolumeParams &params) const
{
	return ( (getPathParams() == params.getPathParams()) &&
			 (getProfileParams() == params.getProfileParams()) &&
			 (mSculptID == params.mSculptID) &&
			 (mSculptType == params.mSculptType) );
}

bool LLVolumeParams::operator!=(const LLVolumeParams &params) const
{
	return ( (getPathParams() != params.getPathParams()) ||
			 (getProfileParams() != params.getProfileParams()) ||
			 (mSculptID != params.mSculptID) ||
			 (mSculptType != params.mSculptType) );
}

bool LLVolumeParams::operator<(const LLVolumeParams &params) const
{
	if( getPathParams() != params.getPathParams() )
	{
		return getPathParams() < params.getPathParams();
	}
	
	if (getProfileParams() != params.getProfileParams())
	{
		return getProfileParams() < params.getProfileParams();
	}
	
	if (mSculptID != params.mSculptID)
	{
		return mSculptID < params.mSculptID;
	}

	return mSculptType < params.mSculptType;


}

void LLVolumeParams::copyParams(const LLVolumeParams &params)
{
	mProfileParams.copyParams(params.mProfileParams);
	mPathParams.copyParams(params.mPathParams);
	mSculptID = params.getSculptID();
	mSculptType = params.getSculptType();
}

// Less restricitve approx 0 for volumes
const F32 APPROXIMATELY_ZERO = 0.001f;
bool approx_zero( F32 f, F32 tolerance = APPROXIMATELY_ZERO)
{
	return (f >= -tolerance) && (f <= tolerance);
}

// return true if in range (or nearly so)
static bool limit_range(F32& v, F32 min, F32 max, F32 tolerance = APPROXIMATELY_ZERO)
{
	F32 min_delta = v - min;
	if (min_delta < 0.f)
	{
		v = min;
		if (!approx_zero(min_delta, tolerance))
			return false;
	}
	F32 max_delta = max - v;
	if (max_delta < 0.f)
	{
		v = max;
		if (!approx_zero(max_delta, tolerance))
			return false;
	}
	return true;
}

bool LLVolumeParams::setBeginAndEndS(const F32 b, const F32 e)
{
	bool valid = true;

	// First, clamp to valid ranges.
	F32 begin = b;
	valid &= limit_range(begin, 0.f, 1.f - MIN_CUT_DELTA);

	F32 end = e;
	if (end >= .0149f && end < MIN_CUT_DELTA) end = MIN_CUT_DELTA; // eliminate warning for common rounding error
	valid &= limit_range(end, MIN_CUT_DELTA, 1.f);

	valid &= limit_range(begin, 0.f, end - MIN_CUT_DELTA, .01f);

	// Now set them.
	mProfileParams.setBegin(begin);
	mProfileParams.setEnd(end);

	return valid;
}

bool LLVolumeParams::setBeginAndEndT(const F32 b, const F32 e)
{
	bool valid = true;

	// First, clamp to valid ranges.
	F32 begin = b;
	valid &= limit_range(begin, 0.f, 1.f - MIN_CUT_DELTA);

	F32 end = e;
	valid &= limit_range(end, MIN_CUT_DELTA, 1.f);

	valid &= limit_range(begin, 0.f, end - MIN_CUT_DELTA, .01f);

	// Now set them.
	mPathParams.setBegin(begin);
	mPathParams.setEnd(end);

	return valid;
}			

bool LLVolumeParams::setHollow(const F32 h)
{
	// Validate the hollow based on path and profile.
	U8 profile 	= mProfileParams.getCurveType() & LL_PCODE_PROFILE_MASK;
	U8 hole_type 	= mProfileParams.getCurveType() & LL_PCODE_HOLE_MASK;
	
	F32 max_hollow = HOLLOW_MAX;

	// Only square holes have trouble.
	if (LL_PCODE_HOLE_SQUARE == hole_type)
	{
		switch(profile)
		{
		case LL_PCODE_PROFILE_CIRCLE:
		case LL_PCODE_PROFILE_CIRCLE_HALF:
		case LL_PCODE_PROFILE_EQUALTRI:
			max_hollow = HOLLOW_MAX_SQUARE;
		}
	}

	F32 hollow = h;
	bool valid = limit_range(hollow, HOLLOW_MIN, max_hollow);
	mProfileParams.setHollow(hollow); 

	return valid;
}	

bool LLVolumeParams::setTwistBegin(const F32 b)
{
	F32 twist_begin = b;
	bool valid = limit_range(twist_begin, TWIST_MIN, TWIST_MAX);
	mPathParams.setTwistBegin(twist_begin);
	return valid;
}

bool LLVolumeParams::setTwistEnd(const F32 e)
{	
	F32 twist_end = e;
	bool valid = limit_range(twist_end, TWIST_MIN, TWIST_MAX);
	mPathParams.setTwistEnd(twist_end);
	return valid;
}

bool LLVolumeParams::setRatio(const F32 x, const F32 y)
{
	F32 min_x = RATIO_MIN;
	F32 max_x = RATIO_MAX;
	F32 min_y = RATIO_MIN;
	F32 max_y = RATIO_MAX;
	// If this is a circular path (and not a sphere) then 'ratio' is actually hole size.
	U8 path_type 	= mPathParams.getCurveType();
	U8 profile_type = mProfileParams.getCurveType() & LL_PCODE_PROFILE_MASK;
	if ( LL_PCODE_PATH_CIRCLE == path_type &&
		 LL_PCODE_PROFILE_CIRCLE_HALF != profile_type)
	{
		// Holes are more restricted...
		min_x = HOLE_X_MIN;
		max_x = HOLE_X_MAX;
		min_y = HOLE_Y_MIN;
		max_y = HOLE_Y_MAX;
	}

	F32 ratio_x = x;
	bool valid = limit_range(ratio_x, min_x, max_x);
	F32 ratio_y = y;
	valid &= limit_range(ratio_y, min_y, max_y);

	mPathParams.setScale(ratio_x, ratio_y);

	return valid;
}

bool LLVolumeParams::setShear(const F32 x, const F32 y)
{
	F32 shear_x = x;
	bool valid = limit_range(shear_x, SHEAR_MIN, SHEAR_MAX);
	F32 shear_y = y;
	valid &= limit_range(shear_y, SHEAR_MIN, SHEAR_MAX);
	mPathParams.setShear(shear_x, shear_y);
	return valid;
}

bool LLVolumeParams::setTaperX(const F32 v)
{
	F32 taper = v;
	bool valid = limit_range(taper, TAPER_MIN, TAPER_MAX);
	mPathParams.setTaperX(taper);
	return valid;
}

bool LLVolumeParams::setTaperY(const F32 v)
{
	F32 taper = v;
	bool valid = limit_range(taper, TAPER_MIN, TAPER_MAX);
	mPathParams.setTaperY(taper);
	return valid;
}

bool LLVolumeParams::setRevolutions(const F32 r)
{
	F32 revolutions = r;
	bool valid = limit_range(revolutions, REV_MIN, REV_MAX);
	mPathParams.setRevolutions(revolutions);
	return valid;
}

bool LLVolumeParams::setRadiusOffset(const F32 offset)
{
	bool valid = true;

	// If this is a sphere, just set it to 0 and get out.
	U8 path_type 	= mPathParams.getCurveType();
	U8 profile_type = mProfileParams.getCurveType() & LL_PCODE_PROFILE_MASK;
	if ( LL_PCODE_PROFILE_CIRCLE_HALF == profile_type ||
		LL_PCODE_PATH_CIRCLE != path_type )
	{
		mPathParams.setRadiusOffset(0.f);
		return true;
	}

	// Limit radius offset, based on taper and hole size y.
	F32 radius_offset	= offset;
	F32 taper_y    		= getTaperY();
	F32 radius_mag		= fabs(radius_offset);
	F32 hole_y_mag 		= fabs(getRatioY());
	F32 taper_y_mag		= fabs(taper_y);
	// Check to see if the taper effects us.
	if ( (radius_offset > 0.f && taper_y < 0.f) ||
			(radius_offset < 0.f && taper_y > 0.f) )
	{
		// The taper does not help increase the radius offset range.
		taper_y_mag = 0.f;
	}
	F32 max_radius_mag = 1.f - hole_y_mag * (1.f - taper_y_mag) / (1.f - hole_y_mag);

	// Enforce the maximum magnitude.
	F32 delta = max_radius_mag - radius_mag;
	if (delta < 0.f)
	{
		// Check radius offset sign.
		if (radius_offset < 0.f)
		{
			radius_offset = -max_radius_mag;
		}
		else
		{
			radius_offset = max_radius_mag;
		}
		valid = approx_zero(delta, .1f);
	}

	mPathParams.setRadiusOffset(radius_offset);
	return valid;
}

bool LLVolumeParams::setSkew(const F32 skew_value)
{
	bool valid = true;

	// Check the skew value against the revolutions.
	F32 skew		= llclamp(skew_value, SKEW_MIN, SKEW_MAX);
	F32 skew_mag	= fabs(skew);
	F32 revolutions = getRevolutions();
	F32 scale_x		= getRatioX();
	F32 min_skew_mag = 1.0f - 1.0f / (revolutions * scale_x + 1.0f);
	// Discontinuity; A revolution of 1 allows skews below 0.5.
	if ( fabs(revolutions - 1.0f) < 0.001)
		min_skew_mag = 0.0f;

	// Clip skew.
	F32 delta = skew_mag - min_skew_mag;
	if (delta < 0.f)
	{
		// Check skew sign.
		if (skew < 0.0f)
		{
			skew = -min_skew_mag;
		}
		else 
		{
			skew = min_skew_mag;
		}
		valid = approx_zero(delta, .01f);
	}

	mPathParams.setSkew(skew);
	return valid;
}

bool LLVolumeParams::setSculptID(const LLUUID sculpt_id, U8 sculpt_type)
{
	mSculptID = sculpt_id;
	mSculptType = sculpt_type;
	return true;
}

bool LLVolumeParams::setType(U8 profile, U8 path)
{
	bool result = true;
	// First, check profile and path for validity.
	U8 profile_type	= profile & LL_PCODE_PROFILE_MASK;
	U8 hole_type 	= (profile & LL_PCODE_HOLE_MASK) >> 4;
	U8 path_type	= path >> 4;

	if (profile_type > LL_PCODE_PROFILE_MAX)
	{
		// Bad profile.  Make it square.
		profile = LL_PCODE_PROFILE_SQUARE;
		result = false;
		LL_WARNS() << "LLVolumeParams::setType changing bad profile type (" << profile_type
			 	<< ") to be LL_PCODE_PROFILE_SQUARE" << LL_ENDL;
	}
	else if (hole_type > LL_PCODE_HOLE_MAX)
	{
		// Bad hole.  Make it the same.
		profile = profile_type;
		result = false;
		LL_WARNS() << "LLVolumeParams::setType changing bad hole type (" << hole_type
			 	<< ") to be LL_PCODE_HOLE_SAME" << LL_ENDL;
	}

	if (path_type < LL_PCODE_PATH_MIN ||
		path_type > LL_PCODE_PATH_MAX)
	{
		// Bad path.  Make it linear.
		result = false;
		LL_WARNS() << "LLVolumeParams::setType changing bad path (" << path
			 	<< ") to be LL_PCODE_PATH_LINE" << LL_ENDL;
		path = LL_PCODE_PATH_LINE;
	}

	mProfileParams.setCurveType(profile);
	mPathParams.setCurveType(path);
	return result;
}

// static 
bool LLVolumeParams::validate(U8 prof_curve, F32 prof_begin, F32 prof_end, F32 hollow,
		U8 path_curve, F32 path_begin, F32 path_end,
		F32 scx, F32 scy, F32 shx, F32 shy,
		F32 twistend, F32 twistbegin, F32 radiusoffset,
		F32 tx, F32 ty, F32 revolutions, F32 skew)
{
	LLVolumeParams test_params;
	if (!test_params.setType		(prof_curve, path_curve))
	{
	    	return false;
	}
	if (!test_params.setBeginAndEndS	(prof_begin, prof_end))
	{
	    	return false;
	}
	if (!test_params.setBeginAndEndT	(path_begin, path_end))
	{
	    	return false;
	}
	if (!test_params.setHollow		(hollow))
	{
	    	return false;
	}
	if (!test_params.setTwistBegin		(twistbegin))
	{
	    	return false;
	}
	if (!test_params.setTwistEnd		(twistend))
	{
	    	return false;
	}
	if (!test_params.setRatio		(scx, scy))
	{
	    	return false;
	}
	if (!test_params.setShear		(shx, shy))
	{
	    	return false;
	}
	if (!test_params.setTaper		(tx, ty))
	{
	    	return false;
	}
	if (!test_params.setRevolutions		(revolutions))
	{
	    	return false;
	}
	if (!test_params.setRadiusOffset	(radiusoffset))
	{
	    	return false;
	}
	if (!test_params.setSkew		(skew))
	{
	    	return false;
	}
	return true;
}

void LLVolume::getLoDTriangleCounts(const LLVolumeParams& params, S32* counts)
{ //attempt to approximate the number of triangles that will result from generating a volume LoD set for the 
	//supplied LLVolumeParams -- inaccurate, but a close enough approximation for determining streaming cost
	F32 detail[] = {1.f, 1.5f, 2.5f, 4.f};	
	for (S32 i = 0; i < 4; i++)
	{
		S32 count = 0;
		S32 path_points = LLPath::getNumPoints(params.getPathParams(), detail[i]);
		S32 profile_points = LLProfile::getNumPoints(params.getProfileParams(), false, detail[i]);

		count = (profile_points-1)*2*(path_points-1);
		count += profile_points*2;

		counts[i] = count;
	}
}


S32 LLVolume::getNumTriangles(S32* vcount) const
{
	U32 triangle_count = 0;
	U32 vertex_count = 0;

	for (S32 i = 0; i < getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& face = getVolumeFace(i);
		triangle_count += face.mNumIndices/3;

		vertex_count += face.mNumVertices;
	}


	if (vcount)
	{
		*vcount = vertex_count;
	}
	
	return triangle_count;
}


//-----------------------------------------------------------------------------
// generateSilhouetteVertices()
//-----------------------------------------------------------------------------
void LLVolume::generateSilhouetteVertices(std::vector<LLVector3> &vertices,
										  std::vector<LLVector3> &normals,
										  const LLVector3& obj_cam_vec_in,
										  const LLMatrix4& mat_in,
										  const LLMatrix3& norm_mat_in,
										  S32 face_mask)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	LLMatrix4a mat;
	mat.loadu(mat_in);

	LLMatrix4a norm_mat;
	norm_mat.loadu(norm_mat_in);
		
	LLVector4a obj_cam_vec;
	obj_cam_vec.load3(obj_cam_vec_in.mV);

	vertices.clear();
	normals.clear();

	if ((mParams.getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
	{
		return;
	}
	
	S32 cur_index = 0;
	//for each face
	for (face_list_t::iterator iter = mVolumeFaces.begin();
		 iter != mVolumeFaces.end(); ++iter)
	{
		LLVolumeFace& face = *iter;
	
		if (!(face_mask & (0x1 << cur_index++)) ||
		     face.mNumIndices == 0 || face.mEdge.empty())
		{
			continue;
		}

		if (face.mTypeMask & (LLVolumeFace::CAP_MASK))
		{
			LLVector4a* v = (LLVector4a*)face.mPositions;
			LLVector4a* n = (LLVector4a*)face.mNormals;

			for (U32 j = 0; j < face.mNumIndices / 3; j++)
			{
				for (S32 k = 0; k < 3; k++)
				{
					S32 index = face.mEdge[j * 3 + k];

					if (index == -1)
					{
						// silhouette edge, currently only cubes, so no other conditions

						S32 v1 = face.mIndices[j * 3 + k];
						S32 v2 = face.mIndices[j * 3 + ((k + 1) % 3)];

						LLVector4a t;
						mat.affineTransform(v[v1], t);
						vertices.push_back(LLVector3(t[0], t[1], t[2]));

						norm_mat.rotate(n[v1], t);

						t.normalize3fast();
						normals.push_back(LLVector3(t[0], t[1], t[2]));

						mat.affineTransform(v[v2], t);
						vertices.push_back(LLVector3(t[0], t[1], t[2]));

						norm_mat.rotate(n[v2], t);
						t.normalize3fast();
						normals.push_back(LLVector3(t[0], t[1], t[2]));
					}
				}
			}
	
		}
		else
		{

			//==============================================
			//DEBUG draw edge map instead of silhouette edge
			//==============================================

#if DEBUG_SILHOUETTE_EDGE_MAP

			//for each triangle
            U32 tri_count = face.mNumIndices / 3;
            for (U32 j = 0; j < tri_count; j++) {
				//get vertices
				S32 v1 = face.mIndices[j*3+0];
				S32 v2 = face.mIndices[j*3+1];
				S32 v3 = face.mIndices[j*3+2];

				//get current face center
				LLVector3 cCenter = (face.mVertices[v1].getPosition() + 
									face.mVertices[v2].getPosition() + 
									face.mVertices[v3].getPosition()) / 3.0f;

				//for each edge
				for (S32 k = 0; k < 3; k++) {
                    S32 nIndex = face.mEdge[j*3+k];
					if (nIndex <= -1) {
						continue;
					}

                    if (nIndex >= (S32)tri_count) {
						continue;
					}
					//get neighbor vertices
					v1 = face.mIndices[nIndex*3+0];
					v2 = face.mIndices[nIndex*3+1];
					v3 = face.mIndices[nIndex*3+2];

					//get neighbor face center
					LLVector3 nCenter = (face.mVertices[v1].getPosition() + 
									face.mVertices[v2].getPosition() + 
									face.mVertices[v3].getPosition()) / 3.0f;

					//draw line
					vertices.push_back(cCenter);
					vertices.push_back(nCenter);
					normals.push_back(LLVector3(1,1,1));
					normals.push_back(LLVector3(1,1,1));
					segments.push_back(vertices.size());
				}
			}
		
			continue;

			//==============================================
			//DEBUG
			//==============================================

			//==============================================
			//DEBUG draw normals instead of silhouette edge
			//==============================================
#elif DEBUG_SILHOUETTE_NORMALS

			//for each vertex
			for (U32 j = 0; j < face.mNumVertices; j++) {
				vertices.push_back(face.mVertices[j].getPosition());
				vertices.push_back(face.mVertices[j].getPosition() + face.mVertices[j].getNormal()*0.1f);
				normals.push_back(LLVector3(0,0,1));
				normals.push_back(LLVector3(0,0,1));
				segments.push_back(vertices.size());
#if DEBUG_SILHOUETTE_BINORMALS
				vertices.push_back(face.mVertices[j].getPosition());
				vertices.push_back(face.mVertices[j].getPosition() + face.mVertices[j].mTangent*0.1f);
				normals.push_back(LLVector3(0,0,1));
				normals.push_back(LLVector3(0,0,1));
				segments.push_back(vertices.size());
#endif
			}
						
			continue;
#else
			//==============================================
			//DEBUG
			//==============================================

			static const U8 AWAY = 0x01,
							TOWARDS = 0x02;

			//for each triangle
			std::vector<U8> fFacing;
			vector_append(fFacing, face.mNumIndices/3);

			LLVector4a* v = (LLVector4a*) face.mPositions;
			LLVector4a* n = (LLVector4a*) face.mNormals;

			for (U32 j = 0; j < face.mNumIndices/3; j++) 
			{
				//approximate normal
				S32 v1 = face.mIndices[j*3+0];
				S32 v2 = face.mIndices[j*3+1];
				S32 v3 = face.mIndices[j*3+2];

				LLVector4a c1,c2;
				c1.setSub(v[v1], v[v2]);
				c2.setSub(v[v2], v[v3]);

				LLVector4a norm;

				norm.setCross3(c1, c2);

				if (norm.dot3(norm) < 0.00000001f) 
				{
					fFacing[j] = AWAY | TOWARDS;
				}
				else 
				{
					//get view vector
					LLVector4a view;
					view.setSub(obj_cam_vec, v[v1]);
					bool away = view.dot3(norm) > 0.0f; 
					if (away) 
					{
						fFacing[j] = AWAY;
					}
					else 
					{
						fFacing[j] = TOWARDS;
					}
				}
			}
			
			//for each triangle
			for (U32 j = 0; j < face.mNumIndices/3; j++) 
			{
				if (fFacing[j] == (AWAY | TOWARDS)) 
				{ //this is a degenerate triangle
					//take neighbor facing (degenerate faces get facing of one of their neighbors)
					// *FIX IF NEEDED:  this does not deal with neighboring degenerate faces
					for (S32 k = 0; k < 3; k++) 
					{
						S32 index = face.mEdge[j*3+k];
						if (index != -1) 
						{
							fFacing[j] = fFacing[index];
							break;
						}
					}
					continue; //skip degenerate face
				}

				//for each edge
				for (S32 k = 0; k < 3; k++) {
					S32 index = face.mEdge[j*3+k];
					if (index != -1 && fFacing[index] == (AWAY | TOWARDS)) {
						//our neighbor is degenerate, make him face our direction
						fFacing[face.mEdge[j*3+k]] = fFacing[j];
						continue;
					}

					if (index == -1 ||		//edge has no neighbor, MUST be a silhouette edge
						(fFacing[index] & fFacing[j]) == 0) { 	//we found a silhouette edge

						S32 v1 = face.mIndices[j*3+k];
						S32 v2 = face.mIndices[j*3+((k+1)%3)];
						
						LLVector4a t;
						mat.affineTransform(v[v1], t);
						vertices.push_back(LLVector3(t[0], t[1], t[2]));

						norm_mat.rotate(n[v1], t);

						t.normalize3fast();
						normals.push_back(LLVector3(t[0], t[1], t[2]));

						mat.affineTransform(v[v2], t);
						vertices.push_back(LLVector3(t[0], t[1], t[2]));
						
						norm_mat.rotate(n[v2], t);
						t.normalize3fast();
						normals.push_back(LLVector3(t[0], t[1], t[2]));
					}
				}		
			}
#endif
		}
	}
}

S32 LLVolume::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end, 
								   S32 face,
								   LLVector4a* intersection,LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent_out)
{
	S32 hit_face = -1;
	
	S32 start_face;
	S32 end_face;
	
	if (face == -1) // ALL_SIDES
	{
		start_face = 0;
		end_face = getNumVolumeFaces() - 1;
	}
	else
	{
		start_face = face;
		end_face = face;
	}

	LLVector4a dir;
	dir.setSub(end, start);

	F32 closest_t = 2.f; // must be larger than 1
	
	end_face = llmin(end_face, getNumVolumeFaces()-1);

	for (S32 i = start_face; i <= end_face; i++)
	{
		LLVolumeFace &face = mVolumeFaces[i];

		LLVector4a box_center;
		box_center.setAdd(face.mExtents[0], face.mExtents[1]);
		box_center.mul(0.5f);

		LLVector4a box_size;
		box_size.setSub(face.mExtents[1], face.mExtents[0]);

        if (LLLineSegmentBoxIntersect(start, end, box_center, box_size))
		{
			if (tangent_out != NULL) // if the caller wants tangents, we may need to generate them
			{
				genTangents(i);
			}

			if (isUnique())
			{ //don't bother with an octree for flexi volumes
				U32 tri_count = face.mNumIndices/3;

				for (U32 j = 0; j < tri_count; ++j)
				{
					U16 idx0 = face.mIndices[j*3+0];
					U16 idx1 = face.mIndices[j*3+1];
					U16 idx2 = face.mIndices[j*3+2];

					const LLVector4a& v0 = face.mPositions[idx0];
					const LLVector4a& v1 = face.mPositions[idx1];
					const LLVector4a& v2 = face.mPositions[idx2];
				
					F32 a,b,t;

					if (LLTriangleRayIntersect(v0, v1, v2,
							start, dir, a, b, t))
					{
						if ((t >= 0.f) &&      // if hit is after start
							(t <= 1.f) &&      // and before end
							(t < closest_t))   // and this hit is closer
						{
							closest_t = t;
							hit_face = i;

							if (intersection != NULL)
							{
								LLVector4a intersect = dir;
								intersect.mul(closest_t);
								intersect.add(start);
								*intersection = intersect;
							}


							if (tex_coord != NULL)
							{
								LLVector2* tc = (LLVector2*) face.mTexCoords;
								*tex_coord = ((1.f - a - b)  * tc[idx0] +
									a              * tc[idx1] +
									b              * tc[idx2]);

							}

							if (normal!= NULL)
							{
								LLVector4a* norm = face.mNormals;
								
								LLVector4a n1,n2,n3;
								n1 = norm[idx0];
								n1.mul(1.f-a-b);
								
								n2 = norm[idx1];
								n2.mul(a);
								
								n3 = norm[idx2];
								n3.mul(b);

								n1.add(n2);
								n1.add(n3);
								
								*normal		= n1; 
							}

							if (tangent_out != NULL)
							{
								LLVector4a* tangents = face.mTangents;
								
								LLVector4a t1,t2,t3;
								t1 = tangents[idx0];
								t1.mul(1.f-a-b);
								
								t2 = tangents[idx1];
								t2.mul(a);
								
								t3 = tangents[idx2];
								t3.mul(b);

								t1.add(t2);
								t1.add(t3);
								
								*tangent_out = t1; 
							}
						}
					}
				}
			}
			else
			{
                if (!face.getOctree())
				{
					face.createOctree();
				}
			
				LLOctreeTriangleRayIntersect intersect(start, dir, &face, &closest_t, intersection, tex_coord, normal, tangent_out);
                intersect.traverse(face.getOctree());
				if (intersect.mHitFace)
				{
					hit_face = i;
				}
			}
		}		
	}
	
	
	return hit_face;
}

class LLVertexIndexPair
{
public:
	LLVertexIndexPair(const LLVector3 &vertex, const S32 index);

	LLVector3 mVertex;
	S32	mIndex;
};

LLVertexIndexPair::LLVertexIndexPair(const LLVector3 &vertex, const S32 index)
{
	mVertex = vertex;
	mIndex = index;
}

const F32 VERTEX_SLOP = 0.00001f;

struct lessVertex
{
	bool operator()(const LLVertexIndexPair *a, const LLVertexIndexPair *b)
	{
		const F32 slop = VERTEX_SLOP;

		if (a->mVertex.mV[0] + slop < b->mVertex.mV[0])
		{
			return TRUE;
		}
		else if (a->mVertex.mV[0] - slop > b->mVertex.mV[0])
		{
			return FALSE;
		}
		
		if (a->mVertex.mV[1] + slop < b->mVertex.mV[1])
		{
			return TRUE;
		}
		else if (a->mVertex.mV[1] - slop > b->mVertex.mV[1])
		{
			return FALSE;
		}
		
		if (a->mVertex.mV[2] + slop < b->mVertex.mV[2])
		{
			return TRUE;
		}
		else if (a->mVertex.mV[2] - slop > b->mVertex.mV[2])
		{
			return FALSE;
		}
		
		return FALSE;
	}
};

struct lessTriangle
{
	bool operator()(const S32 *a, const S32 *b)
	{
		if (*a < *b)
		{
			return TRUE;
		}
		else if (*a > *b)
		{
			return FALSE;
		}

		if (*(a+1) < *(b+1))
		{
			return TRUE;
		}
		else if (*(a+1) > *(b+1))
		{
			return FALSE;
		}

		if (*(a+2) < *(b+2))
		{
			return TRUE;
		}
		else if (*(a+2) > *(b+2))
		{
			return FALSE;
		}

		return FALSE;
	}
};

BOOL equalTriangle(const S32 *a, const S32 *b)
{
	if ((*a == *b) && (*(a+1) == *(b+1)) && (*(a+2) == *(b+2)))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL LLVolumeParams::importFile(LLFILE *fp)
{
	//LL_INFOS() << "importing volume" << LL_ENDL;
	const S32 BUFSIZE = 16384;
	char buffer[BUFSIZE];	/* Flawfinder: ignore */
	// *NOTE: changing the size or type of this buffer will require
	// changing the sscanf below.
	char keyword[256];	/* Flawfinder: ignore */
	keyword[0] = 0;

	while (!feof(fp))
	{
		if (fgets(buffer, BUFSIZE, fp) == NULL)
		{
			buffer[0] = '\0';
		}
		
		sscanf(buffer, " %255s", keyword);	/* Flawfinder: ignore */
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}",keyword))
		{
			break;
		}
		else if (!strcmp("profile", keyword))
		{
			mProfileParams.importFile(fp);
		}
		else if (!strcmp("path",keyword))
		{
			mPathParams.importFile(fp);
		}
		else
		{
			LL_WARNS() << "unknown keyword " << keyword << " in volume import" << LL_ENDL;
		}
	}

	return TRUE;
}

BOOL LLVolumeParams::exportFile(LLFILE *fp) const
{
	fprintf(fp,"\tshape 0\n");
	fprintf(fp,"\t{\n");
	mPathParams.exportFile(fp);
	mProfileParams.exportFile(fp);
	fprintf(fp, "\t}\n");
	return TRUE;
}


BOOL LLVolumeParams::importLegacyStream(std::istream& input_stream)
{
	//LL_INFOS() << "importing volume" << LL_ENDL;
	const S32 BUFSIZE = 16384;
	// *NOTE: changing the size or type of this buffer will require
	// changing the sscanf below.
	char buffer[BUFSIZE];		/* Flawfinder: ignore */
	char keyword[256];		/* Flawfinder: ignore */
	keyword[0] = 0;

	while (input_stream.good())
	{
		input_stream.getline(buffer, BUFSIZE);
		sscanf(buffer, " %255s", keyword);
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}",keyword))
		{
			break;
		}
		else if (!strcmp("profile", keyword))
		{
			mProfileParams.importLegacyStream(input_stream);
		}
		else if (!strcmp("path",keyword))
		{
			mPathParams.importLegacyStream(input_stream);
		}
		else
		{
			LL_WARNS() << "unknown keyword " << keyword << " in volume import" << LL_ENDL;
		}
	}

	return TRUE;
}

BOOL LLVolumeParams::exportLegacyStream(std::ostream& output_stream) const
{
	output_stream <<"\tshape 0\n";
	output_stream <<"\t{\n";
	mPathParams.exportLegacyStream(output_stream);
	mProfileParams.exportLegacyStream(output_stream);
	output_stream << "\t}\n";
	return TRUE;
}

LLSD LLVolumeParams::sculptAsLLSD() const
{
	LLSD sd = LLSD();
	sd["id"] = getSculptID();
	sd["type"] = getSculptType();

	return sd;
}

bool LLVolumeParams::sculptFromLLSD(LLSD& sd)
{
	setSculptID(sd["id"].asUUID(), (U8)sd["type"].asInteger());
	return true;
}

LLSD LLVolumeParams::asLLSD() const
{
	LLSD sd = LLSD();
	sd["path"] = mPathParams;
	sd["profile"] = mProfileParams;
	sd["sculpt"] = sculptAsLLSD();
	
	return sd;
}

bool LLVolumeParams::fromLLSD(LLSD& sd)
{
	mPathParams.fromLLSD(sd["path"]);
	mProfileParams.fromLLSD(sd["profile"]);
	sculptFromLLSD(sd["sculpt"]);
		
	return true;
}

void LLVolumeParams::reduceS(F32 begin, F32 end)
{
	begin = llclampf(begin);
	end = llclampf(end);
	if (begin > end)
	{
		F32 temp = begin;
		begin = end;
		end = temp;
	}
	F32 a = mProfileParams.getBegin();
	F32 b = mProfileParams.getEnd();
	mProfileParams.setBegin(a + begin * (b - a));
	mProfileParams.setEnd(a + end * (b - a));
}

void LLVolumeParams::reduceT(F32 begin, F32 end)
{
	begin = llclampf(begin);
	end = llclampf(end);
	if (begin > end)
	{
		F32 temp = begin;
		begin = end;
		end = temp;
	}
	F32 a = mPathParams.getBegin();
	F32 b = mPathParams.getEnd();
	mPathParams.setBegin(a + begin * (b - a));
	mPathParams.setEnd(a + end * (b - a));
}

const F32 MIN_CONCAVE_PROFILE_WEDGE = 0.125f;	// 1/8 unity
const F32 MIN_CONCAVE_PATH_WEDGE = 0.111111f;	// 1/9 unity

// returns TRUE if the shape can be approximated with a convex shape 
// for collison purposes
BOOL LLVolumeParams::isConvex() const
{
	if (!getSculptID().isNull())
	{
		// can't determine, be safe and say no:
		return FALSE;
	}
	
	F32 path_length = mPathParams.getEnd() - mPathParams.getBegin();
	F32 hollow = mProfileParams.getHollow();
	 
	U8 path_type = mPathParams.getCurveType();
	if ( path_length > MIN_CONCAVE_PATH_WEDGE
		&& ( mPathParams.getTwist() != mPathParams.getTwistBegin()
		     || (hollow > 0.f 
				 && LL_PCODE_PATH_LINE != path_type) ) )
	{
		// twist along a "not too short" path is concave
		return FALSE;
	}

	F32 profile_length = mProfileParams.getEnd() - mProfileParams.getBegin();
	BOOL same_hole = hollow == 0.f 
					 || (mProfileParams.getCurveType() & LL_PCODE_HOLE_MASK) == LL_PCODE_HOLE_SAME;

	F32 min_profile_wedge = MIN_CONCAVE_PROFILE_WEDGE;
	U8 profile_type = mProfileParams.getCurveType() & LL_PCODE_PROFILE_MASK;
	if ( LL_PCODE_PROFILE_CIRCLE_HALF == profile_type )
	{
		// it is a sphere and spheres get twice the minimum profile wedge
		min_profile_wedge = 2.f * MIN_CONCAVE_PROFILE_WEDGE;
	}

	BOOL convex_profile = ( ( profile_length == 1.f
						     || profile_length <= 0.5f )
						   && hollow == 0.f )						// trivially convex
						  || ( profile_length <= min_profile_wedge
							  && same_hole );						// effectvely convex (even when hollow)

	if (!convex_profile)
	{
		// profile is concave
		return FALSE;
	}

	if ( LL_PCODE_PATH_LINE == path_type )
	{
		// straight paths with convex profile
		return TRUE;
	}

	BOOL concave_path = (path_length < 1.0f) && (path_length > 0.5f);
	if (concave_path)
	{
		return FALSE;
	}

	// we're left with spheres, toroids and tubes
	if ( LL_PCODE_PROFILE_CIRCLE_HALF == profile_type )
	{
		// at this stage all spheres must be convex
		return TRUE;
	}

	// it's a toroid or tube		
	if ( path_length <= MIN_CONCAVE_PATH_WEDGE )
	{
		// effectively convex
		return TRUE;
	}

	return FALSE;
}

// debug
void LLVolumeParams::setCube()
{
	mProfileParams.setCurveType(LL_PCODE_PROFILE_SQUARE);
	mProfileParams.setBegin(0.f);
	mProfileParams.setEnd(1.f);
	mProfileParams.setHollow(0.f);

	mPathParams.setBegin(0.f);
	mPathParams.setEnd(1.f);
	mPathParams.setScale(1.f, 1.f);
	mPathParams.setShear(0.f, 0.f);
	mPathParams.setCurveType(LL_PCODE_PATH_LINE);
	mPathParams.setTwistBegin(0.f);
	mPathParams.setTwistEnd(0.f);
	mPathParams.setRadiusOffset(0.f);
	mPathParams.setTaper(0.f, 0.f);
	mPathParams.setRevolutions(0.f);
	mPathParams.setSkew(0.f);
}

LLFaceID LLVolume::generateFaceMask()
{
	LLFaceID new_mask = 0x0000;

	switch(mParams.getProfileParams().getCurveType() & LL_PCODE_PROFILE_MASK)
	{
	case LL_PCODE_PROFILE_CIRCLE:
	case LL_PCODE_PROFILE_CIRCLE_HALF:
		new_mask |= LL_FACE_OUTER_SIDE_0;
		break;
	case LL_PCODE_PROFILE_SQUARE:
		{
			for(S32 side = (S32)(mParams.getProfileParams().getBegin() * 4.f); side < llceil(mParams.getProfileParams().getEnd() * 4.f); side++)
			{
				new_mask |= LL_FACE_OUTER_SIDE_0 << side;
			}
		}
		break;
	case LL_PCODE_PROFILE_ISOTRI:
	case LL_PCODE_PROFILE_EQUALTRI:
	case LL_PCODE_PROFILE_RIGHTTRI:
		{
			for(S32 side = (S32)(mParams.getProfileParams().getBegin() * 3.f); side < llceil(mParams.getProfileParams().getEnd() * 3.f); side++)
			{
				new_mask |= LL_FACE_OUTER_SIDE_0 << side;
			}
		}
		break;
	default:
		LL_ERRS() << "Unknown profile!" << LL_ENDL;
		break;
	}

	// handle hollow objects
	if (mParams.getProfileParams().getHollow() > 0)
	{
		new_mask |= LL_FACE_INNER_SIDE;
	}

	// handle open profile curves
	if (mProfilep->isOpen())
	{
		new_mask |= LL_FACE_PROFILE_BEGIN | LL_FACE_PROFILE_END;
	}

	// handle open path curves
	if (mPathp->isOpen())
	{
		new_mask |= LL_FACE_PATH_BEGIN | LL_FACE_PATH_END;
	}

	return new_mask;
}

BOOL LLVolume::isFaceMaskValid(LLFaceID face_mask)
{
	LLFaceID test_mask = 0;
	for(S32 i = 0; i < getNumFaces(); i++)
	{
		test_mask |= mProfilep->mFaces[i].mFaceID;
	}

	return test_mask == face_mask;
}

BOOL LLVolume::isConvex() const
{
	// mParams.isConvex() may return FALSE even though the final
	// geometry is actually convex due to LOD approximations.
	// TODO -- provide LLPath and LLProfile with isConvex() methods
	// that correctly determine convexity. -- Leviathan
	return mParams.isConvex();
}


std::ostream& operator<<(std::ostream &s, const LLProfileParams &profile_params)
{
	s << "{type=" << (U32) profile_params.mCurveType;
	s << ", begin=" << profile_params.mBegin;
	s << ", end=" << profile_params.mEnd;
	s << ", hollow=" << profile_params.mHollow;
	s << "}";
	return s;
}


std::ostream& operator<<(std::ostream &s, const LLPathParams &path_params)
{
	s << "{type=" << (U32) path_params.mCurveType;
	s << ", begin=" << path_params.mBegin;
	s << ", end=" << path_params.mEnd;
	s << ", twist=" << path_params.mTwistEnd;
	s << ", scale=" << path_params.mScale;
	s << ", shear=" << path_params.mShear;
	s << ", twist_begin=" << path_params.mTwistBegin;
	s << ", radius_offset=" << path_params.mRadiusOffset;
	s << ", taper=" << path_params.mTaper;
	s << ", revolutions=" << path_params.mRevolutions;
	s << ", skew=" << path_params.mSkew;
	s << "}";
	return s;
}


std::ostream& operator<<(std::ostream &s, const LLVolumeParams &volume_params)
{
	s << "{profileparams = " << volume_params.mProfileParams;
	s << ", pathparams = " << volume_params.mPathParams;
	s << "}";
	return s;
}


std::ostream& operator<<(std::ostream &s, const LLProfile &profile)
{
	s << " {open=" << (U32) profile.mOpen;
	s << ", dirty=" << profile.mDirty;
	s << ", totalout=" << profile.mTotalOut;
	s << ", total=" << profile.mTotal;
	s << "}";
	return s;
}


std::ostream& operator<<(std::ostream &s, const LLPath &path)
{
	s << "{open=" << (U32) path.mOpen;
	s << ", dirty=" << path.mDirty;
	s << ", step=" << path.mStep;
	s << ", total=" << path.mTotal;
	s << "}";
	return s;
}

std::ostream& operator<<(std::ostream &s, const LLVolume &volume)
{
	s << "{params = " << volume.getParams();
	s << ", path = " << *volume.mPathp;
	s << ", profile = " << *volume.mProfilep;
	s << "}";
	return s;
}


std::ostream& operator<<(std::ostream &s, const LLVolume *volumep)
{
	s << "{params = " << volumep->getParams();
	s << ", path = " << *(volumep->mPathp);
	s << ", profile = " << *(volumep->mProfilep);
	s << "}";
	return s;
}

LLVolumeFace::LLVolumeFace() : 
	mID(0),
	mTypeMask(0),
	mBeginS(0),
	mBeginT(0),
	mNumS(0),
	mNumT(0),
	mNumVertices(0),
	mNumAllocatedVertices(0),
	mNumIndices(0),
	mPositions(NULL),
	mNormals(NULL),
	mTangents(NULL),
	mTexCoords(NULL),
	mIndices(NULL),
	mWeights(NULL),
#if USE_SEPARATE_JOINT_INDICES_AND_WEIGHTS
    mJustWeights(NULL),
    mJointIndices(NULL),
#endif
    mWeightsScrubbed(FALSE),
	mOctree(NULL),
    mOctreeTriangles(NULL),
	mOptimized(FALSE)
{
	mExtents = (LLVector4a*) ll_aligned_malloc_16(sizeof(LLVector4a)*3);
	mExtents[0].splat(-0.5f);
	mExtents[1].splat(0.5f);
	mCenter = mExtents+2;
}

LLVolumeFace::LLVolumeFace(const LLVolumeFace& src)
:	mID(0),
	mTypeMask(0),
	mBeginS(0),
	mBeginT(0),
	mNumS(0),
	mNumT(0),
	mNumVertices(0),
	mNumAllocatedVertices(0),
	mNumIndices(0),
	mPositions(NULL),
	mNormals(NULL),
	mTangents(NULL),
	mTexCoords(NULL),
	mIndices(NULL),
	mWeights(NULL),
#if USE_SEPARATE_JOINT_INDICES_AND_WEIGHTS
    mJustWeights(NULL),
    mJointIndices(NULL),
#endif
    mWeightsScrubbed(FALSE),
    mOctree(NULL),
    mOctreeTriangles(NULL)
{
	mExtents = (LLVector4a*) ll_aligned_malloc_16(sizeof(LLVector4a)*3);
	mCenter = mExtents+2;
	*this = src;
}

LLVolumeFace& LLVolumeFace::operator=(const LLVolumeFace& src)
{
	if (&src == this)
	{ //self assignment, do nothing
		return *this;
	}

	mID = src.mID;
	mTypeMask = src.mTypeMask;
	mBeginS = src.mBeginS;
	mBeginT = src.mBeginT;
	mNumS = src.mNumS;
	mNumT = src.mNumT;

	mExtents[0] = src.mExtents[0];
	mExtents[1] = src.mExtents[1];
	*mCenter = *src.mCenter;

	mNumVertices = 0;
	mNumIndices = 0;

	freeData();
	
	resizeVertices(src.mNumVertices);
	resizeIndices(src.mNumIndices);

	if (mNumVertices)
	{
		S32 vert_size = mNumVertices*sizeof(LLVector4a);
		S32 tc_size = (mNumVertices*sizeof(LLVector2)+0xF) & ~0xF;
			
		LLVector4a::memcpyNonAliased16((F32*) mPositions, (F32*) src.mPositions, vert_size);

		if (src.mNormals)
		{
		LLVector4a::memcpyNonAliased16((F32*) mNormals, (F32*) src.mNormals, vert_size);
		}

		if(src.mTexCoords)
		{
			LLVector4a::memcpyNonAliased16((F32*) mTexCoords, (F32*) src.mTexCoords, tc_size);
		}

		if (src.mTangents)
		{
			allocateTangents(src.mNumVertices);
			LLVector4a::memcpyNonAliased16((F32*) mTangents, (F32*) src.mTangents, vert_size);
		}
		else
		{
			ll_aligned_free_16(mTangents);
			mTangents = NULL;
		}

		if (src.mWeights)
		{
            llassert(!mWeights); // don't orphan an old alloc here accidentally
			allocateWeights(src.mNumVertices);
			LLVector4a::memcpyNonAliased16((F32*) mWeights, (F32*) src.mWeights, vert_size);            
            mWeightsScrubbed = src.mWeightsScrubbed;
		}
		else
		{
			ll_aligned_free_16(mWeights);            
			mWeights = NULL;            
            mWeightsScrubbed = FALSE;
		}   

    #if USE_SEPARATE_JOINT_INDICES_AND_WEIGHTS
        if (src.mJointIndices)
        {
            llassert(!mJointIndices); // don't orphan an old alloc here accidentally
            allocateJointIndices(src.mNumVertices);
            LLVector4a::memcpyNonAliased16((F32*) mJointIndices, (F32*) src.mJointIndices, src.mNumVertices * sizeof(U8) * 4);
        }
        else*/
        {
            ll_aligned_free_16(mJointIndices);
            mJointIndices = NULL;
        }     
    #endif

	}
    
	if (mNumIndices)
	{
		S32 idx_size = (mNumIndices*sizeof(U16)+0xF) & ~0xF;
		
		LLVector4a::memcpyNonAliased16((F32*) mIndices, (F32*) src.mIndices, idx_size);
	}
	else
    {
        ll_aligned_free_16(mIndices);
        mIndices = NULL;
    }

	mOptimized = src.mOptimized;

	//delete 
	return *this;
}

LLVolumeFace::~LLVolumeFace()
{
	ll_aligned_free_16(mExtents);
	mExtents = NULL;
	mCenter = NULL;

	freeData();
}

void LLVolumeFace::freeData()
{
	ll_aligned_free<64>(mPositions);
	mPositions = NULL;

	//normals and texture coordinates are part of the same buffer as mPositions, do not free them separately
	mNormals = NULL;
	mTexCoords = NULL;

	ll_aligned_free_16(mIndices);
	mIndices = NULL;
	ll_aligned_free_16(mTangents);
	mTangents = NULL;
	ll_aligned_free_16(mWeights);
	mWeights = NULL;

#if USE_SEPARATE_JOINT_INDICES_AND_WEIGHTS
    ll_aligned_free_16(mJointIndices);
	mJointIndices = NULL;
    ll_aligned_free_16(mJustWeights);
	mJustWeights = NULL;
#endif

    destroyOctree();
}

BOOL LLVolumeFace::create(LLVolume* volume, BOOL partial_build)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	//tree for this face is no longer valid
    destroyOctree();

	LL_CHECK_MEMORY
	BOOL ret = FALSE ;
	if (mTypeMask & CAP_MASK)
	{
		ret = createCap(volume, partial_build);
		LL_CHECK_MEMORY
	}
	else if ((mTypeMask & END_MASK) || (mTypeMask & SIDE_MASK))
	{
		ret = createSide(volume, partial_build);
		LL_CHECK_MEMORY
	}
	else
	{
		LL_ERRS() << "Unknown/uninitialized face type!" << LL_ENDL;
	}

	return ret ;
}

void LLVolumeFace::getVertexData(U16 index, LLVolumeFace::VertexData& cv)
{
	cv.setPosition(mPositions[index]);
	if (mNormals)
	{
		cv.setNormal(mNormals[index]);
	}
	else
	{
		cv.getNormal().clear();
	}

	if (mTexCoords)
	{
		cv.mTexCoord = mTexCoords[index];
	}
	else
	{
		cv.mTexCoord.clear();
	}
}

bool LLVolumeFace::VertexMapData::operator==(const LLVolumeFace::VertexData& rhs) const
{
	return getPosition().equals3(rhs.getPosition()) &&
		mTexCoord == rhs.mTexCoord &&
		getNormal().equals3(rhs.getNormal());
}

bool LLVolumeFace::VertexMapData::ComparePosition::operator()(const LLVector3& a, const LLVector3& b) const
{
	if (a.mV[0] != b.mV[0])
	{
		return a.mV[0] < b.mV[0];
	}
	
	if (a.mV[1] != b.mV[1])
	{
		return a.mV[1] < b.mV[1];
	}
	
	return a.mV[2] < b.mV[2];
}

void LLVolumeFace::remap()
{
    // Generate a remap buffer
    std::vector<unsigned int> remap(mNumVertices);
    S32 remap_vertices_count = LLMeshOptimizer::generateRemapMultiU16(&remap[0],
        mIndices,
        mNumIndices,
        mPositions,
        mNormals,
        mTexCoords,
        mNumVertices);

    // Allocate new buffers
    S32 size = ((mNumIndices * sizeof(U16)) + 0xF) & ~0xF;
    U16* remap_indices = (U16*)ll_aligned_malloc_16(size);

    S32 tc_bytes_size = ((remap_vertices_count * sizeof(LLVector2)) + 0xF) & ~0xF;
    LLVector4a* remap_positions = (LLVector4a*)ll_aligned_malloc<64>(sizeof(LLVector4a) * 2 * remap_vertices_count + tc_bytes_size);
    LLVector4a* remap_normals = remap_positions + remap_vertices_count;
    LLVector2* remap_tex_coords = (LLVector2*)(remap_normals + remap_vertices_count);

    // Fill the buffers
    LLMeshOptimizer::remapIndexBufferU16(remap_indices, mIndices, mNumIndices, &remap[0]);
    LLMeshOptimizer::remapPositionsBuffer(remap_positions, mPositions, mNumVertices, &remap[0]);
    LLMeshOptimizer::remapNormalsBuffer(remap_normals, mNormals, mNumVertices, &remap[0]);
    LLMeshOptimizer::remapUVBuffer(remap_tex_coords, mTexCoords, mNumVertices, &remap[0]);

    // Free unused buffers
    ll_aligned_free_16(mIndices);
    ll_aligned_free<64>(mPositions);

    // Tangets are now invalid
    ll_aligned_free_16(mTangents);
    mTangents = NULL;

    // Assign new values
    mIndices = remap_indices;
    mPositions = remap_positions;
    mNormals = remap_normals;
    mTexCoords = remap_tex_coords;
    mNumVertices = remap_vertices_count;
    mNumAllocatedVertices = remap_vertices_count;
}

void LLVolumeFace::optimize(F32 angle_cutoff)
{
	LLVolumeFace new_face;

	//map of points to vector of vertices at that point
	std::map<U64, std::vector<VertexMapData> > point_map;

	LLVector4a range;
	range.setSub(mExtents[1],mExtents[0]);

	//remove redundant vertices
	for (U32 i = 0; i < mNumIndices; ++i)
	{
		U16 index = mIndices[i];

        if (index >= mNumVertices)
        {
            // invalid index
            // replace with a valid index to avoid crashes
            index = mNumVertices - 1;
            mIndices[i] = index;

            // Needs better logging
            LL_DEBUGS_ONCE("LLVOLUME") << "Invalid index, substituting" << LL_ENDL;
        }

		LLVolumeFace::VertexData cv;
		getVertexData(index, cv);
		
		BOOL found = FALSE;

		LLVector4a pos;
		pos.setSub(mPositions[index], mExtents[0]);
		pos.div(range);

		U64 pos64 = 0;

		pos64 = (U16) (pos[0]*65535);
		pos64 = pos64 | (((U64) (pos[1]*65535)) << 16);
		pos64 = pos64 | (((U64) (pos[2]*65535)) << 32);

		std::map<U64, std::vector<VertexMapData> >::iterator point_iter = point_map.find(pos64);
		
		if (point_iter != point_map.end())
		{ //duplicate point might exist
			for (U32 j = 0; j < point_iter->second.size(); ++j)
			{
				LLVolumeFace::VertexData& tv = (point_iter->second)[j];
				if (tv.compareNormal(cv, angle_cutoff))
				{
					found = TRUE;
					new_face.pushIndex((point_iter->second)[j].mIndex);
					break;
				}
			}
		}

		if (!found)
		{
			new_face.pushVertex(cv);
			U16 index = (U16) new_face.mNumVertices-1;
			new_face.pushIndex(index);

			VertexMapData d;
			d.setPosition(cv.getPosition());
			d.mTexCoord = cv.mTexCoord;
			d.setNormal(cv.getNormal());
			d.mIndex = index;
			if (point_iter != point_map.end())
			{
				point_iter->second.push_back(d);
			}
			else
			{
				point_map[pos64].push_back(d);
			}
		}
	}


	if (angle_cutoff > 1.f && !mNormals)
	{
		// Now alloc'd with positions
		//ll_aligned_free_16(new_face.mNormals);
		new_face.mNormals = NULL;
	}

	if (!mTexCoords)
	{
		// Now alloc'd with positions
		//ll_aligned_free_16(new_face.mTexCoords);
		new_face.mTexCoords = NULL;
	}

	// Only swap data if we've actually optimized the mesh
	//
	if (new_face.mNumVertices <= mNumVertices)
	{
        llassert(new_face.mNumIndices == mNumIndices);
		swapData(new_face);
	}

}

class LLVCacheTriangleData;

class LLVCacheVertexData
{
public:
	S32 mIdx;
	S32 mCacheTag;
	F64 mScore;
	U32 mActiveTriangles;
	std::vector<LLVCacheTriangleData*> mTriangles;

	LLVCacheVertexData()
	{
		mCacheTag = -1;
		mScore = 0.0;
		mActiveTriangles = 0;
		mIdx = -1;
	}
};

class LLVCacheTriangleData
{
public:
	bool mActive;
	F64 mScore;
	LLVCacheVertexData* mVertex[3];

	LLVCacheTriangleData()
	{
		mActive = true;
		mScore = 0.0;
		mVertex[0] = mVertex[1] = mVertex[2] = NULL;
	}

	void complete()
	{
		mActive = false;
		for (S32 i = 0; i < 3; ++i)
		{
			if (mVertex[i])
			{
				llassert(mVertex[i]->mActiveTriangles > 0);
				mVertex[i]->mActiveTriangles--;
			}
		}
	}

	bool operator<(const LLVCacheTriangleData& rhs) const
	{ //highest score first
		return rhs.mScore < mScore;
	}
};

const F64 FindVertexScore_CacheDecayPower = 1.5;
const F64 FindVertexScore_LastTriScore = 0.75;
const F64 FindVertexScore_ValenceBoostScale = 2.0;
const F64 FindVertexScore_ValenceBoostPower = 0.5;
const U32 MaxSizeVertexCache = 32;
const F64 FindVertexScore_Scaler = 1.0/(MaxSizeVertexCache-3);

F64 find_vertex_score(LLVCacheVertexData& data)
{
	F64 score = -1.0;

	score = 0.0;

	S32 cache_idx = data.mCacheTag;

	if (cache_idx < 0)
	{
		//not in cache
	}
	else
	{
		if (cache_idx < 3)
		{ //vertex was in the last triangle
			score = FindVertexScore_LastTriScore;
		}
		else
		{ //more points for being higher in the cache
				score = 1.0-((cache_idx-3)*FindVertexScore_Scaler);
				score = pow(score, FindVertexScore_CacheDecayPower);
		}
	}

	//bonus points for having low valence
	F64 valence_boost = pow((F64)data.mActiveTriangles, -FindVertexScore_ValenceBoostPower);
	score += FindVertexScore_ValenceBoostScale * valence_boost;

	return score;
}

class LLVCacheFIFO
{
public:
	LLVCacheVertexData* mCache[MaxSizeVertexCache];
	U32 mMisses;

	LLVCacheFIFO()
	{
		mMisses = 0;
		for (U32 i = 0; i < MaxSizeVertexCache; ++i)
		{
			mCache[i] = NULL;
		}
	}

	void addVertex(LLVCacheVertexData* data)
	{
		if (data->mCacheTag == -1)
		{
			mMisses++;

			S32 end = MaxSizeVertexCache-1;

			if (mCache[end])
			{
				mCache[end]->mCacheTag = -1;
			}

			for (S32 i = end; i > 0; --i)
			{
				mCache[i] = mCache[i-1];
				if (mCache[i])
				{
					mCache[i]->mCacheTag = i;
				}
			}

			mCache[0] = data;
			data->mCacheTag = 0;
		}
	}
};

class LLVCacheLRU
{
public:
	LLVCacheVertexData* mCache[MaxSizeVertexCache+3];

	LLVCacheTriangleData* mBestTriangle;
	
	U32 mMisses;

	LLVCacheLRU()
	{
		for (U32 i = 0; i < MaxSizeVertexCache+3; ++i)
		{
			mCache[i] = NULL;
		}

		mBestTriangle = NULL;
		mMisses = 0;
	}

	void addVertex(LLVCacheVertexData* data)
	{
		S32 end = MaxSizeVertexCache+2;
		if (data->mCacheTag != -1)
		{ //just moving a vertex to the front of the cache
			end = data->mCacheTag;
		}
		else
		{
			mMisses++;
			if (mCache[end])
			{ //adding a new vertex, vertex at end of cache falls off
				mCache[end]->mCacheTag = -1;
			}
		}

		for (S32 i = end; i > 0; --i)
		{ //adjust cache pointers and tags
			mCache[i] = mCache[i-1];

			if (mCache[i])
			{
				mCache[i]->mCacheTag = i;			
			}
		}

		mCache[0] = data;
		mCache[0]->mCacheTag = 0;
	}

	void addTriangle(LLVCacheTriangleData* data)
	{
		addVertex(data->mVertex[0]);
		addVertex(data->mVertex[1]);
		addVertex(data->mVertex[2]);
	}

	void updateScores()
	{
		LLVCacheVertexData** data_iter = mCache+MaxSizeVertexCache;
		LLVCacheVertexData** end_data = mCache+MaxSizeVertexCache+3;

		while(data_iter != end_data)
		{
			LLVCacheVertexData* data = *data_iter++;
			//trailing 3 vertices aren't actually in the cache for scoring purposes
			if (data)
			{
				data->mCacheTag = -1;
			}
		}

		data_iter = mCache;
		end_data = mCache+MaxSizeVertexCache;

		while (data_iter != end_data)
		{ //update scores of vertices in cache
			LLVCacheVertexData* data = *data_iter++;
			if (data)
			{
				data->mScore = find_vertex_score(*data);
			}
		}

		mBestTriangle = NULL;
		//update triangle scores
		data_iter = mCache;
		end_data = mCache+MaxSizeVertexCache+3;

		while (data_iter != end_data)
		{
			LLVCacheVertexData* data = *data_iter++;
			if (data)
			{
				for (std::vector<LLVCacheTriangleData*>::iterator iter = data->mTriangles.begin(), end_iter = data->mTriangles.end(); iter != end_iter; ++iter)
				{
					LLVCacheTriangleData* tri = *iter;
					if (tri->mActive)
					{
						tri->mScore = tri->mVertex[0] ? tri->mVertex[0]->mScore : 0;
						tri->mScore += tri->mVertex[1] ? tri->mVertex[1]->mScore : 0;
						tri->mScore += tri->mVertex[2] ? tri->mVertex[2]->mScore : 0;

						if (!mBestTriangle || mBestTriangle->mScore < tri->mScore)
						{
							mBestTriangle = tri;
						}
					}
				}
			}
		}

		//knock trailing 3 vertices off the cache
		data_iter = mCache+MaxSizeVertexCache;
		end_data = mCache+MaxSizeVertexCache+3;
		while (data_iter != end_data)
		{
			LLVCacheVertexData* data = *data_iter;
			if (data)
			{
				llassert(data->mCacheTag == -1);
				*data_iter = NULL;
			}
			++data_iter;
		}
	}
};


bool LLVolumeFace::cacheOptimize()
{ //optimize for vertex cache according to Forsyth method: 
  // http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
	
	llassert(!mOptimized);
	mOptimized = TRUE;

	LLVCacheLRU cache;
	
	if (mNumVertices < 3 || mNumIndices < 3)
	{ //nothing to do
		return true;
	}

	//mapping of vertices to triangles and indices
	std::vector<LLVCacheVertexData> vertex_data;

	//mapping of triangles do vertices
	std::vector<LLVCacheTriangleData> triangle_data;

	try
	{
		triangle_data.resize(mNumIndices / 3);
		vertex_data.resize(mNumVertices);

        for (U32 i = 0; i < mNumIndices; i++)
        { //populate vertex data and triangle data arrays
            U16 idx = mIndices[i];
            U32 tri_idx = i / 3;

            if (idx >= mNumVertices)
            {
                // invalid index
                // replace with a valid index to avoid crashes
                idx = mNumVertices - 1;
                mIndices[i] = idx;

                // Needs better logging
                LL_DEBUGS_ONCE("LLVOLUME") << "Invalid index, substituting" << LL_ENDL;
            }

            vertex_data[idx].mTriangles.push_back(&(triangle_data[tri_idx]));
            vertex_data[idx].mIdx = idx;
            triangle_data[tri_idx].mVertex[i % 3] = &(vertex_data[idx]);
        }
    }
    catch (std::bad_alloc&)
    {
        // resize or push_back failed
        LL_WARNS("LLVOLUME") << "Resize for " << mNumVertices << " vertices failed" << LL_ENDL;
        return false;
    }

	/*F32 pre_acmr = 1.f;
	//measure cache misses from before rebuild
	{
		LLVCacheFIFO test_cache;
		for (U32 i = 0; i < mNumIndices; ++i)
		{
			test_cache.addVertex(&vertex_data[mIndices[i]]);
		}

		for (U32 i = 0; i < mNumVertices; i++)
		{
			vertex_data[i].mCacheTag = -1;
		}

		pre_acmr = (F32) test_cache.mMisses/(mNumIndices/3);
	}*/

	for (U32 i = 0; i < mNumVertices; i++)
	{ //initialize score values (no cache -- might try a fifo cache here)
		LLVCacheVertexData& data = vertex_data[i];

		data.mScore = find_vertex_score(data);
		data.mActiveTriangles = data.mTriangles.size();

		for (U32 j = 0; j < data.mActiveTriangles; ++j)
		{
			data.mTriangles[j]->mScore += data.mScore;
		}
	}

	//sort triangle data by score
	std::sort(triangle_data.begin(), triangle_data.end());

	std::vector<U16> new_indices;

	LLVCacheTriangleData* tri;

	//prime pump by adding first triangle to cache;
	tri = &(triangle_data[0]);
	cache.addTriangle(tri);
	new_indices.push_back(tri->mVertex[0]->mIdx);
	new_indices.push_back(tri->mVertex[1]->mIdx);
	new_indices.push_back(tri->mVertex[2]->mIdx);
	tri->complete();

	U32 breaks = 0;
	for (U32 i = 1; i < mNumIndices/3; ++i)
	{
		cache.updateScores();
		tri = cache.mBestTriangle;
		if (!tri)
		{
			breaks++;
			for (U32 j = 0; j < triangle_data.size(); ++j)
			{
				if (triangle_data[j].mActive)
				{
					tri = &(triangle_data[j]);
					break;
				}
			}
		}	
		
		cache.addTriangle(tri);
		new_indices.push_back(tri->mVertex[0]->mIdx);
		new_indices.push_back(tri->mVertex[1]->mIdx);
		new_indices.push_back(tri->mVertex[2]->mIdx);
		tri->complete();
	}

	for (U32 i = 0; i < mNumIndices; ++i)
	{
		mIndices[i] = new_indices[i];
	}

	/*F32 post_acmr = 1.f;
	//measure cache misses from after rebuild
	{
		LLVCacheFIFO test_cache;
		for (U32 i = 0; i < mNumVertices; i++)
		{
			vertex_data[i].mCacheTag = -1;
		}

		for (U32 i = 0; i < mNumIndices; ++i)
		{
			test_cache.addVertex(&vertex_data[mIndices[i]]);
		}
		
		post_acmr = (F32) test_cache.mMisses/(mNumIndices/3);
	}*/

	//optimize for pre-TnL cache
	
	//allocate space for new buffer
	S32 num_verts = mNumVertices;
	S32 size = ((num_verts*sizeof(LLVector2)) + 0xF) & ~0xF;
	LLVector4a* pos = (LLVector4a*) ll_aligned_malloc<64>(sizeof(LLVector4a)*2*num_verts+size);
	if (pos == NULL)
	{
		LL_WARNS("LLVOLUME") << "Allocation of positions vector[" << sizeof(LLVector4a) * 2 * num_verts + size  << "] failed. " << LL_ENDL;
		return false;
	}
	LLVector4a* norm = pos + num_verts;
	LLVector2* tc = (LLVector2*) (norm + num_verts);

	LLVector4a* wght = NULL;
	if (mWeights)
	{
		wght = (LLVector4a*)ll_aligned_malloc_16(sizeof(LLVector4a)*num_verts);
		if (wght == NULL)
		{
			ll_aligned_free<64>(pos);
			LL_WARNS("LLVOLUME") << "Allocation of weights[" << sizeof(LLVector4a) * num_verts << "] failed" << LL_ENDL;
			return false;
		}
	}

	LLVector4a* binorm = NULL;
	if (mTangents)
	{
		binorm = (LLVector4a*) ll_aligned_malloc_16(sizeof(LLVector4a)*num_verts);
		if (binorm == NULL)
		{
			ll_aligned_free<64>(pos);
			ll_aligned_free_16(wght);
			LL_WARNS("LLVOLUME") << "Allocation of binormals[" << sizeof(LLVector4a)*num_verts << "] failed" << LL_ENDL;
			return false;
		}
	}

	//allocate mapping of old indices to new indices
	std::vector<S32> new_idx;

	try
	{
		new_idx.resize(mNumVertices, -1);
	}
	catch (std::bad_alloc&)
	{
		ll_aligned_free<64>(pos);
		ll_aligned_free_16(wght);
		ll_aligned_free_16(binorm);
		LL_WARNS("LLVOLUME") << "Resize failed: " << mNumVertices << LL_ENDL;
		return false;
	}

	S32 cur_idx = 0;
	for (U32 i = 0; i < mNumIndices; ++i)
	{
		U16 idx = mIndices[i];
		if (new_idx[idx] == -1)
		{ //this vertex hasn't been added yet
			new_idx[idx] = cur_idx;

			//copy vertex data
			pos[cur_idx] = mPositions[idx];
			norm[cur_idx] = mNormals[idx];
			tc[cur_idx] = mTexCoords[idx];
			if (mWeights)
			{
				wght[cur_idx] = mWeights[idx];
			}
			if (mTangents)
			{
				binorm[cur_idx] = mTangents[idx];
			}

			cur_idx++;
		}
	}

	for (U32 i = 0; i < mNumIndices; ++i)
	{
		mIndices[i] = new_idx[mIndices[i]];
	}
	
	ll_aligned_free<64>(mPositions);
	// DO NOT free mNormals and mTexCoords as they are part of mPositions buffer
	ll_aligned_free_16(mWeights);
	ll_aligned_free_16(mTangents);
#if USE_SEPARATE_JOINT_INDICES_AND_WEIGHTS
    ll_aligned_free_16(mJointIndices);
    ll_aligned_free_16(mJustWeights);
    mJustWeights = NULL;
    mJointIndices = NULL; // filled in later as necessary by skinning code for acceleration
#endif

	mPositions = pos;
	mNormals = norm;
	mTexCoords = tc;
	mWeights = wght;    
	mTangents = binorm;

	//std::string result = llformat("ACMR pre/post: %.3f/%.3f  --  %d triangles %d breaks", pre_acmr, post_acmr, mNumIndices/3, breaks);
	//LL_INFOS() << result << LL_ENDL;

	return true;
}

void LLVolumeFace::createOctree(F32 scaler, const LLVector4a& center, const LLVector4a& size)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

    if (getOctree())
	{
		return;
	}

    llassert(mNumIndices % 3 == 0);

    mOctree = new LLOctreeRoot<LLVolumeTriangle, LLVolumeTriangle*>(center, size, NULL);
	new LLVolumeOctreeListener(mOctree);
    const U32 num_triangles = mNumIndices / 3;
    // Initialize all the triangles we need
    mOctreeTriangles = new LLVolumeTriangle[num_triangles];

    for (U32 triangle_index = 0; triangle_index < num_triangles; ++triangle_index)
	{ //for each triangle
        const U32 index = triangle_index * 3;
        LLVolumeTriangle* tri = &mOctreeTriangles[triangle_index];
				
		const LLVector4a& v0 = mPositions[mIndices[index]];
		const LLVector4a& v1 = mPositions[mIndices[index + 1]];
		const LLVector4a& v2 = mPositions[mIndices[index + 2]];

		//store pointers to vertex data
		tri->mV[0] = &v0;
		tri->mV[1] = &v1;
		tri->mV[2] = &v2;

		//store indices
		tri->mIndex[0] = mIndices[index];
		tri->mIndex[1] = mIndices[index + 1];
		tri->mIndex[2] = mIndices[index + 2];

		//get minimum point
		LLVector4a min = v0;
		min.setMin(min, v1);
		min.setMin(min, v2);

		//get maximum point
		LLVector4a max = v0;
		max.setMax(max, v1);
		max.setMax(max, v2);

		//compute center
		LLVector4a center;
		center.setAdd(min, max);
		center.mul(0.5f);

		tri->mPositionGroup = center;

		//compute "radius"
		LLVector4a size;
		size.setSub(max,min);
		
		tri->mRadius = size.getLength3().getF32() * scaler;
		
		//insert
		mOctree->insert(tri);
	}

	//remove unneeded octree layers
	while (!mOctree->balance())	{ }

	//calculate AABB for each node
	LLVolumeOctreeRebound rebound(this);
	rebound.traverse(mOctree);

	if (gDebugGL)
	{
		LLVolumeOctreeValidate validate;
		validate.traverse(mOctree);
	}
}

void LLVolumeFace::destroyOctree()
{
    delete mOctree;
    mOctree = NULL;
    delete[] mOctreeTriangles;
    mOctreeTriangles = NULL;
}

const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* LLVolumeFace::getOctree() const
{
    return mOctree;
}


void LLVolumeFace::swapData(LLVolumeFace& rhs)
{
	llswap(rhs.mPositions, mPositions);
	llswap(rhs.mNormals, mNormals);
	llswap(rhs.mTangents, mTangents);
	llswap(rhs.mTexCoords, mTexCoords);
	llswap(rhs.mIndices,mIndices);
	llswap(rhs.mNumVertices, mNumVertices);
	llswap(rhs.mNumIndices, mNumIndices);
}

void	LerpPlanarVertex(LLVolumeFace::VertexData& v0,
				   LLVolumeFace::VertexData& v1,
				   LLVolumeFace::VertexData& v2,
				   LLVolumeFace::VertexData& vout,
				   F32	coef01,
				   F32	coef02)
{

	LLVector4a lhs;
	lhs.setSub(v1.getPosition(), v0.getPosition());
	lhs.mul(coef01);
	LLVector4a rhs;
	rhs.setSub(v2.getPosition(), v0.getPosition());
	rhs.mul(coef02);

	rhs.add(lhs);
	rhs.add(v0.getPosition());

	vout.setPosition(rhs);
		
	vout.mTexCoord = v0.mTexCoord + ((v1.mTexCoord-v0.mTexCoord)*coef01)+((v2.mTexCoord-v0.mTexCoord)*coef02);
	vout.setNormal(v0.getNormal());
}

BOOL LLVolumeFace::createUnCutCubeCap(LLVolume* volume, BOOL partial_build)
{
	LL_CHECK_MEMORY		

	const LLAlignedArray<LLVector4a,64>& mesh = volume->getMesh();
	const LLAlignedArray<LLVector4a,64>& profile = volume->getProfile().mProfile;
	S32 max_s = volume->getProfile().getTotal();
	S32 max_t = volume->getPath().mPath.size();

	// S32 i;
	S32	grid_size = (profile.size()-1)/4;
	// VFExtents change
	LLVector4a& min = mExtents[0];
	LLVector4a& max = mExtents[1];

	S32 offset = 0;
	if (mTypeMask & TOP_MASK)
	{
		offset = (max_t-1) * max_s;
	}
	else
	{
		offset = mBeginS;
	}

	{
		VertexData	corners[4];
		VertexData baseVert;
		for(S32 t = 0; t < 4; t++)
		{
			corners[t].getPosition().load4a(mesh[offset + (grid_size*t)].getF32ptr());
			corners[t].mTexCoord.mV[0] = profile[grid_size*t][0]+0.5f;
			corners[t].mTexCoord.mV[1] = 0.5f - profile[grid_size*t][1];
		}

		{
			LLVector4a lhs;
			lhs.setSub(corners[1].getPosition(), corners[0].getPosition());
			LLVector4a rhs;
			rhs.setSub(corners[2].getPosition(), corners[1].getPosition());
			baseVert.getNormal().setCross3(lhs, rhs); 
			baseVert.getNormal().normalize3fast();
		}

		if(!(mTypeMask & TOP_MASK))
		{
			baseVert.getNormal().mul(-1.0f);
		}
		else
		{
			//Swap the UVs on the U(X) axis for top face
			LLVector2 swap;
			swap = corners[0].mTexCoord;
			corners[0].mTexCoord=corners[3].mTexCoord;
			corners[3].mTexCoord=swap;
			swap = corners[1].mTexCoord;
			corners[1].mTexCoord=corners[2].mTexCoord;
			corners[2].mTexCoord=swap;
		}

		S32 size = (grid_size+1)*(grid_size+1);
		resizeVertices(size);
		
		LLVector4a* pos = (LLVector4a*) mPositions;
		LLVector4a* norm = (LLVector4a*) mNormals;
		LLVector2* tc = (LLVector2*) mTexCoords;

		for(int gx = 0;gx<grid_size+1;gx++)
		{
			for(int gy = 0;gy<grid_size+1;gy++)
			{
				VertexData newVert;
				LerpPlanarVertex(
					corners[0],
					corners[1],
					corners[3],
					newVert,
					(F32)gx/(F32)grid_size,
					(F32)gy/(F32)grid_size);

				*pos++ = newVert.getPosition();
				*norm++ = baseVert.getNormal();
				*tc++ = newVert.mTexCoord;
				
				if (gx == 0 && gy == 0)
				{
					min = newVert.getPosition();
					max = min;
				}
				else
				{
					min.setMin(min, newVert.getPosition());
					max.setMax(max, newVert.getPosition());
				}
			}
		}
	
		mCenter->setAdd(min, max);
		mCenter->mul(0.5f); 
	}

	if (!partial_build)
	{
		resizeIndices(grid_size*grid_size*6);
		if (!volume->isMeshAssetLoaded())
		{
            S32 size = grid_size * grid_size * 6;
            try
            {
                mEdge.resize(size);
            }
            catch (std::bad_alloc&)
            {
                LL_WARNS("LLVOLUME") << "Resize of mEdge to " << size << " failed" << LL_ENDL;
                return false;
            }
		}

		U16* out = mIndices;

		S32 idxs[] = {0,1,(grid_size+1)+1,(grid_size+1)+1,(grid_size+1),0};

		int cur_edge = 0;

		for(S32 gx = 0;gx<grid_size;gx++)
		{
			
			for(S32 gy = 0;gy<grid_size;gy++)
			{
				if (mTypeMask & TOP_MASK)
				{
					for(S32 i=5;i>=0;i--)
					{
						*out++ = ((gy*(grid_size+1))+gx+idxs[i]);
					}

					S32 edge_value = grid_size * 2 * gy + gx * 2;

					if (gx > 0)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1; // Mark face to higlight it
					}

					if (gy < grid_size - 1)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1;
					}

					mEdge[cur_edge++] = edge_value;

					if (gx < grid_size - 1)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1;
					}

					if (gy > 0)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1;
					}

					mEdge[cur_edge++] = edge_value;
				}
				else
				{
					for(S32 i=0;i<6;i++)
					{
						*out++ = ((gy*(grid_size+1))+gx+idxs[i]);
					}

					S32 edge_value = grid_size * 2 * gy + gx * 2;

					if (gy > 0)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1;
					}

					if (gx < grid_size - 1)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1;
					}

					mEdge[cur_edge++] = edge_value;

					if (gy < grid_size - 1)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1;
					}

					if (gx > 0)
					{
						mEdge[cur_edge++] = edge_value;
					}
					else
					{
						mEdge[cur_edge++] = -1;
					}

					mEdge[cur_edge++] = edge_value;
				}
			}
		}
	}
		
	LL_CHECK_MEMORY
	return TRUE;
}


BOOL LLVolumeFace::createCap(LLVolume* volume, BOOL partial_build)
{
	if (!(mTypeMask & HOLLOW_MASK) && 
		!(mTypeMask & OPEN_MASK) && 
		((volume->getParams().getPathParams().getBegin()==0.0f)&&
		(volume->getParams().getPathParams().getEnd()==1.0f))&&
		(volume->getParams().getProfileParams().getCurveType()==LL_PCODE_PROFILE_SQUARE &&
		 volume->getParams().getPathParams().getCurveType()==LL_PCODE_PATH_LINE)	
		){
		return createUnCutCubeCap(volume, partial_build);
	}

	S32 num_vertices = 0, num_indices = 0;

	const LLAlignedArray<LLVector4a,64>& mesh = volume->getMesh();
	const LLAlignedArray<LLVector4a,64>& profile = volume->getProfile().mProfile;

	// All types of caps have the same number of vertices and indices
	num_vertices = profile.size();
	num_indices = (profile.size() - 2)*3;

	if (!(mTypeMask & HOLLOW_MASK) && !(mTypeMask & OPEN_MASK))
	{
		resizeVertices(num_vertices+1);
		
		//if (!partial_build)
		{
			resizeIndices(num_indices+3);
		}
	}
	else
	{
		resizeVertices(num_vertices);
		//if (!partial_build)
		{
			resizeIndices(num_indices);
		}
	}

	LL_CHECK_MEMORY;

	S32 max_s = volume->getProfile().getTotal();
	S32 max_t = volume->getPath().mPath.size();

	mCenter->clear();

	S32 offset = 0;
	if (mTypeMask & TOP_MASK)
	{
		offset = (max_t-1) * max_s;
	}
	else
	{
		offset = mBeginS;
	}

	// Figure out the normal, assume all caps are flat faces.
	// Cross product to get normals.
	
	LLVector2 cuv;
	LLVector2 min_uv, max_uv;
	// VFExtents change
	LLVector4a& min = mExtents[0];
	LLVector4a& max = mExtents[1];

	LLVector2* tc = (LLVector2*) mTexCoords;
	LLVector4a* pos = (LLVector4a*) mPositions;
	LLVector4a* norm = (LLVector4a*) mNormals;
	
	// Copy the vertices into the array

	const LLVector4a* src = mesh.mArray+offset;
	const LLVector4a* end = src+num_vertices;
	
	min = *src;
	max = min;
	
	
	const LLVector4a* p = profile.mArray;

	if (mTypeMask & TOP_MASK)
	{
		min_uv.set((*p)[0]+0.5f,
					(*p)[1]+0.5f);

		max_uv = min_uv;

		while(src < end)
		{
			tc->mV[0] = (*p)[0]+0.5f;
			tc->mV[1] = (*p)[1]+0.5f;

			llassert(src->isFinite3()); // MAINT-5660; don't know why this happens, does not affect Release builds
			update_min_max(min,max,*src);
			update_min_max(min_uv, max_uv, *tc);
		
			*pos = *src;
		
			llassert(pos->isFinite3());

			++p;
			++tc;
			++src;
			++pos;
		}
		}
		else
		{

		min_uv.set((*p)[0]+0.5f,
				   0.5f - (*p)[1]);
		max_uv = min_uv;

		while(src < end)
		{
			// Mirror for underside.
			tc->mV[0] = (*p)[0]+0.5f;
			tc->mV[1] = 0.5f - (*p)[1];
		
			llassert(src->isFinite3());
			update_min_max(min,max,*src);
			update_min_max(min_uv, max_uv, *tc);

			*pos = *src;
		
			llassert(pos->isFinite3());
		
			++p;
			++tc;
			++src;
			++pos;
		}
	}

	LL_CHECK_MEMORY

	mCenter->setAdd(min, max);
	mCenter->mul(0.5f); 

	cuv = (min_uv + max_uv)*0.5f;


	VertexData vd;
	vd.setPosition(*mCenter);
	vd.mTexCoord = cuv;
	
	if (!(mTypeMask & HOLLOW_MASK) && !(mTypeMask & OPEN_MASK))
	{
		*pos++ = *mCenter;
		*tc++ = cuv;
		num_vertices++;
	}
		
	LL_CHECK_MEMORY
		
	//if (partial_build)
	//{
	//	return TRUE;
	//}

	if (mTypeMask & HOLLOW_MASK)
	{
		if (mTypeMask & TOP_MASK)
		{
			// HOLLOW TOP
			// Does it matter if it's open or closed? - djs

			S32 pt1 = 0, pt2 = num_vertices - 1;
			S32 i = 0;
			while (pt2 - pt1 > 1)
			{
				// Use the profile points instead of the mesh, since you want
				// the un-transformed profile distances.
				const LLVector4a& p1 = profile[pt1];
				const LLVector4a& p2 = profile[pt2];
				const LLVector4a& pa = profile[pt1+1];
				const LLVector4a& pb = profile[pt2-1];

				const F32* p1V = p1.getF32ptr();
				const F32* p2V = p2.getF32ptr();
				const F32* paV = pa.getF32ptr();
				const F32* pbV = pb.getF32ptr();

				//p1.mV[VZ] = 0.f;
				//p2.mV[VZ] = 0.f;
				//pa.mV[VZ] = 0.f;
				//pb.mV[VZ] = 0.f;

				// Use area of triangle to determine backfacing
				F32 area_1a2, area_1ba, area_21b, area_2ab;
				area_1a2 =  (p1V[0]*paV[1] - paV[0]*p1V[1]) +
							(paV[0]*p2V[1] - p2V[0]*paV[1]) +
							(p2V[0]*p1V[1] - p1V[0]*p2V[1]);

				area_1ba =  (p1V[0]*pbV[1] - pbV[0]*p1V[1]) +
							(pbV[0]*paV[1] - paV[0]*pbV[1]) +
							(paV[0]*p1V[1] - p1V[0]*paV[1]);

				area_21b =  (p2V[0]*p1V[1] - p1V[0]*p2V[1]) +
							(p1V[0]*pbV[1] - pbV[0]*p1V[1]) +
							(pbV[0]*p2V[1] - p2V[0]*pbV[1]);

				area_2ab =  (p2V[0]*paV[1] - paV[0]*p2V[1]) +
							(paV[0]*pbV[1] - pbV[0]*paV[1]) +
							(pbV[0]*p2V[1] - p2V[0]*pbV[1]);

				BOOL use_tri1a2 = TRUE;
				BOOL tri_1a2 = TRUE;
				BOOL tri_21b = TRUE;

				if (area_1a2 < 0)
				{
					tri_1a2 = FALSE;
				}
				if (area_2ab < 0)
				{
					// Can't use, because it contains point b
					tri_1a2 = FALSE;
				}
				if (area_21b < 0)
				{
					tri_21b = FALSE;
				}
				if (area_1ba < 0)
				{
					// Can't use, because it contains point b
					tri_21b = FALSE;
				}

				if (!tri_1a2)
				{
					use_tri1a2 = FALSE;
				}
				else if (!tri_21b)
				{
					use_tri1a2 = TRUE;
				}
				else
				{
					LLVector4a d1;
					d1.setSub(p1, pa);
					
					LLVector4a d2; 
					d2.setSub(p2, pb);

					if (d1.dot3(d1) < d2.dot3(d2))
					{
						use_tri1a2 = TRUE;
					}
					else
					{
						use_tri1a2 = FALSE;
					}
				}

				if (use_tri1a2)
				{
					mIndices[i++] = pt1;
					mIndices[i++] = pt1 + 1;
					mIndices[i++] = pt2;
					pt1++;
				}
				else
				{
					mIndices[i++] = pt1;
					mIndices[i++] = pt2 - 1;
					mIndices[i++] = pt2;
					pt2--;
				}
			}
		}
		else
		{
			// HOLLOW BOTTOM
			// Does it matter if it's open or closed? - djs

			llassert(mTypeMask & BOTTOM_MASK);
			S32 pt1 = 0, pt2 = num_vertices - 1;

			S32 i = 0;
			while (pt2 - pt1 > 1)
			{
				// Use the profile points instead of the mesh, since you want
				// the un-transformed profile distances.
				const LLVector4a& p1 = profile[pt1];
				const LLVector4a& p2 = profile[pt2];
				const LLVector4a& pa = profile[pt1+1];
				const LLVector4a& pb = profile[pt2-1];

				const F32* p1V = p1.getF32ptr();
				const F32* p2V = p2.getF32ptr();
				const F32* paV = pa.getF32ptr();
				const F32* pbV = pb.getF32ptr();

				// Use area of triangle to determine backfacing
				F32 area_1a2, area_1ba, area_21b, area_2ab;
				area_1a2 =  (p1V[0]*paV[1] - paV[0]*p1V[1]) +
							(paV[0]*p2V[1] - p2V[0]*paV[1]) +
							(p2V[0]*p1V[1] - p1V[0]*p2V[1]);

				area_1ba =  (p1V[0]*pbV[1] - pbV[0]*p1V[1]) +
							(pbV[0]*paV[1] - paV[0]*pbV[1]) +
							(paV[0]*p1V[1] - p1V[0]*paV[1]);

				area_21b =  (p2V[0]*p1V[1] - p1V[0]*p2V[1]) +
							(p1V[0]*pbV[1] - pbV[0]*p1V[1]) +
							(pbV[0]*p2V[1] - p2V[0]*pbV[1]);

				area_2ab =  (p2V[0]*paV[1] - paV[0]*p2V[1]) +
							(paV[0]*pbV[1] - pbV[0]*paV[1]) +
							(pbV[0]*p2V[1] - p2V[0]*pbV[1]);

				BOOL use_tri1a2 = TRUE;
				BOOL tri_1a2 = TRUE;
				BOOL tri_21b = TRUE;

				if (area_1a2 < 0)
				{
					tri_1a2 = FALSE;
				}
				if (area_2ab < 0)
				{
					// Can't use, because it contains point b
					tri_1a2 = FALSE;
				}
				if (area_21b < 0)
				{
					tri_21b = FALSE;
				}
				if (area_1ba < 0)
				{
					// Can't use, because it contains point b
					tri_21b = FALSE;
				}

				if (!tri_1a2)
				{
					use_tri1a2 = FALSE;
				}
				else if (!tri_21b)
				{
					use_tri1a2 = TRUE;
				}
				else
				{
					LLVector4a d1;
					d1.setSub(p1,pa);
					LLVector4a d2;
					d2.setSub(p2,pb);

					if (d1.dot3(d1) < d2.dot3(d2))
					{
						use_tri1a2 = TRUE;
					}
					else
					{
						use_tri1a2 = FALSE;
					}
				}

				// Flipped backfacing from top
				if (use_tri1a2)
				{
					mIndices[i++] = pt1;
					mIndices[i++] = pt2;
					mIndices[i++] = pt1 + 1;
					pt1++;
				}
				else
				{
					mIndices[i++] = pt1;
					mIndices[i++] = pt2;
					mIndices[i++] = pt2 - 1;
					pt2--;
				}
			}
		}
	}
	else
	{
		// Not hollow, generate the triangle fan.
		U16 v1 = 2;
		U16 v2 = 1;

		if (mTypeMask & TOP_MASK)
		{
			v1 = 1;
			v2 = 2;
		}

		for (S32 i = 0; i < (num_vertices - 2); i++)
		{
			mIndices[3*i] = num_vertices - 1;
			mIndices[3*i+v1] = i;
			mIndices[3*i+v2] = i + 1;
		}


	}

	LLVector4a d0,d1;
	LL_CHECK_MEMORY
		

	d0.setSub(mPositions[mIndices[1]], mPositions[mIndices[0]]);
	d1.setSub(mPositions[mIndices[2]], mPositions[mIndices[0]]);

	LLVector4a normal;
	normal.setCross3(d0,d1);

	if (normal.dot3(normal).getF32() > F_APPROXIMATELY_ZERO)
	{
		normal.normalize3fast();
	}
	else
	{ //degenerate, make up a value
		if(normal.getF32ptr()[2] >= 0)
			normal.set(0.f,0.f,1.f);
		else
			normal.set(0.f,0.f,-1.f);
	}

	llassert(llfinite(normal.getF32ptr()[0]));
	llassert(llfinite(normal.getF32ptr()[1]));
	llassert(llfinite(normal.getF32ptr()[2]));

	llassert(!llisnan(normal.getF32ptr()[0]));
	llassert(!llisnan(normal.getF32ptr()[1]));
	llassert(!llisnan(normal.getF32ptr()[2]));
	
	for (S32 i = 0; i < num_vertices; i++)
	{
		norm[i].load4a(normal.getF32ptr());
	}

	return TRUE;
}

void CalculateTangentArray(U32 vertexCount, const LLVector4a *vertex, const LLVector4a *normal,
        const LLVector2 *texcoord, U32 triangleCount, const U16* index_array, LLVector4a *tangent);

void LLVolumeFace::createTangents()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	if (!mTangents)
	{
		allocateTangents(mNumVertices);

		//generate tangents
		//LLVector4a* pos = mPositions;
		//LLVector2* tc = (LLVector2*) mTexCoords;
		LLVector4a* binorm = (LLVector4a*) mTangents;

		LLVector4a* end = mTangents+mNumVertices;
		while (binorm < end)
		{
			(*binorm++).clear();
		}

		binorm = mTangents;

		CalculateTangentArray(mNumVertices, mPositions, mNormals, mTexCoords, mNumIndices/3, mIndices, mTangents);

		//normalize tangents
		for (U32 i = 0; i < mNumVertices; i++) 
		{
			//binorm[i].normalize3fast();
			//bump map/planar projection code requires normals to be normalized
			mNormals[i].normalize3fast();
		}
	}
}

void LLVolumeFace::resizeVertices(S32 num_verts)
{
	ll_aligned_free<64>(mPositions);
	//DO NOT free mNormals and mTexCoords as they are part of mPositions buffer
	ll_aligned_free_16(mTangents);

	mTangents = NULL;

	if (num_verts)
	{
		//pad texture coordinate block end to allow for QWORD reads
		S32 tc_size = ((num_verts*sizeof(LLVector2)) + 0xF) & ~0xF;

		mPositions = (LLVector4a*) ll_aligned_malloc<64>(sizeof(LLVector4a)*2*num_verts+tc_size);
		mNormals = mPositions+num_verts;
		mTexCoords = (LLVector2*) (mNormals+num_verts);

		ll_assert_aligned(mPositions, 64);
	}
	else
	{
		mPositions = NULL;
		mNormals = NULL;
		mTexCoords = NULL;
	}


    if (mPositions)
    {
        mNumVertices = num_verts;
        mNumAllocatedVertices = num_verts;
    }
    else
    {
        // Either num_verts is zero or allocation failure
        mNumVertices = 0;
        mNumAllocatedVertices = 0;
    }

    // Force update
    mJointRiggingInfoTab.clear();
}

void LLVolumeFace::pushVertex(const LLVolumeFace::VertexData& cv)
{
	pushVertex(cv.getPosition(), cv.getNormal(), cv.mTexCoord);
}

void LLVolumeFace::pushVertex(const LLVector4a& pos, const LLVector4a& norm, const LLVector2& tc)
{
	S32 new_verts = mNumVertices+1;

	if (new_verts > mNumAllocatedVertices)
	{ 
		// double buffer size on expansion
		new_verts *= 2;

		S32 new_tc_size = ((new_verts*8)+0xF) & ~0xF;
		S32 old_tc_size = ((mNumVertices*8)+0xF) & ~0xF;

		S32 old_vsize = mNumVertices*16;
		
		S32 new_size = new_verts*16*2+new_tc_size;

		LLVector4a* old_buf = mPositions;

		mPositions = (LLVector4a*) ll_aligned_malloc<64>(new_size);
		mNormals = mPositions+new_verts;
		mTexCoords = (LLVector2*) (mNormals+new_verts);

		if (old_buf != NULL)
		{
			// copy old positions into new buffer
			LLVector4a::memcpyNonAliased16((F32*)mPositions, (F32*)old_buf, old_vsize);

			// normals
			LLVector4a::memcpyNonAliased16((F32*)mNormals, (F32*)(old_buf + mNumVertices), old_vsize);

			// tex coords
			LLVector4a::memcpyNonAliased16((F32*)mTexCoords, (F32*)(old_buf + mNumVertices * 2), old_tc_size);
		}

		// just clear tangents
		ll_aligned_free_16(mTangents);
		mTangents = NULL;
		ll_aligned_free<64>(old_buf);

		mNumAllocatedVertices = new_verts;

	}

	mPositions[mNumVertices] = pos;
	mNormals[mNumVertices] = norm;
	mTexCoords[mNumVertices] = tc;

	mNumVertices++;	
}

void LLVolumeFace::allocateTangents(S32 num_verts)
{
	ll_aligned_free_16(mTangents);
	mTangents = (LLVector4a*) ll_aligned_malloc_16(sizeof(LLVector4a)*num_verts);
}

void LLVolumeFace::allocateWeights(S32 num_verts)
{
	ll_aligned_free_16(mWeights);
	mWeights = (LLVector4a*)ll_aligned_malloc_16(sizeof(LLVector4a)*num_verts);
    
}

void LLVolumeFace::allocateJointIndices(S32 num_verts)
{
#if USE_SEPARATE_JOINT_INDICES_AND_WEIGHTS
    ll_aligned_free_16(mJointIndices);
    ll_aligned_free_16(mJustWeights);

    mJointIndices = (U8*)ll_aligned_malloc_16(sizeof(U8) * 4 * num_verts);    
    mJustWeights = (LLVector4a*)ll_aligned_malloc_16(sizeof(LLVector4a) * num_verts);    
#endif
}

void LLVolumeFace::resizeIndices(S32 num_indices)
{
	ll_aligned_free_16(mIndices);
    llassert(num_indices % 3 == 0);
	
	if (num_indices)
	{
		//pad index block end to allow for QWORD reads
		S32 size = ((num_indices*sizeof(U16)) + 0xF) & ~0xF;
		
		mIndices = (U16*) ll_aligned_malloc_16(size);
	}
	else
	{
		mIndices = NULL;
	}

    if (mIndices)
    {
        mNumIndices = num_indices;
    }
    else
    {
        // Either num_indices is zero or allocation failure
        mNumIndices = 0;
    }
}

void LLVolumeFace::pushIndex(const U16& idx)
{
	S32 new_count = mNumIndices + 1;
	S32 new_size = ((new_count*2)+0xF) & ~0xF;

	S32 old_size = ((mNumIndices*2)+0xF) & ~0xF;
	if (new_size != old_size)
	{
		mIndices = (U16*) ll_aligned_realloc_16(mIndices, new_size, old_size);
		ll_assert_aligned(mIndices,16);
	}
	
	mIndices[mNumIndices++] = idx;
}

void LLVolumeFace::fillFromLegacyData(std::vector<LLVolumeFace::VertexData>& v, std::vector<U16>& idx)
{
	resizeVertices(v.size());
	resizeIndices(idx.size());

	for (U32 i = 0; i < v.size(); ++i)
	{
		mPositions[i] = v[i].getPosition();
		mNormals[i] = v[i].getNormal();
		mTexCoords[i] = v[i].mTexCoord;
	}

	for (U32 i = 0; i < idx.size(); ++i)
	{
		mIndices[i] = idx[i];
	}
}

BOOL LLVolumeFace::createSide(LLVolume* volume, BOOL partial_build)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

	LL_CHECK_MEMORY
	BOOL flat = mTypeMask & FLAT_MASK;

	U8 sculpt_type = volume->getParams().getSculptType();
	U8 sculpt_stitching = sculpt_type & LL_SCULPT_TYPE_MASK;
	BOOL sculpt_invert = sculpt_type & LL_SCULPT_FLAG_INVERT;
	BOOL sculpt_mirror = sculpt_type & LL_SCULPT_FLAG_MIRROR;
	BOOL sculpt_reverse_horizontal = (sculpt_invert ? !sculpt_mirror : sculpt_mirror);  // XOR
	
	S32 num_vertices, num_indices;

	const LLAlignedArray<LLVector4a,64>& mesh = volume->getMesh();
	const LLAlignedArray<LLVector4a,64>& profile = volume->getProfile().mProfile;
	const LLAlignedArray<LLPath::PathPt,64>& path_data = volume->getPath().mPath;

	S32 max_s = volume->getProfile().getTotal();

	S32 s, t, i;
	F32 ss, tt;

	num_vertices = mNumS*mNumT;
	num_indices = (mNumS-1)*(mNumT-1)*6;

	partial_build = (num_vertices > mNumVertices || num_indices > mNumIndices) ? FALSE : partial_build;

	if (!partial_build)
	{
		resizeVertices(num_vertices);
		resizeIndices(num_indices);

		if (!volume->isMeshAssetLoaded())
		{
            try
            {
                mEdge.resize(num_indices);
            }
            catch (std::bad_alloc&)
            {
                LL_WARNS("LLVOLUME") << "Resize of mEdge to " << num_indices << " failed" << LL_ENDL;
                return false;
            }
		}
	}

	LL_CHECK_MEMORY

	LLVector4a* pos = (LLVector4a*) mPositions;
	LLVector2* tc = (LLVector2*) mTexCoords;
	F32 begin_stex = floorf(profile[mBeginS][2]);
	S32 num_s = ((mTypeMask & INNER_MASK) && (mTypeMask & FLAT_MASK) && mNumS > 2) ? mNumS/2 : mNumS;

	S32 cur_vertex = 0;
	S32 end_t = mBeginT+mNumT;
	bool test = (mTypeMask & INNER_MASK) && (mTypeMask & FLAT_MASK) && mNumS > 2;

	// Copy the vertices into the array
	for (t = mBeginT; t < end_t; t++)
	{
		tt = path_data[t].mTexT;
		for (s = 0; s < num_s; s++)
		{
			if (mTypeMask & END_MASK)
			{
				if (s)
				{
					ss = 1.f;
				}
				else
				{
					ss = 0.f;
				}
			}
			else
			{
				// Get s value for tex-coord.
                S32 index = mBeginS + s;
                if (index >= profile.size())
                {
                    // edge?
                    ss = flat ? 1.f - begin_stex : 1.f;
                }
				else if (!flat)
				{
					ss = profile[index][2];
				}
				else
				{
					ss = profile[index][2] - begin_stex;
				}
			}

			if (sculpt_reverse_horizontal)
			{
				ss = 1.f - ss;
			}
			
			// Check to see if this triangle wraps around the array.
			if (mBeginS + s >= max_s)
			{
				// We're wrapping
				i = mBeginS + s + max_s*(t-1);
			}
			else
			{
				i = mBeginS + s + max_s*t;
			}

			mesh[i].store4a((F32*)(pos+cur_vertex));
			tc[cur_vertex].set(ss,tt);
		
			cur_vertex++;

			if (test && s > 0)
			{
				mesh[i].store4a((F32*)(pos+cur_vertex));
				tc[cur_vertex].set(ss,tt);
				cur_vertex++;
			}
		}
		
		if ((mTypeMask & INNER_MASK) && (mTypeMask & FLAT_MASK) && mNumS > 2)
		{
			if (mTypeMask & OPEN_MASK)
			{
				s = num_s-1;
			}
			else
			{
				s = 0;
			}

			i = mBeginS + s + max_s*t;
			ss = profile[mBeginS + s][2] - begin_stex;

			mesh[i].store4a((F32*)(pos+cur_vertex));
			tc[cur_vertex].set(ss,tt);
			
			cur_vertex++;
		}
	}	
	LL_CHECK_MEMORY
	
	mCenter->clear();

	LLVector4a* cur_pos = pos;
	LLVector4a* end_pos = pos + mNumVertices;

	//get bounding box for this side
	LLVector4a face_min;
	LLVector4a face_max;
	
	face_min = face_max = *cur_pos++;
		
	while (cur_pos < end_pos)
	{
		update_min_max(face_min, face_max, *cur_pos++);
	}
	// VFExtents change
	mExtents[0] = face_min;
	mExtents[1] = face_max;

	U32 tc_count = mNumVertices;
	if (tc_count%2 == 1)
	{ //odd number of texture coordinates, duplicate last entry to padded end of array
		tc_count++;
		mTexCoords[mNumVertices] = mTexCoords[mNumVertices-1];
	}

	LLVector4a* cur_tc = (LLVector4a*) mTexCoords;
	LLVector4a* end_tc = (LLVector4a*) (mTexCoords+tc_count);

	LLVector4a tc_min; 
	LLVector4a tc_max; 

	tc_min = tc_max = *cur_tc++;

	while (cur_tc < end_tc)
	{
		update_min_max(tc_min, tc_max, *cur_tc++);
	}

	F32* minp = tc_min.getF32ptr();
	F32* maxp = tc_max.getF32ptr();

	mTexCoordExtents[0].mV[0] = llmin(minp[0], minp[2]);
	mTexCoordExtents[0].mV[1] = llmin(minp[1], minp[3]);
	mTexCoordExtents[1].mV[0] = llmax(maxp[0], maxp[2]);
	mTexCoordExtents[1].mV[1] = llmax(maxp[1], maxp[3]);

	mCenter->setAdd(face_min, face_max);
	mCenter->mul(0.5f);

	S32 cur_index = 0;
	S32 cur_edge = 0;
	BOOL flat_face = mTypeMask & FLAT_MASK;

	if (!partial_build)
	{
		// Now we generate the indices.
		for (t = 0; t < (mNumT-1); t++)
		{
			for (s = 0; s < (mNumS-1); s++)
			{	
				mIndices[cur_index++] = s   + mNumS*t;			//bottom left
				mIndices[cur_index++] = s+1 + mNumS*(t+1);		//top right
				mIndices[cur_index++] = s   + mNumS*(t+1);		//top left
				mIndices[cur_index++] = s   + mNumS*t;			//bottom left
				mIndices[cur_index++] = s+1 + mNumS*t;			//bottom right
				mIndices[cur_index++] = s+1 + mNumS*(t+1);		//top right

				mEdge[cur_edge++] = (mNumS-1)*2*t+s*2+1;						//bottom left/top right neighbor face 
				if (t < mNumT-2) {												//top right/top left neighbor face 
					mEdge[cur_edge++] = (mNumS-1)*2*(t+1)+s*2+1;
				}
				else if (mNumT <= 3 || volume->getPath().isOpen() == TRUE) { //no neighbor
					mEdge[cur_edge++] = -1;
				}
				else { //wrap on T
					mEdge[cur_edge++] = s*2+1;
				}
				if (s > 0) {													//top left/bottom left neighbor face
					mEdge[cur_edge++] = (mNumS-1)*2*t+s*2-1;
				}
				else if (flat_face ||  volume->getProfile().isOpen() == TRUE) { //no neighbor
					mEdge[cur_edge++] = -1;
				}
				else {	//wrap on S
					mEdge[cur_edge++] = (mNumS-1)*2*t+(mNumS-2)*2+1;
				}
				
				if (t > 0) {													//bottom left/bottom right neighbor face
					mEdge[cur_edge++] = (mNumS-1)*2*(t-1)+s*2;
				}
				else if (mNumT <= 3 || volume->getPath().isOpen() == TRUE) { //no neighbor
					mEdge[cur_edge++] = -1;
				}
				else { //wrap on T
					mEdge[cur_edge++] = (mNumS-1)*2*(mNumT-2)+s*2;
				}
				if (s < mNumS-2) {												//bottom right/top right neighbor face
					mEdge[cur_edge++] = (mNumS-1)*2*t+(s+1)*2;
				}
				else if (flat_face || volume->getProfile().isOpen() == TRUE) { //no neighbor
					mEdge[cur_edge++] = -1;
				}
				else { //wrap on S
					mEdge[cur_edge++] = (mNumS-1)*2*t;
				}
				mEdge[cur_edge++] = (mNumS-1)*2*t+s*2;							//top right/bottom left neighbor face	
			}
		}
	}

	LL_CHECK_MEMORY

	//clear normals
	F32* dst = (F32*) mNormals;
	F32* end = (F32*) (mNormals+mNumVertices);
	LLVector4a zero = LLVector4a::getZero();

	while (dst < end)
	{
		zero.store4a(dst);
		dst += 4;
	}

	LL_CHECK_MEMORY

	//generate normals 
	U32 count = mNumIndices/3;

	LLVector4a* norm = mNormals;

    static thread_local LLAlignedArray<LLVector4a, 64> triangle_normals;
    try
    {
        triangle_normals.resize(count);
    }
    catch (std::bad_alloc&)
    {
        LL_WARNS("LLVOLUME") << "Resize of triangle_normals to " << count << " failed" << LL_ENDL;
        return false;
    }
	LLVector4a* output = triangle_normals.mArray;
	LLVector4a* end_output = output+count;

	U16* idx = mIndices;

	while (output < end_output)
	{
		LLVector4a b,v1,v2;
		b.load4a((F32*) (pos+idx[0]));
		v1.load4a((F32*) (pos+idx[1]));
		v2.load4a((F32*) (pos+idx[2]));
		
		//calculate triangle normal
		LLVector4a a;
		
		a.setSub(b, v1);
		b.sub(v2);


		LLQuad& vector1 = *((LLQuad*) &v1);
		LLQuad& vector2 = *((LLQuad*) &v2);
		
		LLQuad& amQ = *((LLQuad*) &a);
		LLQuad& bmQ = *((LLQuad*) &b);

		//v1.setCross3(t,v0);
		//setCross3(const LLVector4a& a, const LLVector4a& b)
		// Vectors are stored in memory in w, z, y, x order from high to low
		// Set vector1 = { a[W], a[X], a[Z], a[Y] }
		vector1 = _mm_shuffle_ps( amQ, amQ, _MM_SHUFFLE( 3, 0, 2, 1 ));
		// Set vector2 = { b[W], b[Y], b[X], b[Z] }
		vector2 = _mm_shuffle_ps( bmQ, bmQ, _MM_SHUFFLE( 3, 1, 0, 2 ));
		// mQ = { a[W]*b[W], a[X]*b[Y], a[Z]*b[X], a[Y]*b[Z] }
		vector2 = _mm_mul_ps( vector1, vector2 );
		// vector3 = { a[W], a[Y], a[X], a[Z] }
		amQ = _mm_shuffle_ps( amQ, amQ, _MM_SHUFFLE( 3, 1, 0, 2 ));
		// vector4 = { b[W], b[X], b[Z], b[Y] }
		bmQ = _mm_shuffle_ps( bmQ, bmQ, _MM_SHUFFLE( 3, 0, 2, 1 ));
		// mQ = { 0, a[X]*b[Y] - a[Y]*b[X], a[Z]*b[X] - a[X]*b[Z], a[Y]*b[Z] - a[Z]*b[Y] }
		vector1 = _mm_sub_ps( vector2, _mm_mul_ps( amQ, bmQ ));

		llassert(v1.isFinite3());

		v1.store4a((F32*) output);
		
		
		output++;
		idx += 3;
	}

	idx = mIndices;

	LLVector4a* src = triangle_normals.mArray;
	
	for (U32 i = 0; i < count; i++) //for each triangle
	{
		LLVector4a c;
		c.load4a((F32*) (src++));

		LLVector4a* n0p = norm+idx[0];
		LLVector4a* n1p = norm+idx[1];
		LLVector4a* n2p = norm+idx[2];
		
		idx += 3;

		LLVector4a n0,n1,n2;
		n0.load4a((F32*) n0p);
		n1.load4a((F32*) n1p);
		n2.load4a((F32*) n2p);
		
		n0.add(c);
		n1.add(c);
		n2.add(c);

		llassert(c.isFinite3());

		//even out quad contributions
		switch (i%2+1)
		{
			case 0: n0.add(c); break;
			case 1: n1.add(c); break;
			case 2: n2.add(c); break;
		};

		n0.store4a((F32*) n0p);
		n1.store4a((F32*) n1p);
		n2.store4a((F32*) n2p);
	}
	
	LL_CHECK_MEMORY

	// adjust normals based on wrapping and stitching
	
	LLVector4a top;
	top.setSub(pos[0], pos[mNumS*(mNumT-2)]);
	BOOL s_bottom_converges = (top.dot3(top) < 0.000001f);

	top.setSub(pos[mNumS-1], pos[mNumS*(mNumT-2)+mNumS-1]);
	BOOL s_top_converges = (top.dot3(top) < 0.000001f);

	if (sculpt_stitching == LL_SCULPT_TYPE_NONE)  // logic for non-sculpt volumes
	{
		if (volume->getPath().isOpen() == FALSE)
		{ //wrap normals on T
			for (S32 i = 0; i < mNumS; i++)
			{
				LLVector4a n;
				n.setAdd(norm[i], norm[mNumS*(mNumT-1)+i]);
				norm[i] = n;
				norm[mNumS*(mNumT-1)+i] = n;
			}
		}

		if ((volume->getProfile().isOpen() == FALSE) && !(s_bottom_converges))
		{ //wrap normals on S
			for (S32 i = 0; i < mNumT; i++)
			{
				LLVector4a n;
				n.setAdd(norm[mNumS*i], norm[mNumS*i+mNumS-1]);
				norm[mNumS * i] = n;
				norm[mNumS * i+mNumS-1] = n;
			}
		}
	
		if (volume->getPathType() == LL_PCODE_PATH_CIRCLE &&
			((volume->getProfileType() & LL_PCODE_PROFILE_MASK) == LL_PCODE_PROFILE_CIRCLE_HALF))
		{
			if (s_bottom_converges)
			{ //all lower S have same normal
				for (S32 i = 0; i < mNumT; i++)
				{
					norm[mNumS*i].set(1,0,0);
				}
			}

			if (s_top_converges)
			{ //all upper S have same normal
				for (S32 i = 0; i < mNumT; i++)
				{
					norm[mNumS*i+mNumS-1].set(-1,0,0);
				}
			}
		}
	}
	else  // logic for sculpt volumes
	{
		BOOL average_poles = FALSE;
		BOOL wrap_s = FALSE;
		BOOL wrap_t = FALSE;

		if (sculpt_stitching == LL_SCULPT_TYPE_SPHERE)
			average_poles = TRUE;

		if ((sculpt_stitching == LL_SCULPT_TYPE_SPHERE) ||
			(sculpt_stitching == LL_SCULPT_TYPE_TORUS) ||
			(sculpt_stitching == LL_SCULPT_TYPE_CYLINDER))
			wrap_s = TRUE;

		if (sculpt_stitching == LL_SCULPT_TYPE_TORUS)
			wrap_t = TRUE;
			
		
		if (average_poles)
		{
			// average normals for north pole
		
			LLVector4a average;
			average.clear();

			for (S32 i = 0; i < mNumS; i++)
			{
				average.add(norm[i]);
			}

			// set average
			for (S32 i = 0; i < mNumS; i++)
			{
				norm[i] = average;
			}

			// average normals for south pole
		
			average.clear();

			for (S32 i = 0; i < mNumS; i++)
			{
				average.add(norm[i + mNumS * (mNumT - 1)]);
			}

			// set average
			for (S32 i = 0; i < mNumS; i++)
			{
				norm[i + mNumS * (mNumT - 1)] = average;
			}

		}

		
		if (wrap_s)
		{
			for (S32 i = 0; i < mNumT; i++)
			{
				LLVector4a n;
				n.setAdd(norm[mNumS*i], norm[mNumS*i+mNumS-1]);
				norm[mNumS * i] = n;
				norm[mNumS * i+mNumS-1] = n;
			}
		}

		if (wrap_t)
		{
			for (S32 i = 0; i < mNumS; i++)
			{
				LLVector4a n;
				n.setAdd(norm[i], norm[mNumS*(mNumT-1)+i]);
				norm[i] = n;
				norm[mNumS*(mNumT-1)+i] = n;
			}
		}

	}

	LL_CHECK_MEMORY

	return TRUE;
}

//adapted from Lengyel, Eric. "Computing Tangent Space Basis Vectors for an Arbitrary Mesh". Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
void CalculateTangentArray(U32 vertexCount, const LLVector4a *vertex, const LLVector4a *normal,
        const LLVector2 *texcoord, U32 triangleCount, const U16* index_array, LLVector4a *tangent)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME

    //LLVector4a *tan1 = new LLVector4a[vertexCount * 2];
	LLVector4a* tan1 = (LLVector4a*) ll_aligned_malloc_16(vertexCount*2*sizeof(LLVector4a));
	// new(tan1) LLVector4a;

    LLVector4a* tan2 = tan1 + vertexCount;

    U32 count = vertexCount * 2;
    for (U32 i = 0; i < count; i++)
    {
        tan1[i].clear();
    }

    for (U32 a = 0; a < triangleCount; a++)
    {
        U32 i1 = *index_array++;
        U32 i2 = *index_array++;
        U32 i3 = *index_array++;
        
        const LLVector4a& v1 = vertex[i1];
        const LLVector4a& v2 = vertex[i2];
        const LLVector4a& v3 = vertex[i3];
        
        const LLVector2& w1 = texcoord[i1];
        const LLVector2& w2 = texcoord[i2];
        const LLVector2& w3 = texcoord[i3];
        
		const F32* v1ptr = v1.getF32ptr();
		const F32* v2ptr = v2.getF32ptr();
		const F32* v3ptr = v3.getF32ptr();
		
        float x1 = v2ptr[0] - v1ptr[0];
        float x2 = v3ptr[0] - v1ptr[0];
        float y1 = v2ptr[1] - v1ptr[1];
        float y2 = v3ptr[1] - v1ptr[1];
        float z1 = v2ptr[2] - v1ptr[2];
        float z2 = v3ptr[2] - v1ptr[2];
        
        float s1 = w2.mV[0] - w1.mV[0];
        float s2 = w3.mV[0] - w1.mV[0];
        float t1 = w2.mV[1] - w1.mV[1];
        float t2 = w3.mV[1] - w1.mV[1];
        
		F32 rd = s1*t2-s2*t1;

		float r = ((rd*rd) > FLT_EPSILON) ? (1.0f / rd)
													 : ((rd > 0.0f) ? 1024.f : -1024.f); //some made up large ratio for division by zero

		llassert(llfinite(r));
		llassert(!llisnan(r));

		LLVector4a sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r);
		LLVector4a tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r);
        
		tan1[i1].add(sdir);
		tan1[i2].add(sdir);
		tan1[i3].add(sdir);
        
		tan2[i1].add(tdir);
		tan2[i2].add(tdir);
		tan2[i3].add(tdir);
    }
    
    for (U32 a = 0; a < vertexCount; a++)
    {
        LLVector4a n = normal[a];

		const LLVector4a& t = tan1[a];

		LLVector4a ncrosst;
		ncrosst.setCross3(n,t);

        // Gram-Schmidt orthogonalize
        n.mul(n.dot3(t).getF32());

		LLVector4a tsubn;
		tsubn.setSub(t,n);

		if (tsubn.dot3(tsubn).getF32() > F_APPROXIMATELY_ZERO)
		{
			tsubn.normalize3fast();
		
			// Calculate handedness
			F32 handedness = ncrosst.dot3(tan2[a]).getF32() < 0.f ? -1.f : 1.f;
		
			tsubn.getF32ptr()[3] = handedness;

			tangent[a] = tsubn;
		}
		else
		{ //degenerate, make up a value
			tangent[a].set(0,0,1,1);
		}
    }
    
	ll_aligned_free_16(tan1);
}


