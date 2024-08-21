/**
 * @file llbakingavatar.h
 * @brief Declaration of LLBakingAvatar class which is a derivation of LLAvatarAppearance
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLBAKINGAVATAR_H
#define LL_LLBAKINGAVATAR_H

#include "llavatarappearance.h"

class LLBakingAvatar : public LLAvatarAppearance
{
    LOG_CLASS(LLBakingAvatar);

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/
public:
    void* operator new(size_t size)
    {
        return ll_aligned_malloc_16(size);
    }

    void operator delete(void* ptr)
    {
        ll_aligned_free_16(ptr);
    }

    LLBakingAvatar(LLWearableData* wearable_data, S32 bakeTextureSize = 512);
    virtual ~LLBakingAvatar();

    static void initClass(); // initializes static members

/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    INHERITED
 **/

    //--------------------------------------------------------------------
    // LLCharacter interface
    //--------------------------------------------------------------------
public:
    LLVector3       getCharacterPosition() override;
    LLQuaternion    getCharacterRotation() override;
    LLVector3       getCharacterVelocity() override;
    LLVector3       getCharacterAngularVelocity() override;

    const LLUUID&   getID() const override;
    void            addDebugText(const std::string& text) override;
    F32             getTimeDilation() override;
    void            getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm) override;
    F32             getPixelArea() const override;
    LLVector3d      getPosGlobalFromAgent(const LLVector3 &position) override;
    LLVector3       getPosAgentFromGlobal(const LLVector3d &position) override;

    //--------------------------------------------------------------------
    // LLAvatarAppearance interface
    //--------------------------------------------------------------------
public:
    void    applyMorphMask(const U8* tex_data, S32 width, S32 height, S32 num_components,
                            LLAvatarAppearanceDefines::EBakedTextureIndex index) override;
    void    invalidateComposite(LLTexLayerSet* layerset) override;
    void    updateMeshTextures() override;
    void    dirtyMesh() override; // Dirty the avatar mesh
    void    onGlobalColorChanged(const LLTexGlobalColor* global_color) override;
    bool    isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex type, U32 index = 0) const override;
    bool    isUsingLocalAppearance() const override { return FALSE; }
    bool    isEditingAppearance() const override { return FALSE; }
private:
    void    dirtyMesh(S32 priority) override; // Dirty the avatar mesh, with priority

    // LLAvatarAppearance instance factories:
protected:
    LLAvatarJoint*  createAvatarJoint() override;
    LLAvatarJoint*  createAvatarJoint(S32 joint_num) override;
    LLAvatarJointMesh*  createAvatarJointMesh() override;
    LLTexLayerSet*  createTexLayerSet() override;


/**                    Inherited
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/
public:
    bool    isSelf() const override { return true; }
    bool    isValid() const override { return TRUE; }

/**                    State
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    BAKING
 **/
public:
    void    bakedTextureDatasAsLLSD(LLSD& sd) const;
    S32 bakeTextureSize() const { return mBakeTextureSize; }

/**                    Baking
 **                                                                            **
 *******************************************************************************/

private:
    S32 mBakeTextureSize;
};

#endif /* LL_LLBAKINGAVATAR_H */

