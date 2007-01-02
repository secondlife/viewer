/** 
 * @file llmaniprotate.h
 * @brief LLManipRotate class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
		LLVector3	mAxisU;
		LLVector3	mAxisV;
		U32			mManipID;

		ManipulatorHandle(LLVector3 axis_u, LLVector3 axis_v, U32 id) : mAxisU(axis_u), mAxisV(axis_v), mManipID(id){}
	};
	
	LLManipRotate( LLToolComposite* composite );

	virtual BOOL	handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL	handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL	handleHover( S32 x, S32 y, MASK mask );
	virtual void	render();

	virtual void	handleSelect();
	virtual void	handleDeselect();

	BOOL			handleMouseDownOnPart(S32 x, S32 y, MASK mask);
	virtual void	highlightManipulators(S32 x, S32 y);
	EManipPart		getHighlightedPart() { return mHighlightedPart; }
private:
	void			updateHoverView();

	void			drag( S32 x, S32 y );
	LLVector3		projectToSphere( F32 x, F32 y, BOOL* on_sphere );

	void			renderSnapGuides();
	void			renderActiveRing(F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color);

	BOOL			updateVisiblity();
	LLVector3		findNearestPointOnRing( S32 x, S32 y, const LLVector3& center, const LLVector3& axis );

	LLQuaternion	dragUnconstrained( S32 x, S32 y );
	LLQuaternion	dragConstrained( S32 x, S32 y );
	LLVector3		getConstraintAxis();
	S32				getObjectAxisClosestToMouse(LLVector3& axis);

	// Utility functions
	static void			mouseToRay( S32 x, S32 y, LLVector3* ray_pt, LLVector3* ray_dir );
	static LLVector3	intersectMouseWithSphere( S32 x, S32 y, const LLVector3& sphere_center, F32 sphere_radius );
	static LLVector3	intersectRayWithSphere( const LLVector3& ray_pt, const LLVector3& ray_dir, const LLVector3& sphere_center, F32 sphere_radius);

private:
	LLVector3d			mRotationCenter;			
	LLCoordGL			mCenterScreen;
//	S32					mLastHoverMouseX;		// used to suppress hover if mouse doesn't move
//	S32					mLastHoverMouseY;
	LLQuaternion		mRotation;
	
	LLVector3			mMouseDown;
	LLVector3			mMouseCur;
	F32					mRadiusMeters;
	
	LLVector3			mCenterToCam;
	LLVector3			mCenterToCamNorm;
	F32					mCenterToCamMag;
	LLVector3			mCenterToProfilePlane;
	F32					mCenterToProfilePlaneMag;

	EManipPart			mManipPart;

	BOOL				mSendUpdateOnMouseUp;
	EManipPart			mHighlightedPart;

	BOOL				mSmoothRotate;
	BOOL				mCamEdgeOn;

	LLVector4			mManipulatorVertices[6];
	LLVector4			mManipulatorScales;
};

#endif  // LL_LLMANIPROTATE_H
