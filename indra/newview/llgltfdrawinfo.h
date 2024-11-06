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

// Draw call information that fits on a cache line (64 bytes)
// Called LLGLTFDrawInfo, but also used for Blinn-phong but with different data meaning
// Unions used to clarify what means what in which context
class LLGLTFDrawInfo
{
public:
    // put mMaterialID first for cache coherency during sorts
    size_t mMaterialID;

    // Use direct values of VBO/IBO and texture names to avoid dereferencing pointers
    // NOTE: if these GL resources are freed while still in use, something has gone wrong in LLVertexBuffer/LLImageGL
    // The bug is there, not here.
    U32 mVBO;
    U32 mIBO;
    U32 mVBOVertexCount;
    union
    {
        U16 mBaseColorMap;
        U16 mDiffuseMap;
    };
    U16 mNormalMap;
    union
    {
        U16 mMetallicRoughnessMap;
        U16 mSpecularMap;
    };
    U16 mEmissiveMap;
    U32 mElementCount;
    U32 mElementOffset;
    U32 mTransformUBO;
    U32 mInstanceMapUBO;
    U32 mMaterialUBO;
    U32 mTextureTransformUBO;
    U16 mInstanceCount;
    U16 mBaseInstance;
    U8 mIndicesSize;  // 0 - 2 bytes, 1 - 4 bytes

    void texNameCheck(U32 texName);
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

    enum TexMask
    {
        BASE_COLOR_MAP          = 1,
        DIFFUSE_MAP             = 1,
        NORMAL_MAP              = 2,
        SPECULAR_MAP            = 4,
        METALLIC_ROUGHNESS_MAP  = 4,
        EMISSIVE_MAP            = 8
    };

    static constexpr U8 MAX_TEX_MASK = 16;
    static constexpr U8 MAX_PBR_TEX_MASK = 16;
    static constexpr U8 MAX_BP_TEX_MASK = 8;

    typedef std::vector<LLGLTFDrawInfo> gltf_draw_info_list_t;
    typedef std::vector<LLSkinnedGLTFDrawInfo> skinned_gltf_draw_info_list_t;
    typedef gltf_draw_info_list_t gltf_draw_info_map_t[3][MAX_TEX_MASK][2][2][2];
    typedef skinned_gltf_draw_info_list_t skinned_gltf_draw_info_map_t[3][MAX_TEX_MASK][2][2][2];

    // collections of GLTFDrawInfo
    // indexed by [LLGLTFMaterial::mAlphaMode][texture mask][Double Sided][Planar Projection][Texture Animation]
    gltf_draw_info_map_t mDrawInfo;
    skinned_gltf_draw_info_map_t mSkinnedDrawInfo;

    struct BatchList
    {
        LLGLTFMaterial::AlphaMode alpha_mode;
        U8 tex_mask;
        bool double_sided;
        bool planar;
        bool tex_anim;
        gltf_draw_info_list_t* draw_info;
    };

    struct SkinnedBatchList
    {
        LLGLTFMaterial::AlphaMode alpha_mode;
        U8 tex_mask;
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
    LLGLTFDrawInfo* create(LLGLTFMaterial::AlphaMode alpha_mode, U8 tex_mask, bool double_sided, bool planar, bool tex_anim, LLGLTFDrawInfoHandle& handle);

    // add a sikinned draw info to the appropriate list
    LLSkinnedGLTFDrawInfo* createSkinned(LLGLTFMaterial::AlphaMode alpha_mode, U8 tex_mask, bool double_sided, bool planar, bool tex_anim, LLGLTFDrawInfoHandle& handle);

    // add the given LLGLTFBatches to these LLGLTFBatches
    void add(const LLGLTFBatches& other);

    // add the alpha blend and alpha mask draw infos to the given list
    void addShadow(const LLGLTFBatches& other);

    template <typename T>
    void sort(LLGLTFMaterial::AlphaMode alpha_mode, T comparator)
    {
        for (U32 tex_mask = 0; tex_mask < MAX_TEX_MASK; ++tex_mask)
        {
            for (U32 double_sided = 0; double_sided < 2; ++double_sided)
            {
                for (U32 planar = 0; planar < 2; ++planar)
                {
                    for (U32 tex_anim = 0; tex_anim < 2; ++tex_anim)
                    {
                        std::sort(mDrawInfo[alpha_mode][tex_mask][double_sided][planar][tex_anim].begin(), mDrawInfo[alpha_mode][tex_mask][double_sided][planar][tex_anim].end(), comparator);
                    }
                }
            }
        }
    }

    template <typename T>
    void sortSkinned(LLGLTFMaterial::AlphaMode alpha_mode, T comparator)
    {
        for (U32 tex_mask = 0; tex_mask < MAX_TEX_MASK; ++tex_mask)
        {
            for (U32 double_sided = 0; double_sided < 2; ++double_sided)
            {
                for (U32 planar = 0; planar < 2; ++planar)
                {
                    for (U32 tex_anim = 0; tex_anim < 2; ++tex_anim)
                    {
                        std::sort(mSkinnedDrawInfo[alpha_mode][tex_mask][double_sided][planar][tex_anim].begin(), mSkinnedDrawInfo[alpha_mode][tex_mask][double_sided][planar][tex_anim].end(), comparator);
                    }
                }
            }
        }
    }

    void texNameCheck(U32 texName);
};

// handle to a GLTFDrawInfo
// Can be invalidated if mContainer is destroyed or resized
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

    // get the LLGLTFDrawInfo this handle points to
    // Makes an attempt to assert pointer is valid, but does not guarantee safety
    // MUST NOT be called unless you are certain the handle is valid
    LLGLTFDrawInfo* get();

    LLGLTFDrawInfo* operator->()
    {
        return get();
    }

    // return true if this handle was set to a valid draw info at some point
    // DOES NOT indicate pointer returned by get() is valid
    // MAY be called on an invalid handle
    operator bool() const
    {
        return mIndex >= 0;
    }

    // Clear the handle
    // Sets mIndex to -1, but maintains other state for debugging
    void clear();
};
