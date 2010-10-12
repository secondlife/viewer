/** 
 * @file llmorphview.h
 * @brief Container for character morph controls
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLMORPHVIEW_H
#define LL_LLMORPHVIEW_H

#include "llview.h"
#include "v3dmath.h"
#include "llframetimer.h"

class LLJoint;

class LLMorphView : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Params()
		{
			mouse_opaque(false);
			follows.flags(FOLLOWS_ALL);
		}
	};
	LLMorphView(const LLMorphView::Params&);
	
	void		shutdown();

	// inherited methods
	/*virtual*/ void	setVisible(BOOL visible);

	void		setCameraTargetJoint(LLJoint *joint)		{mCameraTargetJoint = joint;}
	LLJoint*	getCameraTargetJoint()						{return mCameraTargetJoint;}

	void		setCameraOffset(const LLVector3d& camera_offset)	{mCameraOffset = camera_offset;}
	void		setCameraTargetOffset(const LLVector3d& camera_target_offset) {mCameraTargetOffset = camera_target_offset;}

	void		updateCamera();
	void		setCameraDrivenByKeys( BOOL b );

protected:
	void		initialize();

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

	BOOL		mCameraDrivenByKeys;
};

//
// Globals
//

extern LLMorphView *gMorphView;

#endif 
