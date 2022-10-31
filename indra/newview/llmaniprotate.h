/** 
 * @file llmaniprotate.h
 * @brief LLManipRotate class definition
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

#ifndef LL_LLMANIPROTATE_H
#define LL_LLMANIPROTATE_H

#include "lltool.h"
#include "v3math.h"
#include "v4math.h"
#include "llquaternion.h"
#include "llregionposition.h"
#include "llmanip.h"
#include "llviewerobject.h"

class LLToolComposite;
class LLColor4;

class LLManipRotate : public LLManip
{
public:
    class ManipulatorHandle
    {
    public:
        LLVector3   mAxisU;
        LLVector3   mAxisV;
        U32         mManipID;

        ManipulatorHandle(LLVector3 axis_u, LLVector3 axis_v, U32 id) : mAxisU(axis_u), mAxisV(axis_v), mManipID(id){}
    };
    
    LLManipRotate( LLToolComposite* composite );

    virtual BOOL    handleMouseDown( S32 x, S32 y, MASK mask );
    virtual BOOL    handleMouseUp( S32 x, S32 y, MASK mask );
    virtual BOOL    handleHover( S32 x, S32 y, MASK mask );
    virtual void    render();

    virtual void    handleSelect();

    virtual BOOL    handleMouseDownOnPart(S32 x, S32 y, MASK mask);
    virtual void    highlightManipulators(S32 x, S32 y);
    virtual BOOL    canAffectSelection();
    
private:
    void            updateHoverView();

    void            drag( S32 x, S32 y );
    LLVector3       projectToSphere( F32 x, F32 y, BOOL* on_sphere );

    void            renderSnapGuides();
    void            renderActiveRing(F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color);

    BOOL            updateVisiblity();
    LLVector3       findNearestPointOnRing( S32 x, S32 y, const LLVector3& center, const LLVector3& axis );

    LLQuaternion    dragUnconstrained( S32 x, S32 y );
    LLQuaternion    dragConstrained( S32 x, S32 y );
    LLVector3       getConstraintAxis();
    S32             getObjectAxisClosestToMouse(LLVector3& axis);

    // Utility functions
    static void         mouseToRay( S32 x, S32 y, LLVector3* ray_pt, LLVector3* ray_dir );
    static LLVector3    intersectMouseWithSphere( S32 x, S32 y, const LLVector3& sphere_center, F32 sphere_radius );
    static LLVector3    intersectRayWithSphere( const LLVector3& ray_pt, const LLVector3& ray_dir, const LLVector3& sphere_center, F32 sphere_radius);

private:
    LLVector3d          mRotationCenter;            
    LLCoordGL           mCenterScreen;
//  S32                 mLastHoverMouseX;       // used to suppress hover if mouse doesn't move
//  S32                 mLastHoverMouseY;
    LLQuaternion        mRotation;
    
    LLVector3           mMouseDown;
    LLVector3           mMouseCur;
    LLVector3           mAgentSelfAtAxis; // Own agent uses separate rotation method
    F32                 mRadiusMeters;
    
    LLVector3           mCenterToCam;
    LLVector3           mCenterToCamNorm;
    F32                 mCenterToCamMag;
    LLVector3           mCenterToProfilePlane;
    F32                 mCenterToProfilePlaneMag;

    BOOL                mSendUpdateOnMouseUp;

    BOOL                mSmoothRotate;
    BOOL                mCamEdgeOn;

    LLVector4           mManipulatorVertices[6];
    LLVector4           mManipulatorScales;
};

#endif  // LL_LLMANIPROTATE_H
