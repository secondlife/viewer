/** 
 * @file llmorphview.h
 * @brief Container for character morph controls
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMORPHVIEW_H
#define LL_LLMORPHVIEW_H

#include "llview.h"
#include "v3dmath.h"
#include "llframetimer.h"

class LLJoint;
class LLFloaterCustomize;

class LLMorphView : public LLView
{
public:
	LLMorphView(const std::string& name, const LLRect& rect);
	
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	void		initialize();
	void		shutdown();

	// inherited methods
	/*virtual*/ void	setVisible(BOOL visible);

	void		setCameraTargetJoint(LLJoint *joint)		{mCameraTargetJoint = joint;}
	LLJoint*	getCameraTargetJoint()						{return mCameraTargetJoint;}

	void		setCameraOffset(const LLVector3d& camera_offset)	{mCameraOffset = camera_offset;}
	void		setCameraTargetOffset(const LLVector3d& camera_target_offset) {mCameraTargetOffset = camera_target_offset;}
	void		setCameraDistToDefault()					{ mCameraDist = -1.f; }

	void		updateCamera();
	void		setCameraDrivenByKeys( BOOL b );

protected:
	LLJoint*	mCameraTargetJoint;
	LLVector3d	mCameraOffset;
	LLVector3d	mCameraTargetOffset;
	LLVector3d	mOldCameraPos;
	LLVector3d	mOldTargetPos;
	F32			mOldCameraNearClip;
	LLFrameTimer mCameraMoveTimer;

	// camera rotation
	F32			mCameraPitch;
	F32			mCameraYaw;

	// camera zoom
	F32			mCameraDist;

	BOOL		mCameraDrivenByKeys;
};

//
// Globals
//

extern LLMorphView *gMorphView;

#endif 
