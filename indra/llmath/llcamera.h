/** 
 * @file llcamera.h
 * @brief Header file for the LLCamera class.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_CAMERA_H
#define LL_CAMERA_H


#include "llmath.h"
#include "llcoordframe.h"
#include "llplane.h"

const F32 DEFAULT_FIELD_OF_VIEW 	= 60.f * DEG_TO_RAD;
const F32 DEFAULT_ASPECT_RATIO 		= 640.f / 480.f;
const F32 DEFAULT_NEAR_PLANE 		= 0.25f;
const F32 DEFAULT_FAR_PLANE 		= 64.f;	// far reaches across two horizontal, not diagonal, regions

const F32 MAX_FIELD_OF_VIEW = F_PI;
const F32 MAX_ASPECT_RATIO 	= 50.0f;
const F32 MAX_NEAR_PLANE 	= 10.f;
const F32 MAX_FAR_PLANE 	= 100000.0f; //1000000.0f; // Max allowed. Not good Z precision though.
const F32 MAX_FAR_CLIP		= 1024.0f;

const F32 MIN_FIELD_OF_VIEW = 0.1f;
const F32 MIN_ASPECT_RATIO 	= 0.02f;
const F32 MIN_NEAR_PLANE 	= 0.1f;
const F32 MIN_FAR_PLANE 	= 0.2f;

static const LLVector3 X_AXIS(1.f,0.f,0.f);
static const LLVector3 Y_AXIS(0.f,1.f,0.f);
static const LLVector3 Z_AXIS(0.f,0.f,1.f);

static const LLVector3 NEG_X_AXIS(-1.f,0.f,0.f);
static const LLVector3 NEG_Y_AXIS(0.f,-1.f,0.f);
static const LLVector3 NEG_Z_AXIS(0.f,0.f,-1.f);


// An LLCamera is an LLCoorFrame with a view frustum.
// This means that it has several methods for moving it around 
// that are inherited from the LLCoordFrame() class :
//
// setOrigin(), setAxes()
// translate(), rotate()
// roll(), pitch(), yaw()
// etc...


class LLCamera
: 	public LLCoordFrame
{
public:
	enum {
		PLANE_LEFT = 0,
		PLANE_RIGHT = 1,
		PLANE_BOTTOM = 2,
		PLANE_TOP = 3,
		PLANE_NUM = 4
	};
	enum {
		PLANE_LEFT_MASK = (1<<PLANE_LEFT),
		PLANE_RIGHT_MASK = (1<<PLANE_RIGHT),
		PLANE_BOTTOM_MASK = (1<<PLANE_BOTTOM),
		PLANE_TOP_MASK = (1<<PLANE_TOP),
		PLANE_ALL_MASK = 0xf
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

protected:
	F32 mView;					// angle between top and bottom frustum planes in radians.
	F32 mAspect;				// width/height
	S32 mViewHeightInPixels;	// for ViewHeightInPixels() only
	F32 mNearPlane;
	F32 mFarPlane;
	LLPlane mLocalPlanes[4];
	F32 mFixedDistance;			// Always return this distance, unless < 0
	LLVector3 mFrustCenter;		// center of frustum and radius squared for ultra-quick exclusion test
	F32 mFrustRadiusSquared;
	
	LLPlane mWorldPlanes[PLANE_NUM];
	LLPlane mHorizPlanes[HORIZ_PLANE_NUM];
	LLPlane mAgentPlanes[6];		//frustum in agent space a la gluUnproject (I'm a bastard, I know) - DaveP
	U8 mAgentPlaneMask[6];
	LLVector3 mWorldPlanePos;		// Position of World Planes (may be offset from camera)
public:
	LLVector3 mAgentFrustum[8];
	
public:
	LLCamera();
	LLCamera(F32 z_field_of_view, F32 aspect_ratio, S32 view_height_in_pixels, F32 near_plane, F32 far_plane);

	void setView(F32 new_view);
	void setViewHeightInPixels(S32 height);
	void setAspect(F32 new_aspect);
	void setNear(F32 new_near);
	void setFar(F32 new_far);

	F32 getView() const							{ return mView; }				// vertical FOV in radians
	S32 getViewHeightInPixels() const			{ return mViewHeightInPixels; }
	F32 getAspect() const						{ return mAspect; }				// width / height
	F32 getNear() const							{ return mNearPlane; }			// meters
	F32 getFar() const							{ return mFarPlane; }			// meters
	
	F32 getYaw() const
	{
		return atan2f(mXAxis[VY], mXAxis[VX]);
	}
	F32 getPitch() const
	{
		F32 xylen = sqrtf(mXAxis[VX]*mXAxis[VX] + mXAxis[VY]*mXAxis[VY]);
		return atan2f(mXAxis[VZ], xylen);
	}

	const LLPlane& getWorldPlane(S32 index) const	{ return mWorldPlanes[index]; }
	const LLVector3& getWorldPlanePos() const		{ return mWorldPlanePos; }
	
	// Copy mView, mAspect, mNearPlane, and mFarPlane to buffer.
	// Return number of bytes copied.
	size_t writeFrustumToBuffer(char *buffer) const;

	// Copy mView, mAspect, mNearPlane, and mFarPlane from buffer.
	// Return number of bytes copied.
	size_t readFrustumFromBuffer(const char *buffer);
	void calcAgentFrustumPlanes(LLVector3* frust);
	// Returns 1 if partly in, 2 if fully in.
	// NOTE: 'center' is in absolute frame.
	S32 sphereInFrustumOld(const LLVector3 &center, const F32 radius) const;
	S32 sphereInFrustum(const LLVector3 &center, const F32 radius) const;
	S32 pointInFrustum(const LLVector3 &point) const { return sphereInFrustum(point, 0.0f); }
	S32 sphereInFrustumFull(const LLVector3 &center, const F32 radius) const { return sphereInFrustum(center, radius); }
	S32 AABBInFrustum(const LLVector3 &center, const LLVector3& radius);
	//does a quick 'n dirty sphere-sphere check
	S32 sphereInFrustumQuick(const LLVector3 &sphere_center, const F32 radius); 

	// Returns height of object in pixels (must be height because field of view
	// is based on window height).
	F32 heightInPixels(const LLVector3 &center, F32 radius ) const;

	// return the distance from pos to camera if visible (-distance if not visible)
	F32 visibleDistance(const LLVector3 &pos, F32 rad, F32 fudgescale = 1.0f, U32 planemask = PLANE_ALL_MASK) const;
	F32 visibleHorizDistance(const LLVector3 &pos, F32 rad, F32 fudgescale = 1.0f, U32 planemask = HORIZ_PLANE_ALL_MASK) const;
	void setFixedDistance(F32 distance) { mFixedDistance = distance; }
	
	friend std::ostream& operator<<(std::ostream &s, const LLCamera &C);

protected:
	void calculateFrustumPlanes();
	void calculateFrustumPlanes(F32 left, F32 right, F32 top, F32 bottom);
	void calculateFrustumPlanesFromWindow(F32 x1, F32 y1, F32 x2, F32 y2);
	void calculateWorldFrustumPlanes();
};


#endif



