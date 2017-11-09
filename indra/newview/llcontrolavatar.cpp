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
#include "llviewercontrol.h"
#include "llmeshrepository.h"
#include "llviewerregion.h"

LLControlAvatar::LLControlAvatar(const LLUUID& id, const LLPCode pcode, LLViewerRegion* regionp) :
    LLVOAvatar(id, pcode, regionp),
    mPlaying(false),
    mGlobalScale(1.0f),
    mMarkedForDeath(false)
{
    mIsControlAvatar = true;
    mEnableDefaultMotions = false;
}

// virtual
LLControlAvatar::~LLControlAvatar()
{
}

void LLControlAvatar::matchVolumeTransform()
{
    if (mRootVolp)
    {
        if (mRootVolp->isAttachment())
        {
            LLVOAvatar *attached_av = mRootVolp->getAvatarAncestor();
            if (attached_av)
            {
                LLViewerJointAttachment *attach = attached_av->getTargetAttachmentPoint(mRootVolp);
                setPositionAgent(mRootVolp->getRenderPosition());
				attach->updateWorldPRSParent();
                LLVector3 joint_pos = attach->getWorldPosition();
                LLQuaternion joint_rot = attach->getWorldRotation();
                LLVector3 obj_pos = mRootVolp->mDrawable->getPosition();
                LLQuaternion obj_rot = mRootVolp->mDrawable->getRotation();
                obj_pos.rotVec(joint_rot);
                mRoot->setWorldPosition(obj_pos + joint_pos);
                mRoot->setWorldRotation(obj_rot * joint_rot);
                setRotation(mRoot->getRotation());
            }
            else
            {
                LL_WARNS_ONCE() << "can't find attached av!" << LL_ENDL;
            }
        }
        else
        {
            setPositionAgent(mRootVolp->getRenderPosition());
            LLQuaternion obj_rot = mRootVolp->getRotation();
            LLQuaternion result_rot = obj_rot;
            setRotation(result_rot);
            mRoot->setWorldRotation(result_rot);
            mRoot->setPosition(mRootVolp->getRenderPosition());
        }
    }
}

void LLControlAvatar::setGlobalScale(F32 scale)
{
    if (scale <= 0.0)
    {
        LL_WARNS() << "invalid global scale " << scale << LL_ENDL;
        return;
    }
    if (scale != mGlobalScale)
    {
        F32 adjust_scale = scale/mGlobalScale;
        LL_INFOS() << "scale " << scale << " adjustment " << adjust_scale << LL_ENDL;
        // AXON - should we be scaling from the pelvis or the root?
        recursiveScaleJoint(mPelvisp,adjust_scale);
        mGlobalScale = scale;
    }
}

void LLControlAvatar::recursiveScaleJoint(LLJoint* joint, F32 factor)
{
    joint->setScale(factor * joint->getScale());
    
	for (LLJoint::child_list_t::iterator iter = joint->mChildren.begin();
		 iter != joint->mChildren.end(); ++iter)
	{
		LLJoint* child = *iter;
		recursiveScaleJoint(child, factor);
	}
}

