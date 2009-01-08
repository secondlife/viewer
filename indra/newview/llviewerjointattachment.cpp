/** 
 * @file llviewerjointattachment.cpp
 * @brief Implementation of LLViewerJointAttachment class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llviewerjointattachment.h"

#include "llagentconstants.h"

#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llgl.h"
#include "llrender.h"
#include "llvoavatar.h"
#include "llvolume.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llinventorymodel.h"
#include "llviewerobjectlist.h"
#include "llface.h"
#include "llvoavatar.h"

#include "llglheaders.h"

extern LLPipeline gPipeline;

//-----------------------------------------------------------------------------
// LLViewerJointAttachment()
//-----------------------------------------------------------------------------
LLViewerJointAttachment::LLViewerJointAttachment() :
mAttachedObject(NULL),
mVisibleInFirst(FALSE),
mGroup(0),
mIsHUDAttachment(FALSE),
mPieSlice(-1)
{
	mValid = FALSE;
	mUpdateXform = FALSE;
}

//-----------------------------------------------------------------------------
// ~LLViewerJointAttachment()
//-----------------------------------------------------------------------------
LLViewerJointAttachment::~LLViewerJointAttachment()
{
}

//-----------------------------------------------------------------------------
// isTransparent()
//-----------------------------------------------------------------------------
BOOL LLViewerJointAttachment::isTransparent()
{
	return FALSE;
}

//-----------------------------------------------------------------------------
// drawShape()
//-----------------------------------------------------------------------------
U32 LLViewerJointAttachment::drawShape( F32 pixelArea, BOOL first_pass )
{
	if (LLVOAvatar::sShowAttachmentPoints)
	{
		LLGLDisable cull_face(GL_CULL_FACE);
		
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		gGL.begin(LLRender::QUADS);
		{
			gGL.vertex3f(-0.1f, 0.1f, 0.f);
			gGL.vertex3f(-0.1f, -0.1f, 0.f);
			gGL.vertex3f(0.1f, -0.1f, 0.f);
			gGL.vertex3f(0.1f, 0.1f, 0.f);
		}gGL.end();
	}
	return 0;
}

void LLViewerJointAttachment::setupDrawable(LLDrawable* drawablep)
{
	drawablep->mXform.setParent(&mXform); // LLViewerJointAttachment::lazyAttach
	drawablep->makeActive();
	LLVector3 current_pos = mAttachedObject->getRenderPosition();
	LLQuaternion current_rot = mAttachedObject->getRenderRotation();
	LLQuaternion attachment_pt_inv_rot = ~getWorldRotation();

	current_pos -= getWorldPosition();
	current_pos.rotVec(attachment_pt_inv_rot);

	current_rot = current_rot * attachment_pt_inv_rot;

	drawablep->mXform.setPosition(current_pos);
	drawablep->mXform.setRotation(current_rot);
	gPipeline.markMoved(drawablep);
	gPipeline.markTextured(drawablep); // face may need to change draw pool to/from POOL_HUD
	drawablep->setState(LLDrawable::USE_BACKLIGHT);
	
	if(mIsHUDAttachment)
	{
		for (S32 face_num = 0; face_num < drawablep->getNumFaces(); face_num++)
		{
			drawablep->getFace(face_num)->setState(LLFace::HUD_RENDER);
		}
	}

	LLViewerObject::const_child_list_t& child_list = mAttachedObject->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (childp && childp->mDrawable.notNull())
		{
			childp->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
			gPipeline.markTextured(childp->mDrawable); // face may need to change draw pool to/from POOL_HUD
			if(mIsHUDAttachment)
			{
				for (S32 face_num = 0; face_num < childp->mDrawable->getNumFaces(); face_num++)
				{
					childp->mDrawable->getFace(face_num)->setState(LLFace::HUD_RENDER);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// addObject()
//-----------------------------------------------------------------------------
BOOL LLViewerJointAttachment::addObject(LLViewerObject* object)
{
	if (mAttachedObject)
	{
		llwarns << "Attempted to attach object where an attachment already exists!" << llendl;
		
		if (mAttachedObject == object) {
			llinfos << "(same object re-attached)" << llendl;
			removeObject(mAttachedObject);
			// Pass through anyway to let setupDrawable()
			// re-connect object to the joint correctly
		}
		else {
			llinfos << "(objects differ, removing existing object)" << llendl;
			// Rather hacky, but no-one can think of something
			// better to do for this case.
			gObjectList.killObject(mAttachedObject);
			// Proceed with new object attachment
		}
	}
	mAttachedObject = object;
	
	LLUUID item_id;

	// Find the inventory item ID of the attached object
	LLNameValue* item_id_nv = object->getNVPair("AttachItemID");
	if( item_id_nv )
	{
		const char* s = item_id_nv->getString();
		if( s )
		{
			item_id.set( s );
			lldebugs << "getNVPair( AttachItemID ) = " << item_id << llendl;
		}
	}

	mItemID = item_id;

	LLDrawable* drawablep = object->mDrawable;

	if (drawablep)
	{
		//if object is active, make it static
		if(drawablep->isActive())
		{
			drawablep->makeStatic(FALSE) ;
		}

		setupDrawable(drawablep);
	}

	if (mIsHUDAttachment)
	{
		if (object->mText.notNull())
		{
			object->mText->setOnHUDAttachment(TRUE);
		}
		LLViewerObject::const_child_list_t& child_list = object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* childp = *iter;
			if (childp && childp->mText.notNull())
			{
				childp->mText->setOnHUDAttachment(TRUE);
			}
		}
	}
	calcLOD();
	mUpdateXform = TRUE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// removeObject()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::removeObject(LLViewerObject *object)
{
	// force object visibile
	setAttachmentVisibility(TRUE);

	if (object->mDrawable.notNull())
	{
		//if object is active, make it static
		if(object->mDrawable->isActive())
		{
			object->mDrawable->makeStatic(FALSE) ;
		}

		LLVector3 cur_position = object->getRenderPosition();
		LLQuaternion cur_rotation = object->getRenderRotation();

		object->mDrawable->mXform.setPosition(cur_position);
		object->mDrawable->mXform.setRotation(cur_rotation);
		gPipeline.markMoved(object->mDrawable, TRUE);
		gPipeline.markTextured(object->mDrawable); // face may need to change draw pool to/from POOL_HUD
		object->mDrawable->clearState(LLDrawable::USE_BACKLIGHT);

		if (mIsHUDAttachment)
		{
			for (S32 face_num = 0; face_num < object->mDrawable->getNumFaces(); face_num++)
			{
				object->mDrawable->getFace(face_num)->clearState(LLFace::HUD_RENDER);
			}
		}
	}

	LLViewerObject::const_child_list_t& child_list = object->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (childp && childp->mDrawable.notNull())
		{
			childp->mDrawable->clearState(LLDrawable::USE_BACKLIGHT);
			gPipeline.markTextured(childp->mDrawable); // face may need to change draw pool to/from POOL_HUD
			if (mIsHUDAttachment)
			{
				for (S32 face_num = 0; face_num < childp->mDrawable->getNumFaces(); face_num++)
				{
					childp->mDrawable->getFace(face_num)->clearState(LLFace::HUD_RENDER);
				}
			}
		}
	} 

	if (mIsHUDAttachment)
	{
		if (object->mText.notNull())
		{
			object->mText->setOnHUDAttachment(FALSE);
		}
		LLViewerObject::const_child_list_t& child_list = object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* childp = *iter;
			if (childp->mText.notNull())
			{
				childp->mText->setOnHUDAttachment(FALSE);
			}
		}
	}

	mAttachedObject = NULL;
	mUpdateXform = FALSE;
	mItemID.setNull();
}

//-----------------------------------------------------------------------------
// setAttachmentVisibility()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::setAttachmentVisibility(BOOL visible)
{
	if (!mAttachedObject || mAttachedObject->mDrawable.isNull() || 
		!(mAttachedObject->mDrawable->getSpatialBridge()))
		return;

	if (visible)
	{
		// Hack to make attachments not visible by disabling their type mask!
		// This will break if you can ever attach non-volumes! - djs 02/14/03
		mAttachedObject->mDrawable->getSpatialBridge()->mDrawableType = 
			mAttachedObject->isHUDAttachment() ? LLPipeline::RENDER_TYPE_HUD : LLPipeline::RENDER_TYPE_VOLUME;
	}
	else
	{
		mAttachedObject->mDrawable->getSpatialBridge()->mDrawableType = 0;
	}
}

//-----------------------------------------------------------------------------
// setOriginalPosition()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::setOriginalPosition(LLVector3& position)
{
	mOriginalPos = position;
	setPosition(position);
}

//-----------------------------------------------------------------------------
// clampObjectPosition()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::clampObjectPosition()
{
	if (mAttachedObject)
	{
		// *NOTE: object can drift when hitting maximum radius
		LLVector3 attachmentPos = mAttachedObject->getPosition();
		F32 dist = attachmentPos.normVec();
		dist = llmin(dist, MAX_ATTACHMENT_DIST);
		attachmentPos *= dist;
		mAttachedObject->setPosition(attachmentPos);
	}
}

//-----------------------------------------------------------------------------
// calcLOD()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::calcLOD()
{
	F32 maxarea = mAttachedObject->getMaxScale() * mAttachedObject->getMidScale();
	LLViewerObject::const_child_list_t& child_list = mAttachedObject->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		F32 area = childp->getMaxScale() * childp->getMidScale();
		maxarea = llmax(maxarea, area);
	}
	maxarea = llclamp(maxarea, .01f*.01f, 1.f);
	F32 avatar_area = (4.f * 4.f); // pixels for an avatar sized attachment
	F32 min_pixel_area = avatar_area / maxarea;
	setLOD(min_pixel_area);
}

//-----------------------------------------------------------------------------
// updateLOD()
//-----------------------------------------------------------------------------
BOOL LLViewerJointAttachment::updateLOD(F32 pixel_area, BOOL activate)
{
	BOOL res = FALSE;
	if (!mValid)
	{
		setValid(TRUE, TRUE);
		res = TRUE;
	}
	return res;
}

