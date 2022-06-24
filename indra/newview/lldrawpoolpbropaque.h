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

class LLDrawPoolPBROpaque : public LLRenderPass
{
public:
    enum
    {
        // See: DEFERRED_VB_MASK
        VERTEX_DATA_MASK = 0
                         | LLVertexBuffer::MAP_VERTEX
                         | LLVertexBuffer::MAP_NORMAL
                         | LLVertexBuffer::MAP_TEXCOORD0 // Diffuse
                         | LLVertexBuffer::MAP_TEXCOORD1 // Normal
                         | LLVertexBuffer::MAP_TEXCOORD2 // Spec <-- ORM Occlusion Roughness Metal
                         | LLVertexBuffer::MAP_TANGENT
                         | LLVertexBuffer::MAP_COLOR
    };
    virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

    LLDrawPoolPBROpaque();

    /*virtual*/ S32 getNumDeferredPasses() { return 1; }
    /*virtual*/ void beginDeferredPass(S32 pass);
    /*virtual*/ void endDeferredPass(S32 pass);
    /*virtual*/ void renderDeferred(S32 pass);

    // Non ALM isn't supported
    /*virtual*/ void beginRenderPass(S32 pass);
    /*virtual*/ void endRenderPass(S32 pass);
    /*virtual*/ S32  getNumPasses() { return 0; }
    /*virtual*/ void render(S32 pass = 0);
    /*virtual*/ void prerender();

};

#endif // LL_LLDRAWPOOLPBROPAQUE_H
