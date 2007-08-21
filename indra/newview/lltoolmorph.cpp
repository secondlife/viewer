/** 
 * @file lltoolmorph.cpp
 * @brief A tool to manipulate faces..
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

// File includes
#include "lltoolmorph.h" 

// Library includes
#include "audioengine.h"
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
#include "lltoolview.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "pipeline.h"
#include "viewer.h"

//LLToolMorph *gToolMorph = NULL;

//static
LLLinkedList<LLVisualParamHint> LLVisualParamHint::sInstances;
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
	LLDynamicTexture(width, height, 3, LLDynamicTexture::ORDER_MIDDLE, TRUE ),
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
	LLVisualParamHint::sInstances.addData( this );
	LLUUID id;
	id.set( gViewerArt.getString("avatar_thumb_bkgrnd.tga") );
	mBackgroundp = gImageList.getImage(id, FALSE, TRUE);


	llassert(width != 0);
	llassert(height != 0);
}

//-----------------------------------------------------------------------------
// ~LLVisualParamHint()
//-----------------------------------------------------------------------------
LLVisualParamHint::~LLVisualParamHint()
{
	LLVisualParamHint::sInstances.removeData( this );
}

//-----------------------------------------------------------------------------
// static
// requestHintUpdates()
// Requests updates for all instances (excluding two possible exceptions)  Grungy but efficient.
//-----------------------------------------------------------------------------
void LLVisualParamHint::requestHintUpdates( LLVisualParamHint* exception1, LLVisualParamHint* exception2 )
{
	S32 delay_frames = 0;
	for(LLVisualParamHint* instance = sInstances.getFirstData();
		instance;
		instance = sInstances.getNextData())
	{
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
	return mNeedsUpdate && mDelayFrames-- <= 0 && !gAgent.getAvatarObject()->mAppearanceAnimating && mAllowsUpdates;
}

void LLVisualParamHint::preRender(BOOL clear_depth)
{
	LLVOAvatar* avatarp = gAgent.getAvatarObject();

	mLastParamWeight = avatarp->getVisualParamWeight(mVisualParam);
	avatarp->setVisualParamWeight(mVisualParam, mVisualParamWeight);
	avatarp->setVisualParamWeight("Blink_Left", 0.f);
	avatarp->setVisualParamWeight("Blink_Right", 0.f);
	avatarp->updateComposites();
	avatarp->updateVisualParams();
	avatarp->updateGeometry(avatarp->mDrawable);
	avatarp->updateLOD();

	LLDynamicTexture::preRender(clear_depth);
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLVisualParamHint::render()
{
	LLVisualParamReset::sDirty = TRUE;
	LLVOAvatar* avatarp = gAgent.getAvatarObject();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLSUIDefault gls_ui;
	//LLGLState::verify(TRUE);
	LLViewerImage::bindTexture(mBackgroundp);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	gl_rect_2d_simple_tex( mWidth, mHeight );
	mBackgroundp->unbindTexture(0, GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	mNeedsUpdate = FALSE;
	mIsVisible = TRUE;

	LLViewerJointMesh* cam_target_joint = NULL;
	const std::string& cam_target_mesh_name = mVisualParam->getCameraTargetName();
	if( !cam_target_mesh_name.empty() )
	{
		cam_target_joint = (LLViewerJointMesh*)avatarp->getJoint( cam_target_mesh_name );
	}
	if( !cam_target_joint )
	{
		cam_target_joint = (LLViewerJointMesh*)gMorphView->getCameraTargetJoint();
	}
	if( !cam_target_joint )
	{
		cam_target_joint = (LLViewerJointMesh*)avatarp->getJoint("mHead");
	}

	LLQuaternion avatar_rotation;
	LLJoint* root_joint = avatarp->getRootJoint();
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
	
	gCamera->setAspect((F32)mWidth / (F32)mHeight);
	gCamera->setOriginAndLookAt(
		camera_pos,		// camera
		LLVector3(0.f, 0.f, 1.f),						// up
		target_pos );	// point of interest

	gCamera->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mWidth, mHeight, FALSE);

	if (avatarp->mDrawable.notNull())
	{
		LLDrawPoolAvatar *avatarPoolp = (LLDrawPoolAvatar *)avatarp->mDrawable->getFace(0)->getPool();
		LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
		avatarPoolp->renderAvatars(avatarp);  // renders only one avatar
	}
	avatarp->setVisualParamWeight(mVisualParam, mLastParamWeight);
	
	return TRUE;
}


//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLVisualParamHint::draw()
{
	if (!mIsVisible) return;

	bindTexture();

	glColor4f(1.f, 1.f, 1.f, 1.f);

	LLGLSUIDefault gls_ui;
	glBegin(GL_QUADS);
	{
		glTexCoord2i(0, 1);
		glVertex2i(0, mHeight);
		glTexCoord2i(0, 0);
		glVertex2i(0, 0);
		glTexCoord2i(1, 0);
		glVertex2i(mWidth, 0);
		glTexCoord2i(1, 1);
		glVertex2i(mWidth, mHeight);
	}
	glEnd();

	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
}

//-----------------------------------------------------------------------------
// LLVisualParamReset()
//-----------------------------------------------------------------------------
LLVisualParamReset::LLVisualParamReset() : LLDynamicTexture(1, 1, 1, ORDER_RESET, FALSE)
{	
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLVisualParamReset::render()
{
	if (sDirty)
	{
		LLVOAvatar* avatarp = gAgent.getAvatarObject();
		avatarp->updateComposites();
		avatarp->updateVisualParams();
		avatarp->updateGeometry(avatarp->mDrawable);
		sDirty = FALSE;
	}

	return FALSE;
}
