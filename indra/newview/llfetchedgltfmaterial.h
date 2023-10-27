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

class LLFetchedGLTFMaterial: public LLGLTFMaterial
{
    friend class LLGLTFMaterialList; // for lifetime management
public:
    LLFetchedGLTFMaterial();
    virtual ~LLFetchedGLTFMaterial();

    LLFetchedGLTFMaterial& operator=(const LLFetchedGLTFMaterial& rhs);

    // If this material is loaded, fire the given function
    void onMaterialComplete(std::function<void()> material_complete);

    // bind this material for rendering
    //   media_tex - optional media texture that may override the base color texture
    void bind(LLViewerTexture* media_tex = nullptr);

    bool isFetching() const { return mFetching; }

    virtual bool replaceLocalTexture(const LLUUID& old_id, const LLUUID& new_id) override;
    virtual void updateTextureTracking() override;

    // Textures used for fetching/rendering
    LLPointer<LLViewerFetchedTexture> mBaseColorTexture;
    LLPointer<LLViewerFetchedTexture> mNormalTexture;
    LLPointer<LLViewerFetchedTexture> mMetallicRoughnessTexture;
    LLPointer<LLViewerFetchedTexture> mEmissiveTexture;

protected:
    // Lifetime management
    
    void materialBegin();
    void materialComplete();

    F64 mExpectedFlusTime; // since epoch in seconds
    bool mActive;
    bool mFetching;
    std::vector<std::function<void()>> materialCompleteCallbacks;
};

