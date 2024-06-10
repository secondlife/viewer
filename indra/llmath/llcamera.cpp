/**
 * @file llcamera.cpp
 * @brief Implementation of the LLCamera class.
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

#include "linden_common.h"

#include "llmath.h"
#include "llcamera.h"

// ---------------- Constructors and destructors ----------------

LLCamera::LLCamera() :
    LLCoordFrame(),
    mView(DEFAULT_FIELD_OF_VIEW),
    mAspect(DEFAULT_ASPECT_RATIO),
    mViewHeightInPixels( -1 ),          // invalid height
    mNearPlane(DEFAULT_NEAR_PLANE),
    mFarPlane(DEFAULT_FAR_PLANE),
    mFixedDistance(-1.f),
    mPlaneCount(6),
    mFrustumCornerDist(0.f)
{
    for (U32 i = 0; i < PLANE_MASK_NUM; i++)
    {
        mPlaneMask[i] = PLANE_MASK_NONE;
    }

    calculateFrustumPlanes();
}

LLCamera::LLCamera(F32 vertical_fov_rads, F32 aspect_ratio, S32 view_height_in_pixels, F32 near_plane, F32 far_plane) :
    LLCoordFrame(),
    mViewHeightInPixels(view_height_in_pixels),
    mFixedDistance(-1.f),
    mPlaneCount(6),
    mFrustumCornerDist(0.f)
{
    for (U32 i = 0; i < PLANE_MASK_NUM; i++)
    {
        mPlaneMask[i] = PLANE_MASK_NONE;
    }

    mAspect = llclamp(aspect_ratio, MIN_ASPECT_RATIO, MAX_ASPECT_RATIO);
    mNearPlane = llclamp(near_plane, MIN_NEAR_PLANE, MAX_NEAR_PLANE);
    if(far_plane < 0) far_plane = DEFAULT_FAR_PLANE;
    mFarPlane = llclamp(far_plane, MIN_FAR_PLANE, MAX_FAR_PLANE);

    setView(vertical_fov_rads);
}

LLCamera::~LLCamera()
{

}

// ---------------- LLCamera::getFoo() member functions ----------------

F32 LLCamera::getMinView() const
{
    // minimum vertical fov needs to be constrained in narrow windows.
    return mAspect > 1
        ? MIN_FIELD_OF_VIEW // wide views
        : MIN_FIELD_OF_VIEW * 1/mAspect; // clamps minimum width in narrow views
}

F32 LLCamera::getMaxView() const
{
    // maximum vertical fov needs to be constrained in wide windows.
    return mAspect > 1
        ? MAX_FIELD_OF_VIEW / mAspect  // clamps maximum width in wide views
        : MAX_FIELD_OF_VIEW; // narrow views
}

LLPlane LLCamera::getUserClipPlane()
{
    return mAgentPlanes[AGENT_PLANE_USER_CLIP];
}

// ---------------- LLCamera::setFoo() member functions ----------------

void LLCamera::setUserClipPlane(LLPlane& plane)
{
    mPlaneCount = AGENT_PLANE_USER_CLIP_NUM;
    mAgentPlanes[AGENT_PLANE_USER_CLIP] = plane;
    mPlaneMask[AGENT_PLANE_USER_CLIP] = plane.calcPlaneMask();
}

void LLCamera::disableUserClipPlane()
{
    mPlaneCount = AGENT_PLANE_NO_USER_CLIP_NUM;
}

void LLCamera::setView(F32 vertical_fov_rads)
{
    mView = llclamp(vertical_fov_rads, MIN_FIELD_OF_VIEW, MAX_FIELD_OF_VIEW);
    calculateFrustumPlanes();
}

void LLCamera::setViewHeightInPixels(S32 height)
{
    mViewHeightInPixels = height;

    // Don't really need to do this, but update the pixel meter ratio with it.
    calculateFrustumPlanes();
}

void LLCamera::setAspect(F32 aspect_ratio)
{
    mAspect = llclamp(aspect_ratio, MIN_ASPECT_RATIO, MAX_ASPECT_RATIO);
    calculateFrustumPlanes();
}


void LLCamera::setNear(F32 near_plane)
{
    mNearPlane = llclamp(near_plane, MIN_NEAR_PLANE, MAX_NEAR_PLANE);
    calculateFrustumPlanes();
}


void LLCamera::setFar(F32 far_plane)
{
    mFarPlane = llclamp(far_plane, MIN_FAR_PLANE, MAX_FAR_PLANE);
    calculateFrustumPlanes();
}


// ---------------- read/write to buffer ----------------

size_t LLCamera::writeFrustumToBuffer(char *buffer) const
{
    memcpy(buffer, &mView, sizeof(F32));        /* Flawfinder: ignore */
    buffer += sizeof(F32);
    memcpy(buffer, &mAspect, sizeof(F32));      /* Flawfinder: ignore */
    buffer += sizeof(F32);
    memcpy(buffer, &mNearPlane, sizeof(F32));   /* Flawfinder: ignore */
    buffer += sizeof(F32);
    memcpy(buffer, &mFarPlane, sizeof(F32));        /* Flawfinder: ignore */
    return 4*sizeof(F32);
}

