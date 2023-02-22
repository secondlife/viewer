/** 
 * @file lldrawpoolpbropaque.h
 * @brief LLDrawPoolPBrOpaque class definition
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

#ifndef LL_LLDRAWPOOLPBROPAQUE_H
#define LL_LLDRAWPOOLPBROPAQUE_H

#include "lldrawpool.h"

class LLDrawPoolGLTFPBR final : public LLRenderPass
{
public:
    LLDrawPoolGLTFPBR(U32 type = LLDrawPool::POOL_GLTF_PBR);

    U32 mRenderType = 0;

    S32 getNumDeferredPasses() override;
    void renderDeferred(S32 pass) override;

    S32 getNumPostDeferredPasses() override;
    void renderPostDeferred(S32 pass) override;
};

#endif // LL_LLDRAWPOOLPBROPAQUE_H
