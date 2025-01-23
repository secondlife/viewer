/**
 * @file llgltfmaterialpreviewmgr.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llgltfmaterialpreviewmgr.h"

#include <memory>
#include <vector>

#include "llavatarappearancedefines.h"
#include "llenvironment.h"
#include "llselectmgr.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvolumemgr.h"
#include "pipeline.h"

LLGLTFMaterialPreviewMgr gGLTFMaterialPreviewMgr;

namespace
{
    constexpr S32 FULLY_LOADED = 0;
    constexpr S32 NOT_LOADED = 99;
};

LLGLTFPreviewTexture::MaterialLoadLevels::MaterialLoadLevels()
{
    for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        levels[i] = NOT_LOADED;
    }
}

bool LLGLTFPreviewTexture::MaterialLoadLevels::isFullyLoaded()
{
    for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (levels[i] != FULLY_LOADED) { return false; }
    }
    return true;
}

S32& LLGLTFPreviewTexture::MaterialLoadLevels::operator[](size_t i)
{
    llassert(i >= 0 && i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT);
    return levels[i];
}

const S32& LLGLTFPreviewTexture::MaterialLoadLevels::operator[](size_t i) const
{
    llassert(i >= 0 && i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT);
    return levels[i];
}

bool LLGLTFPreviewTexture::MaterialLoadLevels::operator<(const MaterialLoadLevels& other) const
{
    bool less = false;
    for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (((*this)[i] > other[i])) { return false; }
        less = less || ((*this)[i] < other[i]);
    }
    return less;
}

bool LLGLTFPreviewTexture::MaterialLoadLevels::operator>(const MaterialLoadLevels& other) const
{
    bool great = false;
    for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (((*this)[i] < other[i])) { return false; }
        great = great || ((*this)[i] > other[i]);
    }
    return great;
}

namespace
{
    void fetch_texture_for_ui(LLPointer<LLViewerFetchedTexture>& img, const LLUUID& id)
    {
        if (!img && id.notNull())
        {
            if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(id))
            {
                LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
                if (obj)
                {
                    LLViewerTexture* viewerTexture = obj->getBakedTextureForMagicId(id);
                    img = viewerTexture ? dynamic_cast<LLViewerFetchedTexture*>(viewerTexture) : NULL;
                }
            }
            else
            {
                img = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
            }
        }
        if (img)
        {
            img->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
            img->forceToSaveRawImage(0);
        }
    };

    // *NOTE: Does not use the same conventions as texture discard level. Lower is better.
    S32 get_texture_load_level(const LLPointer<LLViewerFetchedTexture>& texture)
    {
        if (!texture) { return FULLY_LOADED; }
        const S32 raw_level = texture->getDiscardLevel();
        if (raw_level < 0) { return NOT_LOADED; }
        return raw_level;
    }

    LLGLTFPreviewTexture::MaterialLoadLevels get_material_load_levels(LLFetchedGLTFMaterial& material)
    {
        llassert(!material.isFetching());

        using MaterialTextures = LLPointer<LLViewerFetchedTexture>*[LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT];

        MaterialTextures textures;

        textures[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] = &material.mBaseColorTexture;
        textures[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] = &material.mNormalTexture;
        textures[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] = &material.mMetallicRoughnessTexture;
        textures[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] = &material.mEmissiveTexture;

        LLGLTFPreviewTexture::MaterialLoadLevels levels;

        for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
        {
            fetch_texture_for_ui(*textures[i], material.mTextureId[i]);
            levels[i] = get_texture_load_level(*textures[i]);
        }

        return levels;
    }

    // Is the material loaded enough to start rendering a preview?
    bool is_material_loaded_enough_for_ui(LLFetchedGLTFMaterial& material)
    {
        if (material.isFetching())
        {
            return false;
        }

        LLGLTFPreviewTexture::MaterialLoadLevels levels = get_material_load_levels(material);

        for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
        {
            if (levels[i] == NOT_LOADED)
            {
                return false;
            }
        }

        return true;
    }

};  // namespace

LLGLTFPreviewTexture::LLGLTFPreviewTexture(LLPointer<LLFetchedGLTFMaterial> material, S32 width)
    : LLViewerDynamicTexture(width, width, 4, EOrder::ORDER_MIDDLE, false)
    , mGLTFMaterial(material)
{
}

// static
LLPointer<LLGLTFPreviewTexture> LLGLTFPreviewTexture::create(LLPointer<LLFetchedGLTFMaterial> material)
{
    return new LLGLTFPreviewTexture(material, LLPipeline::MAX_PREVIEW_WIDTH);
}

bool LLGLTFPreviewTexture::needsRender()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

    if (!mShouldRender && mBestLoad.isFullyLoaded()) { return false; }
    MaterialLoadLevels current_load = get_material_load_levels(*mGLTFMaterial.get());
    if (current_load < mBestLoad)
    {
        mShouldRender = true;
        mBestLoad = current_load;
        return true;
    }
    return false;
}

void LLGLTFPreviewTexture::preRender(bool clear_depth)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

    llassert(mShouldRender);
    if (!mShouldRender) { return; }

    LLViewerDynamicTexture::preRender(clear_depth);
}


namespace {

struct GLTFPreviewModel
{
    GLTFPreviewModel(LLPointer<LLDrawInfo>& info, const LLMatrix4& mat)
    : mDrawInfo(info)
    , mModelMatrix(mat)
    {
        mDrawInfo->mModelMatrix = &mModelMatrix;
    }
    GLTFPreviewModel(GLTFPreviewModel&) = delete;
    ~GLTFPreviewModel()
    {
        // No model matrix necromancy
        llassert(gGLLastMatrix != &mModelMatrix);
        gGLLastMatrix = nullptr;
    }
    LLPointer<LLDrawInfo> mDrawInfo;
    LLMatrix4 mModelMatrix; // Referenced by mDrawInfo
};

using PreviewSpherePart = std::unique_ptr<GLTFPreviewModel>;
using PreviewSphere = std::vector<PreviewSpherePart>;

// Like LLVolumeGeometryManager::registerFace but without batching or too-many-indices/vertices checking.
PreviewSphere create_preview_sphere(LLPointer<LLFetchedGLTFMaterial>& material, const LLMatrix4& model_matrix)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

    const LLColor4U vertex_color(material->mBaseColor);

    LLPrimitive prim;
    prim.init_primitive(LL_PCODE_VOLUME);
    LLVolumeParams params;
    params.setType(LL_PCODE_PROFILE_CIRCLE_HALF, LL_PCODE_PATH_CIRCLE);
    params.setBeginAndEndS(0.f, 1.f);
    params.setBeginAndEndT(0.f, 1.f);
    params.setRatio(1, 1);
    params.setShear(0, 0);
    constexpr auto MAX_LOD = LLVolumeLODGroup::NUM_LODS - 1;
    prim.setVolume(params, MAX_LOD);

    LLVolume* volume = prim.getVolume();
    llassert(volume);
    for (LLVolumeFace& face : volume->getVolumeFaces())
    {
        face.createTangents();
    }

    PreviewSphere preview_sphere;
    preview_sphere.reserve(volume->getNumFaces());

    LLPointer<LLVertexBuffer> buf = new LLVertexBuffer(
        LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_NORMAL |
        LLVertexBuffer::MAP_TEXCOORD0 |
        LLVertexBuffer::MAP_COLOR |
        LLVertexBuffer::MAP_TANGENT
    );
    U32 nv = 0;
    U32 ni = 0;
    for (LLVolumeFace& face : volume->getVolumeFaces())
    {
        nv += face.mNumVertices;
        ni += face.mNumIndices;
    }
    buf->allocateBuffer(nv, ni);

    // UV hacks
    // Higher factor helps to see more details on the preview sphere
    const LLVector2 uv_factor(2.0f, 2.0f);
    // Offset places center of material in center of view
    const LLVector2 uv_offset(-0.5f, -0.5f);

    LLStrider<U16> indices;
    LLStrider<LLVector4a> positions;
    LLStrider<LLVector4a> normals;
    LLStrider<LLVector2> texcoords;
    LLStrider<LLColor4U> colors;
    LLStrider<LLVector4a> tangents;
    buf->getIndexStrider(indices);
    buf->getVertexStrider(positions);
    buf->getNormalStrider(normals);
    buf->getTexCoord0Strider(texcoords);
    buf->getColorStrider(colors);
    buf->getTangentStrider(tangents);
    U32 index_offset = 0;
    U32 vertex_offset = 0;
    for (const LLVolumeFace& face : volume->getVolumeFaces())
    {
        for (S32 i = 0; i < face.mNumIndices; ++i)
        {
            *indices++ = face.mIndices[i] + vertex_offset;
        }
        for (S32 v = 0; v < face.mNumVertices; ++v)
        {
            *positions++ = face.mPositions[v];
            *normals++ = face.mNormals[v];
            LLVector2 uv(face.mTexCoords[v]);
            uv.scaleVec(uv_factor);
            uv += uv_offset;
            *texcoords++ = uv;
            *colors++ = vertex_color;
            *tangents++ = face.mTangents[v];
        }

        constexpr LLViewerTexture* no_media = nullptr;
        LLPointer<LLDrawInfo> info = new LLDrawInfo(U16(vertex_offset), U16(vertex_offset + face.mNumVertices - 1), face.mNumIndices, index_offset, no_media, buf.get());
        info->mGLTFMaterial = material;
        preview_sphere.emplace_back(std::make_unique<GLTFPreviewModel>(info, model_matrix));
        index_offset += face.mNumIndices;
        vertex_offset += face.mNumVertices;
    }

    buf->unmapBuffer();

    return preview_sphere;
}

void set_preview_sphere_material(PreviewSphere& preview_sphere, LLPointer<LLFetchedGLTFMaterial>& material)
{
    llassert(!preview_sphere.empty());
    if (preview_sphere.empty()) { return; }

    const LLColor4U vertex_color(material->mBaseColor);

    // See comments about unmapBuffer in llvertexbuffer.h
    for (PreviewSpherePart& part : preview_sphere)
    {
        LLDrawInfo* info = part->mDrawInfo.get();
        info->mGLTFMaterial = material;
        LLVertexBuffer* buf = info->mVertexBuffer.get();
        LLStrider<LLColor4U> colors;
        const S32 count = info->mEnd - info->mStart + 1;
        buf->getColorStrider(colors, info->mStart, count);
        for (S32 i = 0; i < count; ++i)
        {
            *colors++ = vertex_color;
        }
        buf->unmapBuffer();
    }
}

PreviewSphere& get_preview_sphere(LLPointer<LLFetchedGLTFMaterial>& material, const LLMatrix4& model_matrix)
{
    static PreviewSphere preview_sphere;
    if (preview_sphere.empty())
    {
        preview_sphere = create_preview_sphere(material, model_matrix);
    }
    else
    {
        set_preview_sphere_material(preview_sphere, material);
    }
    return preview_sphere;
}

// Final, direct modifications to shader constants, just before render
void fixup_shader_constants(LLGLSLShader& shader)
{
    // Sunlight intensity of 0 no matter what
    shader.uniform1i(LLShaderMgr::SUN_UP_FACTOR, 1);
    shader.uniform3fv(LLShaderMgr::SUNLIGHT_COLOR, 1, LLColor3::white.mV);
    shader.uniform1f(LLShaderMgr::DENSITY_MULTIPLIER, 0.0f);

    // Ignore sun shadow (if enabled)
    for (U32 i = 0; i < 6; i++)
    {
        const S32 channel = shader.getTextureChannel(LLShaderMgr::DEFERRED_SHADOW0+i);
        if (channel != -1)
        {
            gGL.getTexUnit(channel)->bind(LLViewerFetchedTexture::sWhiteImagep, true);
        }
    }
}

// Set a variable to a value temporarily, and restor the variable's old value
// when this object leaves scope.
template<typename T>
struct SetTemporarily
{
    T* mRef;
    T mOldVal;
    SetTemporarily(T* var, T temp_val)
    {
        mRef = var;
        mOldVal = *mRef;
        *mRef = temp_val;
    }
    ~SetTemporarily()
    {
        *mRef = mOldVal;
    }
};

}; // namespace

bool LLGLTFPreviewTexture::render()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

    if (!mShouldRender) { return false; }

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    LLGLDepthTest(GL_FALSE);
    LLGLDisable stencil(GL_STENCIL_TEST);
    LLGLDisable scissor(GL_SCISSOR_TEST);
    SetTemporarily<bool> no_dof(&LLPipeline::RenderDepthOfField, false);
    SetTemporarily<bool> no_glow(&LLPipeline::sRenderGlow, false);
    SetTemporarily<bool> no_ssr(&LLPipeline::RenderScreenSpaceReflections, false);
    SetTemporarily<U32> no_aa(&LLPipeline::RenderFSAAType, U32(0));
    SetTemporarily<LLPipeline::RenderTargetPack*> use_auxiliary_render_target(&gPipeline.mRT, &gPipeline.mAuxillaryRT);

    LLVector3 light_dir3(1.0f, 1.0f, 1.0f);
    light_dir3.normalize();
    const LLVector4 light_dir = LLVector4(light_dir3, 0);
    const S32 old_local_light_count = gSavedSettings.get<S32>("RenderLocalLightCount");
    gSavedSettings.set<S32>("RenderLocalLightCount", 0);

    gPipeline.mReflectionMapManager.forceDefaultProbeAndUpdateUniforms();

    LLViewerCamera camera;

    // Calculate the object distance at which the object of a given radius will
    // span the partial width of the screen given by fill_ratio.
    // Assume the primitive has a scale of 1 (this is the default).
    constexpr F32 fill_ratio = 0.8f;
    constexpr F32 object_radius = 0.5f;
    const F32 object_distance = (object_radius / fill_ratio) * tan(camera.getDefaultFOV());
    // Negative coordinate shows the textures on the sphere right-side up, when
    // combined with the UV hacks in create_preview_sphere
    const LLVector3 object_position(0.0, -object_distance, 0.0);
    LLMatrix4 object_transform;
    object_transform.translate(object_position);

    // Set up camera and viewport
    const LLVector3 origin(0.0, 0.0, 0.0);
    camera.lookAt(origin, object_position);
    camera.setAspect((F32)(mFullHeight / mFullWidth));
    const LLRect texture_rect(0, mFullHeight, mFullWidth, 0);
    camera.setPerspective(NOT_FOR_SELECTION, texture_rect.mLeft, texture_rect.mBottom, texture_rect.getWidth(), texture_rect.getHeight(), false, camera.getNear(), MAX_FAR_CLIP*2.f);

    // Generate sphere object on-the-fly. Discard afterwards. (Vertex buffer is
    // discarded, but the sphere should be cached in LLVolumeMgr.)
    PreviewSphere& preview_sphere = get_preview_sphere(mGLTFMaterial, object_transform);

    gPipeline.setupHWLights();
    glm::mat4 mat = get_current_modelview();
    glm::vec4 transformed_light_dir(light_dir);
    transformed_light_dir = mat * transformed_light_dir;
    SetTemporarily<LLVector4> force_sun_direction_high_graphics(&gPipeline.mTransformedSunDir, LLVector4(transformed_light_dir));
    // Override lights to ensure the sun is always shining from a certain direction (low graphics)
    // See also force_sun_direction_high_graphics and fixup_shader_constants
    {
        LLLightState* light = gGL.getLight(0);
        light->setPosition(light_dir);
        constexpr bool sun_up = true;
        light->setSunPrimary(sun_up);
    }

    LLRenderTarget& screen = gPipeline.mAuxillaryRT.screen;

    // *HACK: Force reset of the model matrix
    gGLLastMatrix = nullptr;

#if 0
    if (mGLTFMaterial->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_OPAQUE || mGLTFMaterial->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
    {
        // *TODO: Opaque/alpha mask rendering
    }
    else
#endif
    {
        // Alpha blend rendering

        screen.bindTarget();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        LLGLSLShader& shader = gDeferredPBRAlphaProgram;

        gPipeline.bindDeferredShader(shader);
        fixup_shader_constants(shader);

        for (PreviewSpherePart& part : preview_sphere)
        {
            LLRenderPass::pushGLTFBatch(*part->mDrawInfo);
        }

        gPipeline.unbindDeferredShader(shader);

        screen.flush();
    }

    // *HACK: Hide mExposureMap from generateExposure
    gPipeline.mExposureMap.swapFBORefs(gPipeline.mLastExposure);

    gPipeline.copyScreenSpaceReflections(&screen, &gPipeline.mSceneMap);
    gPipeline.generateLuminance(&screen, &gPipeline.mLuminanceMap);
    gPipeline.generateExposure(&gPipeline.mLuminanceMap, &gPipeline.mExposureMap, /*use_history = */ false);
    gPipeline.gammaCorrect(&screen, &gPipeline.mPostMap);
    LLVertexBuffer::unbind();
    gPipeline.generateGlow(&gPipeline.mPostMap);
    gPipeline.combineGlow(&gPipeline.mPostMap, &screen);
    gPipeline.renderDoF(&screen, &gPipeline.mPostMap);
    gPipeline.applyFXAA(&gPipeline.mPostMap, &screen);

    // *HACK: Restore mExposureMap (it will be consumed by generateExposure next frame)
    gPipeline.mExposureMap.swapFBORefs(gPipeline.mLastExposure);

    // Final render

    gDeferredPostNoDoFProgram.bind();

    // From LLPipeline::renderFinalize: "Whatever is last in the above post processing chain should _always_ be rendered directly here.  If not, expect problems."
    gDeferredPostNoDoFProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, &screen);
    gDeferredPostNoDoFProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH, mBoundTarget, true);

    {
        LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_ALWAYS);
        gPipeline.mScreenTriangleVB->setBuffer();
        gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    }

    gDeferredPostNoDoFProgram.unbind();

    // Clean up
    gPipeline.setupHWLights();
    gPipeline.mReflectionMapManager.forceDefaultProbeAndUpdateUniforms(false);
    gSavedSettings.set<S32>("RenderLocalLightCount", old_local_light_count);

    return true;
}

void LLGLTFPreviewTexture::postRender(bool success)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

    if (!mShouldRender) { return; }
    mShouldRender = false;

    LLViewerDynamicTexture::postRender(success);
}

LLPointer<LLViewerTexture> LLGLTFMaterialPreviewMgr::getPreview(LLPointer<LLFetchedGLTFMaterial> &material)
{
    if (!material)
    {
        return nullptr;
    }

    static LLCachedControl<bool> sUIPreviewMaterial(gSavedSettings, "UIPreviewMaterial", false);
    if (!sUIPreviewMaterial)
    {
        fetch_texture_for_ui(material->mBaseColorTexture, material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR]);
        return material->mBaseColorTexture;
    }

    if (!is_material_loaded_enough_for_ui(*material))
    {
        return nullptr;
    }

    return LLGLTFPreviewTexture::create(material);
}
