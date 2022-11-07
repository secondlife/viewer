/** 
 * @file lldrawpoolsky.cpp
 * @brief LLDrawPoolSky class implementation
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

#include "lldrawpoolsky.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llvosky.h"
#include "llworld.h" // To get water height
#include "pipeline.h"
#include "llviewershadermgr.h"

LLDrawPoolSky::LLDrawPoolSky()
:   LLFacePool(POOL_SKY),
    
    mSkyTex(NULL),
    mShader(NULL)
{
}

void LLDrawPoolSky::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT); 
    gSky.mVOSkyp->updateGeometry(gSky.mVOSkyp->mDrawable);
}

void LLDrawPoolSky::render(S32 pass)
{
    gGL.flush();

    if (mDrawFace.empty())
    {
        return;
    }

    // Don't draw the sky box if we can and are rendering the WL sky dome.
    if (gPipeline.canUseWindLightShaders())
    {
        return;
    }
    
    // don't render sky under water (background just gets cleared to fog color)
    if(mShaderLevel > 0 && LLPipeline::sUnderWaterRender)
    {
        return;
    }


    //just use the UI shader (generic single texture no lighting)
    gOneTextureNoColorProgram.bind();

    LLGLSPipelineDepthTestSkyBox gls_skybox(true, false);

    LLGLEnable fog_enable( (mShaderLevel < 1 && LLViewerCamera::getInstance()->cameraUnderWater()) ? GL_FOG : 0);
    
    gGL.pushMatrix();
    LLVector3 origin = LLViewerCamera::getInstance()->getOrigin();
    gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);

    S32 face_count = (S32)mDrawFace.size();

    LLVertexBuffer::unbind();
    gGL.diffuseColor4f(1,1,1,1);

    for (S32 i = 0; i < face_count; ++i)
    {
        renderSkyFace(i);
    }

    gGL.popMatrix();
}

void LLDrawPoolSky::renderSkyFace(U8 index)
{
    LLFace* face = mDrawFace[index];

    if (!face || !face->getGeomCount())
    {
        return;
    }

    if (index < 6) // sky tex...interp
    {
        llassert(mSkyTex);
        mSkyTex[index].bindTexture(true); // bind the current tex

        face->renderIndexed();
    }
    else // heavenly body faces, no interp...
    {
        LLGLEnable blend(GL_BLEND);

        LLViewerTexture* tex = face->getTexture(LLRender::DIFFUSE_MAP);
        if (tex)
        {
            gGL.getTexUnit(0)->bind(tex, true);
            face->renderIndexed();
        }
    }
}

void LLDrawPoolSky::endRenderPass( S32 pass )
{
}

