/** 
 * @file llviewerpartsource.h
 * @brief LLViewerPartSource class header file
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLVIEWERPARTSOURCE_H
#define LL_LLVIEWERPARTSOURCE_H

#include "llrefcount.h"
#include "llpartdata.h"
#include "llpointer.h"
#include "llquaternion.h"
#include "v3math.h"

////////////////////
//
// A particle source - subclassed to generate particles with different behaviors
//
//

class LLViewerTexture;
class LLViewerObject;
class LLViewerPart;
class LLVOAvatar;

class LLViewerPartSource : public LLRefCount
{
public:
    enum
    {
        LL_PART_SOURCE_NULL,
        LL_PART_SOURCE_SCRIPT,
        LL_PART_SOURCE_SPIRAL,
        LL_PART_SOURCE_BEAM,
        LL_PART_SOURCE_CHAT
    };

    LLViewerPartSource(const U32 type);

    virtual void update(const F32 dt); // Return FALSE if this source is dead...

    virtual void setDead();
    BOOL isDead() const             { return mIsDead; }
    void setSuspended( BOOL state ) { mIsSuspended = state; }
    BOOL isSuspended() const        { return mIsSuspended; }
    U32 getType() const             { return mType; }
    static void updatePart(LLViewerPart &part, const F32 dt);
    void setOwnerUUID(const LLUUID& owner_id) { mOwnerUUID = owner_id; }
    LLUUID getOwnerUUID() const { return mOwnerUUID; }
    U32 getID() const { return mID; }
    LLUUID getImageUUID() const;
    void  setStart() ;

    LLVector3   mPosAgent; // Location of the particle source
    LLVector3   mTargetPosAgent; // Location of the target position
    LLVector3   mLastUpdatePosAgent;
    LLPointer<LLViewerObject>   mSourceObjectp;
    U32 mID;
    LLViewerPart* mLastPart; //last particle emitted (for making particle ribbons)

protected:
    U32         mType;
    BOOL        mIsDead;
    BOOL        mIsSuspended;
    F32         mLastUpdateTime;
    F32         mLastPartTime;
    LLUUID      mOwnerUUID;
    LLPointer<LLVOAvatar> mOwnerAvatarp;
    LLPointer<LLViewerTexture>  mImagep;
    // Particle information
    U32         mPartFlags; // Flags for the particle
    U32         mDelay ; //delay to start particles
};



///////////////////////////////
//
// LLViewerPartSourceScript
//
// Particle source that handles the "generic" script-drive particle source
// attached to objects
//


class LLViewerPartSourceScript : public LLViewerPartSource
{
public:
    LLViewerPartSourceScript(LLViewerObject *source_objp);
    /*virtual*/ void update(const F32 dt);

    /*virtual*/ void setDead();

    BOOL updateFromMesg();

    // Returns a new particle source to attach to an object...
    static LLPointer<LLViewerPartSourceScript> unpackPSS(LLViewerObject *source_objp, LLPointer<LLViewerPartSourceScript> pssp, const S32 block_num);
    static LLPointer<LLViewerPartSourceScript> unpackPSS(LLViewerObject *source_objp, LLPointer<LLViewerPartSourceScript> pssp, LLDataPacker &dp, bool legacy);
    static LLPointer<LLViewerPartSourceScript> createPSS(LLViewerObject *source_objp, const LLPartSysData& particle_parameters);

    LLViewerTexture *getImage() const               { return mImagep; }
    void setImage(LLViewerTexture *imagep);
    LLPartSysData               mPartSysData;

    void setTargetObject(LLViewerObject *objp);

protected:
    LLQuaternion                mRotation;          // Current rotation for particle source
    LLPointer<LLViewerObject>   mTargetObjectp;     // Target object for the particle source
};


////////////////////////////
//
// Particle source for spiral effect (customize avatar, mostly)
//

class LLViewerPartSourceSpiral : public LLViewerPartSource
{
public:
    LLViewerPartSourceSpiral(const LLVector3 &pos);

    /*virtual*/ void setDead();

    /*virtual*/ void update(const F32 dt);

    void setSourceObject(LLViewerObject *objp);
    void setColor(const LLColor4 &color);

    static void updatePart(LLViewerPart &part, const F32 dt);
    LLColor4 mColor;
protected:
    LLVector3d mLKGSourcePosGlobal;
};


////////////////////////////
//
// Particle source for tractor(editing) beam
//

class LLViewerPartSourceBeam : public LLViewerPartSource
{
public:
    LLViewerPartSourceBeam();

    /*virtual*/ void setDead();

    /*virtual*/ void update(const F32 dt);

    void setSourceObject(LLViewerObject *objp);
    void setTargetObject(LLViewerObject *objp);
    void setSourcePosGlobal(const LLVector3d &pos_global);
    void setTargetPosGlobal(const LLVector3d &pos_global);
    void setColor(const LLColor4 &color);

    static void updatePart(LLViewerPart &part, const F32 dt);
    LLPointer<LLViewerObject>   mTargetObjectp;
    LLVector3d      mLKGTargetPosGlobal;
    LLColor4 mColor;
protected:
    ~LLViewerPartSourceBeam();
};



//////////////////////////
//
// Particle source for chat effect
//

class LLViewerPartSourceChat : public LLViewerPartSource
{
public:
    LLViewerPartSourceChat(const LLVector3 &pos);

    /*virtual*/ void setDead();

    /*virtual*/ void update(const F32 dt);

    void setSourceObject(LLViewerObject *objp);
    void setColor(const LLColor4 &color);
    static void updatePart(LLViewerPart &part, const F32 dt);
    LLColor4 mColor;
protected:
    LLVector3d mLKGSourcePosGlobal;
};


#endif // LL_LLVIEWERPARTSOURCE_H
