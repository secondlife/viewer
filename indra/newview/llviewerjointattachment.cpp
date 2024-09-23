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
constexpr F32 MAX_ATTACHMENT_DIST = 3.5f; // meters

//-----------------------------------------------------------------------------
// LLViewerJointAttachment()
//-----------------------------------------------------------------------------
LLViewerJointAttachment::LLViewerJointAttachment() :
    mVisibleInFirst(false),
    mGroup(0),
    mIsHUDAttachment(false),
    mPieSlice(-1)
{
    mValid = false;
    mUpdateXform = false;
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
bool LLViewerJointAttachment::isTransparent()
{
    return false;
}

//-----------------------------------------------------------------------------
// drawShape()
//-----------------------------------------------------------------------------
U32 LLViewerJointAttachment::drawShape( F32 pixelArea, bool first_pass, bool is_dummy )
{
    if (LLVOAvatar::sShowAttachmentPoints)
    {
        LLGLDisable cull_face(GL_CULL_FACE);

        gGL.color4f(1.f, 1.f, 1.f, 1.f);
        gGL.begin(LLRender::TRIANGLES);
        {
            gGL.vertex3f(-0.1f, 0.1f, 0.f);
            gGL.vertex3f(-0.1f, -0.1f, 0.f);
            gGL.vertex3f(0.1f, -0.1f, 0.f);

            gGL.vertex3f(-0.1f, 0.1f, 0.f);
            gGL.vertex3f(0.1f, -0.1f, 0.f);
            gGL.vertex3f(0.1f, 0.1f, 0.f);
        }
        gGL.end();
    }
    return 0;
}

void LLViewerJointAttachment::setupDrawable(LLViewerObject *object)
{
    if (!object->mDrawable)
        return;
    if (object->mDrawable->isActive())
    {
        object->mDrawable->makeStatic(false);
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
bool LLViewerJointAttachment::addObject(LLViewerObject* object)
{
    object->extractAttachmentItemID();

    // Same object reattached
    if (isObjectAttached(object))
    {
        LL_INFOS() << "(same object re-attached)" << LL_ENDL;
        removeObject(object);
        // Pass through anyway to let setupDrawable()
        // re-connect object to the joint correctly
    }

    // Two instances of the same inventory item attached --
    // Request detach, and kill the object in the meantime.
    if (getAttachedObject(object->getAttachmentItemID()))
    {
        LL_INFOS() << "(same object re-attached)" << LL_ENDL;
        object->markDead();

        // If this happens to be attached to self, then detach.
        LLVOAvatarSelf::detachAttachmentIntoInventory(object->getAttachmentItemID());
        return false;
    }

    mAttachedObjects.push_back(object);
    setupDrawable(object);

    if (mIsHUDAttachment)
    {
        if (object->mText.notNull())
        {
            object->mText->setOnHUDAttachment(true);
        }
        LLViewerObject::const_child_list_t& child_list = object->getChildren();
        for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
             iter != child_list.end(); ++iter)
        {
            LLViewerObject* childp = *iter;
            if (childp && childp->mText.notNull())
            {
                childp->mText->setOnHUDAttachment(true);
            }
        }
    }
    calcLOD();
    mUpdateXform = true;

    return true;
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
        LLViewerObject *attached_object = iter->get();
        if (attached_object == object)
        {
            break;
        }
    }
    if (iter == mAttachedObjects.end())
    {
        LL_WARNS() << "Could not find object to detach" << LL_ENDL;
        return;
    }

    // force object visibile
    setAttachmentVisibility(true);

    mAttachedObjects.erase(iter);
    if (object->mDrawable.notNull())
    {
        //if object is active, make it static
        if(object->mDrawable->isActive())
        {
            object->mDrawable->makeStatic(false);
        }

        LLVector3 cur_position = object->getRenderPosition();
        LLQuaternion cur_rotation = object->getRenderRotation();

        object->mDrawable->mXform.setPosition(cur_position);
        object->mDrawable->mXform.setRotation(cur_rotation);
        gPipeline.markMoved(object->mDrawable, true);
        gPipeline.markTextured(object->mDrawable); // face may need to change draw pool to/from POOL_HUD

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
            object->mText->setOnHUDAttachment(false);
        }
        LLViewerObject::const_child_list_t& child_list = object->getChildren();
        for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
             iter != child_list.end(); ++iter)
        {
            LLViewerObject* childp = *iter;
            if (childp->mText.notNull())
            {
                childp->mText->setOnHUDAttachment(false);
            }
        }
    }
    if (mAttachedObjects.size() == 0)
    {
        mUpdateXform = false;
    }
    object->setAttachmentItemID(LLUUID::null);
}

//-----------------------------------------------------------------------------
// setAttachmentVisibility()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::setAttachmentVisibility(bool visible)
{
    for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
         iter != mAttachedObjects.end();
         ++iter)
    {
        LLViewerObject *attached_obj = iter->get();
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
    // SL-315
    setPosition(position);
}

//-----------------------------------------------------------------------------
// getNumAnimatedObjects()
//-----------------------------------------------------------------------------
S32 LLViewerJointAttachment::getNumAnimatedObjects() const
{
    S32 count = 0;
    for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
         iter != mAttachedObjects.end();
         ++iter)
    {
        const LLViewerObject *attached_object = iter->get();
        if (attached_object->isAnimatedObject())
        {
            count++;
        }
    }
    return count;
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
        if (LLViewerObject *attached_object = iter->get())
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
        if (LLViewerObject *attached_object = iter->get())
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
bool LLViewerJointAttachment::updateLOD(F32 pixel_area, bool activate)
{
    bool res{ false };
    if (!mValid)
    {
        setValid(true, true);
        res = true;
    }
    return res;
}

bool LLViewerJointAttachment::isObjectAttached(const LLViewerObject *viewer_object) const
{
    for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
         iter != mAttachedObjects.end();
         ++iter)
    {
        const LLViewerObject* attached_object = iter->get();
        if (attached_object == viewer_object)
        {
            return true;
        }
    }
    return false;
}

const LLViewerObject *LLViewerJointAttachment::getAttachedObject(const LLUUID &object_id) const
{
    for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
         iter != mAttachedObjects.end();
         ++iter)
    {
        const LLViewerObject* attached_object = iter->get();
        if (attached_object->getAttachmentItemID() == object_id)
        {
            return attached_object;
        }
    }
    return nullptr;
}

LLViewerObject *LLViewerJointAttachment::getAttachedObject(const LLUUID &object_id)
{
    for (attachedobjs_vec_t::iterator iter = mAttachedObjects.begin();
         iter != mAttachedObjects.end();
         ++iter)
    {
        LLViewerObject* attached_object = iter->get();
        if (attached_object->getAttachmentItemID() == object_id)
        {
            return attached_object;
        }
    }
    return nullptr;
}
