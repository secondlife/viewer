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

#include "lldrawpoolwaterexclusion.h"
#include "llviewershadermgr.h"
#include "pipeline.h"
#include "llglcommonfunc.h"
#include "llvoavatar.h"
#include "lldrawpoolwater.h"

LLDrawPoolWaterExclusion::LLDrawPoolWaterExclusion() : LLRenderPass(LLDrawPool::POOL_WATEREXCLUSION)
{
    LL_INFOS("DPInvisible") << "Creating water exclusion draw pool" << LL_ENDL;
}


void LLDrawPoolWaterExclusion::render(S32 pass)
{                                             // render invisiprims
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; // LL_RECORD_BLOCK_TIME(FTM_RENDER_INVISIBLE);

    if (gPipeline.shadersLoaded())
    {
        gDrawColorProgram.bind();
    }


    LLGLDepthTest depth(GL_TRUE);
    gDrawColorProgram.uniform4f(LLShaderMgr::DIFFUSE_COLOR, 1, 1, 1, 1);

    LLDrawPoolWater* pwaterpool = (LLDrawPoolWater*)gPipeline.getPool(LLDrawPool::POOL_WATER);
    if (pwaterpool)
    {
        // Just treat our water planes as double sided for the purposes of generating the exclusion mask.
        LLGLDisable cullface(GL_CULL_FACE);
        pwaterpool->pushWaterPlanes(0);

        // Take care of the edge water tiles.
        pwaterpool->pushWaterPlanes(1);
    }

    gDrawColorProgram.uniform4f(LLShaderMgr::DIFFUSE_COLOR, 0, 0, 0, 1);

    static LLStaticHashedString waterSign("waterSign");
    gDrawColorProgram.uniform1f(waterSign, 1.f);

    pushBatches(LLRenderPass::PASS_INVISIBLE, false, false);


    if (gPipeline.shadersLoaded())
    {
        gDrawColorProgram.unbind();
    }
}
