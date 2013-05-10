/** 
 * @file llmorphview.cpp
 * @brief Container for Morph functionality
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

#include "llviewerprecompiledheaders.h"

#include "llmorphview.h"

#include "lljoint.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
//#include "llfirstuse.h"
#include "llfloatertools.h"
#include "llresmgr.h"
#include "lltoolmgr.h"
#include "lltoolmorph.h"
#include "llviewercamera.h"
#include "llvoavatarself.h"
#include "llviewerwindow.h"
#include "pipeline.h"

LLMorphView *gMorphView = NULL;


const F32 EDIT_AVATAR_ORBIT_SPEED = 0.1f;
const F32 EDIT_AVATAR_MAX_CAMERA_PITCH = 0.5f;

const F32 CAMERA_MOVE_TIME = 0.5f;
const F32 MORPH_NEAR_CLIP = 0.1f;

const F32 CAMERA_DIST_MIN  = 0.4f;
const F32 CAMERA_DIST_MAX  = 4.0f;
const F32 CAMERA_DIST_STEP = 1.5f;

//-----------------------------------------------------------------------------
// LLMorphView()
//-----------------------------------------------------------------------------
LLMorphView::LLMorphView(const LLMorphView::Params& p)
: 	LLView(p),
	mCameraTargetJoint( NULL ),
	mCameraOffset(-0.5f, 0.05f, 0.07f ),
	mCameraTargetOffset(0.f, 0.f, 0.05f ),
	mOldCameraNearClip( 0.f ),
	mCameraPitch( 0.f ),
	mCameraYaw( 0.f ),
	mCameraDrivenByKeys( FALSE )
{}

//-----------------------------------------------------------------------------
// initialize()
//-----------------------------------------------------------------------------
void	LLMorphView::initialize()
{
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;

	if (!isAgentAvatarValid() || gAgentAvatarp->isDead())
	{
		gAgentCamera.changeCameraToDefault();
		return;
	}

	gAgentAvatarp->stopMotion( ANIM_AGENT_BODY_NOISE );
	gAgentAvatarp->mSpecialRenderMode = 3;
	
	// set up camera for close look at avatar
	mOldCameraNearClip = LLViewerCamera::getInstance()->getNear();
	LLViewerCamera::getInstance()->setNear(MORPH_NEAR_CLIP);	
}

//-----------------------------------------------------------------------------
// shutdown()
//-----------------------------------------------------------------------------
void	LLMorphView::shutdown()
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->startMotion( ANIM_AGENT_BODY_NOISE );
		gAgentAvatarp->mSpecialRenderMode = 0;
		// reset camera
		LLViewerCamera::getInstance()->setNear(mOldCameraNearClip);
	}
}


//-----------------------------------------------------------------------------
// setVisible()
//-----------------------------------------------------------------------------
void LLMorphView::setVisible(BOOL visible)
{
	if( visible != getVisible() )
	{
		LLView::setVisible(visible);

		if (visible)
		{
			// TODO: verify some user action has already opened outfit editor? - Nyx
			initialize();

			// First run dialog
			//LLFirstUse::useAppearance();
		}
		else
		{
			// TODO: verify some user action has already closed outfit editor ? - Nyx
			shutdown();
		}
	}
}

void LLMorphView::updateCamera()
{
	if (!mCameraTargetJoint)
	{
		setCameraTargetJoint(gAgentAvatarp->getJoint("mHead"));
	}	
	if (!isAgentAvatarValid()) return;

	LLJoint* root_joint = gAgentAvatarp->getRootJoint();
	if( !root_joint )
	{
		return;
	}

	const LLQuaternion& avatar_rot = root_joint->getWorldRotation();

	LLVector3d joint_pos = gAgent.getPosGlobalFromAgent(mCameraTargetJoint->getWorldPosition());
	LLVector3d target_pos = joint_pos + mCameraTargetOffset * avatar_rot;

	LLQuaternion camera_rot_yaw(mCameraYaw, LLVector3::z_axis);
	LLQuaternion camera_rot_pitch(mCameraPitch, LLVector3::y_axis);

	LLVector3d camera_pos = joint_pos + mCameraOffset * camera_rot_pitch * camera_rot_yaw * avatar_rot;

	gAgentCamera.setCameraPosAndFocusGlobal( camera_pos, target_pos, gAgent.getID() );
}

void LLMorphView::setCameraDrivenByKeys(BOOL b)
{
	if( mCameraDrivenByKeys != b )
	{
		if( b )
		{
			// Reset to the default camera position specified by mCameraPitch, mCameraYaw, etc.
			updateCamera();
		}
		mCameraDrivenByKeys = b;
	}
}
