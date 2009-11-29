/** 
 * @file llviewercamera.h
 * @brief LLViewerCamera class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLVIEWERCAMERA_H
#define LL_LLVIEWERCAMERA_H

#include "llcamera.h"
#include "llsingleton.h"
#include "llstat.h"
#include "lltimer.h"
#include "m4math.h"

class LLCoordGL;
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
extern template class LLViewerCamera* LLSingleton<class LLViewerCamera>::getInstance();

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
