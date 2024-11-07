/**
 * @file   llfetchedgltfmaterial.h
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "llgltfmaterial.h"
#include "llpointer.h"
#include "llviewertexture.h"

class LLGLSLShader;
class LLGLTFMaterialList;
class LLTerrainMaterials;

class LLFetchedGLTFMaterial: public LLGLTFMaterial
{
    // for lifetime management
    friend class LLGLTFMaterialList;
    friend class LLTerrainMaterials;
public:
    LLFetchedGLTFMaterial();
    virtual ~LLFetchedGLTFMaterial();

    LLFetchedGLTFMaterial& operator=(const LLFetchedGLTFMaterial& rhs);
    // LLGLTFMaterial::operator== is defined, but LLFetchedGLTFMaterial::operator== is not.
    bool operator==(const LLGLTFMaterial& rhs) const = delete;

    // If this material is loaded, fire the given function
    void onMaterialComplete(std::function<void()> material_complete);

    // bind this material for rendering
    //   media_tex - optional media texture that may override the base color texture
    void bind(LLViewerTexture* media_tex = nullptr);

    bool isFetching() const { return mFetching; }
    bool isLoaded() const { return !mFetching && mFetchSuccess; }

    void addTextureEntry(LLTextureEntry* te) override;
    void removeTextureEntry(LLTextureEntry* te) override;
    virtual bool replaceLocalTexture(const LLUUID& tracking_id, const LLUUID& old_id, const LLUUID& new_id) override;
    virtual void updateTextureTracking() override;

    // Textures used for fetching/rendering
    LLPointer<LLViewerFetchedTexture> mBaseColorTexture;
    LLPointer<LLViewerFetchedTexture> mNormalTexture;
    LLPointer<LLViewerFetchedTexture> mMetallicRoughnessTexture;
    LLPointer<LLViewerFetchedTexture> mEmissiveTexture;

    std::set<LLTextureEntry*> mTextureEntires;

    // default material for when assets don't have one
    static LLFetchedGLTFMaterial sDefault;
protected:
    // Lifetime management

    void materialBegin();
    void materialComplete(bool success);

    F64 mExpectedFlusTime; // since epoch in seconds
    bool mActive = true;
    bool mFetching = false;
    bool mFetchSuccess = false;
    std::vector<std::function<void()>> materialCompleteCallbacks;
};

