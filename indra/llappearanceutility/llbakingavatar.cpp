/**
 * @File llbakingavatar.cpp
 * @brief Implementation of LLBakingAvatar class which is a derivation of LLAvatarAppearance
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

// linden includes
#include "linden_common.h"
#include "v3dmath.h"

// appearance includes
#include "llavatarappearancedefines.h"

// project includes
#include "llbakingavatar.h"
#include "llbakingjoint.h"
#include "llbakingjointmesh.h"
#include "llbakingtexlayer.h"

using namespace LLAvatarAppearanceDefines;

LLBakingAvatar::LLBakingAvatar(LLWearableData* wearable_data,S32 bakeTextureSize) :
    LLAvatarAppearance(wearable_data),
    mBakeTextureSize(bakeTextureSize)
{
}

// virtual
LLBakingAvatar::~LLBakingAvatar()
{
}

//-----------------------------------------------------------------------------
// Implemented methods
//-----------------------------------------------------------------------------

LLAvatarJoint* LLBakingAvatar::createAvatarJoint()
{
    return new LLBakingJoint();
}

LLAvatarJoint* LLBakingAvatar::createAvatarJoint(S32 joint_num)
{
    return new LLBakingJoint(joint_num);
}

LLAvatarJointMesh* LLBakingAvatar::createAvatarJointMesh()
{
    return new LLBakingJointMesh();
}

LLTexLayerSet* LLBakingAvatar::createTexLayerSet()
{
    return new LLBakingTexLayerSet(this);
}

void LLBakingAvatar::bakedTextureDatasAsLLSD(LLSD& sd) const
{
    bakedtexturedata_vec_t::const_iterator baked_iter = mBakedTextureDatas.begin();
    bakedtexturedata_vec_t::const_iterator baked_end  = mBakedTextureDatas.end();
    for (; baked_iter != baked_end; ++baked_iter)
    {
        LLBakingTexLayerSet* layer_set = dynamic_cast<LLBakingTexLayerSet*>(baked_iter->mTexLayerSet);
        LLSD layer_sd;
        layer_set->asLLSD(layer_sd);
        sd[LLAvatarAppearance::getDictionary()->getTexture(baked_iter->mTextureIndex)->mName] = layer_sd;
    }
}

//-----------------------------------------------------------------------------
// (Ignored) Non-implemented methods.
//-----------------------------------------------------------------------------

void LLBakingAvatar::bodySizeChanged() {}
void LLBakingAvatar::applyMorphMask(const U8* tex_data, S32 width, S32 height, S32 num_components,
        LLAvatarAppearanceDefines::EBakedTextureIndex index) {}
void LLBakingAvatar::invalidateComposite(LLTexLayerSet* layerset) {}
void LLBakingAvatar::updateMeshTextures() {}
void LLBakingAvatar::dirtyMesh() {}
void LLBakingAvatar::dirtyMesh(S32 priority) {}
void LLBakingAvatar::onGlobalColorChanged(const LLTexGlobalColor* global_color) {}

bool LLBakingAvatar::isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const
{
    return TRUE;
}


//-----------------------------------------------------------------------------
// (LLERR) Non-implemented methods.
//-----------------------------------------------------------------------------

LLVector3 LLBakingAvatar::getCharacterPosition()
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return LLVector3::zero;
}

LLQuaternion LLBakingAvatar::getCharacterRotation()
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return LLQuaternion::DEFAULT;
}

LLVector3 LLBakingAvatar::getCharacterVelocity()
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return LLVector3::zero;
}

LLVector3 LLBakingAvatar::getCharacterAngularVelocity()
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return LLVector3::zero;
}

const LLUUID& LLBakingAvatar::getID() const
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return LLUUID::null;
}

void LLBakingAvatar::addDebugText(const std::string& text)
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
}

F32 LLBakingAvatar::getTimeDilation()
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return 0.0f;
}

void LLBakingAvatar::getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm)
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
}

F32 LLBakingAvatar::getPixelArea() const
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return 0.0f;
}

LLVector3d LLBakingAvatar::getPosGlobalFromAgent(const LLVector3 &position)
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return LLVector3d::zero;
}

LLVector3 LLBakingAvatar::getPosAgentFromGlobal(const LLVector3d &position)
{
    LL_ERRS("AppearanceUtility") << "Not implemented." << LL_ENDL;
    return LLVector3::zero;
}



