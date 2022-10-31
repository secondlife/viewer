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
#include "lltimer.h"
#include "m4math.h"
#include "llcoord.h"
#include "lltrace.h"

class LLViewerObject;
const BOOL FOR_SELECTION = TRUE;
const BOOL NOT_FOR_SELECTION = FALSE;

class alignas(16) LLViewerCamera : public LLCamera, public LLSimpleton<LLViewerCamera>
{
    LL_ALIGN_NEW
public:
    LLViewerCamera();

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

    static eCameraID sCurCameraID;

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

    LLVector3 getVelocityDir() const {return mVelocityDir;}
    static LLTrace::CountStatHandle<>* getVelocityStat()           {return &sVelocityStat; }
    static LLTrace::CountStatHandle<>* getAngularVelocityStat()  {return &sAngularVelocityStat; }
    F32     getCosHalfFov() {return mCosHalfCameraFOV;}
    F32     getAverageSpeed() {return mAverageSpeed ;}
    F32     getAverageAngularSpeed() {return mAverageAngularSpeed;}

    void getPixelVectors(const LLVector3 &pos_agent, LLVector3 &up, LLVector3 &right);
    LLVector3 roundToPixel(const LLVector3 &pos_agent);

    // Sets the current matrix
    /* virtual */ void setView(F32 vertical_fov_rads);

    void setDefaultFOV(F32 fov) ;
    F32 getDefaultFOV() { return mCameraFOVDefault; }

    BOOL isDefaultFOVChanged();

    BOOL cameraUnderWater() const;
    BOOL areVertsVisible(LLViewerObject* volumep, BOOL all_verts);

    const LLVector3 &getPointOfInterest() { return mLastPointOfInterest; }
    F32 getPixelMeterRatio() const              { return mPixelMeterRatio; }
    S32 getScreenPixelArea() const              { return mScreenPixelArea; }

    void setZoomParameters(F32 factor, S16 subregion) { mZoomFactor = factor; mZoomSubregion = subregion; }
    F32 getZoomFactor() { return mZoomFactor; }                             
    S16 getZoomSubRegion() { return mZoomSubregion; } 

protected:
    void calcProjection(const F32 far_distance) const;

    static LLTrace::CountStatHandle<> sVelocityStat;
    static LLTrace::CountStatHandle<> sAngularVelocityStat;

    LLVector3 mVelocityDir ;
    F32       mAverageSpeed ;
    F32       mAverageAngularSpeed ;
    mutable LLMatrix4   mProjectionMatrix;  // Cache of perspective matrix
    mutable LLMatrix4   mModelviewMatrix;
    F32                 mCameraFOVDefault;
    F32                 mPrevCameraFOVDefault;
    F32                 mCosHalfCameraFOV;
    LLVector3           mLastPointOfInterest;
    F32                 mPixelMeterRatio; // Divide by distance from camera to get pixels per meter at that distance.
    S32                 mScreenPixelArea; // Pixel area of entire window
    F32                 mZoomFactor;
    S16                 mZoomSubregion;

public:
};


#endif // LL_LLVIEWERCAMERA_H
