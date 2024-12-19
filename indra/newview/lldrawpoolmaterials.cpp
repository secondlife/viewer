/**
 * @file lldrawpool.cpp
 * @brief LLDrawPoolMaterials class implementation
 * @author Jonathan "Geenz" Goodman
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "lldrawpoolmaterials.h"
#include "llviewershadermgr.h"
#include "pipeline.h"
#include "llglcommonfunc.h"
#include "llvoavatar.h"

LLDrawPoolMaterials::LLDrawPoolMaterials()
:  LLRenderPass(LLDrawPool::POOL_MATERIALS)
{

}

void LLDrawPoolMaterials::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

S32 LLDrawPoolMaterials::getNumDeferredPasses()
{
    // 12 render passes times 2 (one for each rigged and non rigged)
    return 12*2;
}

void LLDrawPoolMaterials::beginDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MATERIAL;

    bool rigged = false;
    if (pass >= 12)
    {
        rigged = true;
        pass -= 12;
    }
    U32 shader_idx[] =
    {
        0, //LLRenderPass::PASS_MATERIAL,
        //1, //LLRenderPass::PASS_MATERIAL_ALPHA,
        2, //LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
        3, //LLRenderPass::PASS_MATERIAL_ALPHA_GLOW,
        4, //LLRenderPass::PASS_SPECMAP,
        //5, //LLRenderPass::PASS_SPECMAP_BLEND,
        6, //LLRenderPass::PASS_SPECMAP_MASK,
        7, //LLRenderPass::PASS_SPECMAP_GLOW,
        8, //LLRenderPass::PASS_NORMMAP,
        //9, //LLRenderPass::PASS_NORMMAP_BLEND,
        10, //LLRenderPass::PASS_NORMMAP_MASK,
        11, //LLRenderPass::PASS_NORMMAP_GLOW,
        12, //LLRenderPass::PASS_NORMSPEC,
        //13, //LLRenderPass::PASS_NORMSPEC_BLEND,
        14, //LLRenderPass::PASS_NORMSPEC_MASK,
        15, //LLRenderPass::PASS_NORMSPEC_GLOW,
    };

    U32 idx = shader_idx[pass];

    mShader = &(gDeferredMaterialProgram[idx]);

    if (rigged)
    {
        llassert(mShader->mRiggedVariant != nullptr);
        mShader = mShader->mRiggedVariant;
    }

    gPipeline.bindDeferredShader(*mShader);
}

void LLDrawPoolMaterials::endDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MATERIAL;

    mShader->unbind();

    LLRenderPass::endRenderPass(pass);
}

void LLDrawPoolMaterials::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MATERIAL;
    static const U32 type_list[] =
    {
        LLRenderPass::PASS_MATERIAL,
        //LLRenderPass::PASS_MATERIAL_ALPHA,
        LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
        LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
        LLRenderPass::PASS_SPECMAP,
        //LLRenderPass::PASS_SPECMAP_BLEND,
        LLRenderPass::PASS_SPECMAP_MASK,
        LLRenderPass::PASS_SPECMAP_EMISSIVE,
        LLRenderPass::PASS_NORMMAP,
        //LLRenderPass::PASS_NORMMAP_BLEND,
        LLRenderPass::PASS_NORMMAP_MASK,
        LLRenderPass::PASS_NORMMAP_EMISSIVE,
        LLRenderPass::PASS_NORMSPEC,
        //LLRenderPass::PASS_NORMSPEC_BLEND,
        LLRenderPass::PASS_NORMSPEC_MASK,
        LLRenderPass::PASS_NORMSPEC_EMISSIVE,
    };

    bool rigged = false;
    if (pass >= 12)
    {
        rigged = true;
        pass -= 12;
    }

    llassert(pass < sizeof(type_list)/sizeof(U32));

    U32 type = type_list[pass];
    if (rigged)
    {
        type += 1;
    }

    LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type);
    LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type);

    F32 lastIntensity = 0.f;
    F32 lastFullbright = 0.f;
    F32 lastMinimumAlpha = 0.f;
    LLVector4 lastSpecular = LLVector4(0, 0, 0, 0);

    GLint intensity = mShader->getUniformLocation(LLShaderMgr::ENVIRONMENT_INTENSITY);
    GLint brightness = mShader->getUniformLocation(LLShaderMgr::EMISSIVE_BRIGHTNESS);
    GLint minAlpha = mShader->getUniformLocation(LLShaderMgr::MINIMUM_ALPHA);
    GLint specular = mShader->getUniformLocation(LLShaderMgr::SPECULAR_COLOR);

    GLint diffuseChannel = mShader->enableTexture(LLShaderMgr::DIFFUSE_MAP);
    GLint specChannel = mShader->enableTexture(LLShaderMgr::SPECULAR_MAP);
    GLint normChannel = mShader->enableTexture(LLShaderMgr::BUMP_MAP);

    LLTexture* lastNormalMap = nullptr;
    LLTexture* lastSpecMap = nullptr;
    LLTexture* lastDiffuse = nullptr;

    gGL.getTexUnit(diffuseChannel)->unbindFast(LLTexUnit::TT_TEXTURE);

    if (intensity > -1)
    {
        glUniform1f(intensity, lastIntensity);
    }

    if (brightness > -1)
    {
        glUniform1f(brightness, lastFullbright);
    }

    if (minAlpha > -1)
    {
        glUniform1f(minAlpha, lastMinimumAlpha);
    }

    if (specular > -1)
    {
        glUniform4fv(specular, 1, lastSpecular.mV);
    }

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_MATERIAL("materials draw loop");
        LLDrawInfo& params = **i;

        LLCullResult::increment_iterator(i, end);

        if (specular > -1 && params.mSpecColor != lastSpecular)
        {
            lastSpecular = params.mSpecColor;
            glUniform4fv(specular, 1, lastSpecular.mV);
        }

        if (intensity != -1 && lastIntensity != params.mEnvIntensity)
        {
            lastIntensity = params.mEnvIntensity;
            glUniform1f(intensity, lastIntensity);
        }

        if (minAlpha > -1 && lastMinimumAlpha != params.mAlphaMaskCutoff)
        {
            lastMinimumAlpha = params.mAlphaMaskCutoff;
            glUniform1f(minAlpha, lastMinimumAlpha);
        }

        F32 fullbright = params.mFullbright ? 1.f : 0.f;
        if (brightness > -1 && lastFullbright != fullbright)
        {
            lastFullbright = fullbright;
            glUniform1f(brightness, lastFullbright);
        }

        if (normChannel > -1 && params.mNormalMap != lastNormalMap)
        {
            lastNormalMap = params.mNormalMap;
            llassert(lastNormalMap);
            gGL.getTexUnit(normChannel)->bindFast(lastNormalMap);
        }

        if (specChannel > -1 && params.mSpecularMap != lastSpecMap)
        {
            lastSpecMap = params.mSpecularMap;
            llassert(lastSpecMap);
            gGL.getTexUnit(specChannel)->bindFast(lastSpecMap);
        }

        if (params.mTexture != lastDiffuse)
        {
            lastDiffuse = params.mTexture;
            if (lastDiffuse)
            {
                gGL.getTexUnit(diffuseChannel)->bindFast(lastDiffuse);
            }
            else
            {
                gGL.getTexUnit(diffuseChannel)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }

        // upload matrix palette to shader
        if (rigged)
        {
            if (!uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
            {
                continue;
            }
        }

        applyModelMatrix(params);

        bool tex_setup = false;

        //not batching textures or batch has only 1 texture -- might need a texture matrix
        if (params.mTextureMatrix)
        {
            gGL.getTexUnit(0)->activate();
            gGL.matrixMode(LLRender::MM_TEXTURE);

            gGL.loadMatrix((GLfloat*)params.mTextureMatrix->mMatrix);
            gPipeline.mTextureMatrixOps++;

            tex_setup = true;
        }

        /*if (params.mGroup)  // TOO LATE
        {
            params.mGroup->rebuildMesh();
        }*/

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

        if (tex_setup)
        {
            gGL.getTexUnit(0)->activate();
            gGL.loadIdentity();
            gGL.matrixMode(LLRender::MM_MODELVIEW);
        }
    }
}