size_t LLCamera::readFrustumFromBuffer(const char *buffer)
{
    memcpy(&mView, buffer, sizeof(F32));        /* Flawfinder: ignore */
    buffer += sizeof(F32);
    memcpy(&mAspect, buffer, sizeof(F32));      /* Flawfinder: ignore */
    buffer += sizeof(F32);
    memcpy(&mNearPlane, buffer, sizeof(F32));   /* Flawfinder: ignore */
    buffer += sizeof(F32);
    memcpy(&mFarPlane, buffer, sizeof(F32));        /* Flawfinder: ignore */
    return 4*sizeof(F32);
}


// ---------------- test methods  ----------------

static  const LLVector4a sFrustumScaler[] =
{
    LLVector4a(-1,-1,-1),
    LLVector4a( 1,-1,-1),
    LLVector4a(-1, 1,-1),
    LLVector4a( 1, 1,-1),
    LLVector4a(-1,-1, 1),
    LLVector4a( 1,-1, 1),
    LLVector4a(-1, 1, 1),
    LLVector4a( 1, 1, 1)        // 8 entries
};

bool LLCamera::isChanged()
{
    bool changed = false;
    for (U32 i = 0; i < mPlaneCount; i++)
    {
        U8 mask = mPlaneMask[i];
        if (mask != 0xff && !changed)
        {
            changed = !mAgentPlanes[i].equal(mLastAgentPlanes[i]);
        }
        mLastAgentPlanes[i].set(mAgentPlanes[i]);
    }

    return changed;
}

S32 LLCamera::AABBInFrustum(const LLVector4a &center, const LLVector4a& radius, const LLPlane* planes)
{
    if(!planes)
    {
        //use agent space
        planes = mAgentPlanes;
    }

    U8 mask = 0;
    bool result = false;
    LLVector4a rscale, maxp, minp;
    LLSimdScalar d;
    U32 max_planes = llmin(mPlaneCount, (U32) AGENT_PLANE_USER_CLIP_NUM);       // mAgentPlanes[] size is 7
    for (U32 i = 0; i < max_planes; i++)
    {
        mask = mPlaneMask[i];
        if (mask < PLANE_MASK_NUM)
        {
            const LLPlane& p(planes[i]);
            p.getAt<3>(d);
            rscale.setMul(radius, sFrustumScaler[mask]);
            minp.setSub(center, rscale);
            d = -d;
            if (p.dot3(minp).getF32() > d)
            {
                return 0;
            }

            if(!result)
            {
                maxp.setAdd(center, rscale);
                result = (p.dot3(maxp).getF32() > d);
            }
        }
    }

    return result?1:2;
}

//exactly same as the function AABBInFrustum(...)
//except uses mRegionPlanes instead of mAgentPlanes.
S32 LLCamera::AABBInRegionFrustum(const LLVector4a& center, const LLVector4a& radius)
{
    return AABBInFrustum(center, radius, mRegionPlanes);
}

S32 LLCamera::AABBInFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius, const LLPlane* planes)
{
    if(!planes)
    {
        //use agent space
        planes = mAgentPlanes;
    }

    U8 mask = 0;
    bool result = false;
    LLVector4a rscale, maxp, minp;
    LLSimdScalar d;
    U32 max_planes = llmin(mPlaneCount, (U32) AGENT_PLANE_USER_CLIP_NUM);       // mAgentPlanes[] size is 7
    for (U32 i = 0; i < max_planes; i++)
    {
        mask = mPlaneMask[i];
        if ((i != 5) && (mask < PLANE_MASK_NUM))
        {
            const LLPlane& p(planes[i]);
            p.getAt<3>(d);
            rscale.setMul(radius, sFrustumScaler[mask]);
            minp.setSub(center, rscale);
            d = -d;
            if (p.dot3(minp).getF32() > d)
            {
                return 0;
            }

            if(!result)
            {
                maxp.setAdd(center, rscale);
                result = (p.dot3(maxp).getF32() > d);
            }
        }
    }

    return result?1:2;
}

