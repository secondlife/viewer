/** 
 * @file lltoolmorph.cpp
 * @brief A tool to manipulate faces..
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

// File includes
#include "lltoolmorph.h" 
#include "llrender.h"

// Library includes
#include "llaudioengine.h"
#include "llviewercontrol.h"
#include "llfontgl.h"
#include "sound_ids.h"
#include "v3math.h"
#include "v3color.h"

// Viewer includes
#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
#include "llmorphview.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "lltexlayer.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "pipeline.h"


//static
LLVisualParamHint::instance_list_t LLVisualParamHint::sInstances;
BOOL LLVisualParamReset::sDirty = FALSE;

//-----------------------------------------------------------------------------
// LLVisualParamHint()
//-----------------------------------------------------------------------------

// static
LLVisualParamHint::LLVisualParamHint(
	S32 pos_x, S32 pos_y,
	S32 width, S32 height, 
	LLViewerJointMesh *mesh, 
	LLViewerVisualParam *param,
	LLWearable *wearable,
	F32 param_weight,
	LLJoint* jointp)
	:
	LLViewerDynamicTexture(width, height, 3, LLViewerDynamicTexture::ORDER_MIDDLE, TRUE ),
	mNeedsUpdate( TRUE ),
	mIsVisible( FALSE ),
	mJointMesh( mesh ),
	mVisualParam( param ),
	mWearablePtr( wearable ),
	mVisualParamWeight( param_weight ),
	mAllowsUpdates( TRUE ),
	mDelayFrames( 0 ),
	mRect( pos_x, pos_y + height, pos_x + width, pos_y ),
	mLastParamWeight(0.f),
	mCamTargetJoint(jointp)
{
	LLVisualParamHint::sInstances.insert( this );
	mBackgroundp = LLUI::getUIImage("avatar_thumb_bkgrnd.png");

	llassert(width != 0);
	llassert(height != 0);
}

//-----------------------------------------------------------------------------
// ~LLVisualParamHint()
//-----------------------------------------------------------------------------
LLVisualParamHint::~LLVisualParamHint()
{
	LLVisualParamHint::sInstances.erase( this );
}

//virtual
S8 LLVisualParamHint::getType() const
{
	return LLViewerDynamicTexture::LL_VISUAL_PARAM_HINT ;
}

//-----------------------------------------------------------------------------
// static
// requestHintUpdates()
// Requests updates for all instances (excluding two possible exceptions)  Grungy but efficient.
//-----------------------------------------------------------------------------
void LLVisualParamHint::requestHintUpdates( LLVisualParamHint* exception1, LLVisualParamHint* exception2 )
{
	S32 delay_frames = 0;
	for (instance_list_t::iterator iter = sInstances.begin();
		 iter != sInstances.end(); ++iter)
	{
		LLVisualParamHint* instance = *iter;
		if( (instance != exception1) && (instance != exception2) )
		{
			if( instance->mAllowsUpdates )
			{
				instance->mNeedsUpdate = TRUE;
				instance->mDelayFrames = delay_frames;
				delay_frames++;
			}
			else
			{
				instance->mNeedsUpdate = TRUE;
				instance->mDelayFrames = 0;
			}
		}
	}
}

BOOL LLVisualParamHint::needsRender()
{
	return mNeedsUpdate && mDelayFrames-- <= 0 && !gAgentAvatarp->getIsAppearanceAnimating() && mAllowsUpdates;
}

void LLVisualParamHint::preRender(BOOL clear_depth)
{
	mLastParamWeight = mVisualParam->getWeight();
	mWearablePtr->setVisualParamWeight(mVisualParam->getID(), mVisualParamWeight, FALSE);
	gAgentAvatarp->setVisualParamWeight(mVisualParam->getID(), mVisualParamWeight, FALSE);
	gAgentAvatarp->setVisualParamWeight("Blink_Left", 0.f);
	gAgentAvatarp->setVisualParamWeight("Blink_Right", 0.f);
	gAgentAvatarp->updateComposites();
	gAgentAvatarp->updateVisualParams();
	gAgentAvatarp->updateGeometry(gAgentAvatarp->mDrawable);
	gAgentAvatarp->updateLOD();

	LLViewerDynamicTexture::preRender(clear_depth);
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLVisualParamHint::render()
{
	LLVisualParamReset::sDirty = TRUE;

	gGL.pushUIMatrix();
	gGL.loadUIIdentity();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho(0.0f, mFullWidth, 0.0f, mFullHeight, -1.0f, 1.0f);

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadIdentity();

	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}

	LLGLSUIDefault gls_ui;
	//LLGLState::verify(TRUE);
	mBackgroundp->draw(0, 0, mFullWidth, mFullHeight);

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();

	mNeedsUpdate = FALSE;
	mIsVisible = TRUE;

	LLQuaternion avatar_rotation;
	LLJoint* root_joint = gAgentAvatarp->getRootJoint();
	if( root_joint )
	{
		avatar_rotation = root_joint->getWorldRotation();
	}

	LLVector3 target_joint_pos = mCamTargetJoint->getWorldPosition();

	LLVector3 target_offset( 0, 0, mVisualParam->getCameraElevation() );
	LLVector3 target_pos = target_joint_pos + (target_offset * avatar_rotation);

	F32 cam_angle_radians = mVisualParam->getCameraAngle() * DEG_TO_RAD;
	LLVector3 camera_snapshot_offset( 
		mVisualParam->getCameraDistance() * cosf( cam_angle_radians ),
		mVisualParam->getCameraDistance() * sinf( cam_angle_radians ),
		mVisualParam->getCameraElevation() );
	LLVector3 camera_pos = target_joint_pos + (camera_snapshot_offset * avatar_rotation);
	
	gGL.flush();
	
	LLViewerCamera::getInstance()->setAspect((F32)mFullWidth / (F32)mFullHeight);
	LLViewerCamera::getInstance()->setOriginAndLookAt(
		camera_pos,			// camera
		LLVector3::z_axis,	// up
		target_pos );		// point of interest

	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, FALSE);

	if (gAgentAvatarp->mDrawable.notNull() &&
		gAgentAvatarp->mDrawable->getFace(0))
	{
		LLDrawPoolAvatar *avatarPoolp = (LLDrawPoolAvatar *)gAgentAvatarp->mDrawable->getFace(0)->getPool();
		LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
		gGL.setAlphaRejectSettings(LLRender::CF_ALWAYS);
		gGL.setSceneBlendType(LLRender::BT_REPLACE);
		avatarPoolp->renderAvatars(gAgentAvatarp);  // renders only one avatar
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	}
	gAgentAvatarp->setVisualParamWeight(mVisualParam->getID(), mLastParamWeight);
	mWearablePtr->setVisualParamWeight(mVisualParam->getID(), mLastParamWeight, FALSE);
	gAgentAvatarp->updateVisualParams();
	gGL.color4f(1,1,1,1);
	mGLTexturep->setGLTextureCreated(true);
	gGL.popUIMatrix();

	return TRUE;
}


//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLVisualParamHint::draw(F32 alpha)
{
	if (!mIsVisible) return;

	gGL.getTexUnit(0)->bind(this);

	gGL.color4f(1.f, 1.f, 1.f, alpha);

	LLGLSUIDefault gls_ui;
	gGL.begin(LLRender::QUADS);
	{
		gGL.texCoord2i(0, 1);
		gGL.vertex2i(0, mFullHeight);
		gGL.texCoord2i(0, 0);
		gGL.vertex2i(0, 0);
		gGL.texCoord2i(1, 0);
		gGL.vertex2i(mFullWidth, 0);
		gGL.texCoord2i(1, 1);
		gGL.vertex2i(mFullWidth, mFullHeight);
	}
	gGL.end();

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

//-----------------------------------------------------------------------------
// LLVisualParamReset()
//-----------------------------------------------------------------------------
LLVisualParamReset::LLVisualParamReset() : LLViewerDynamicTexture(1, 1, 1, ORDER_RESET, FALSE)
{	
}

//virtual
S8 LLVisualParamReset::getType() const
{
	return LLViewerDynamicTexture::LL_VISUAL_PARAM_RESET ;
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLVisualParamReset::render()
{
	if (sDirty)
	{
		gAgentAvatarp->updateComposites();
		gAgentAvatarp->updateVisualParams();
		gAgentAvatarp->updateGeometry(gAgentAvatarp->mDrawable);
		sDirty = FALSE;
	}

	return FALSE;
}
