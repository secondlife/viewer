#pragma once

/**
 * @file gltfscenemanager.h
 * @brief Builds menus out of items.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
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

#include "llsingleton.h"
#include "llviewerobject.h"
#include "gltf/common.h"

class LLVOVolume;
class LLDrawable;

namespace LL
{
    class GLTFSceneManager : public LLSimpleton<GLTFSceneManager>
    {
    public:
        ~GLTFSceneManager();
        // load GLTF file from disk

        void load(); // open filepicker to choose asset
        void load(const std::string& filename); // load asset from filename

        void saveAs(); // open filepicker and choose file to save selected asset to
        void save(const std::string& filename); // save selected asset to filename (suitable for use in external programs)
        void uploadSelection(); // decompose selected asset and upload to simulator

        void update();
        void render(bool opaque, bool rigged = false, bool unlit = false);

        // render the given variant of all assets
        // variant - bitmask according to LLGLSLShader::GLTFVariant flags
        void render(U8 variant);

        void render(LL::GLTF::Asset& asset, U8 variant);

        // bind the given material for rendering
        void bind(LL::GLTF::Asset& asset, LL::GLTF::Material& material);
        void bindTexture(LL::GLTF::Asset& asset, LL::GLTF::TextureType texture_type, LL::GLTF::TextureInfo& info, LLViewerTexture* fallback);
        void renderOpaque();
        void renderAlpha();

        LLDrawable* lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
            bool pick_transparent,
            bool pick_rigged,
            bool pick_unselectable,
            bool pick_reflection_probe,
            S32* node_hit,                   // return the index of the node that was hit
            S32* primitive_hit,              // return the index of the primitive that was hit
            LLVector4a* intersection,         // return the intersection point
            LLVector2* tex_coord,            // return the texture coordinates of the intersection point
            LLVector4a* normal,               // return the surface normal at the intersection point
            LLVector4a* tangent);           // return the surface tangent at the intersection point

        bool lineSegmentIntersect(LLVOVolume* obj, LL::GLTF::Asset* asset, const LLVector4a& start, const LLVector4a& end, S32 face, bool pick_transparent, bool pick_rigged, bool pick_unselectable, S32* face_hitp, S32* primitive_hitp,
            LLVector4a* intersection, LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent);

        void renderDebug();

        void addGLTFObject(LLViewerObject* object, LLUUID gltf_id);
        static void onGLTFLoadComplete(const LLUUID& id, LLAssetType::EType asset_type, void* user_data, S32 status, LLExtStat ext_status);
        static void onGLTFBinLoadComplete(const LLUUID& id, LLAssetType::EType asset_type, void* user_data, S32 status, LLExtStat ext_status);

        std::vector<LLPointer<LLViewerObject>> mObjects;

        std::shared_ptr<GLTF::Asset> mUploadingAsset;
        bool mGLTFUploadPending = false;
        LLPointer<LLViewerObject> mUploadingObject;
        U32 mPendingImageUploads = 0;
        U32 mPendingBinaryUploads = 0;
        U32 mPendingGLTFUploads = 0;

        U32 mJointUBO = 0;


        // render loop state
        S32 mLastTexture[GLTF::TEXTURE_TYPE_COUNT] = { -2, -2, -2, -2, -2 };

    };
}