//exactly same as the function AABBInFrustumNoFarClip(...)
//except uses mRegionPlanes instead of mAgentPlanes.
S32 LLCamera::AABBInRegionFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius)
{
    return AABBInFrustumNoFarClip(center, radius, mRegionPlanes);
}

int LLCamera::sphereInFrustumQuick(const LLVector3 &sphere_center, const F32 radius)
{
    LLVector3 dist = sphere_center-mFrustCenter;
    float dsq = dist * dist;
    float rsq = mFarPlane*0.5f + radius;
    rsq *= rsq;

    if (dsq < rsq)
    {
        return 1;
    }

    return 0;
}

// Return 1 if sphere is in frustum, 2 if fully in frustum, otherwise 0.
// NOTE: 'center' is in absolute frame.
int LLCamera::sphereInFrustum(const LLVector3 &sphere_center, const F32 radius) const
{
    // Returns 1 if sphere is in frustum, 0 if not.
    bool res = false;
    for (int i = 0; i < 6; i++)
    {
        if (mPlaneMask[i] != PLANE_MASK_NONE)
        {
            float d = mAgentPlanes[i].dist(sphere_center);

            if (d > radius)
            {
                return 0;
            }
            res = res || (d > -radius);
        }
    }

    return res?1:2;
}


// return height of a sphere of given radius, located at center, in pixels
F32 LLCamera::heightInPixels(const LLVector3 &center, F32 radius ) const
{
    if (radius == 0.f) return 0.f;

    // If height initialized
    if (mViewHeightInPixels > -1)
    {
        // Convert sphere to coord system with 0,0,0 at camera
        LLVector3 vec = center - mOrigin;

        // Compute distance to sphere
        F32 dist = vec.magVec();

        // Calculate angle of whole object
        F32 angle = 2.0f * (F32) atan2(radius, dist);

        // Calculate fraction of field of view
        F32 fraction_of_fov = angle / mView;

        // Compute number of pixels tall, based on vertical field of view
        return (fraction_of_fov * mViewHeightInPixels);
    }
    else
    {
        // return invalid height
        return -1.0f;
    }
}


// ---------------- friends and operators ----------------

std::ostream& operator<<(std::ostream &s, const LLCamera &C)
{
    s << "{ \n";
    s << "  Center = " << C.getOrigin() << "\n";
    s << "  AtAxis = " << C.getXAxis() << "\n";
    s << "  LeftAxis = " << C.getYAxis() << "\n";
    s << "  UpAxis = " << C.getZAxis() << "\n";
    s << "  View = " << C.getView() << "\n";
    s << "  Aspect = " << C.getAspect() << "\n";
    s << "  NearPlane   = " << C.mNearPlane << "\n";
    s << "  FarPlane    = " << C.mFarPlane << "\n";
    s << "}";
    return s;
}



// ----------------  private member functions ----------------

void LLCamera::calculateFrustumPlanes()
{
    // The planes only change when any of the frustum descriptions change.
    // They are not affected by changes of the position of the Frustum
    // because they are known in the view frame and the position merely
    // provides information on how to get from the absolute frame to the
    // view frame.

    F32 left,right,top,bottom;
    top = mFarPlane * (F32)tanf(0.5f * mView);
    bottom = -top;
    left = top * mAspect;
    right = -left;

    calculateFrustumPlanes(left, right, top, bottom);
}

LLPlane planeFromPoints(LLVector3 p1, LLVector3 p2, LLVector3 p3)
{
    LLVector3 n = ((p2-p1)%(p3-p1));
    n.normVec();

    return LLPlane(p1, n);
}


void LLCamera::ignoreAgentFrustumPlane(S32 idx)
{
    if (idx < 0 || idx > (S32) mPlaneCount)
    {
        return;
    }

    mPlaneMask[idx] = PLANE_MASK_NONE;
    mAgentPlanes[idx].clear();
}

