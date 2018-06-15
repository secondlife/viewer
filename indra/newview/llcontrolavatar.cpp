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
    mIsDummy = TRUE;
    mIsControlAvatar = true;
    mEnableDefaultMotions = false;
}

// virtual
LLControlAvatar::~LLControlAvatar()
{
}

// virtual
void LLControlAvatar::initInstance()
{
	// Potential optimizations here: avoid creating system
	// avatar mesh content since it's not used. For now we just clean some
	// things up after the fact in releaseMeshData().
    LLVOAvatar::initInstance();

	createDrawable(&gPipeline);
	updateJointLODs();
	updateGeometry(mDrawable);
	hideSkirt();
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

            LLVector3 vol_pos = mRootVolp->getRenderPosition();
            LLVector3 pos_box_offset;
            LLVector3 box_offset;

            // Fix up position if needed to prevent visual encroachment
            if (box_valid_and_non_zero(getLastAnimExtents())) // wait for state to settle down
            {
                const F32 MAX_LEGAL_OFFSET = 3.0;
                
                // The goal here is to ensure that the extent of the avatar's 
                // bounding box does not wander too far from the
                // official position of the corresponding volume. We
                // do this by tracking the distance and applying a
                // correction to the control avatar position if
                // needed.
                LLVector3 uncorrected_extents[2];
                uncorrected_extents[0] = getLastAnimExtents()[0] - mPositionConstraintFixup;
                uncorrected_extents[1] = getLastAnimExtents()[1] - mPositionConstraintFixup;
                pos_box_offset = point_to_box_offset(vol_pos, uncorrected_extents);
                F32 offset_dist = pos_box_offset.length();
                if (offset_dist > MAX_LEGAL_OFFSET)
                {
                    F32 target_dist = (offset_dist - MAX_LEGAL_OFFSET);
                    box_offset = (target_dist/offset_dist)*pos_box_offset;
                }
            }

            mPositionConstraintFixup = box_offset;

            // Currently if you're doing something like playing an
            // animation that moves the pelvis (on an avatar or
            // animated object), the name tag and debug text will be
            // left behind. Ideally setPosition() would follow the
            // skeleton around in a smarter way, so name tags,
            // complexity info and such line up better. Should defer
            // this until avatars also get fixed.
            setPositionAgent(vol_pos);

            LLQuaternion obj_rot = mRootVolp->getRotation();
            LLQuaternion result_rot = obj_rot;
            setRotation(result_rot);
            mRoot->setWorldRotation(result_rot);
            mRoot->setPosition(vol_pos + mPositionConstraintFixup);
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
        // should we be scaling from the pelvis or the root?
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

    // Initial exploration of allowing scaling skeleton to match root
    // prim bounding box. If enabled, would probably be controlled by
    // an additional checkbox and default to off. Not enabled for
    // initial release.

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
	LLControlAvatar *cav = (LLControlAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR, gAgent.getRegion(), CO_FLAG_CONTROL_AVATAR);

    cav->mRootVolp = obj;

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
        std::string animated_object_flag_string;
        S32 total_tris = 0;
        S32 total_verts = 0;
        F32 est_tris = 0.f;
        F32 est_streaming_tris = 0.f;
        F32 streaming_cost = 0.f;
        
        for (std::vector<LLVOVolume*>::iterator it = volumes.begin();
             it != volumes.end(); ++it)
        {
            LLVOVolume *volp = *it;
            S32 verts = 0;
            total_tris += volp->getTriangleCount(&verts);
            total_verts += verts;
            est_tris += volp->getEstTrianglesMax();
            est_streaming_tris += volp->getEstTrianglesStreamingCost();
            streaming_cost += volp->getStreamingCost();
            lod_string += llformat("%d",volp->getLOD());
            if (volp && volp->mDrawable)
            {
                bool is_animated_flag = volp->getExtendedMeshFlags() & LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG;
                if (is_animated_flag)
                {
                    animated_object_flag_string += "1";
                }
                else
                {
                    animated_object_flag_string += "0";
                }
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
                    // Rigged/animatable mesh
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
        addDebugText(llformat("CAV obj %d anim %d active %s impost %d strcst %f",
                              total_linkset_count, animated_volume_count, 
                              active_string.c_str(), (S32) isImpostor(), streaming_cost));
        addDebugText(llformat("types %s lods %s", type_string.c_str(), lod_string.c_str()));
        addDebugText(llformat("flags %s", animated_object_flag_string.c_str()));
        addDebugText(llformat("tris %d (est %.1f, streaming %.1f), verts %d", total_tris, est_tris, est_streaming_tris, total_verts));
        addDebugText(llformat("pxarea %s rank %d", LLStringOps::getReadableNumber(getPixelArea()).c_str(), getVisibilityRank()));
        if (mPositionConstraintFixup.length() > 0.0f)
        {
            addDebugText(llformat("pos fix (%.1f %.1f %.1f)", 
                                  mPositionConstraintFixup[0], mPositionConstraintFixup[1], mPositionConstraintFixup[2]));
        }
        
#if 0
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
#endif
        
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
        LL_WARNS_ONCE("AnimatedObjectsNotify") << "No root vol" << LL_ENDL;
        return;
    }

    std::vector<LLVOVolume*> volumes;
    getAnimatedVolumes(volumes);
    
    // Rebuild mSignaledAnimations from the associated volumes.
	std::map<LLUUID, S32> anims;
    for (std::vector<LLVOVolume*>::iterator vol_it = volumes.begin(); vol_it != volumes.end(); ++vol_it)
    {
        LLVOVolume *volp = *vol_it;
        //LL_INFOS("AnimatedObjects") << "updating anim for vol " << volp->getID() << " root " << mRootVolp->getID() << LL_ENDL;
        signaled_animation_map_t& signaled_animations = LLObjectSignaledAnimationMap::instance().getMap()[volp->getID()];
        for (std::map<LLUUID,S32>::iterator anim_it = signaled_animations.begin();
             anim_it != signaled_animations.end();
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
            LL_DEBUGS("AnimatedObjectsNotify") << "found anim for vol " << volp->getID() << " anim " << anim_it->first << " root " << mRootVolp->getID() << LL_ENDL;
        }
    }
    if (!mPlaying)
    {
        mPlaying = true;
        if (!mRootVolp->isAnySelected())
        {
            updateVolumeGeom();
            mRootVolp->recursiveMarkForUpdate(TRUE);
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

// virtual
std::string LLControlAvatar::getFullname() const
{
    if (mRootVolp)
    {
        return "AO_" + mRootVolp->getID().getString();
    }
    else
    {
        return "AO_no_root_vol";
    }
}
