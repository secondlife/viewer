/**
 * @file llpolyskeletaldistortion.h
 * @brief Implementation of LLPolyMesh class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#pragma once

#include "llcommon.h"

#include <string>
#include <map>
#include "llstl.h"

#include "v3math.h"
#include "v2math.h"
#include "llquaternion.h"
//#include "llpolymorph.h"
#include "lljoint.h"
#include "llviewervisualparam.h"

//class LLSkinJoint;
class LLAvatarAppearance;

//#define USE_STRIPS    // Use tri-strips for rendering.

//-----------------------------------------------------------------------------
// LLPolySkeletalDeformationInfo
// Shared information for LLPolySkeletalDeformations
//-----------------------------------------------------------------------------
struct LLPolySkeletalBoneInfo
{
    LLPolySkeletalBoneInfo(std::string &name, LLVector3 &scale, LLVector3 &pos, bool haspos)
        : mBoneName(name),
          mScaleDeformation(scale),
          mPositionDeformation(pos),
          mHasPositionDeformation(haspos) {}
    std::string mBoneName;
    LLVector3 mScaleDeformation;
    LLVector3 mPositionDeformation;
    bool mHasPositionDeformation;
};

class alignas(16) LLPolySkeletalDistortionInfo : public LLViewerVisualParamInfo
{
    LL_ALIGN_NEW
    friend class LLPolySkeletalDistortion;
public:

    LLPolySkeletalDistortionInfo();
    ~LLPolySkeletalDistortionInfo() override = default;

    bool parseXml(LLXmlTreeNode* node) override;

protected:
    using bone_info_list_t = std::vector<LLPolySkeletalBoneInfo>;
    bone_info_list_t mBoneInfoList;
};

//-----------------------------------------------------------------------------
// LLPolySkeletalDeformation
// A set of joint scale data for deforming the avatar mesh
//-----------------------------------------------------------------------------
class alignas(16) LLPolySkeletalDistortion : public LLViewerVisualParam
{
    LL_ALIGN_NEW
public:
    explicit LLPolySkeletalDistortion(LLAvatarAppearance *avatarp);
    ~LLPolySkeletalDistortion();

    // Special: These functions are overridden by child classes
    LLPolySkeletalDistortionInfo*   getInfo() const { return (LLPolySkeletalDistortionInfo*)mInfo; }
    //   This sets mInfo and calls initialization functions
    bool                            setInfo(LLPolySkeletalDistortionInfo *info);

    LLViewerVisualParam* cloneParam(LLWearable* wearable) const override;

    // LLVisualParam Virtual functions
    ///*virtual*/ bool              parseData(LLXmlTreeNode* node);
    void                apply( ESex sex ) override;

    // LLViewerVisualParam Virtual functions
    F32                 getTotalDistortion() override { return 0.1f; }
    const LLVector4a&   getAvgDistortion() override  { return mDefaultVec; }
    F32                 getMaxDistortion() override { return 0.1f; }
    LLVector4a          getVertexDistortion(S32 index, LLPolyMesh *poly_mesh) override {return LLVector4a(0.001f, 0.001f, 0.001f);}
    const LLVector4a*   getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh) override {index = 0; poly_mesh = NULL; return &mDefaultVec;};
    const LLVector4a*   getNextDistortion(U32 *index, LLPolyMesh **poly_mesh) override {index = 0; poly_mesh = NULL; return NULL;};

protected:
    LLPolySkeletalDistortion(const LLPolySkeletalDistortion& pOther);

    LL_ALIGN_16(LLVector4a mDefaultVec);
    using joint_vec_map_t = std::map<LLJoint*, LLVector3>;
    joint_vec_map_t mJointScales;
    joint_vec_map_t mJointOffsets;
    // Backlink only; don't make this an LLPointer.
    LLAvatarAppearance *mAvatar;
} LL_ALIGN_POSTFIX(16);


