/**
 * @file llcontrolavatar.cpp
 * @brief Implementation for special dummy avatar used to drive rigged meshes.
 *
 * $LicenseInfo:firstyear=2017&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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
#include "llcontrolavatar.h"
#include "llagent.h" //  Get state values from here
#include "llviewerobjectlist.h"
#include "pipeline.h"
#include "llanimationstates.h"

LLControlAvatar::LLControlAvatar(const LLUUID& id, const LLPCode pcode, LLViewerRegion* regionp) :
    LLVOAvatar(id, pcode, regionp),
    mPlaying(false)
{
    mIsControlAvatar = true;
}

// virtual
LLControlAvatar::~LLControlAvatar()
{
}

void LLControlAvatar::matchTransform(LLVOVolume *obj)
{
	setPositionAgent(obj->getRenderPosition());
	//slamPosition();

    LLQuaternion fix_axes_rot(-F_PI_BY_TWO, LLVector3(0,0,1));
    LLQuaternion obj_rot = obj->getRotation();
    LLQuaternion result_rot = fix_axes_rot * obj_rot;
	setRotation(result_rot);
    mRoot->setWorldRotation(result_rot);
    mRoot->setPosition(obj->getRenderPosition());
}

// Based on LLViewerJointAttachment::setupDrawable(), without the attaching part.
void LLControlAvatar::updateGeom(LLVOVolume *obj)
{
	if (!obj->mDrawable)
		return;
	if (obj->mDrawable->isActive())
	{
		obj->mDrawable->makeStatic(FALSE);
	}
	obj->mDrawable->makeActive();
	gPipeline.markMoved(obj->mDrawable);
	gPipeline.markTextured(obj->mDrawable); // face may need to change draw pool to/from POOL_HUD
	obj->mDrawable->setState(LLDrawable::USE_BACKLIGHT);

	LLViewerObject::const_child_list_t& child_list = obj->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); ++iter)
	{
		LLViewerObject* childp = *iter;
		if (childp && childp->mDrawable.notNull())
		{
			childp->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
			gPipeline.markTextured(childp->mDrawable); // face may need to change draw pool to/from POOL_HUD
			gPipeline.markMoved(childp->mDrawable);
        }
    }

    gPipeline.markRebuild(obj->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
    obj->markForUpdate(TRUE);

    matchTransform(obj);
    //addAttachmentOverridesForObject(obj);
}

LLControlAvatar *LLControlAvatar::createControlAvatar(LLVOVolume *obj)
{
    // AXON Lifted from LLPreviewAnimation
	LLControlAvatar *cav = (LLControlAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR, gAgent.getRegion(), CO_FLAG_CONTROL_AVATAR);
	cav->createDrawable(&gPipeline);
	cav->mIsDummy = TRUE;
	cav->mSpecialRenderMode = 1;
	cav->updateJointLODs();
	cav->updateGeometry(cav->mDrawable);
	cav->startMotion(ANIM_AGENT_STAND, 5.0f);
	cav->hideSkirt();

	// stop extraneous animations
	cav->stopMotion( ANIM_AGENT_HEAD_ROT, TRUE );
	cav->stopMotion( ANIM_AGENT_EYE, TRUE );
	cav->stopMotion( ANIM_AGENT_BODY_NOISE, TRUE );
	cav->stopMotion( ANIM_AGENT_BREATHE_ROT, TRUE );

    // Sync up position/rotation with object
    cav->matchTransform(obj);

    return cav;
}

