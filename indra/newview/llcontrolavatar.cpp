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
#include "llskinningutil.h"

//#pragma optimize("", off)

const F32 LLControlAvatar::MAX_LEGAL_OFFSET = 3.0f;
const F32 LLControlAvatar::MAX_LEGAL_SIZE = 64.0f;

//static
boost::signals2::connection LLControlAvatar::sRegionChangedSlot;

LLControlAvatar::LLControlAvatar(const LLUUID& id, const LLPCode pcode, LLViewerRegion* regionp) :
    LLVOAvatar(id, pcode, regionp),
    mPlaying(false),
    mGlobalScale(1.0f),
    mMarkedForDeath(false),
    mRootVolp(NULL),
    mScaleConstraintFixup(1.0),
	mRegionChanged(false)
{
    mIsDummy = TRUE;
    mIsControlAvatar = true;
    mEnableDefaultMotions = false;
}

// virtual
LLControlAvatar::~LLControlAvatar()
{
	// Should already have been unlinked before destruction
	llassert(!mRootVolp);
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

    mInitFlags |= 1<<4;
}

void LLControlAvatar::getNewConstraintFixups(LLVector3& new_pos_fixup, F32& new_scale_fixup) const
{

    F32 max_legal_offset = MAX_LEGAL_OFFSET;
    if (gSavedSettings.getControl("AnimatedObjectsMaxLegalOffset"))
    {
        max_legal_offset = gSavedSettings.getF32("AnimatedObjectsMaxLegalOffset");
    }
	max_legal_offset = llmax(max_legal_offset,0.f);

    F32 max_legal_size = MAX_LEGAL_SIZE;
    if (gSavedSettings.getControl("AnimatedObjectsMaxLegalSize"))
    {
        max_legal_size = gSavedSettings.getF32("AnimatedObjectsMaxLegalSize");
    }
	max_legal_size = llmax(max_legal_size, 1.f);
    
    new_pos_fixup = LLVector3();
    new_scale_fixup = 1.0f;
	LLVector3 vol_pos = mRootVolp->getRenderPosition();

    // Fix up position if needed to prevent visual encroachment
    if (box_valid_and_non_zero(getLastAnimExtents())) // wait for state to settle down
    {
        // The goal here is to ensure that the extent of the avatar's 
        // bounding box does not wander too far from the
        // official position of the corresponding volume. We
        // do this by tracking the distance and applying a
        // correction to the control avatar position if
        // needed.
        const LLVector3 *extents = getLastAnimExtents();
		LLVector3 unshift_extents[2];
		unshift_extents[0] = extents[0] - mPositionConstraintFixup;
		unshift_extents[1] = extents[1] - mPositionConstraintFixup;
        LLVector3 box_dims = extents[1]-extents[0];
        F32 box_size = llmax(box_dims[0],box_dims[1],box_dims[2]);

		if (!mRootVolp->isAttachment())
		{
			LLVector3 pos_box_offset = point_to_box_offset(vol_pos, unshift_extents);
			F32 offset_dist = pos_box_offset.length();
			if (offset_dist > max_legal_offset && offset_dist > 0.f)
			{
				F32 target_dist = (offset_dist - max_legal_offset);
				new_pos_fixup = (target_dist/offset_dist)*pos_box_offset;
			}
			if (new_pos_fixup != mPositionConstraintFixup)
			{
				LL_DEBUGS("ConstraintFix") << getFullname() << " pos fix, offset_dist " << offset_dist << " pos fixup " 
										   << new_pos_fixup << " was " << mPositionConstraintFixup << LL_ENDL;
				LL_DEBUGS("ConstraintFix") << "vol_pos " << vol_pos << LL_ENDL;
				LL_DEBUGS("ConstraintFix") << "extents " << extents[0] << " " << extents[1] << LL_ENDL;
				LL_DEBUGS("ConstraintFix") << "unshift_extents " << unshift_extents[0] << " " << unshift_extents[1] << LL_ENDL;
				
			}
		}
        if (box_size/mScaleConstraintFixup > max_legal_size)
        {
            new_scale_fixup = mScaleConstraintFixup*max_legal_size/box_size;
            LL_DEBUGS("ConstraintFix") << getFullname() << " scale fix, box_size " << box_size << " fixup " 
									   << mScaleConstraintFixup << " max legal " << max_legal_size 
									   << " -> new scale " << new_scale_fixup << LL_ENDL;
        }
    }
}