// Based on LLViewerJointAttachment::setupDrawable(), without the attaching part.
void LLControlAvatar::updateVolumeGeom()
{
	if (!mRootVolp->mDrawable)
		return;
	if (mRootVolp->mDrawable->isActive())
	{
		mRootVolp->mDrawable->makeStatic(FALSE);
	}
	mRootVolp->mDrawable->makeActive();
	gPipeline.markMoved(mRootVolp->mDrawable);
	gPipeline.markTextured(mRootVolp->mDrawable); // face may need to change draw pool to/from POOL_HUD
	mRootVolp->mDrawable->setState(LLDrawable::USE_BACKLIGHT);

	LLViewerObject::const_child_list_t& child_list = mRootVolp->getChildren();
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

    gPipeline.markRebuild(mRootVolp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
    mRootVolp->markForUpdate(TRUE);

    // Note that attachment overrides aren't needed here, have already
    // been applied at the time the mControlAvatar was created, in
    // llvovolume.cpp.

    matchVolumeTransform();

    // AXON testing scale

    // What should the scale be? What we really want is the ratio
    // between the scale at which the object was originally designed
    // and rigged, and the scale to which it has been subsequently
    // modified - for example, if the object has been scaled down by a
    // factor of 2 then we should use 0.5 as the global scale. But we
    // don't have the original scale stored anywhere, just the current
    // scale. Possibilities - 1) remember the original scale
    // somewhere, 2) add another field to let the user specify the
    // global scale, 3) approximate the original scale by looking at
    // the proportions of the skeleton after joint positions have
    // been applied
    
    //LLVector3 obj_scale = obj->getScale();
    //F32 obj_scale_z = llmax(obj_scale[2],0.1f);
    //setGlobalScale(obj_scale_z/2.0f); // roughly fit avatar height range (2m) into object height
}

LLControlAvatar *LLControlAvatar::createControlAvatar(LLVOVolume *obj)
{
    // AXON Lifted from LLPreviewAnimation
	LLControlAvatar *cav = (LLControlAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR, gAgent.getRegion(), CO_FLAG_CONTROL_AVATAR);

    cav->mRootVolp = obj;
    
	cav->createDrawable(&gPipeline);
	cav->mIsDummy = TRUE;
	cav->mSpecialRenderMode = 1;
	cav->updateJointLODs();
	cav->updateGeometry(cav->mDrawable);
	cav->hideSkirt();

    // Sync up position/rotation with object
    cav->matchVolumeTransform();

    return cav;
}

void LLControlAvatar::markForDeath()
{
    mMarkedForDeath = true;
}

void LLControlAvatar::idleUpdate(LLAgent &agent, const F64 &time)
{
    if (mMarkedForDeath)
    {
        markDead();
        mMarkedForDeath = false;
    }
    else
    {
        LLVOAvatar::idleUpdate(agent,time);
    }
}

BOOL LLControlAvatar::updateCharacter(LLAgent &agent)
{
    return LLVOAvatar::updateCharacter(agent);
}

//virtual
void LLControlAvatar::updateDebugText()
{
	if (gSavedSettings.getBOOL("DebugAnimatedObjects"))
    {
        S32 total_linkset_count = 0;
        if (mRootVolp)
        {
            total_linkset_count = 1 + mRootVolp->getChildren().size();
        }
        std::vector<LLVOVolume*> volumes;
        getAnimatedVolumes(volumes);
        S32 animated_volume_count = volumes.size();
        std::string active_string;
        std::string type_string;
        std::string lod_string;
        S32 total_tris = 0;
        S32 total_verts = 0;
        for (std::vector<LLVOVolume*>::iterator it = volumes.begin();
             it != volumes.end(); ++it)
        {
            LLVOVolume *volp = *it;
            S32 verts = 0;
            total_tris += volp->getTriangleCount(&verts);
            total_verts += verts;
            lod_string += llformat("%d",volp->getLOD());
            if (volp && volp->mDrawable)
            {
                if (volp->mDrawable->isActive())
                {
                    active_string += "A";
                }
                else
                {
                    active_string += "S";
                }
                if (volp->isRiggedMesh())
                {
                    // Rigged/animateable mesh
                    type_string += "R";
                }
                else if (volp->isMesh())
                {
                    // Static mesh
                    type_string += "M";
                }
                else
                {
                    // Any other prim
                    type_string += "P";
                }
            }
            else
            {
                active_string += "-";
                type_string += "-";
            }
        }
        addDebugText(llformat("CAV obj %d anim %d active %s",
                              total_linkset_count, animated_volume_count, active_string.c_str()));
        addDebugText(llformat("types %s lods %s", type_string.c_str(), lod_string.c_str()));
        addDebugText(llformat("tris %d verts %d", total_tris, total_verts));
        std::string region_name = "no region";
        if (mRootVolp->getRegion())
        {
            region_name = mRootVolp->getRegion()->getName();
        }
        std::string skel_region_name = "skel no region";
        if (getRegion())
        {
            skel_region_name = getRegion()->getName();
        }
        addDebugText(llformat("region %x %s skel %x %s",
                              mRootVolp->getRegion(), region_name.c_str(),
                              getRegion(), skel_region_name.c_str()));
        //addDebugText(llformat("anim time %.1f (step %f factor %f)", 
        //                      mMotionController.getAnimTime(),
        //                      mMotionController.getTimeStep(), 
        //                      mMotionController.getTimeFactor()));
        
    }
    LLVOAvatar::updateDebugText();
}

void LLControlAvatar::getAnimatedVolumes(std::vector<LLVOVolume*>& volumes)
{
    if (!mRootVolp)
    {
        return;
    }

    volumes.push_back(mRootVolp);
    
	LLViewerObject::const_child_list_t& child_list = mRootVolp->getChildren();
	for (LLViewerObject::const_child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); ++iter)
	{
		LLViewerObject* childp = *iter;
        LLVOVolume *child_volp = dynamic_cast<LLVOVolume*>(childp);
        if (child_volp && child_volp->isAnimatedObject())
        {
            volumes.push_back(child_volp);
        }
    }
}

// This is called after an associated object receives an animation
// message. Combine the signaled animations for all associated objects
// and process any resulting state changes.
void LLControlAvatar::updateAnimations()
{
    if (!mRootVolp)
    {
        LL_WARNS_ONCE("AXON") << "No root vol" << LL_ENDL;
        return;
    }

    std::vector<LLVOVolume*> volumes;
    getAnimatedVolumes(volumes);
    
    // Rebuild mSignaledAnimations from the associated volumes.
	std::map<LLUUID, S32> anims;
    for (std::vector<LLVOVolume*>::iterator vol_it = volumes.begin(); vol_it != volumes.end(); ++vol_it)
    {
        LLVOVolume *volp = *vol_it;
        for (std::map<LLUUID,S32>::iterator anim_it = volp->mObjectSignaledAnimations.begin();
             anim_it != volp->mObjectSignaledAnimations.end();
             ++anim_it)
        {
            std::map<LLUUID,S32>::iterator found_anim_it = anims.find(anim_it->first);
            if (found_anim_it != anims.end())
            {
                // Animation already present, use the larger sequence id
                anims[anim_it->first] = llmax(found_anim_it->second, anim_it->second);
            }
            else
            {
                // Animation not already present, use this sequence id.
                anims[anim_it->first] = anim_it->second;
            }
        }
    }
    mSignaledAnimations = anims;

    processAnimationStateChanges();
}

// virtual
LLViewerObject* LLControlAvatar::lineSegmentIntersectRiggedAttachments(const LLVector4a& start, const LLVector4a& end,
									  S32 face,
									  BOOL pick_transparent,
									  BOOL pick_rigged,
									  S32* face_hit,
									  LLVector4a* intersection,
									  LLVector2* tex_coord,
									  LLVector4a* normal,
									  LLVector4a* tangent)
{
	LLViewerObject* hit = NULL;

	if (lineSegmentBoundingBox(start, end))
	{
		LLVector4a local_end = end;
		LLVector4a local_intersection;

        if (mRootVolp &&
            mRootVolp->lineSegmentIntersect(start, local_end, face, pick_transparent, pick_rigged, face_hit, &local_intersection, tex_coord, normal, tangent))
        {
            local_end = local_intersection;
            if (intersection)
            {
                *intersection = local_intersection;
            }
			
            hit = mRootVolp;
        }
	}
		
	return hit;
}
