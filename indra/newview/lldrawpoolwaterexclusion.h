/**
 * @file lldrawpoolwaterexclusion.h
 * @brief LLDrawPoolWaterExclusion class definition
 * @author Jonathan "Geenz" Goodman
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
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

#ifndef LL_LLDRAWPOOLWATEREXCLUSION_H
#define LL_LLDRAWPOOLWATEREXCLUSION_H

#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "llvertexbuffer.h"
#include "lldrawpool.h"

class LLViewerTexture;
class LLDrawInfo;
class LLGLSLShader;

class LLDrawPoolWaterExclusion : public LLRenderPass
{
public:
    LLDrawPoolWaterExclusion();

    enum
    {
        VERTEX_DATA_MASK = LLVertexBuffer::MAP_VERTEX
    };

    virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

    virtual void prerender() {}

    virtual void render(S32 pass = 0);
    virtual void beginRenderPass(S32 pass) {}
    virtual void endRenderPass(S32 pass) {}
    virtual S32  getNumPasses() { return 1; }
};

#endif // LL_LLDRAWPOOLWATEREXCLUSION_H
