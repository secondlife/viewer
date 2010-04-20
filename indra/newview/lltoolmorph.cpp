/** 
 * @file lltoolmorph.cpp
 * @brief A tool to manipulate faces..
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
#include "llfloatercustomize.h"
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
	F32 param_weight)
	:
	LLViewerDynamicTexture(width, height, 3, LLViewerDynamicTexture::ORDER_MIDDLE, TRUE ),
	mNeedsUpdate( TRUE ),
	mIsVisible( FALSE ),
	mJointMesh( mesh ),
	mVisualParam( param ),
	mVisualParamWeight( param_weight ),
	mAllowsUpdates( TRUE ),
	mDelayFrames( 0 ),
	mRect( pos_x, pos_y + height, pos_x + width, pos_y ),
	mLastParamWeight(0.f)
{
	LLVisualParamHint::sInstances.insert( this );
	mBackgroundp = LLUI::getUIImage("avatar_thumb_bkgrnd.j2c");


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
	return mNeedsUpdate && mDelayFrames-- <= 0 && !gAgentAvatarp->mAppearanceAnimating && mAllowsUpdates;
}

void LLVisualParamHint::preRender(BOOL clear_depth)
{
	mLastParamWeight = mVisualParam->getWeight();
	mVisualParam->setWeight(mVisualParamWeight, FALSE);
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

	glMatrixMode(GL_PROJECTION);
	gGL.pushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mFullWidth, 0.0f, mFullHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	gGL.pushMatrix();
	glLoadIdentity();

	LLGLSUIDefault gls_ui;
	//LLGLState::verify(TRUE);
	mBackgroundp->draw(0, 0, mFullWidth, mFullHeight);

	glMatrixMode(GL_PROJECTION);
	gGL.popMatrix();

	glMatrixMode(GL_MODELVIEW);
	gGL.popMatrix();

	mNeedsUpdate = FALSE;
	mIsVisible = TRUE;

	LLViewerJointMesh* cam_target_joint = NULL;
	const std::string& cam_target_mesh_name = mVisualParam->getCameraTargetName();
	if( !cam_target_mesh_name.empty() )
	{
		cam_target_joint = (LLViewerJointMesh*)gAgentAvatarp->getJoint( cam_target_mesh_name );
	}
	if( !cam_target_joint )
	{
		cam_target_joint = (LLViewerJointMesh*)gMorphView->getCameraTargetJoint();
	}
	if( !cam_target_joint )
	{
		cam_target_joint = (LLViewerJointMesh*)gAgentAvatarp->getJoint("mHead");
	}

	LLQuaternion avatar_rotation;
	LLJoint* root_joint = gAgentAvatarp->getRootJoint();
	if( root_joint )
	{
		avatar_rotation = root_joint->getWorldRotation();
	}

	LLVector3 target_joint_pos = cam_target_joint->getWorldPosition();

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
		camera_pos,		// camera
		LLVector3(0.f, 0.f, 1.f),						// up
		target_pos );	// point of interest

	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, FALSE);

	if (gAgentAvatarp->mDrawable.notNull())
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
	mVisualParam->setWeight(mLastParamWeight, FALSE);
	gGL.color4f(1,1,1,1);
	mGLTexturep->setGLTextureCreated(true);
	gGL.popUIMatrix();
	return TRUE;
}


//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLVisualParamHint::draw()
{
	if (!mIsVisible) return;

	gGL.getTexUnit(0)->bind(this);

	gGL.color4f(1.f, 1.f, 1.f, 1.f);

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