void LLCamera::calcAgentFrustumPlanes(LLVector3* frust)
{

    for (int i = 0; i < AGENT_FRUSTRUM_NUM; i++)
    {
        mAgentFrustum[i] = frust[i];
    }

    mFrustumCornerDist = (frust[5] - getOrigin()).magVec();

    //frust contains the 8 points of the frustum, calculate 6 planes

    //order of planes is important, keep most likely to fail in the front of the list

    //near - frust[0], frust[1], frust[2]
    mAgentPlanes[AGENT_PLANE_NEAR] = planeFromPoints(frust[0], frust[1], frust[2]);

    //far
    mAgentPlanes[AGENT_PLANE_FAR] = planeFromPoints(frust[5], frust[4], frust[6]);

    //left
    mAgentPlanes[AGENT_PLANE_LEFT] = planeFromPoints(frust[4], frust[0], frust[7]);

    //right
    mAgentPlanes[AGENT_PLANE_RIGHT] = planeFromPoints(frust[1], frust[5], frust[6]);

    //top
    mAgentPlanes[AGENT_PLANE_TOP] = planeFromPoints(frust[3], frust[2], frust[6]);

    //bottom
    mAgentPlanes[AGENT_PLANE_BOTTOM] = planeFromPoints(frust[1], frust[0], frust[4]);

    //cache plane octant facing mask for use in AABBInFrustum
    for (U32 i = 0; i < mPlaneCount; i++)
    {
        mPlaneMask[i] = mAgentPlanes[i].calcPlaneMask();
    }
}

//calculate regional planes from mAgentPlanes.
//vector "shift" is the vector of the region origin in the agent space.
void LLCamera::calcRegionFrustumPlanes(const LLVector3& shift, F32 far_clip_distance)
{
    F32 far_w;
    {
        LLVector3 p = getOrigin();
        LLVector3 n(mAgentPlanes[5][0], mAgentPlanes[5][1], mAgentPlanes[5][2]);
        F32 dd = n * p;
        if(dd + mAgentPlanes[5][3] < 0) //signed distance
        {
            far_w = -far_clip_distance - dd;
        }
        else
        {
            far_w = far_clip_distance - dd;
        }
        far_w += n * shift;
    }

    F32 d;
    LLVector3 n;
    for(S32 i = 0 ; i < 7; i++)
    {
        if (mPlaneMask[i] != 0xff)
        {
            n.setVec(mAgentPlanes[i][0], mAgentPlanes[i][1], mAgentPlanes[i][2]);

            if(i != 5)
            {
                d = mAgentPlanes[i][3] + n * shift;
            }
            else
            {
                d = far_w;
            }
            mRegionPlanes[i].setVec(n, d);
        }
    }
}

void LLCamera::calculateFrustumPlanes(F32 left, F32 right, F32 top, F32 bottom)
{
    //calculate center and radius squared of frustum in world absolute coordinates
    static LLVector3 const X_AXIS(1.f, 0.f, 0.f);
    mFrustCenter = X_AXIS*mFarPlane*0.5f;
    mFrustCenter = transformToAbsolute(mFrustCenter);
    mFrustRadiusSquared = mFarPlane*0.5f;
    mFrustRadiusSquared *= mFrustRadiusSquared * 1.05f; //pad radius squared by 5%
}

// x and y are in WINDOW space, so x = Y-Axis (left/right), y= Z-Axis(Up/Down)
void LLCamera::calculateFrustumPlanesFromWindow(F32 x1, F32 y1, F32 x2, F32 y2)
{
    F32 bottom, top, left, right;
    F32 view_height = (F32)tanf(0.5f * mView) * mFarPlane;
    F32 view_width = view_height * mAspect;

    left =   x1 * -2.f * view_width;
    right =  x2 * -2.f * view_width;
    bottom = y1 * 2.f * view_height;
    top =    y2 * 2.f * view_height;

    calculateFrustumPlanes(left, right, top, bottom);
}

// NOTE: this is the OpenGL matrix that will transform the default OpenGL view
// (-Z=at, Y=up) to the default view of the LLCamera class (X=at, Z=up):
//
// F32 cfr_transform =  {  0.f,  0.f, -1.f,  0.f,   // -Z becomes X
//                        -1.f,  0.f,  0.f,  0.f,   // -X becomes Y
//                         0.f,  1.f,  0.f,  0.f,   //  Y becomes Z
//                         0.f,  0.f,  0.f,  1.f };
