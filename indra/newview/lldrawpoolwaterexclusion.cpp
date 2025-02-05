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

LLDrawPoolWaterExclusion::LLDrawPoolWaterExclusion() : LLRenderPass(LLDrawPool::POOL_INVISIBLE)
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

    gDrawColorProgram.uniform4f(LLShaderMgr::DIFFUSE_COLOR, 0, 0, 0, 0);
    gDrawColorProgram.uniform4fv(LLShaderMgr::WATER_WATERPLANE, 1, LLDrawPoolAlpha::sWaterPlane.mV);

    static LLStaticHashedString waterSign("waterSign");
    gDrawColorProgram.uniform1f(waterSign, 1.f);

    gGL.setColorMask(true, false);
    pushBatches(LLRenderPass::PASS_INVISIBLE, false, false);
    gGL.setColorMask(true, false);

    if (gPipeline.shadersLoaded())
    {
        gDrawColorProgram.unbind();
    }
}
