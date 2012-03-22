/** 
 * @file llviewercamera.h
 * @brief LLViewerCamera class header file
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

#ifndef LL_LLVIEWERCAMERA_H
#define LL_LLVIEWERCAMERA_H

#include "llcamera.h"
#include "llsingleton.h"
#include "llstat.h"
#include "lltimer.h"
#include "m4math.h"
#include "llcoord.h"

class LLViewerObject;

// This rotation matrix moves the default OpenGL reference frame 
// (-Z at, Y up) to Cory's favorite reference frame (X at, Z up)
const F32 OGL_TO_CFR_ROTATION[16] = {  0.f,  0.f, -1.f,  0.f, 	// -Z becomes X
									  -1.f,  0.f,  0.f,  0.f, 	// -X becomes Y
									   0.f,  1.f,  0.f,  0.f,	//  Y becomes Z
									   0.f,  0.f,  0.f,  1.f };

const BOOL FOR_SELECTION = TRUE;
const BOOL NOT_FOR_SELECTION = FALSE;

// Build time optimization, generate this once in .cpp file
#ifndef LLVIEWERCAMERA_CPP
extern template class LLViewerCamera* LLSingleton<class LLViewerCamera>::getInstance();
#endif

class LLViewerCamera : public LLCamera, public LLSingleton<LLViewerCamera>
{
public:

	typedef enum
	{
		CAMERA_WORLD = 0,
		CAMERA_SHADOW0,
		CAMERA_SHADOW1,
		CAMERA_SHADOW2,
		CAMERA_SHADOW3,
		CAMERA_SHADOW4,
		CAMERA_SHADOW5,
		CAMERA_WATER0,
		CAMERA_WATER1,
		CAMERA_GI_SOURCE,
		NUM_CAMERAS
	} eCameraID;

	static U32 sCurCameraID;

	LLViewerCamera();

	void updateCameraLocation(const LLVector3 &center,
								const LLVector3 &up_direction,
								const LLVector3 &point_of_interest);

	static void updateFrustumPlanes(LLCamera& camera, BOOL ortho = FALSE, BOOL zflip = FALSE, BOOL no_hacks = FALSE);
	static void updateCameraAngle(void* user_data, const LLSD& value);
	void setPerspective(BOOL for_selection, S32 x, S32 y_from_bot, S32 width, S32 height, BOOL limit_select_distance, F32 z_near = 0, F32 z_far = 0);

	const LLMatrix4 &getProjection() const;
	const LLMatrix4 &getModelview() const;

	// Warning!  These assume the current global matrices are correct
	void projectScreenToPosAgent(const S32 screen_x, const S32 screen_y, LLVector3* pos_agent ) const;
	BOOL projectPosAgentToScreen(const LLVector3 &pos_agent, LLCoordGL &out_point, const BOOL clamp = TRUE) const;
	BOOL projectPosAgentToScreenEdge(const LLVector3 &pos_agent, LLCoordGL &out_point) const;

	const LLVector3* getVelocityDir() const {return &mVelocityDir;}
	LLStat *getVelocityStat() { return &mVelocityStat; }
	LLStat *getAngularVelocityStat() { return &mAngularVelocityStat; }
	F32     getCosHalfFov() {return mCosHalfCameraFOV;}
	F32     getAverageSpeed() {return mAverageSpeed ;}
	F32     getAverageAngularSpeed() {return mAverageAngularSpeed;}

	void getPixelVectors(const LLVector3 &pos_agent, LLVector3 &up, LLVector3 &right);
	LLVector3 roundToPixel(const LLVector3 &pos_agent);

	// Sets the current matrix
	/* virtual */ void setView(F32 vertical_fov_rads);

	void setDefaultFOV(F32 fov) ;
	F32 getDefaultFOV() { return mCameraFOVDefault; }

	BOOL cameraUnderWater() const;

	const LLVector3 &getPointOfInterest() { return mLastPointOfInterest; }
	BOOL areVertsVisible(LLViewerObject* volumep, BOOL all_verts);
	F32 getPixelMeterRatio() const				{ return mPixelMeterRatio; }
	S32 getScreenPixelArea() const				{ return mScreenPixelArea; }

	void setZoomParameters(F32 factor, S16 subregion) { mZoomFactor = factor; mZoomSubregion = subregion; }
	F32 getZoomFactor() { return mZoomFactor; }                             
	S16 getZoomSubRegion() { return mZoomSubregion; } 

protected:
	void calcProjection(const F32 far_distance) const;

	LLStat mVelocityStat;
	LLStat mAngularVelocityStat;
	LLVector3 mVelocityDir ;
	F32       mAverageSpeed ;
	F32       mAverageAngularSpeed ;

	mutable LLMatrix4	mProjectionMatrix;	// Cache of perspective matrix
	mutable LLMatrix4	mModelviewMatrix;
	F32					mCameraFOVDefault;
	F32					mCosHalfCameraFOV;
	LLVector3			mLastPointOfInterest;
	F32					mPixelMeterRatio; // Divide by distance from camera to get pixels per meter at that distance.
	S32					mScreenPixelArea; // Pixel area of entire window
	F32					mZoomFactor;
	S16					mZoomSubregion;

public:
};

#endif // LL_LLVIEWERCAMERA_H
