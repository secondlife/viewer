/** 
 * @file llcamera.h
 * @brief Header file for the LLCamera class.
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

#ifndef LL_CAMERA_H
#define LL_CAMERA_H


#include "llmath.h"
#include "llcoordframe.h"
#include "llplane.h"
#include "llvector4a.h"

const F32 DEFAULT_FIELD_OF_VIEW 	= 60.f * DEG_TO_RAD;
const F32 DEFAULT_ASPECT_RATIO 		= 640.f / 480.f;
const F32 DEFAULT_NEAR_PLANE 		= 0.25f;
const F32 DEFAULT_FAR_PLANE 		= 64.f;	// far reaches across two horizontal, not diagonal, regions

const F32 MAX_ASPECT_RATIO 	= 50.0f;
const F32 MAX_NEAR_PLANE 	= 1023.9f; // Clamp the near plane just before the skybox ends
const F32 MAX_FAR_PLANE 	= 100000.0f; //1000000.0f; // Max allowed. Not good Z precision though.
const F32 MAX_FAR_CLIP		= 512.0f;

const F32 MIN_ASPECT_RATIO 	= 0.02f;
const F32 MIN_NEAR_PLANE 	= 0.1f;
const F32 MIN_FAR_PLANE 	= 0.2f;

// Min/Max FOV values for square views. Call getMin/MaxView to get extremes based on current aspect ratio.
static const F32 MIN_FIELD_OF_VIEW = 5.0f * DEG_TO_RAD;
static const F32 MAX_FIELD_OF_VIEW = 175.f * DEG_TO_RAD;

// An LLCamera is an LLCoorFrame with a view frustum.
// This means that it has several methods for moving it around 
// that are inherited from the LLCoordFrame() class :
//
// setOrigin(), setAxes()
// translate(), rotate()
// roll(), pitch(), yaw()
// etc...

LL_ALIGN_PREFIX(16)
class LLCamera
: 	public LLCoordFrame
{
public:
	
	LLCamera(const LLCamera& rhs)
	{
		*this = rhs;
	}
	
	enum {
		PLANE_LEFT = 0,
		PLANE_RIGHT = 1,
		PLANE_BOTTOM = 2,
		PLANE_TOP = 3,
		PLANE_NUM = 4,
		PLANE_MASK_NONE = 0xff		// Disable this plane
	};
	enum {
		PLANE_LEFT_MASK = (1<<PLANE_LEFT),
		PLANE_RIGHT_MASK = (1<<PLANE_RIGHT),
		PLANE_BOTTOM_MASK = (1<<PLANE_BOTTOM),
		PLANE_TOP_MASK = (1<<PLANE_TOP),
		PLANE_ALL_MASK = 0xf,
	};

	enum
	{	// Indexes to mAgentPlanes[] and mPlaneMask[]
		AGENT_PLANE_LEFT = 0,
		AGENT_PLANE_RIGHT = 1,
		AGENT_PLANE_NEAR = 2,
		AGENT_PLANE_BOTTOM = 3,
		AGENT_PLANE_TOP = 4,
		AGENT_PLANE_FAR = 5,
		AGENT_PLANE_USER_CLIP = 6
	};
	enum
	{	// Sizes for mAgentPlanes[].  7th entry is special case for user clip
		AGENT_PLANE_NO_USER_CLIP_NUM = 6,
		AGENT_PLANE_USER_CLIP_NUM = 7,
		PLANE_MASK_NUM = 8			// 7 actually used, 8 is for alignment
	};

	enum
	{
		AGENT_FRUSTRUM_NUM = 8
	};
	
	enum {
		HORIZ_PLANE_LEFT = 0,
		HORIZ_PLANE_RIGHT = 1,
		HORIZ_PLANE_NUM = 2
	};
	enum {
		HORIZ_PLANE_LEFT_MASK = (1<<HORIZ_PLANE_LEFT),
		HORIZ_PLANE_RIGHT_MASK = (1<<HORIZ_PLANE_RIGHT),
		HORIZ_PLANE_ALL_MASK = 0x3
	};

private:
	LL_ALIGN_16(LLPlane mAgentPlanes[AGENT_PLANE_USER_CLIP_NUM]);  //frustum planes in agent space a la gluUnproject (I'm a bastard, I know) - DaveP
	LL_ALIGN_16(LLPlane mRegionPlanes[AGENT_PLANE_USER_CLIP_NUM]);  //frustum planes in a local region space, derived from mAgentPlanes
	LL_ALIGN_16(LLPlane mLastAgentPlanes[AGENT_PLANE_USER_CLIP_NUM]);
	U8 mPlaneMask[PLANE_MASK_NUM];         // 8 for alignment	
	
	F32 mView;					// angle between top and bottom frustum planes in radians.
	F32 mAspect;				// width/height
	S32 mViewHeightInPixels;	// for ViewHeightInPixels() only
	F32 mNearPlane;
	F32 mFarPlane;
	F32 mFixedDistance;			// Always return this distance, unless < 0
	LLVector3 mFrustCenter;		// center of frustum and radius squared for ultra-quick exclusion test
	F32 mFrustRadiusSquared;
	
	U32 mPlaneCount;  //defaults to 6, if setUserClipPlane is called, uses user supplied clip plane in

	LLVector3 mWorldPlanePos;		// Position of World Planes (may be offset from camera)
public:
	LLVector3 mAgentFrustum[AGENT_FRUSTRUM_NUM];  //8 corners of 6-plane frustum
	F32	mFrustumCornerDist;		//distance to corner of frustum against far clip plane
	LLPlane& getAgentPlane(U32 idx) { return mAgentPlanes[idx]; }

public:
	LLCamera();
	LLCamera(F32 vertical_fov_rads, F32 aspect_ratio, S32 view_height_in_pixels, F32 near_plane, F32 far_plane);
	virtual ~LLCamera();
	
	bool isChanged(); //check if mAgentPlanes changed since last frame.

    LLPlane getUserClipPlane();
	void setUserClipPlane(LLPlane& plane);
	void disableUserClipPlane();
	virtual void setView(F32 vertical_fov_rads);
	void setViewHeightInPixels(S32 height);
	void setAspect(F32 new_aspect);
	void setNear(F32 new_near);
	void setFar(F32 new_far);

	F32 getView() const							{ return mView; }				// vertical FOV in radians
	S32 getViewHeightInPixels() const			{ return mViewHeightInPixels; }
	F32 getAspect() const						{ return mAspect; }				// width / height
	F32 getNear() const							{ return mNearPlane; }			// meters
	F32 getFar() const							{ return mFarPlane; }			// meters

	// The values returned by the min/max view getters depend upon the aspect ratio
	// at the time they are called and therefore should not be cached.
	F32 getMinView() const;
	F32 getMaxView() const;
	
	F32 getYaw() const
	{
		return atan2f(mXAxis[VY], mXAxis[VX]);
	}
	F32 getPitch() const
	{
		F32 xylen = sqrtf(mXAxis[VX]*mXAxis[VX] + mXAxis[VY]*mXAxis[VY]);
		return atan2f(mXAxis[VZ], xylen);
	}

	const LLVector3& getWorldPlanePos() const		{ return mWorldPlanePos; }
	
	// Copy mView, mAspect, mNearPlane, and mFarPlane to buffer.
	// Return number of bytes copied.
	size_t writeFrustumToBuffer(char *buffer) const;

	// Copy mView, mAspect, mNearPlane, and mFarPlane from buffer.
	// Return number of bytes copied.
	size_t readFrustumFromBuffer(const char *buffer);
	void calcAgentFrustumPlanes(LLVector3* frust);
	void calcRegionFrustumPlanes(const LLVector3& shift, F32 far_clip_distance); //calculate regional planes from mAgentPlanes.
	void ignoreAgentFrustumPlane(S32 idx);

	// Returns 1 if partly in, 2 if fully in.
	// NOTE: 'center' is in absolute frame.
	S32 sphereInFrustum(const LLVector3 &center, const F32 radius) const;
	S32 pointInFrustum(const LLVector3 &point) const { return sphereInFrustum(point, 0.0f); }
	S32 sphereInFrustumFull(const LLVector3 &center, const F32 radius) const { return sphereInFrustum(center, radius); }
	S32 AABBInFrustum(const LLVector4a& center, const LLVector4a& radius, const LLPlane* planes = NULL);
	S32 AABBInRegionFrustum(const LLVector4a& center, const LLVector4a& radius);
	S32 AABBInFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius, const LLPlane* planes = NULL);
	S32 AABBInRegionFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius);

	//does a quick 'n dirty sphere-sphere check
	S32 sphereInFrustumQuick(const LLVector3 &sphere_center, const F32 radius); 

	// Returns height of object in pixels (must be height because field of view
	// is based on window height).
	F32 heightInPixels(const LLVector3 &center, F32 radius ) const;

	// return the distance from pos to camera if visible (-distance if not visible)
	void setFixedDistance(F32 distance) { mFixedDistance = distance; }
	
	friend std::ostream& operator<<(std::ostream &s, const LLCamera &C);

protected:
	void calculateFrustumPlanes();
	void calculateFrustumPlanes(F32 left, F32 right, F32 top, F32 bottom);
	void calculateFrustumPlanesFromWindow(F32 x1, F32 y1, F32 x2, F32 y2);
} LL_ALIGN_POSTFIX(16);


#endif



