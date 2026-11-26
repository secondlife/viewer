/**
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "lldrawpoolalpha.h"

#include "llglheaders.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llcubemap.h"
#include "llsky.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"    // For debugging
#include "llviewerobjectlist.h" // For debugging
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "llspatialpartition.h"
#include "llglcommonfunc.h"
#include "llvoavatar.h"
#include "llvopartgroup.h"
#include "gltfscenemanager.h"

#include "llenvironment.h"

bool LLDrawPoolAlpha::sShowDebugAlpha = false;

#define current_shader (LLGLSLShader::sCurBoundShaderPtr)

LLVector4 LLDrawPoolAlpha::sWaterPlane;

// minimum alpha before discarding a fragment
static const F32 MINIMUM_ALPHA = 0.004f; // ~ 1/255

// minimum alpha before discarding a fragment when rendering impostors
static const F32 MINIMUM_IMPOSTOR_ALPHA = 0.1f;

LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
        LLRenderPass(type), target_shader(NULL),
        mColorSFactor(LLRender::BF_UNDEF), mColorDFactor(LLRender::BF_UNDEF),
        mAlphaSFactor(LLRender::BF_UNDEF), mAlphaDFactor(LLRender::BF_UNDEF)
{

}

LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}


void LLDrawPoolAlpha::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

S32 LLDrawPoolAlpha::getNumPostDeferredPasses()
{
    return 1;
}

// set some common parameters on the given shader to prepare for alpha rendering
static void prepare_alpha_shader(LLGLSLShader* shader, bool deferredEnvironment, F32 water_sign)
{
    static LLCachedControl<F32> displayGamma(gSavedSettings, "RenderDeferredDisplayGamma");
    F32 gamma = displayGamma;

    static LLStaticHashedString waterSign("waterSign");

    // Does this deferred shader need environment uniforms set such as sun_dir, etc. ?
    // NOTE: We don't actually need a gbuffer since we are doing forward rendering (for transparency) post deferred rendering
    // TODO: bindDeferredShader() probably should have the updating of the environment uniforms factored out into updateShaderEnvironmentUniforms()
    // i.e. shaders\class1\deferred\alphaF.glsl
    if (deferredEnvironment)
    {
        shader->mCanBindFast = false;
    }

    shader->bind();
    shader->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f / 2.2f));

    if (LLPipeline::sRenderingHUDs)
    { // for HUD attachments, only the pre-water pass is executed and we never want to clip anything
        LLVector4 near_clip(0, 0, -1, 0);
        shader->uniform1f(waterSign, 1.f);
        shader->uniform4fv(LLShaderMgr::WATER_WATERPLANE, 1, near_clip.mV);
    }
    else
    {
        shader->uniform1f(waterSign, water_sign);
        shader->uniform4fv(LLShaderMgr::WATER_WATERPLANE, 1, LLDrawPoolAlpha::sWaterPlane.mV);
    }

    if (LLPipeline::sImpostorRender)
    {
        shader->setMinimumAlpha(MINIMUM_IMPOSTOR_ALPHA);
    }
    else
    {
        shader->setMinimumAlpha(MINIMUM_ALPHA);
    }

    //also prepare rigged variant
    if (shader->mRiggedVariant && shader->mRiggedVariant != shader)
    {
        prepare_alpha_shader(shader->mRiggedVariant, deferredEnvironment, water_sign);
    }
}

extern bool gCubeSnapshot;

void LLDrawPoolAlpha::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (LLPipeline::isWaterClip() && getType() == LLDrawPool::POOL_ALPHA_PRE_WATER)
    { // don't render alpha objects on the other side of the water plane if water is opaque
        return;
    }

    F32 water_sign = 1.f;

    if (getType() == LLDrawPool::POOL_ALPHA_PRE_WATER)
    {
        water_sign = -1.f;
    }

    if (LLPipeline::sUnderWaterRender)
    {
        water_sign *= -1.f;
    }

    // prepare shaders
    llassert(LLPipeline::sRenderDeferred);

    emissive_shader = &gDeferredEmissiveProgram;
    prepare_alpha_shader(emissive_shader, false, water_sign);

    pbr_emissive_shader = &gPBRGlowProgram;
    prepare_alpha_shader(pbr_emissive_shader, false, water_sign);

    particle_emissive_shader = &gDeferredEmissiveParticleProgram;
    prepare_alpha_shader(particle_emissive_shader, false, water_sign);


    fullbright_shader   =
        (LLPipeline::sImpostorRender) ? &gDeferredFullbrightAlphaMaskProgram :
        (LLPipeline::sRenderingHUDs) ? &gHUDFullbrightAlphaMaskAlphaProgram :
        &gDeferredFullbrightAlphaMaskAlphaProgram;
    prepare_alpha_shader(fullbright_shader, true, water_sign);

    particle_fullbright_shader = &gDeferredFullbrightAlphaMaskAlphaParticleProgram;
    prepare_alpha_shader(particle_fullbright_shader, true, water_sign);

    simple_shader   =
        (LLPipeline::sImpostorRender) ? &gDeferredAlphaImpostorProgram :
        (LLPipeline::sRenderingHUDs) ? &gHUDAlphaProgram :
        &gDeferredAlphaProgram;

    prepare_alpha_shader(simple_shader, true, water_sign); //prime simple shader (loads shadow relevant uniforms)

    particle_shader = &gDeferredAlphaParticleProgram;
    prepare_alpha_shader(particle_shader, true, water_sign);

    LLGLSLShader* materialShader = gDeferredMaterialProgram;
    for (int i = 0; i < LLMaterial::SHADER_COUNT*2; ++i)
    {
        prepare_alpha_shader(&materialShader[i], true, water_sign);
    }

    pbr_shader =
        (LLPipeline::sRenderingHUDs) ? &gHUDPBRAlphaProgram :
        &gDeferredPBRAlphaProgram;

    prepare_alpha_shader(pbr_shader, true, water_sign);

    transmissive_shader = nullptr;
    transmissive_tint_shader = nullptr;
    mTransmissionCap = 0;
    mTransmissionCopies = 0;
    mTransmissionHasDirty = false;

    static LLCachedControl<U32> transmission_quality(gSavedSettings, "RenderTransmissionQuality", 1);
    static const S32 transmission_caps[4] = { 0, 1, 2, 4 };

    if (!LLPipeline::sRenderingHUDs && !LLPipeline::sImpostorRender && !gCubeSnapshot &&
        gDeferredPBRTransmissiveProgram.isComplete() && gDeferredPBRTransmissiveTintProgram.isComplete())
    {
        mTransmissionCap = transmission_caps[llmin((U32)transmission_quality, (U32)3)];
        transmissive_shader = &gDeferredPBRTransmissiveProgram;
        prepare_alpha_shader(transmissive_shader, true, water_sign);
        transmissive_tint_shader = &gDeferredPBRTransmissiveTintProgram;
        prepare_alpha_shader(transmissive_tint_shader, true, water_sign);
    }

    // explicitly unbind here so render loop doesn't make assumptions about the last shader
    // already being setup for rendering
    LLGLSLShader::unbind();

    if (!LLPipeline::sRenderingHUDs)
    {
        // first pass, render rigged objects only and render to depth buffer
        forwardRender(true);
    }

    // second pass, regular forward alpha rendering
    forwardRender();

    endTransmission();

    // final pass, render to depth for depth of field effects
    if (!LLPipeline::sImpostorRender && LLPipeline::RenderDepthOfField && !gCubeSnapshot && !LLPipeline::sRenderingHUDs && getType() == LLDrawPool::POOL_ALPHA_POST_WATER)
    {
        //update depth buffer sampler
        particle_shader = particle_fullbright_shader = &gDeferredFullbrightAlphaMaskParticleProgram;
        particle_shader->bind();
        particle_shader->setMinimumAlpha(0.33f);

        simple_shader = fullbright_shader = &gDeferredFullbrightAlphaMaskProgram;

        simple_shader->bind();
        simple_shader->setMinimumAlpha(0.33f);

        // mask off color buffer writes as we're only writing to depth buffer
        gGL.setColorMask(false, false);

        // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
        // We don't want the nearly invisible objects to cause of DoF effects
        renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2,
            true); // <--- discard mostly transparent faces

        gGL.setColorMask(true, false);
    }
}

void LLDrawPoolAlpha::forwardRender(bool rigged)
{
    gPipeline.enableLightsDynamic();

    LLGLSPipelineAlpha gls_pipeline_alpha;

    //enable writing to alpha for emissive effects
    gGL.setColorMask(true, true);

    bool write_depth = rigged ||
        LLDrawPoolWater::sSkipScreenCopy
        // we want depth written so that rendered alpha will
        // contribute to the alpha mask used for impostors
        || LLPipeline::sImpostorRenderAlphaDepthPass
        || getType() == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER; // needed for accurate water fog


    LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);

    mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
    mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
    mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
    mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
    gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    if (rigged && mType == LLDrawPool::POOL_ALPHA_POST_WATER)
    { // draw GLTF scene to depth buffer before rigged alpha
        LL::GLTFSceneManager::instance().render(false, false);
        LL::GLTFSceneManager::instance().render(false, true);
        LL::GLTFSceneManager::instance().render(false, false, true);
        LL::GLTFSceneManager::instance().render(false, true, true);
    }

    // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
    // We don't want the nearly invisible objects to cause of DoF effects
    renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2, false, rigged);

    gGL.setColorMask(true, false);

    if (!rigged && (LLPipeline::sRenderingHUDs || getType() == LLDrawPoolAlpha::POOL_ALPHA_POST_WATER))
    { //render "highlight alpha" on final non-rigged pass for non-HUDs (HUDs only run pre-water alpha pass)
        // NOTE -- hacky call here protected by !rigged instead of alongside "forwardRender"
        // so renderDebugAlpha is executed while gls_pipeline_alpha and depth GL state
        // variables above are still in scope
        renderDebugAlpha();
    }
}

void LLDrawPoolAlpha::renderDebugAlpha()
{
    if (sShowDebugAlpha && !gCubeSnapshot && !LLPipeline::sReflectionRender)
    {
        gHighlightProgram.bind();
        gGL.diffuseColor4f(1, 0, 0, 1);
        gGL.getTexUnit(0)->bindFast(LLViewerFetchedTexture::getSmokeImage());


        renderAlphaHighlight();

        pushUntexturedBatches(LLRenderPass::PASS_ALPHA_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_ALPHA_INVISIBLE);

        // Material alpha mask
        gGL.diffuseColor4f(0, 0, 1, 1);
        pushUntexturedBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_NORMMAP_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_SPECMAP_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_NORMSPEC_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK);

        gGL.diffuseColor4f(0, 1, 0, 1);
        pushUntexturedBatches(LLRenderPass::PASS_INVISIBLE);

        gHighlightProgram.mRiggedVariant->bind();
        gGL.diffuseColor4f(1, 0, 0, 1);

        pushRiggedBatches(LLRenderPass::PASS_ALPHA_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_ALPHA_INVISIBLE_RIGGED, false);

        // Material alpha mask
        gGL.diffuseColor4f(0, 0, 1, 1);
        pushRiggedBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_NORMMAP_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_SPECMAP_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_NORMSPEC_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK_RIGGED, false);

        gGL.diffuseColor4f(0, 1, 0, 1);
        pushRiggedBatches(LLRenderPass::PASS_INVISIBLE_RIGGED, false);
        LLGLSLShader::sCurBoundShaderPtr->unbind();
    }
}

void LLDrawPoolAlpha::renderAlphaHighlight()
{
    for (int pass = 0; pass < 2; ++pass)
    { //two passes, one rigged and one not
        const LLVOAvatar* lastAvatar = nullptr;
        U64 lastMeshId = 0;
        bool skipLastSkin = false;

        LLCullResult::sg_iterator begin = pass == 0 ? gPipeline.beginAlphaGroups() : gPipeline.beginRiggedAlphaGroups();
        LLCullResult::sg_iterator end = pass == 0 ? gPipeline.endAlphaGroups() : gPipeline.endRiggedAlphaGroups();

        for (LLCullResult::sg_iterator i = begin; i != end; ++i)
        {
            LLSpatialGroup* group = *i;
            if (group->getSpatialPartition()->mRenderByGroup &&
                !group->isDead())
            {
                bool particle = group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE;

                LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA+pass]; // <-- hacky + pass to use PASS_ALPHA_RIGGED on second pass

                for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
                {
                    LLDrawInfo& params = **k;

                    bool rigged = (params.mAvatar != nullptr);
                    (particle ? gHighlightParticleProgram : gHighlightProgram).bind(rigged);

                    if (rigged)
                    {
                        if (!uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
                        { // failed to upload matrix palette, skip rendering
                            continue;
                        }
                    }

                    gGL.diffuseColor4f(1, 0, 0, 1);
                    LLRenderPass::applyModelMatrix(params);
                    params.mVertexBuffer->setBuffer();
                    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
                }
            }
        }
    }

    // make sure static version of highlight shader is bound before returning
    gHighlightProgram.bind();
}

inline bool IsFullbright(LLDrawInfo& params)
{
    return params.mFullbright;
}

inline bool IsMaterial(LLDrawInfo& params)
{
    return params.mMaterial != nullptr;
}

inline bool IsEmissive(LLDrawInfo& params)
{
    return params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE);
}

inline void Draw(LLDrawInfo* draw, U32 mask)
{
    draw->mVertexBuffer->setBuffer();
    LLRenderPass::applyModelMatrix(*draw);
    draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}

bool LLDrawPoolAlpha::TexSetup(LLDrawInfo* draw, bool use_material)
{
    bool tex_setup = false;

    if (draw->mGLTFMaterial)
    {
        if (draw->mTextureMatrix)
        {
            tex_setup = true;
            gGL.getTexUnit(0)->activate();
            gGL.matrixMode(LLRender::MM_TEXTURE);
            gGL.loadMatrix((GLfloat*)draw->mTextureMatrix->mMatrix);
            gPipeline.mTextureMatrixOps++;
        }
    }
    else
    {
        if (!LLPipeline::sRenderingHUDs && use_material && current_shader)
        {
            if (draw->mNormalMap)
            {
                current_shader->bindTexture(LLShaderMgr::BUMP_MAP, draw->mNormalMap);
            }

            if (draw->mSpecularMap)
            {
                current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, draw->mSpecularMap);
            }
        }
        else if (current_shader == simple_shader || current_shader == simple_shader->mRiggedVariant)
        {
            current_shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);
            current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
        }
        if (draw->mTextureList.size() > 1)
        {
            for (U32 i = 0; i < draw->mTextureList.size(); ++i)
            {
                if (draw->mTextureList[i].notNull())
                {
                    gGL.getTexUnit(i)->bindFast(draw->mTextureList[i]);
                }
            }
        }
        else
        { //not batching textures or batch has only 1 texture -- might need a texture matrix
            if (draw->mTexture.notNull())
            {
                if (use_material)
                {
                    current_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, draw->mTexture);
                }
                else
                {
                    gGL.getTexUnit(0)->bindFast(draw->mTexture);
                }

                if (draw->mTextureMatrix)
                {
                    tex_setup = true;
                    gGL.getTexUnit(0)->activate();
                    gGL.matrixMode(LLRender::MM_TEXTURE);
                    gGL.loadMatrix((GLfloat*)draw->mTextureMatrix->mMatrix);
                    gPipeline.mTextureMatrixOps++;
                }
            }
            else
            {
                gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }
    }

    return tex_setup;
}

void LLDrawPoolAlpha::RestoreTexSetup(bool tex_setup)
{
    if (tex_setup)
    {
        gGL.getTexUnit(0)->activate();
        gGL.matrixMode(LLRender::MM_TEXTURE);
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
}

// Keep in sync with TRANSMISSION_MAX_LOD in pbralphaF.glsl
constexpr U32 TRANSMISSION_MAX_MIP = 6;

// Screen rect of a world-space AABB; false means part of it is behind the camera
static bool project_extents(const LLVector4a* ext, S32 width, S32 height, LLRect& rect)
{
    F32 min_x = FLT_MAX;
    F32 min_y = FLT_MAX;
    F32 max_x = -FLT_MAX;
    F32 max_y = -FLT_MAX;

    for (S32 corner = 0; corner < 8; ++corner)
    {
        F32 point[4] = { ext[corner & 1].getF32ptr()[0], ext[(corner >> 1) & 1].getF32ptr()[1], ext[(corner >> 2) & 1].getF32ptr()[2], 1.f };

        F32 eye[4];
        F32 clip[4];

        for (S32 row = 0; row < 4; ++row)
        {
            eye[row] = gGLModelView[row] * point[0] + gGLModelView[4 + row] * point[1] + gGLModelView[8 + row] * point[2] + gGLModelView[12 + row];
        }

        for (S32 row = 0; row < 4; ++row)
        {
            clip[row] = gGLProjection[row] * eye[0] + gGLProjection[4 + row] * eye[1] + gGLProjection[8 + row] * eye[2] + gGLProjection[12 + row] * eye[3];
        }

        if (clip[3] <= 0.001f)
        {
            return false;
        }

        F32 screen_x = (clip[0] / clip[3] * 0.5f + 0.5f) * width;
        F32 screen_y = (clip[1] / clip[3] * 0.5f + 0.5f) * height;
        min_x = llmin(min_x, screen_x);
        max_x = llmax(max_x, screen_x);
        min_y = llmin(min_y, screen_y);
        max_y = llmax(max_y, screen_y);
    }

    // padded for the jittered projection and the mip filter footprint
    rect.mLeft = llclamp((S32)floorf(min_x) - 2, 0, width);
    rect.mRight = llclamp((S32)ceilf(max_x) + 2, 0, width);
    rect.mBottom = llclamp((S32)floorf(min_y) - 2, 0, height);
    rect.mTop = llclamp((S32)ceilf(max_y) + 2, 0, height);
    return true;
}

void LLDrawPoolAlpha::setTransmissionUniforms(const LLGLTFMaterial* gltf_mat, bool has_copy)
{
    static LLStaticHashedString sTransmissionHasCopy("transmissionHasCopy");
    static LLStaticHashedString sAlphaOpaque("alphaOpaque");
    static LLStaticHashedString sTransmissionSamples("transmissionSamples");
    static LLCachedControl<U32> ggx_samples(gSavedSettings, "RenderTransmissionGGXSamples", 4);

    S32 copy_flag = 0;
    if (has_copy)
    {
        copy_flag = 1;
    }

    S32 alpha_opaque = 0;
    if (gltf_mat->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_BLEND)
    {
        alpha_opaque = 1;
    }

    current_shader->uniform1i(sTransmissionHasCopy, copy_flag);
    current_shader->uniform1i(sAlphaOpaque, alpha_opaque);
    current_shader->uniform1i(sTransmissionSamples, (S32)llclamp((U32)ggx_samples, 1u, 16u));
}

void LLDrawPoolAlpha::copyTransmissionScene(const LLRect* dirty)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_GPU_ZONE("transmission copy");

    LLRenderTarget& src = gPipeline.mRT->screen;
    LLRenderTarget& depth_src = gPipeline.mRT->deferredScreen;
    LLRenderTarget& dst = gPipeline.mWaterDis;

    gGL.flush();

    LLGLDisable blend(GL_BLEND);
    LLGLDisable cull_face(GL_CULL_FACE);
    LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

    GLenum scissor_state = 0;

    if (dirty)
    {
        scissor_state = GL_SCISSOR_TEST;
    }

    LLGLEnable scissor(scissor_state);

    dst.bindTarget();

    if (dirty)
    {
        glScissor(dirty->mLeft, dirty->mBottom, dirty->getWidth(), dirty->getHeight());
    }

    gCopyDepthProgram.bind();

    S32 diff_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
    S32 depth_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);

    gGL.getTexUnit(diff_map)->bind(&src);
    gGL.getTexUnit(depth_map)->bind(&depth_src, true);

    gPipeline.mScreenTriangleVB->setBuffer();
    gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    buildTransmissionMips(dirty);

    dst.flush();

    // the copy clobbered texture units that fast-bound alpha shaders rely on
    LLGLSLShader* alpha_shaders[] = {
        simple_shader, fullbright_shader, particle_shader, particle_fullbright_shader,
        pbr_shader, transmissive_shader, transmissive_tint_shader
    };

    for (LLGLSLShader* shader : alpha_shaders)
    {
        if (shader)
        {
            shader->mCanBindFast = false;

            if (shader->mRiggedVariant)
            {
                shader->mRiggedVariant->mCanBindFast = false;
            }
        }
    }

    for (S32 i = 0; i < LLMaterial::SHADER_COUNT * 2; ++i)
    {
        gDeferredMaterialProgram[i].mCanBindFast = false;
    }
}

// dst must be bound; leaves it bound
void LLDrawPoolAlpha::buildTransmissionMips(const LLRect* dirty)
{
    LLRenderTarget& dst = gPipeline.mWaterDis;

    U32 mip_levels = dst.getMipLevels();

    if (mip_levels <= 1 || !gSSRDownsampleProgram.isComplete())
    {
        return;
    }

    U32 built_mip = llmin(mip_levels - 1, TRANSMISSION_MAX_MIP);

    LLGLDisable blend(GL_BLEND);
    LLGLDepthTest depth(GL_FALSE, GL_FALSE);

    gGL.matrixMode(gGL.MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadIdentity();

    gGL.matrixMode(gGL.MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();

    gGL.flush();

    static LLStaticHashedString texelStep("texelStep");
    static LLStaticHashedString sKarisWeight("karisWeight");

    gSSRDownsampleProgram.bind();
    gSSRDownsampleProgram.uniform1i(sKarisWeight, 0);
    S32 diffuse_channel = gSSRDownsampleProgram.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, LLTexUnit::TT_TEXTURE);

    for (U32 mip = 1; mip <= built_mip; ++mip)
    {
        F32 src_width = (F32)llmax(1, (S32)(dst.getWidth() >> (mip - 1)));
        F32 src_height = (F32)llmax(1, (S32)(dst.getHeight() >> (mip - 1)));

        gSSRDownsampleProgram.uniform2f(texelStep, 0.5f / src_width, 0.5f / src_height);

        gGL.getTexUnit(diffuse_channel)->bind(&dst);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, mip - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip - 1);

        dst.bindColorMipLevel(mip);

        if (dirty)
        {
            S32 left = llmax(0, (dirty->mLeft >> mip) - 1);
            S32 bottom = llmax(0, (dirty->mBottom >> mip) - 1);
            S32 right = (dirty->mRight >> mip) + 2;
            S32 top = (dirty->mTop >> mip) + 2;
            glScissor(left, bottom, right - left, top - bottom);
        }

        gPipeline.mScreenTriangleVB->setBuffer();
        gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    }

    gGL.getTexUnit(diffuse_channel)->bind(&dst);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, built_mip);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    dst.resetColorMipLevel();

    gSSRDownsampleProgram.unbind();

    gGL.matrixMode(gGL.MM_PROJECTION);
    gGL.popMatrix();

    gGL.matrixMode(gGL.MM_MODELVIEW);
    gGL.popMatrix();
}

// Other mWaterDis readers sample it without mips
void LLDrawPoolAlpha::endTransmission()
{
    if (mTransmissionCopies <= 0)
    {
        return;
    }

    LLRenderTarget& dst = gPipeline.mWaterDis;
    gGL.getTexUnit(0)->bind(&dst);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, dst.getMipLevels() - 1);

    mTransmissionCopies = 0;
    mTransmissionHasDirty = false;
}

void LLDrawPoolAlpha::drawEmissive(LLDrawInfo* draw)
{
    LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);
    draw->mVertexBuffer->setBuffer();
    draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}


void LLDrawPoolAlpha::renderEmissives(std::vector<LLDrawInfo*>& emissives, LLGLSLShader* shader)
{
    shader->bind();
    shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    for (LLDrawInfo* draw : emissives)
    {
        bool tex_setup = TexSetup(draw, false);
        drawEmissive(draw);
        RestoreTexSetup(tex_setup);
    }
}

void LLDrawPoolAlpha::renderPbrEmissives(std::vector<LLDrawInfo*>& emissives)
{
    pbr_emissive_shader->bind();

    for (LLDrawInfo* draw : emissives)
    {
        llassert(draw->mGLTFMaterial);
        LLGLDisable cull_face(draw->mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);
        draw->mGLTFMaterial->bind(draw->mTexture);
        draw->mVertexBuffer->setBuffer();
        draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
    }
}

void LLDrawPoolAlpha::renderRiggedEmissives(std::vector<LLDrawInfo*>& emissives)
{
    LLGLDepthTest depth(GL_TRUE, GL_FALSE); //disable depth writes since "emissive" is additive so sorting doesn't matter
    LLGLSLShader* shader = emissive_shader->mRiggedVariant;
    shader->bind();
    shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLDrawInfo* draw : emissives)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("Emissives");

        if (uploadMatrixPalette(draw->mAvatar, draw->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            bool tex_setup = TexSetup(draw, false);
            drawEmissive(draw);
            RestoreTexSetup(tex_setup);
        }
    }
}

void LLDrawPoolAlpha::renderRiggedPbrEmissives(std::vector<LLDrawInfo*>& emissives)
{
    LLGLDepthTest depth(GL_TRUE, GL_FALSE); //disable depth writes since "emissive" is additive so sorting doesn't matter
    pbr_emissive_shader->bind(true);

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLDrawInfo* draw : emissives)
    {
        if (!uploadMatrixPalette(draw->mAvatar, draw->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        { // failed to upload matrix palette, skip rendering
            continue;
        }

        LLGLDisable cull_face(draw->mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);
        draw->mGLTFMaterial->bind(draw->mTexture);
        draw->mVertexBuffer->setBuffer();
        draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
    }
}

void LLDrawPoolAlpha::renderAlpha(U32 mask, bool depth_only, bool rigged)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    bool initialized_lighting = false;
    bool light_enabled = true;

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    const LLGLSLShader* lastAvatarShader = nullptr;
    bool skipLastSkin = false;

    LLCullResult::sg_iterator begin;
    LLCullResult::sg_iterator end;

    if (rigged)
    {
        begin = gPipeline.beginRiggedAlphaGroups();
        end = gPipeline.endRiggedAlphaGroups();
    }
    else
    {
        begin = gPipeline.beginAlphaGroups();
        end = gPipeline.endAlphaGroups();
    }

    LLEnvironment& env = LLEnvironment::instance();
    F32 water_height = env.getWaterHeight();

    bool above_water = getType() == LLDrawPool::POOL_ALPHA_POST_WATER;
    if (LLPipeline::sUnderWaterRender)
    {
        above_water = !above_water;
    }


    for (LLCullResult::sg_iterator i = begin; i != end; ++i)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("renderAlpha - group");
        LLSpatialGroup* group = *i;
        llassert(group);
        llassert(group->getSpatialPartition());

        if (group->getSpatialPartition()->mRenderByGroup &&
            !group->isDead())
        {

            LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();
            const LLVector4a* ext = bridge ? bridge->getSpatialExtents() : group->getExtents();

            if (!LLPipeline::sRenderingHUDs) // ignore above/below water for HUD render
            {
                if (above_water)
                { // reject any spatial groups that have no part above water
                    if (ext[1].getF32ptr()[2] < water_height)
                    {
                        continue;
                    }
                }
                else
                { // reject any spatial groups that he no part below water
                    if (ext[0].getF32ptr()[2] > water_height)
                    {
                        continue;
                    }
                }
            }

            static std::vector<LLDrawInfo*> emissives;
            static std::vector<LLDrawInfo*> rigged_emissives;
            static std::vector<LLDrawInfo*> pbr_emissives;
            static std::vector<LLDrawInfo*> pbr_rigged_emissives;

            emissives.resize(0);
            rigged_emissives.resize(0);
            pbr_emissives.resize(0);
            pbr_rigged_emissives.resize(0);

            bool is_particle_or_hud_particle = group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE
                                                      || group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD_PARTICLE;

            bool disable_cull = is_particle_or_hud_particle;
            LLGLDisable cull(disable_cull ? GL_CULL_FACE : 0);

            LLSpatialGroup::drawmap_elem_t& draw_info = rigged ? group->mDrawMap[LLRenderPass::PASS_ALPHA_RIGGED] : group->mDrawMap[LLRenderPass::PASS_ALPHA];

            LLRect group_rect;
            bool group_rect_valid = false;

            for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
            {
                LLDrawInfo& params = **k;
                if ((bool)params.mAvatar != rigged)
                {
                    continue;
                }

                LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("ra - push batch");

                LLRenderPass::applyModelMatrix(params);

                LLMaterial* mat = NULL;
                LLGLTFMaterial *gltf_mat = params.mGLTFMaterial;

                LLGLDisable cull_face(gltf_mat && gltf_mat->mDoubleSided ? GL_CULL_FACE : 0);

                bool transmissive = gltf_mat && gltf_mat->mTransmissionFactor > FLT_EPSILON;
                bool transmissive_draw = transmissive && transmissive_shader && !depth_only;
                bool refracting = transmissive_draw && mTransmissionCap > 0;

                if (gltf_mat && (gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND || transmissive))
                {
                    target_shader = pbr_shader;

                    if (refracting)
                    {
                        target_shader = transmissive_shader;
                    }
                    else if (transmissive_draw)
                    {
                        // no copy: tint the background first, the surface is a second draw below
                        target_shader = transmissive_tint_shader;
                    }

                    if (refracting)
                    {
                        if (!group_rect_valid)
                        {
                            group_rect_valid = true;

                            if (!project_extents(ext, gPipeline.mWaterDis.getWidth(), gPipeline.mWaterDis.getHeight(), group_rect))
                            {
                                group_rect = LLRect(0, gPipeline.mWaterDis.getHeight(), gPipeline.mWaterDis.getWidth(), 0);
                            }
                        }

                        if (mTransmissionCopies == 0)
                        {
                            copyTransmissionScene(nullptr);
                            mTransmissionCopies = 1;
                            mTransmissionHasDirty = false;
                        }
                        else if (mTransmissionCopies < mTransmissionCap && mTransmissionHasDirty && group_rect.overlaps(mTransmissionDirty))
                        {
                            copyTransmissionScene(&mTransmissionDirty);
                            ++mTransmissionCopies;
                            mTransmissionHasDirty = false;
                        }

                        if (mTransmissionHasDirty)
                        {
                            mTransmissionDirty.unionWith(group_rect);
                        }
                        else
                        {
                            mTransmissionDirty = group_rect;
                            mTransmissionHasDirty = true;
                        }
                    }

                    if (params.mAvatar != nullptr)
                    {
                        target_shader = target_shader->mRiggedVariant;
                    }

                    // shader must be bound before LLGLTFMaterial::bind
                    if (current_shader != target_shader)
                    {
                        gPipeline.bindDeferredShaderFast(*target_shader);
                    }

                    params.mGLTFMaterial->bind(params.mTexture);

                    if (transmissive_draw)
                    {
                        setTransmissionUniforms(gltf_mat, refracting);
                    }

                    if (refracting)
                    {
                        S32 transmission_channel = current_shader->enableTexture(LLShaderMgr::TRANSMISSION_SCENE_MAP);

                        if (transmission_channel > -1)
                        {
                            gGL.getTexUnit(transmission_channel)->bind(&gPipeline.mWaterDis);
                        }

                        S32 depth_channel = current_shader->enableTexture(LLShaderMgr::DEFERRED_DEPTH);

                        if (depth_channel > -1)
                        {
                            gGL.getTexUnit(depth_channel)->bind(&gPipeline.mWaterDis, true);
                        }
                    }
                }
                else
                {
                    mat = LLPipeline::sRenderingHUDs ? nullptr : params.mMaterial;

                    if (params.mFullbright)
                    {
                        // Turn off lighting if it hasn't already been so.
                        if (light_enabled || !initialized_lighting)
                        {
                            initialized_lighting = true;
                            target_shader = fullbright_shader;

                            light_enabled = false;
                        }
                    }
                    // Turn on lighting if it isn't already.
                    else if (!light_enabled || !initialized_lighting)
                    {
                        initialized_lighting = true;
                        target_shader = simple_shader;
                        light_enabled = true;
                    }

                    if (LLPipeline::sRenderingHUDs)
                    {
                        target_shader = fullbright_shader;
                    }
                    else if (mat)
                    {
                        U32 mask = params.mShaderMask;

                        llassert(mask < LLMaterial::SHADER_COUNT);
                        target_shader = &(gDeferredMaterialProgram[mask]);
                    }
                    else if (!params.mFullbright)
                    {
                        target_shader = simple_shader;
                    }
                    else
                    {
                        target_shader = fullbright_shader;
                    }

                    if (is_particle_or_hud_particle && !LLPipeline::sRenderingHUDs)
                    {
                        target_shader = params.mFullbright ? particle_fullbright_shader : particle_shader;
                    }

                    if (params.mAvatar != nullptr)
                    {
                        llassert(target_shader->mRiggedVariant != nullptr);
                        target_shader = target_shader->mRiggedVariant;
                    }

                    if (current_shader != target_shader)
                    {// If we need shaders, and we're not ALREADY using the proper shader, then bind it
                    // (this way we won't rebind shaders unnecessarily).
                        gPipeline.bindDeferredShaderFast(*target_shader);

                        if (params.mFullbright)
                        { // make sure the bind the exposure map for fullbright shaders so they can cancel out exposure
                            S32 channel = target_shader->enableTexture(LLShaderMgr::EXPOSURE_MAP);
                            if (channel > -1)
                            {
                                gGL.getTexUnit(channel)->bind(&gPipeline.mExposureMap);
                            }
                        }
                    }

                    LLVector4 spec_color(1, 1, 1, 1);
                    F32 env_intensity = 0.0f;
                    F32 brightness = 1.0f;

                    // We have a material.  Supply the appropriate data here.
                    if (mat)
                    {
                        spec_color = params.mSpecColor;
                        env_intensity = params.mEnvIntensity;
                        brightness = params.mFullbright ? 1.f : 0.f;
                    }

                    if (current_shader)
                    {
                        current_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, spec_color.mV[VRED], spec_color.mV[VGREEN], spec_color.mV[VBLUE], spec_color.mV[VALPHA]);
                        current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, env_intensity);
                        current_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, brightness);
                    }
                }

                if (params.mAvatar && !uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, lastAvatarShader, skipLastSkin))
                {
                    continue;
                }

                bool tex_setup = TexSetup(&params, (mat != nullptr));

                {
                    bool reset_minimum_alpha = false;
                    bool opaque_transmissive = refracting && gltf_mat->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_BLEND;
                    bool tinted_transmissive = transmissive_draw && !refracting;

                    if (opaque_transmissive)
                    {
                        // blending off for color, destination alpha (glow) preserved
                        gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ZERO, LLRender::BF_ZERO, LLRender::BF_ONE);

                        F32 cutoff = 0.f;

                        if (gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
                        {
                            cutoff = gltf_mat->mAlphaCutoff;
                        }

                        current_shader->setMinimumAlpha(cutoff);
                        reset_minimum_alpha = true;
                    }
                    else if (tinted_transmissive)
                    {
                        // background *= transmittance; the surface is added by a second draw below
                        gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_SOURCE_COLOR, LLRender::BF_ZERO, LLRender::BF_ONE);
                    }
                    else
                    {
                        gGL.blendFunc((LLRender::eBlendFactor) params.mBlendFuncSrc, (LLRender::eBlendFactor) params.mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);
                    }

                    if (!opaque_transmissive &&
                        !LLPipeline::sImpostorRender &&
                        params.mBlendFuncDst != LLRender::BF_SOURCE_ALPHA &&
                        params.mBlendFuncSrc != LLRender::BF_SOURCE_ALPHA)
                    { // this draw call has a custom blend function that may require rendering of "invisible" fragments
                        current_shader->setMinimumAlpha(0.f);
                        reset_minimum_alpha = true;
                    }

                    params.mVertexBuffer->setBuffer();
                    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
                    stop_glerror();

                    if (tinted_transmissive)
                    {
                        LLGLSLShader* surface_shader = transmissive_shader;

                        if (params.mAvatar != nullptr)
                        {
                            surface_shader = surface_shader->mRiggedVariant;
                        }

                        gPipeline.bindDeferredShaderFast(*surface_shader);
                        params.mGLTFMaterial->bind(params.mTexture);
                        setTransmissionUniforms(gltf_mat, false);

                        if (params.mAvatar == nullptr || uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, lastAvatarShader, skipLastSkin))
                        {
                            gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ZERO, LLRender::BF_ONE);
                            params.mVertexBuffer->setBuffer();
                            params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
                            stop_glerror();
                        }
                    }

                    if (reset_minimum_alpha)
                    {
                        current_shader->setMinimumAlpha(MINIMUM_ALPHA);
                    }
                }

                // If this alpha mesh has glow, then draw it a second time to add the destination-alpha (=glow).  Interleaving these state-changing calls is expensive, but glow must be drawn Z-sorted with alpha.
                if (getType() != LLDrawPool::POOL_ALPHA_PRE_WATER &&
                    params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE))
                {
                    if (params.mAvatar != nullptr)
                    {
                        if (params.mGLTFMaterial.isNull())
                        {
                            rigged_emissives.push_back(&params);
                        }
                        else
                        {
                            pbr_rigged_emissives.push_back(&params);
                        }
                    }
                    else
                    {
                        if (params.mGLTFMaterial.isNull())
                        {
                            emissives.push_back(&params);
                        }
                        else
                        {
                            pbr_emissives.push_back(&params);
                        }
                    }
                }

                if (tex_setup)
                {
                    gGL.getTexUnit(0)->activate();
                    gGL.matrixMode(LLRender::MM_TEXTURE);
                    gGL.loadIdentity();
                    gGL.matrixMode(LLRender::MM_MODELVIEW);
                }
            }

            // render emissive faces into alpha channel for bloom effects
            if (!depth_only)
            {
                gPipeline.enableLightsDynamic();

                // install glow-accumulating blend mode
                // don't touch color, add to alpha (glow)
                gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);

                bool rebind = false;
                LLGLSLShader* lastShader = current_shader;
                if (!emissives.empty())
                {
                    light_enabled = true;
                    renderEmissives(emissives, (is_particle_or_hud_particle && !LLPipeline::sRenderingHUDs) ? particle_emissive_shader : emissive_shader);
                    rebind = true;
                }

                if (!pbr_emissives.empty())
                {
                    light_enabled = true;
                    renderPbrEmissives(pbr_emissives);
                    rebind = true;
                }

                if (!rigged_emissives.empty())
                {
                    light_enabled = true;
                    renderRiggedEmissives(rigged_emissives);
                    rebind = true;
                }

                if (!pbr_rigged_emissives.empty())
                {
                    light_enabled = true;
                    renderRiggedPbrEmissives(pbr_rigged_emissives);
                    rebind = true;
                }

                // restore our alpha blend mode
                gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

                if (lastShader && rebind)
                {
                    lastShader->bind();
                }
            }
        }
    }

    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    LLVertexBuffer::unbind();

    if (!light_enabled)
    {
        gPipeline.enableLightsDynamic();
    }
}

S32 LLDrawPoolAlpha::getNumVelocityPasses()
{
    // Only render velocity from one instance to avoid double-stamping mLastModelMatrix
    if (getType() == LLDrawPool::POOL_ALPHA_PRE_WATER)
        return 0;
    return 1;
}

void LLDrawPoolAlpha::beginVelocityPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    gVelocityAlphaProgram.bind();
    gVelocityAlphaProgram.uniformMatrix4fv(LLShaderMgr::LAST_MODELVIEW_MATRIX, 1, GL_FALSE, gGLLastModelView);
    gVelocityAlphaProgram.uniformMatrix4fv(LLShaderMgr::CURRENT_MODELVIEW_MATRIX, 1, GL_FALSE, gGLModelView);
    gVelocityAlphaProgram.uniform4f(LLShaderMgr::VIEWPORT, (F32)gGLViewport[0], (F32)gGLViewport[1], (F32)gGLViewport[2], (F32)gGLViewport[3]);
}

void LLDrawPoolAlpha::endVelocityPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    gVelocityAlphaProgram.unbind();
}

void LLDrawPoolAlpha::renderVelocity(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    LLGLEnable cull(GL_CULL_FACE);
    pushVelocityBatchesTextured(LLRenderPass::PASS_ALPHA);

    if (LLVOPartGroup::gpuBillboardsEnabled())
    {
        static const LLMatrix4 identity;
        gVelocityAlphaParticleProgram.bind();
        gVelocityAlphaParticleProgram.uniformMatrix4fv(LLShaderMgr::LAST_MODELVIEW_MATRIX, 1, GL_FALSE, gGLLastModelView);
        gVelocityAlphaParticleProgram.uniformMatrix4fv(LLShaderMgr::CURRENT_MODELVIEW_MATRIX, 1, GL_FALSE, gGLModelView);
        gVelocityAlphaParticleProgram.uniform4f(LLShaderMgr::VIEWPORT, (F32)gGLViewport[0], (F32)gGLViewport[1], (F32)gGLViewport[2], (F32)gGLViewport[3]);
        gVelocityAlphaParticleProgram.uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*) identity.mMatrix);

        LLGLDisable no_cull(GL_CULL_FACE);
        for (LLCullResult::sg_iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
        {
            LLSpatialGroup* group = *i;
            if (group->isDead() || group->getSpatialPartition()->mPartitionType != LLViewerRegion::PARTITION_PARTICLE)
            {
                continue;
            }

            LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];
            for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
            {
                LLDrawInfo& params = **k;
                if (params.mVertexBuffer.isNull())
                {
                    continue;
                }

                LLRenderPass::applyModelMatrix(params);
                if (params.mTexture.notNull())
                {
                    gGL.getTexUnit(0)->bindFast(params.mTexture);
                }
                params.mVertexBuffer->setBuffer();
                params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
            }
        }
    }

    gVelocityAlphaProgram.bind(true);
    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_MODELVIEW_MATRIX, 1, GL_FALSE, gGLLastModelView);
    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::CURRENT_MODELVIEW_MATRIX, 1, GL_FALSE, gGLModelView);
    LLGLSLShader::sCurBoundShaderPtr->uniform4f(LLShaderMgr::VIEWPORT, (F32)gGLViewport[0], (F32)gGLViewport[1], (F32)gGLViewport[2], (F32)gGLViewport[3]);
    pushRiggedVelocityBatchesTextured(LLRenderPass::PASS_ALPHA_RIGGED);
}
