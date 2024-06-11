/**
 * @file llgltfmaterialpreviewmgr.h
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#include "lldrawpool.h"
#include "lldynamictexture.h"
#include "llfetchedgltfmaterial.h"
#include "llsingleton.h"
#include "lltexture.h"

class LLGLTFPreviewTexture : public LLViewerDynamicTexture
{
protected:
    LLGLTFPreviewTexture(LLPointer<LLFetchedGLTFMaterial> material, S32 width);

public:
    // Width scales with size of material's textures
    static LLPointer<LLGLTFPreviewTexture> create(LLPointer<LLFetchedGLTFMaterial> material);

    bool needsRender() override;
    void preRender(bool clear_depth = true) override;
    bool render() override;
    void postRender(bool success) override;

    struct MaterialLoadLevels
    {
        S32 levels[LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT];

        MaterialLoadLevels();
        bool isFullyLoaded();
        S32& operator[](size_t i);
        const S32& operator[](size_t i) const;
        // Less is better
        // Returns false if lhs is not strictly less or equal for all levels
        bool operator<(const MaterialLoadLevels& other) const;
        // Less is better
        // Returns false if lhs is not strictly greater or equal for all levels
        bool operator>(const MaterialLoadLevels& other) const;
    };

private:
    LLPointer<LLFetchedGLTFMaterial> mGLTFMaterial;
    bool mShouldRender = true;
    MaterialLoadLevels mBestLoad;
};

class LLGLTFMaterialPreviewMgr
{
  public:
    // Returns null if the material is not loaded yet.
    // *NOTE: User should cache the texture if the same material is being previewed
    LLPointer<LLViewerTexture> getPreview(LLPointer<LLFetchedGLTFMaterial> &material);
};

extern LLGLTFMaterialPreviewMgr gGLTFMaterialPreviewMgr;
