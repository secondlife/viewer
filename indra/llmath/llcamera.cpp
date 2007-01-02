/** 
 * @file llcamera.cpp
 * @brief Implementation of the LLCamera class.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llmath.h"
#include "llcamera.h"

// ---------------- Constructors and destructors ----------------

LLCamera::LLCamera() :
	LLCoordFrame(),
	mView(DEFAULT_FIELD_OF_VIEW),
	mAspect(DEFAULT_ASPECT_RATIO),
	mViewHeightInPixels( -1 ),			// invalid height
	mNearPlane(DEFAULT_NEAR_PLANE),
	mFarPlane(DEFAULT_FAR_PLANE),
	mFixedDistance(-1.f)
{
	calculateFrustumPlanes();
} 


LLCamera::LLCamera(F32 z_field_of_view, F32 aspect_ratio, S32 view_height_in_pixels, F32 near_plane, F32 far_plane) :
	LLCoordFrame(),
	mView(z_field_of_view),
	mAspect(aspect_ratio),
	mViewHeightInPixels(view_height_in_pixels),
	mNearPlane(near_plane),
	mFarPlane(far_plane),
	mFixedDistance(-1.f)
{
	if (mView < MIN_FIELD_OF_VIEW) 			{ mView = MIN_FIELD_OF_VIEW; }
	else if (mView > MAX_FIELD_OF_VIEW)		{ mView = MAX_FIELD_OF_VIEW; }

	if (mAspect < MIN_ASPECT_RATIO)			{ mAspect = MIN_ASPECT_RATIO; }
	else if (mAspect > MAX_ASPECT_RATIO)	{ mAspect = MAX_ASPECT_RATIO; }

	if (mNearPlane < MIN_NEAR_PLANE)		{ mNearPlane = MIN_NEAR_PLANE; }
	else if (mNearPlane > MAX_NEAR_PLANE)	{ mNearPlane = MAX_NEAR_PLANE; }

	if (mFarPlane < 0) 						{ mFarPlane = DEFAULT_FAR_PLANE; }
	else if (mFarPlane < MIN_FAR_PLANE)		{ mFarPlane = MIN_FAR_PLANE; }
	else if (mFarPlane > MAX_FAR_PLANE)		{ mFarPlane = MAX_FAR_PLANE; }

	calculateFrustumPlanes();
} 



// ---------------- LLCamera::setFoo() member functions ----------------

void LLCamera::setView(F32 field_of_view) 
{
	mView = field_of_view;
	if (mView < MIN_FIELD_OF_VIEW) 			{ mView = MIN_FIELD_OF_VIEW; }
	else if (mView > MAX_FIELD_OF_VIEW)		{ mView = MAX_FIELD_OF_VIEW; }
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
	mAspect = aspect_ratio;
	if (mAspect < MIN_ASPECT_RATIO)			{ mAspect = MIN_ASPECT_RATIO; }
	else if (mAspect > MAX_ASPECT_RATIO)	{ mAspect = MAX_ASPECT_RATIO; }
	calculateFrustumPlanes();
}


void LLCamera::setNear(F32 near_plane) 
{
	mNearPlane = near_plane;
	if (mNearPlane < MIN_NEAR_PLANE)		{ mNearPlane = MIN_NEAR_PLANE; }
	else if (mNearPlane > MAX_NEAR_PLANE)	{ mNearPlane = MAX_NEAR_PLANE; }
	calculateFrustumPlanes();
}


void LLCamera::setFar(F32 far_plane) 
{
	mFarPlane = far_plane;
	if (mFarPlane < MIN_FAR_PLANE)			{ mFarPlane = MIN_FAR_PLANE; }
	else if (mFarPlane > MAX_FAR_PLANE)		{ mFarPlane = MAX_FAR_PLANE; }
	calculateFrustumPlanes();
}


// ---------------- read/write to buffer ---------------- 

size_t LLCamera::writeFrustumToBuffer(char *buffer) const
{
	memcpy(buffer, &mView, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(buffer, &mAspect, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(buffer, &mNearPlane, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(buffer, &mFarPlane, sizeof(F32));
	return 4*sizeof(F32);
}

size_t LLCamera::readFrustumFromBuffer(const char *buffer)
{
	memcpy(&mView, buffer, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(&mAspect, buffer, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(&mNearPlane, buffer, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(&mFarPlane, buffer, sizeof(F32));
	return 4*sizeof(F32);
}


// ---------------- test methods  ---------------- 

int LLCamera::AABBInFrustum(const LLVector3 &center, const LLVector3& radius) 
{
	static const LLVector3 scaler[] = {
		LLVector3(-1,-1,-1),
		LLVector3( 1,-1,-1),
		LLVector3(-1, 1,-1),
		LLVector3( 1, 1,-1),
		LLVector3(-1,-1, 1),
		LLVector3( 1,-1, 1),
		LLVector3(-1, 1, 1),
		LLVector3( 1, 1, 1)
	};

	U8 mask = 0;
	S32 result = 2;

	for (int i = 0; i < 6; i++)
	{
		mask = mAgentPlaneMask[i];
		LLPlane p = mAgentPlanes[i];
		LLVector3 n = LLVector3(p);
		float d = p.mV[3];
		LLVector3 rscale = radius.scaledVec(scaler[mask]);

		LLVector3 minp = center - rscale;
		LLVector3 maxp = center + rscale;

		if (n * minp > -d) 
		{
			return 0;
		}
	
		if (n * maxp > -d)
		{
			result = 1;
		}
	}

	return result;
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

// HACK: This version is still around because the version below doesn't work
// unless the agent planes are initialized.
// Return 1 if sphere is in frustum, 2 if fully in frustum, otherwise 0.
// NOTE: 'center' is in absolute frame.
int LLCamera::sphereInFrustumOld(const LLVector3 &sphere_center, const F32 radius) const 
{
	// Returns 1 if sphere is in frustum, 0 if not.
	// modified so that default view frust is along X with Z vertical
	F32 x, y, z, rightDist, leftDist, topDist, bottomDist;

	// Subtract the view position 
	//LLVector3 relative_center;
	//relative_center = sphere_center - getOrigin();
	LLVector3 rel_center(sphere_center);
	rel_center -= mOrigin;

	bool all_in = TRUE;

	// Transform relative_center.x to camera frame
	x = mXAxis * rel_center;
	if (x < MIN_NEAR_PLANE - radius)
	{
		return 0;
	}
	else if (x < MIN_NEAR_PLANE + radius)
	{
		all_in = FALSE;
	}

	if (x > mFarPlane + radius)
	{
		return 0;
	}
	else if (x > mFarPlane - radius)
	{
		all_in = FALSE;
	}

	// Transform relative_center.y to camera frame
	y = mYAxis * rel_center;

	// distance to plane is the dot product of (x, y, 0) * plane_normal
	rightDist = x * mLocalPlanes[PLANE_RIGHT][VX] + y * mLocalPlanes[PLANE_RIGHT][VY];
	if (rightDist < -radius)
	{
		return 0;
	}
	else if (rightDist < radius)
	{
		all_in = FALSE;
	}

	leftDist = x * mLocalPlanes[PLANE_LEFT][VX] + y * mLocalPlanes[PLANE_LEFT][VY];
	if (leftDist < -radius)
	{
		return 0;
	}
	else if (leftDist < radius)
	{
		all_in = FALSE;
	}

	// Transform relative_center.y to camera frame
	z = mZAxis * rel_center;

	topDist = x * mLocalPlanes[PLANE_TOP][VX] + z * mLocalPlanes[PLANE_TOP][VZ];
	if (topDist < -radius)
	{
		return 0;
	}
	else if (topDist < radius)
	{
		all_in = FALSE;
	}

	bottomDist = x * mLocalPlanes[PLANE_BOTTOM][VX] + z * mLocalPlanes[PLANE_BOTTOM][VZ];
	if (bottomDist < -radius)
	{
		return 0;
	}
	else if (bottomDist < radius)
	{
		all_in = FALSE;
	}

	if (all_in)
	{
		return 2;
	}

	return 1;
}


// HACK: This (presumably faster) version only currently works if you set up the
// frustum planes using GL.  At some point we should get those planes through another
// mechanism, and then we can get rid of the "old" version above.

// Return 1 if sphere is in frustum, 2 if fully in frustum, otherwise 0.
// NOTE: 'center' is in absolute frame.
int LLCamera::sphereInFrustum(const LLVector3 &sphere_center, const F32 radius) const 
{
	// Returns 1 if sphere is in frustum, 0 if not.
	int res = 2;
	for (int i = 0; i < 6; i++)
	{
		float d = mAgentPlanes[i].dist(sphere_center);

		if (d > radius) 
		{
			return 0;
		}

		if (d > -radius)
		{
			res = 1;
		}
	}

	return res;
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

// If pos is visible, return the distance from pos to the camera.
// Use fudge distance to scale rad against top/bot/left/right planes
// Otherwise, return -distance
F32 LLCamera::visibleDistance(const LLVector3 &pos, F32 rad, F32 fudgedist, U32 planemask) const
{
	if (mFixedDistance > 0)
	{
		return mFixedDistance;
	}
	LLVector3 dvec = pos - mOrigin;
	// Check visibility
	F32 dist = dvec.magVec();
	if (dist > rad)
	{
 		F32 dp,tdist;
 		dp = dvec * mXAxis;
  		if (dp < -rad)
  			return -dist;

		rad *= fudgedist;
		LLVector3 tvec(pos);
		for (int p=0; p<PLANE_NUM; p++)
		{
			if (!(planemask & (1<<p)))
				continue;
			tdist = -(mWorldPlanes[p].dist(tvec));
			if (tdist > rad)
				return -dist;
		}
	}
	return dist;
}

// Like visibleDistance, except uses mHorizPlanes[], which are left and right
//  planes perpindicular to (0,0,1) in world space
F32 LLCamera::visibleHorizDistance(const LLVector3 &pos, F32 rad, F32 fudgedist, U32 planemask) const
{
	if (mFixedDistance > 0)
	{
		return mFixedDistance;
	}
	LLVector3 dvec = pos - mOrigin;
	// Check visibility
	F32 dist = dvec.magVec();
	if (dist > rad)
	{
		rad *= fudgedist;
		LLVector3 tvec(pos);
		for (int p=0; p<HORIZ_PLANE_NUM; p++)
		{
			if (!(planemask & (1<<p)))
				continue;
			F32 tdist = -(mHorizPlanes[p].dist(tvec));
			if (tdist > rad)
				return -dist;
		}
	}
	return dist;
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
	s << "  TopPlane    = " << C.mLocalPlanes[LLCamera::PLANE_TOP][VX] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_TOP][VY] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_TOP][VZ] << "\n";
	s << "  BottomPlane = " << C.mLocalPlanes[LLCamera::PLANE_BOTTOM][VX] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_BOTTOM][VY] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_BOTTOM][VZ] << "\n";
	s << "  LeftPlane   = " << C.mLocalPlanes[LLCamera::PLANE_LEFT][VX] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_LEFT][VY] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_LEFT][VZ] << "\n";
	s << "  RightPlane  = " << C.mLocalPlanes[LLCamera::PLANE_RIGHT][VX] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_RIGHT][VY] << "  " 
							<< C.mLocalPlanes[LLCamera::PLANE_RIGHT][VZ] << "\n";
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


void LLCamera::calcAgentFrustumPlanes(LLVector3* frust)
{

	for (int i = 0; i < 8; i++)
	{
		mAgentFrustum[i] = frust[i];
	}

	//frust contains the 8 points of the frustum, calculate 6 planes

	//order of planes is important, keep most likely to fail in the front of the list

	//near - frust[0], frust[1], frust[2]
	mAgentPlanes[2] = planeFromPoints(frust[0], frust[1], frust[2]);

	//far  
	mAgentPlanes[5] = planeFromPoints(frust[5], frust[4], frust[6]);

	//left  
	mAgentPlanes[0] = planeFromPoints(frust[4], frust[0], frust[7]);

	//right  
	mAgentPlanes[1] = planeFromPoints(frust[1], frust[5], frust[6]);

	//top  
	mAgentPlanes[4] = planeFromPoints(frust[3], frust[2], frust[6]);

	//bottom  
	mAgentPlanes[3] = planeFromPoints(frust[1], frust[0], frust[4]);

	//cache plane octant facing mask for use in AABBInFrustum
	for (int i = 0; i < 8; i++)
	{
		U8 mask = 0;
		LLPlane p = mAgentPlanes[i];
		LLVector3 n = LLVector3(p);

		if (n.mV[0] >= 0)
		{
			mask |= 1;
		}
		if (n.mV[1] >= 0)
		{
			mask |= 2;
		}
		if (n.mV[2] >= 0)
		{
			mask |= 4;
		}
		mAgentPlaneMask[i] = mask;
	}
}

void LLCamera::calculateFrustumPlanes(F32 left, F32 right, F32 top, F32 bottom)
{
	LLVector3 a, b, c;

	// For each plane we need to define 3 points (LLVector3's) in camera view space.  
	// The order in which we pass the points to planeFromPoints() matters, because the 
	// plane normal has a degeneracy of 2; we want it pointing _into_ the frustum. 

	a.setVec(0.0f, 0.0f, 0.0f);
	b.setVec(mFarPlane, right, top);
	c.setVec(mFarPlane, right, bottom);
	mLocalPlanes[PLANE_RIGHT].setVec(a, b, c);

	c.setVec(mFarPlane, left, top);
	mLocalPlanes[PLANE_TOP].setVec(a, c, b);

	b.setVec(mFarPlane, left, bottom);
	mLocalPlanes[PLANE_LEFT].setVec(a, b, c);

	c.setVec(mFarPlane, right, bottom);
	mLocalPlanes[PLANE_BOTTOM].setVec( a, c, b); 

	//calculate center and radius squared of frustum in world absolute coordinates
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
	
	left = 	 x1 * -2.f * view_width;
	right =  x2 * -2.f * view_width;
	bottom = y1 * 2.f * view_height; 
	top = 	 y2 * 2.f * view_height;

	calculateFrustumPlanes(left, right, top, bottom);
}

void LLCamera::calculateWorldFrustumPlanes() 
{
	F32 d;
	LLVector3 center = mOrigin - mXAxis*mNearPlane;
	mWorldPlanePos = center;
	for (int p=0; p<4; p++)
	{
		LLVector3 pnorm = LLVector3(mLocalPlanes[p]);
		LLVector3 norm = rotateToAbsolute(pnorm);
		norm.normVec();
		d = -(center * norm);
		mWorldPlanes[p] = LLPlane(norm, d);
	}
	// horizontal planes, perpindicular to (0,0,1);
	LLVector3 zaxis(0, 0, 1.0f);
	F32 yaw = getYaw();
	{
		LLVector3 tnorm = LLVector3(mLocalPlanes[PLANE_LEFT]);
		tnorm.rotVec(yaw, zaxis);
		d = -(mOrigin * tnorm);
		mHorizPlanes[HORIZ_PLANE_LEFT] = LLPlane(tnorm, d);
	}
	{
		LLVector3 tnorm = LLVector3(mLocalPlanes[PLANE_RIGHT]);
		tnorm.rotVec(yaw, zaxis);
		d = -(mOrigin * tnorm);
		mHorizPlanes[HORIZ_PLANE_RIGHT] = LLPlane(tnorm, d);
	}
}

// NOTE: this is the OpenGL matrix that will transform the default OpenGL view 
// (-Z=at, Y=up) to the default view of the LLCamera class (X=at, Z=up):
// 
// F32 cfr_transform =  {  0.f,  0.f, -1.f,  0.f,   // -Z becomes X
// 						  -1.f,  0.f,  0.f,  0.f,   // -X becomes Y
//  					   0.f,  1.f,  0.f,  0.f,   //  Y becomes Z
// 						   0.f,  0.f,  0.f,  1.f };
