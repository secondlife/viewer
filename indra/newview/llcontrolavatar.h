/**
 * @file llcontrolavatar.h
 * @brief Special dummy avatar used to drive rigged meshes.
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

#ifndef LL_CONTROLAVATAR_H
#define LL_CONTROLAVATAR_H

#include "llvoavatar.h"
#include "llvovolume.h"

class LLControlAvatar:
    public LLVOAvatar
{
    LOG_CLASS(LLControlAvatar);

public:
    LLControlAvatar(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual void 			initInstance(); // Called after construction to initialize the class.
	virtual	~LLControlAvatar();

    void matchVolumeTransform();
    void updateVolumeGeom();
        
    void setGlobalScale(F32 scale);
    void recursiveScaleJoint(LLJoint *joint, F32 factor);
    static LLControlAvatar *createControlAvatar(LLVOVolume *obj);

    // Delayed kill so we don't make graphics pipeline unhappy calling
    // markDead() inside other graphics pipeline operations.
    void markForDeath();

    virtual void idleUpdate(LLAgent &agent, const F64 &time);
	virtual BOOL updateCharacter(LLAgent &agent);

    void getAnimatedVolumes(std::vector<LLVOVolume*>& volumes);
    void updateAnimations();  
    
	virtual LLViewerObject*	lineSegmentIntersectRiggedAttachments(
        const LLVector4a& start, const LLVector4a& end,
        S32 face = -1,                    // which face to check, -1 = ALL_SIDES
        BOOL pick_transparent = FALSE,
        BOOL pick_rigged = FALSE,
        S32* face_hit = NULL,             // which face was hit
        LLVector4a* intersection = NULL,   // return the intersection point
        LLVector2* tex_coord = NULL,      // return the texture coordinates of the intersection point
        LLVector4a* normal = NULL,         // return the surface normal at the intersection point
        LLVector4a* tangent = NULL);     // return the surface tangent at the intersection point

	virtual void	updateDebugText();

    virtual std::string getFullname() const;

    bool mPlaying;

    F32 mGlobalScale;

    LLVOVolume *mRootVolp;

    bool mMarkedForDeath;

};

#endif //LL_CONTROLAVATAR_H
