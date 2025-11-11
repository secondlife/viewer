/**
 * @file llhudeffectresetskeleton.cpp
 * @brief LLHUDEffectResetSkeleton class implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llhudeffectresetskeleton.h"

#include "llagent.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "message.h"

// packet layout
const S32 TARGET_OBJECT = 0; // This is to allow for targetting owned animesh
const S32 RESET_ANIMATIONS = 16; //This can also be a flags if needed
const S32 PKT_SIZE = 17;

//-----------------------------------------------------------------------------
// LLHUDEffectResetSkeleton()
//-----------------------------------------------------------------------------
LLHUDEffectResetSkeleton::LLHUDEffectResetSkeleton(const U8 type) :
    LLHUDEffect(type)
{
}

//-----------------------------------------------------------------------------
// ~LLHUDEffectResetSkeleton()
//-----------------------------------------------------------------------------
LLHUDEffectResetSkeleton::~LLHUDEffectResetSkeleton()
{
}

//-----------------------------------------------------------------------------
// packData()
//-----------------------------------------------------------------------------
void LLHUDEffectResetSkeleton::render()
{
    // HUDEffectResetSkeleton is a fake effect meant to reset skeleton only.
    // Just wait for an update() call to do its work and then die.
}

//-----------------------------------------------------------------------------
// packData()
//-----------------------------------------------------------------------------
void LLHUDEffectResetSkeleton::packData(LLMessageSystem *mesgsys)
{
    // Pack the default data
    LLHUDEffect::packData(mesgsys);

    // Pack the type-specific data.  Uses a fun packed binary format.  Whee!
    U8 packed_data[PKT_SIZE];
    memset(packed_data, 0, PKT_SIZE);

    // pack both target object and position
    // position interpreted as offset if target object is non-null
    if (mTargetObject)
    {
        htolememcpy(&(packed_data[TARGET_OBJECT]), mTargetObject->mID.mData, MVT_LLUUID, 16);
    }
    else
    {
        htolememcpy(&(packed_data[TARGET_OBJECT]), LLUUID::null.mData, MVT_LLUUID, 16);
    }

    U8 resetAnimations = (U8)mResetAnimations;
    htolememcpy(&(packed_data[RESET_ANIMATIONS]), &resetAnimations, MVT_U8, 1);

    mesgsys->addBinaryDataFast(_PREHASH_TypeData, packed_data, PKT_SIZE);
}

//-----------------------------------------------------------------------------
// unpackData()
//-----------------------------------------------------------------------------
void LLHUDEffectResetSkeleton::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
    LLVector3d new_target;
    U8 packed_data[PKT_SIZE];


    LLHUDEffect::unpackData(mesgsys, blocknum);

    LLUUID source_id;
    mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_AgentID, source_id, blocknum);

    LLViewerObject *objp = gObjectList.findObject(source_id);
    if (objp && objp->isAvatar())
    {
        setSourceObject(objp);
    }
    else
    {
        //LL_WARNS() << "Could not find source avatar for ResetSkeleton effect" << LL_ENDL;
        return;
    }

    S32 size = mesgsys->getSizeFast(_PREHASH_Effect, blocknum, _PREHASH_TypeData);
    if (size != PKT_SIZE)
    {
        LL_WARNS() << "ResetSkeleton effect with bad size " << size << LL_ENDL;
        return;
    }

    mesgsys->getBinaryDataFast(_PREHASH_Effect, _PREHASH_TypeData, packed_data, PKT_SIZE, blocknum);

    LLUUID target_id;
    htolememcpy(target_id.mData, &(packed_data[TARGET_OBJECT]), MVT_LLUUID, 16);

    // The purpose for having a target ID is if we want to reset animesh, or
    // other things in the future.
    // I implemented this, but due to issues regarding various permission
    // checks, I scrapped it for now. --Chaser Zaks
    // See https://github.com/secondlife/viewer/pull/1212 for additional info

    if (target_id.isNull())
    {
        target_id = source_id;
    }

    objp = gObjectList.findObject(target_id);

    if (objp)
    {
        setTargetObject(objp);
    }

    U8 resetAnimations = 0;
    htolememcpy(&resetAnimations, &(packed_data[RESET_ANIMATIONS]), MVT_U8, 1);

    // Pre-emptively assume this is going to be flags in the future.
    // It isn't needed now, but this will assure that only bit 1 is set
    mResetAnimations = resetAnimations & 1;

    update();
}

//-----------------------------------------------------------------------------
// setTargetObjectAndOffset()
//-----------------------------------------------------------------------------
void LLHUDEffectResetSkeleton::setTargetObject(LLViewerObject *objp)
{
    mTargetObject = objp;
}


//-----------------------------------------------------------------------------
// markDead()
//-----------------------------------------------------------------------------
void LLHUDEffectResetSkeleton::markDead()
{
    LLHUDEffect::markDead();
}

void LLHUDEffectResetSkeleton::setSourceObject(LLViewerObject* objectp)
{
    // restrict source objects to avatars
    if (objectp && objectp->isAvatar())
    {
        LLHUDEffect::setSourceObject(objectp);
    }
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
void LLHUDEffectResetSkeleton::update()
{
    // If the target object is dead, set the target object to NULL
    if (mTargetObject.isNull() || mTargetObject->isDead())
    {
        markDead();
        return;
    }

    if (mSourceObject.isNull() || mSourceObject->isDead())
    {
        markDead();
        return;
    }

    if (mTargetObject->isAvatar())
    {
        // Only the owner of a avatar can reset their skeleton like this
        // Also allow reset if we created the effect (Local resetting)
        if (mSourceObject->getID() == mTargetObject->getID() || getOriginatedHere())
        {
            LLVOAvatar* avatar = mTargetObject->asAvatar();
            avatar->resetSkeleton(mResetAnimations);
        }
    }
    else
    {
        LL_WARNS() << mSourceObject->getID() << " attempted to reset skeleton on "
                << mTargetObject->getID() << ", but it is not a avatar!" << LL_ENDL;
    }

    markDead();
}