void LLControlAvatar::matchVolumeTransform()
{
    if (mRootVolp)
    {
		LLVector3 new_pos_fixup;
		F32 new_scale_fixup;
		if (mRegionChanged)
		{
			new_scale_fixup = mScaleConstraintFixup;
			new_pos_fixup = mPositionConstraintFixup;
			mRegionChanged = false;
		}
		else
		{
			getNewConstraintFixups(new_pos_fixup, new_scale_fixup);
		}
		mPositionConstraintFixup = new_pos_fixup;
		mScaleConstraintFixup = new_scale_fixup;

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

				F32 global_scale = gSavedSettings.getF32("AnimatedObjectsGlobalScale");
				setGlobalScale(global_scale * mScaleConstraintFixup);
            }
            else
            {
                LL_WARNS_ONCE() << "can't find attached av!" << LL_ENDL;
            }
        }
        else
        {
            LLVector3 vol_pos = mRootVolp->getRenderPosition();

            // FIXME: Currently if you're doing something like playing an
            // animation that moves the pelvis (on an avatar or
            // animated object), the name tag and debug text will be
            // left behind. Ideally setPosition() would follow the
            // skeleton around in a smarter way, so name tags,
            // complexity info and such line up better. Should defer
            // this until avatars also get fixed.

            LLQuaternion obj_rot;
            if (mRootVolp->mDrawable)
            {
                obj_rot = mRootVolp->mDrawable->getRotation();
            }
            else
            {
                obj_rot = mRootVolp->getRotation();
            }
            
			LLMatrix3 bind_mat;

            LLQuaternion bind_rot;
#define MATCH_BIND_SHAPE
#ifdef MATCH_BIND_SHAPE
            // MAINT-8671 - based on a patch from Beq Janus
	        const LLMeshSkinInfo* skin_info = mRootVolp->getSkinInfo();
			if (skin_info)
			{
                LL_DEBUGS("BindShape") << getFullname() << " bind shape " << skin_info->mBindShapeMatrix << LL_ENDL;
                bind_rot = LLSkinningUtil::getUnscaledQuaternion(skin_info->mBindShapeMatrix);
			}
#endif
			setRotation(bind_rot*obj_rot);
            mRoot->setWorldRotation(bind_rot*obj_rot);
			setPositionAgent(vol_pos);
			mRoot->setPosition(vol_pos + mPositionConstraintFixup);

            F32 global_scale = gSavedSettings.getF32("AnimatedObjectsGlobalScale");
            setGlobalScale(global_scale * mScaleConstraintFixup);
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

    if (cav)
    {
        cav->mRootVolp = obj;

        // Sync up position/rotation with object
        cav->matchVolumeTransform();
    }

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
        std::string cam_dist_string = "";
        S32 cam_dist_count = 0;
        F32 lod_radius = mRootVolp->mLODRadius;

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
                    lod_radius = volp->mLODRadius;
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
                if (cam_dist_count < 4)
                {
                    cam_dist_string += LLStringOps::getReadableNumber(volp->mLODDistance) + "/" +
                        LLStringOps::getReadableNumber(volp->mLODAdjustedDistance) + " ";
                    cam_dist_count++;
                }
            }
            else
            {
                active_string += "-";
                type_string += "-";
            }
        }
        addDebugText(llformat("CAV obj %d anim %d active %s impost %d upprd %d strcst %f",
                              total_linkset_count, animated_volume_count, 
                              active_string.c_str(), (S32) isImpostor(), getUpdatePeriod(), streaming_cost));
        addDebugText(llformat("types %s lods %s", type_string.c_str(), lod_string.c_str()));
        addDebugText(llformat("flags %s", animated_object_flag_string.c_str()));
        addDebugText(llformat("tris %d (est %.1f, streaming %.1f), verts %d", total_tris, est_tris, est_streaming_tris, total_verts));
        addDebugText(llformat("pxarea %s rank %d", LLStringOps::getReadableNumber(getPixelArea()).c_str(), getVisibilityRank()));
        addDebugText(llformat("lod_radius %s dists %s", LLStringOps::getReadableNumber(lod_radius).c_str(),cam_dist_string.c_str()));
        if (mPositionConstraintFixup.length() > 0.0f || mScaleConstraintFixup != 1.0f)
        {
            addDebugText(llformat("pos fix (%.1f %.1f %.1f) scale %f", 
                                  mPositionConstraintFixup[0], 
                                  mPositionConstraintFixup[1],
                                  mPositionConstraintFixup[2],
                                  mScaleConstraintFixup));
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
        //if (!mRootVolp->isAnySelected())
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
    if (!mRootVolp)
    {
        return NULL;
    }

    LLViewerObject* hit = NULL;

    if (lineSegmentBoundingBox(start, end))
    {
        LLVector4a local_end = end;
        LLVector4a local_intersection;
        if (mRootVolp->lineSegmentIntersect(start, local_end, face, pick_transparent, pick_rigged, face_hit, &local_intersection, tex_coord, normal, tangent))
        {
            local_end = local_intersection;
            if (intersection)
            {
                *intersection = local_intersection;
            }
            hit = mRootVolp;
        }
        else
        {
            std::vector<LLVOVolume*> volumes;
            getAnimatedVolumes(volumes);

            for (std::vector<LLVOVolume*>::iterator vol_it = volumes.begin(); vol_it != volumes.end(); ++vol_it)
            {
                LLVOVolume *volp = *vol_it;
                if (mRootVolp != volp && volp->lineSegmentIntersect(start, local_end, face, pick_transparent, pick_rigged, face_hit, &local_intersection, tex_coord, normal, tangent))
                {
                    local_end = local_intersection;
                    if (intersection)
                    {
                        *intersection = local_intersection;
                    }
                    hit = volp;
                    break;
                }
            }
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

// virtual
bool LLControlAvatar::shouldRenderRigged() const
{
    if (mRootVolp && mRootVolp->isAttachment())
    {
        LLVOAvatar *attached_av = mRootVolp->getAvatarAncestor();
        if (attached_av)
        {
            return attached_av->shouldRenderRigged();
        }
    }
    return true;
}

// virtual
BOOL LLControlAvatar::isImpostor()
{
    if (mRootVolp && mRootVolp->isAttachment())
    {
		// Attached animated objects should match state of their attached av.
        LLVOAvatar *attached_av = mRootVolp->getAvatarAncestor();
		if (attached_av)
		{
			return attached_av->isImpostor();
		}
    }
	return LLVOAvatar::isImpostor();
}

//static
void LLControlAvatar::onRegionChanged()
{
	std::vector<LLCharacter*>::iterator it = LLCharacter::sInstances.begin();
	for ( ; it != LLCharacter::sInstances.end(); ++it)
	{
		LLControlAvatar* cav = dynamic_cast<LLControlAvatar*>(*it);
		if (!cav) continue;
		cav->mRegionChanged = true;
	}
}
