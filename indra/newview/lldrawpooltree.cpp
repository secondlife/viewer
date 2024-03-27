/** 
 * @file lldrawpooltree.cpp
 * @brief LLDrawPoolTree class implementation
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

#include "lldrawpooltree.h"

#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llvotree.h"
#include "pipeline.h"
#include "llviewercamera.h"
#include "llviewershadermgr.h"
#include "llrender.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llenvironment.h"

S32 LLDrawPoolTree::sDiffTex = 0;
static LLGLSLShader* shader = NULL;

LLDrawPoolTree::LLDrawPoolTree(LLViewerTexture *texturep) :
	LLFacePool(POOL_TREE),
	mTexturep(texturep)
{
	mTexturep->setAddressMode(LLTexUnit::TAM_WRAP);
}

//============================================
// deferred implementation
//============================================
void LLDrawPoolTree::beginDeferredPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_TREES);
		
	shader = &gDeferredTreeProgram;
	shader->bind();
	shader->setMinimumAlpha(0.5f);
}

void LLDrawPoolTree::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;

    if (mDrawFace.empty())
    {
        return;
    }


    gGL.getTexUnit(sDiffTex)->bindFast(mTexturep);
    mTexturep->addTextureStats(1024.f * 1024.f); // <=== keep Linden tree textures at full res

    for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
        iter != mDrawFace.end(); iter++)
    {
        LLFace* face = *iter;
        LLVertexBuffer* buff = face->getVertexBuffer();

        if (buff)
        {
            LLMatrix4* model_matrix = &(face->getDrawable()->getRegion()->mRenderMatrix);

            if (model_matrix != gGLLastMatrix)
            {
                gGLLastMatrix = model_matrix;
                gGL.loadMatrix(gGLModelView);
                if (model_matrix)
                {
                    llassert(gGL.getMatrixMode() == LLRender::MM_MODELVIEW);
                    gGL.multMatrix((GLfloat*)model_matrix->mMatrix);
                }
                gPipeline.mMatrixOpCount++;
            }

            buff->setBuffer();
            buff->drawRange(LLRender::TRIANGLES, 0, buff->getNumVerts() - 1, buff->getNumIndices(), 0);
        }
    }
}

void LLDrawPoolTree::endDeferredPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_TREES);
		
	shader->unbind();
}

//============================================
// shadow implementation
//============================================
void LLDrawPoolTree::beginShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
	
	glPolygonOffset(gSavedSettings.getF32("RenderDeferredTreeShadowOffset"),
					gSavedSettings.getF32("RenderDeferredTreeShadowBias"));

    LLEnvironment& environment = LLEnvironment::instance();

	gDeferredTreeShadowProgram.bind();
    gDeferredTreeShadowProgram.uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);
	gDeferredTreeShadowProgram.setMinimumAlpha(0.5f);
}

void LLDrawPoolTree::renderShadow(S32 pass)
{
	renderDeferred(pass);
}

void LLDrawPoolTree::endShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
	
	glPolygonOffset(gSavedSettings.getF32("RenderDeferredSpotShadowOffset"),
						gSavedSettings.getF32("RenderDeferredSpotShadowBias"));
	gDeferredTreeShadowProgram.unbind();
}

BOOL LLDrawPoolTree::verify() const
{
	return TRUE;
}

LLViewerTexture *LLDrawPoolTree::getTexture()
{
	return mTexturep;
}

LLViewerTexture *LLDrawPoolTree::getDebugTexture()
{
	return mTexturep;
}


LLColor3 LLDrawPoolTree::getDebugColor() const
{
	return LLColor3(1.f, 0.f, 1.f);
}

