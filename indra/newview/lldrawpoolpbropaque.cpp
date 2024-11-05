/**
 * @file lldrawpoolpbropaque.cpp
 * @brief LLDrawPoolGLTFPBR class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpool.h"
#include "lldrawpoolpbropaque.h"
#include "llviewershadermgr.h"
#include "pipeline.h"
#include "gltfscenemanager.h"

extern LLCullResult* sCull;

LLDrawPoolGLTFPBR::LLDrawPoolGLTFPBR(U32 type) :
    LLRenderPass(type)
{
    if (type == LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK)
    {
        mRenderType = LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK;
    }
    else
    {
        mRenderType = LLPipeline::RENDER_TYPE_PASS_GLTF_PBR;
    }
}

S32 LLDrawPoolGLTFPBR::getNumDeferredPasses()
{
    return 1;
}

void LLDrawPoolGLTFPBR::renderDeferred(S32 pass)
{
    llassert(!LLPipeline::sRenderingHUDs);

    if (mRenderType == LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK)
    {
        // opaque
        LL::GLTFSceneManager::instance().render(true);
        // opaque rigged
        LL::GLTFSceneManager::instance().render(true, true);
    }

    LLGLTFMaterial::AlphaMode alpha_mode = mRenderType == LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK ? LLGLTFMaterial::ALPHA_MODE_MASK : LLGLTFMaterial::ALPHA_MODE_OPAQUE;

    for (U32 double_sided = 0; double_sided < 2; ++double_sided)
    {
        LLGLDisable cull(double_sided ? GL_CULL_FACE : 0);
        for (U32 planar = 0; planar < 2; ++planar)
        {
            for (U32 tex_anim = 0; tex_anim < 2; ++tex_anim)
            {
                LLGLSLShader& shader = gGLTFPBRShaderPack.mShader[alpha_mode][double_sided][planar][tex_anim];
                shader.bind();
                pushGLTFBatches(sCull->mGLTFBatches.mDrawInfo[alpha_mode][double_sided][planar][tex_anim], planar, tex_anim);

                shader.bind(true);
                pushRiggedGLTFBatches(sCull->mGLTFBatches.mSkinnedDrawInfo[alpha_mode][double_sided][planar][tex_anim], planar, tex_anim);

                if (!double_sided && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_MATERIALS))
                {
                    for (U32 norm_map = 0; norm_map < 2; ++norm_map)
                    {
                        LLGLSLShader& shader = gBPShaderPack.mShader[alpha_mode][norm_map][planar][tex_anim];
                        shader.bind();
                        pushBPBatches(sCull->mBPBatches.mDrawInfo[alpha_mode][norm_map][planar][tex_anim], planar, tex_anim);

                        shader.bind(true);
                        pushRiggedBPBatches(sCull->mBPBatches.mSkinnedDrawInfo[alpha_mode][norm_map][planar][tex_anim], planar, tex_anim);
                    }
                }
            }
        }
    }
}

S32 LLDrawPoolGLTFPBR::getNumPostDeferredPasses()
{
    return 1;
}

void LLDrawPoolGLTFPBR::renderPostDeferred(S32 pass)
{
    if (LLPipeline::sRenderingHUDs)
    {
        gHUDPBROpaqueProgram.bind();
        //pushGLTFBatches(mRenderType);
    }
    else if (mRenderType == LLPipeline::RENDER_TYPE_PASS_GLTF_PBR) // HACK -- don't render glow except for the non-alpha masked implementation
    {
#if 0
        gGL.setColorMask(false, true);
        gPBRGlowProgram.bind();
        pushGLTFBatches(LLRenderPass::PASS_GLTF_GLOW);

        gPBRGlowProgram.bind(true);
        pushRiggedGLTFBatches(LLRenderPass::PASS_GLTF_GLOW_RIGGED);
#endif

        gGL.setColorMask(true, false);
    }
}

