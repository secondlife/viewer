/**
 * @file llgltfdrawinfo.cpp
 * @brief LLGLTFDrawInfo implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llgltfdrawinfo.h"

void LLGLTFBatches::clear()
{
    mBatchList.clear();
    mSkinnedBatchList.clear();

    for (U32 i = 0; i < 3; i++)
    {
        for (U32 j = 0; j < 2; j++)
        {
            for (U32 k = 0; k < 2; ++k)
            {
                for (U32 l = 0; l < 2; ++l)
                {
                    mDrawInfo[i][j][k][l].clear();
                    mSkinnedDrawInfo[i][j][k][l].clear();
                }
            }
        }
    }
}

void LLGLTFDrawInfo::handleTexNameChanged(const LLImageGL* image, U32 old_texname)
{
    if (mBaseColorMap == old_texname)
    {
        mBaseColorMap = image->getTexName();
    }

    if (mMetallicRoughnessMap == old_texname)
    {
        mMetallicRoughnessMap = image->getTexName();
    }

    if (mNormalMap == old_texname)
    {
        mNormalMap = image->getTexName();
    }

    if (mEmissiveMap == old_texname)
    {
        mEmissiveMap = image->getTexName();
    }
}

LLGLTFDrawInfo* LLGLTFBatches::create(LLGLTFMaterial::AlphaMode alpha_mode, bool double_sided, bool planar, bool tex_anim, LLGLTFDrawInfoHandle* handle)
{
    auto& draw_info = mDrawInfo[alpha_mode][double_sided][planar][tex_anim];

    if (draw_info.empty())
    {
        mBatchList.push_back({ alpha_mode, double_sided, planar, tex_anim, &draw_info });
    }

    if (handle)
    {
        handle->mSkinned = false;
        handle->mContainer = &draw_info;
        handle->mIndex = (S32)draw_info.size();
    }

    return &draw_info.emplace_back();
}

LLSkinnedGLTFDrawInfo* LLGLTFBatches::createSkinned(LLGLTFMaterial::AlphaMode alpha_mode, bool double_sided, bool planar, bool tex_anim, LLGLTFDrawInfoHandle* handle)
{
    auto& draw_info = mSkinnedDrawInfo[alpha_mode][double_sided][planar][tex_anim];

    if (draw_info.empty())
    {
        mSkinnedBatchList.push_back({ alpha_mode, double_sided, planar, tex_anim, &draw_info });
    }

    if (handle)
    {
        handle->mSkinned = true;
        handle->mSkinnedContainer = &draw_info;
        handle->mIndex = (S32)draw_info.size();
    }

    return &draw_info.emplace_back();
}

void LLGLTFBatches::add(const LLGLTFBatches& other)
{
    for (auto& batch : other.mBatchList)
    {
        auto& draw_info = mDrawInfo[batch.alpha_mode][batch.double_sided][batch.planar][batch.tex_anim];
        draw_info.insert(draw_info.end(), batch.draw_info->begin(), batch.draw_info->end());
    }

    for (auto& batch : other.mSkinnedBatchList)
    {
        auto& draw_info = mSkinnedDrawInfo[batch.alpha_mode][batch.double_sided][batch.planar][batch.tex_anim];
        draw_info.insert(draw_info.end(), batch.draw_info->begin(), batch.draw_info->end());
    }
}

