/**
 * @file llgltfdrawinfo.h
 * @brief LLDrawInfo equivalent for GLTF material render pipe
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#pragma once

#include "llvoavatar.h"

class LLSpatialGroup;

class LLGLTFDrawInfo
{
public:
    // put mMaterialID first for cache coherency during sorts
    size_t mMaterialID;

    // Use direct values of VAO and texture names to avoid dereferencing pointers
    // NOTE: if these GL resources are freed while still in use, something has gone wrong in LLVertexBuffer/LLImageGL
    // The bug is there, not here.
    U32 mVAO;
    U32 mBaseColorMap;
    U32 mNormalMap;
    U32 mMetallicRoughnessMap;
    U32 mEmissiveMap;
    U32 mElementCount;
    U32 mElementOffset;
    U32 mInstanceCount;
    U32 mBaseInstance;
    U32 mTransformUBO;
    U32 mInstanceMapUBO;
    U32 mMaterialUBO;
    U32 mPrimScaleUBO;
    U32 mTextureTransformUBO;

    void handleTexNameChanged(const LLImageGL* image, U32 old_texname);
};

class LLSkinnedGLTFDrawInfo : public LLGLTFDrawInfo
{
public:
    LLPointer<LLVOAvatar> mAvatar = nullptr;
    LLMeshSkinInfo* mSkinInfo = nullptr;
};

class LLGLTFDrawInfoHandle;

class LLGLTFBatches
{
public:
    typedef std::vector<LLGLTFDrawInfo> gltf_draw_info_list_t;
    typedef std::vector<LLSkinnedGLTFDrawInfo> skinned_gltf_draw_info_list_t;
    typedef gltf_draw_info_list_t gltf_draw_info_map_t[3][2][2][2];
    typedef skinned_gltf_draw_info_list_t skinned_gltf_draw_info_map_t[3][2][2][2];

    // collections of GLTFDrawInfo
    // indexed by [LLGLTFMaterial::mAlphaMode][Double Sided][Planar Projection][Texture Animation]
    gltf_draw_info_map_t mDrawInfo;
    skinned_gltf_draw_info_map_t mSkinnedDrawInfo;

    struct BatchList
    {
        LLGLTFMaterial::AlphaMode alpha_mode;
        bool double_sided;
        bool planar;
        bool tex_anim;
        gltf_draw_info_list_t* draw_info;
    };

    struct SkinnedBatchList
    {
        LLGLTFMaterial::AlphaMode alpha_mode;
        bool double_sided;
        bool planar;
        bool tex_anim;
        skinned_gltf_draw_info_list_t* draw_info;
    };

    // collections that point to non-empty collections in mDrawInfo to accelerate iteration over all draw infos
    std::vector<BatchList> mBatchList;
    std::vector<SkinnedBatchList> mSkinnedBatchList;

    // clear all draw infos
    void clear();

    // add a draw info to the appropriate list
    LLGLTFDrawInfo* create(LLGLTFMaterial::AlphaMode alpha_mode, bool double_sided = false, bool planar = false, bool tex_anim = false, LLGLTFDrawInfoHandle* handle = nullptr);

    // add a sikinned draw info to the appropriate list
    LLSkinnedGLTFDrawInfo* createSkinned(LLGLTFMaterial::AlphaMode alpha_mode, bool double_sided = false, bool planar = false, bool  tex_anim = false, LLGLTFDrawInfoHandle* handle = nullptr);

    // add the given LLGLTFBatches to these LLGLTFBatches
    void add(const LLGLTFBatches& other);

    template <typename T>
    void sort(LLGLTFMaterial::AlphaMode i, T comparator)
    {
        for (U32 j = 0; j < 2; ++j)
        {
            for (U32 k = 0; k < 2; ++k)
            {
                for (U32 l = 0; l < 2; ++l)
                {
                    std::sort(mDrawInfo[i][j][k][l].begin(), mDrawInfo[i][j][k][l].end(), comparator);
                }
            }
        }
    }

    template <typename T>
    void sortSkinned(LLGLTFMaterial::AlphaMode i, T comparator)
    {
        for (U32 j = 0; j < 2; ++j)
        {
            for (U32 k = 0; k < 2; ++k)
            {
                for (U32 l = 0; l < 2; ++l)
                {
                    std::sort(mSkinnedDrawInfo[i][j][k][l].begin(), mSkinnedDrawInfo[i][j][k][l].end(), comparator);
                }
            }
        }
    }
};

// handle to a GLTFDrawInfo
// Can be invalidated if mContainer is destroyed
class LLGLTFDrawInfoHandle
{
public:
    // Vector GLTFDrawInfo is stored in
    union
    {
        LLGLTFBatches::gltf_draw_info_list_t* mContainer;
        LLGLTFBatches::skinned_gltf_draw_info_list_t* mSkinnedContainer = nullptr;
    };

    LLSpatialGroup* mSpatialGroup = nullptr;

    // whether this is a skinned or non-skinned draw info
    bool mSkinned = false;

    // index into that vector
    S32 mIndex = -1;

    LLGLTFDrawInfo* get();

    LLGLTFDrawInfo* operator->()
    {
        return get();
    }

    operator bool() const
    {
        llassert(mIndex < 0 ||
            (mContainer != nullptr &&
                mSkinned ? (mSkinnedContainer->size() > mIndex) :
                (mContainer->size() > mIndex)));
        return mIndex >= 0;
    }

    void clear();
};
