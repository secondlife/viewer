/** 
 * @file llmorphview.cpp
 * @brief Container for Morph functionality
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llmorphview.h"

#include "lljoint.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
//#include "llfirstuse.h"
#include "llfloatercustomize.h"
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
	mCameraDist( -1.f ),
	mCameraDrivenByKeys( FALSE )
{}

//-----------------------------------------------------------------------------
// initialize()
//-----------------------------------------------------------------------------
void	LLMorphView::initialize()
{
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;
	mCameraDist = -1.f;

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
	LLVOAvatarSelf::onCustomizeEnd();

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
			llassert( !gFloaterCustomize );
			gFloaterCustomize = new LLFloaterCustomize();
			gFloaterCustomize->fetchInventory();
			gFloaterCustomize->openFloater();

			// Must do this _after_ gFloaterView is initialized.
			gFloaterCustomize->switchToDefaultSubpart();

			initialize();

			// First run dialog
			//LLFirstUse::useAppearance();
		}
		else
		{
			if( gFloaterCustomize )
			{
				gFloaterView->removeChild( gFloaterCustomize );
				delete gFloaterCustomize;
				gFloaterCustomize = NULL;
			}

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
