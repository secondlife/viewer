/** 
 * @file llviewerjointattachment.cpp
 * @brief Implementation of LLViewerJointAttachment class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerjointattachment.h"

#include "llagentconstants.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llgl.h"
#include "llhudtext.h"
#include "llrender.h"
#include "llvoavatarself.h"
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
	mVisibleInFirst(FALSE),
	mGroup(0),
	mIsHUDAttachment(FALSE),
	mPieSlice(-1)
{
	mValid = FALSE;
	mUpdateXform = FALSE;
	mAttachedObjects.clear();
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
U32 LLViewerJointAttachment::drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy )
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

void LLViewerJointAttachment::setupDrawable(LLViewerObject *object)
{
	if (!object->mDrawable)
		return;
	if (object->mDrawable->isActive())
	{
		object->mDrawable->makeStatic(FALSE);
	}

	object->mDrawable->mXform.setParent(getXform()); // LLViewerJointAttachment::lazyAttach
	object->mDrawable->makeActive();
	LLVector3 current_pos = object->getRenderPosition();
	LLQuaternion current_rot = object->getRenderRotation();
	LLQuaternion attachment_pt_inv_rot = ~(getWorldRotation());

	current_pos -= getWorldPosition();
	current_pos.rotVec(attachment_pt_inv_rot);

	current_rot = current_rot * attachment_pt_inv_rot;

	object->mDrawable->mXform.setPosition(current_pos);
	object->mDrawable->mXform.setRotation(current_rot);
	gPipeline.markMoved(object->mDrawable);
	gPipeline.markTextured(object->mDrawable); // face may need to change draw pool to/from POOL_HUD
	object->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
	
	if(mIsHUDAttachment)
	{
		for (S32 face_num = 0; face_num < object->mDrawable->getNumFaces(); face_num++)
		{
			LLFace *face = object->mDrawable->getFace(face_num);
			if (face)
			{
				face->setState(LLFace::HUD_RENDER);
			}
		}
	}

	LLViewerObject::const_child_list_t& child_list = object->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); ++iter)
	{
		LLViewerObject* childp = *iter;
		if (childp && childp->mDrawable.notNull())
		{
			childp->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
			gPipeline.markTextured(childp->mDrawable); // face may need to change draw pool to/from POOL_HUD
			gPipeline.markMoved(childp->mDrawable);

			if(mIsHUDAttachment)
			{
				for (S32 face_num = 0; face_num < childp->mDrawable->getNumFaces(); face_num++)
				{
					LLFace * face = childp->mDrawable->getFace(face_num);
					if (face)
					{
						face->setState(LLFace::HUD_RENDER);
					}
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
	object->extractAttachmentItemID();

	// Same object reattached
	if (isObjectAttached(object))
	{
		llinfos << "(same object re-attached)" << llendl;
		removeObject(object);
		// Pass through anyway to let setupDrawable()
		// re-connect object to the joint correctly
	}
	
	// Two instances of the same inventory item attached --
	// Request detach, and kill the object in the meantime.
	if (getAttachedObject(object->getAttachmentItemID()))
	{
		llinfos << "(same object re-attached)" << llendl;
		object->markDead();

		// If this happens to be attached to self, then detach.
		LLVOAvatarSelf::detachAttachmentIntoInventory(object->getAttachmentItemID());
		return FALSE;
	}

	mAttachedObjects.push_back(object);
	setupDrawable(object);
	
	if (mIsHUDAttachment)
	{
		if (object->mText.notNull())
		{
			object->mText->setOnHUDAttachment(TRUE);
		}
		LLViewerObject::const_child_list_t& child_list = object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); ++iter)
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
	attachedobjs_vec_t::iterator iter;
	for (iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		LLViewerObject *attached_object = (*iter);
		if (attached_object == object)
		{
			break;
		}
	}
	if (iter == mAttachedObjects.end())
	{
		llwarns << "Could not find object to detach" << llendl;
		return;
	}

	// force object visibile
	setAttachmentVisibility(TRUE);

	mAttachedObjects.erase(iter);
	if (object->mDrawable.notNull())
	{
		//if object is active, make it static
		if(object->mDrawable->isActive())
		{
			object->mDrawable->makeStatic(FALSE);
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
				LLFace * face = object->mDrawable->getFace(face_num);
				if (face)
				{
					face->clearState(LLFace::HUD_RENDER);
				}
			}
		}
	}

	LLViewerObject::const_child_list_t& child_list = object->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); ++iter)
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
					LLFace * face = childp->mDrawable->getFace(face_num);
					if (face)
					{
						face->clearState(LLFace::HUD_RENDER);
					}
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
			 iter != child_list.end(); ++iter)
		{
			LLViewerObject* childp = *iter;
			if (childp->mText.notNull())
			{
				childp->mText->setOnHUDAttachment(FALSE);
			}
		}
	}
	if (mAttachedObjects.size() == 0)
	{
		mUpdateXform = FALSE;
	}
	object->setAttachmentItemID(LLUUID::null);
}

//-----------------------------------------------------------------------------
// setAttachmentVisibility()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::setAttachmentVisibility(BOOL visible)
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		LLViewerObject *attached_obj = (*iter);
		if (!attached_obj || attached_obj->mDrawable.isNull() || 
			!(attached_obj->mDrawable->getSpatialBridge()))
			continue;
		
		if (visible)
		{
			// Hack to make attachments not visible by disabling their type mask!
			// This will break if you can ever attach non-volumes! - djs 02/14/03
			attached_obj->mDrawable->getSpatialBridge()->mDrawableType = 
				attached_obj->isHUDAttachment() ? LLPipeline::RENDER_TYPE_HUD : LLPipeline::RENDER_TYPE_VOLUME;
		}
		else
		{
			attached_obj->mDrawable->getSpatialBridge()->mDrawableType = 0;
		}
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
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		if (LLViewerObject *attached_object = (*iter))
		{
			// *NOTE: object can drift when hitting maximum radius
			LLVector3 attachmentPos = attached_object->getPosition();
			F32 dist = attachmentPos.normVec();
			dist = llmin(dist, MAX_ATTACHMENT_DIST);
			attachmentPos *= dist;
			attached_object->setPosition(attachmentPos);
		}
	}
}

//-----------------------------------------------------------------------------
// calcLOD()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::calcLOD()
{
	F32 maxarea = 0;
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		if (LLViewerObject *attached_object = (*iter))
		{
			maxarea = llmax(maxarea,attached_object->getMaxScale() * attached_object->getMidScale());
			LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
			for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
				 iter != child_list.end(); ++iter)
			{
				LLViewerObject* childp = *iter;
				F32 area = childp->getMaxScale() * childp->getMidScale();
				maxarea = llmax(maxarea, area);
			}
		}
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

BOOL LLViewerJointAttachment::isObjectAttached(const LLViewerObject *viewer_object) const
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		const LLViewerObject* attached_object = (*iter);
		if (attached_object == viewer_object)
		{
			return TRUE;
		}
	}
	return FALSE;
}

const LLViewerObject *LLViewerJointAttachment::getAttachedObject(const LLUUID &object_id) const
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		const LLViewerObject* attached_object = (*iter);
		if (attached_object->getAttachmentItemID() == object_id)
		{
			return attached_object;
		}
	}
	return NULL;
}

LLViewerObject *LLViewerJointAttachment::getAttachedObject(const LLUUID &object_id)
{
	for (attachedobjs_vec_t::iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		LLViewerObject* attached_object = (*iter);
		if (attached_object->getAttachmentItemID() == object_id)
		{
			return attached_object;
		}
	}
	return NULL;
}
