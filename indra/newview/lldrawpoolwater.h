/**
 * @file lldrawpoolwater.h
 * @brief LLDrawPoolWater class definition
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

#ifndef LL_LLDRAWPOOLWATER_H
#define LL_LLDRAWPOOLWATER_H

#include "lldrawpool.h"


class LLFace;
class LLHeavenBody;
class LLWaterSurface;
class LLGLSLShader;

class LLDrawPoolWater final: public LLFacePool
{
protected:
    LLPointer<LLViewerTexture> mWaterImagep[2];
    LLPointer<LLViewerTexture> mWaterNormp[2];

    LLPointer<LLViewerTexture> mOpaqueWaterImagep;

public:
    static bool sSkipScreenCopy;
    static bool sNeedsReflectionUpdate;
    static bool sNeedsDistortionUpdate;
    static F32 sWaterFogEnd;

    enum
    {
        VERTEX_DATA_MASK =  LLVertexBuffer::MAP_VERTEX |
                            LLVertexBuffer::MAP_NORMAL |
                            LLVertexBuffer::MAP_TEXCOORD0
    };

    virtual U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

    LLDrawPoolWater();
    /*virtual*/ ~LLDrawPoolWater();

    S32 getNumPostDeferredPasses() override;
    void beginPostDeferredPass(S32 pass) override;
    void renderPostDeferred(S32 pass) override;

    void prerender() override;

    LLViewerTexture *getDebugTexture() override;
    LLColor3 getDebugColor() const; // For AGP debug display

    void setTransparentTextures(const LLUUID& transparentTextureId, const LLUUID& nextTransparentTextureId);
    void setOpaqueTexture(const LLUUID& opaqueTextureId);
    void setNormalMaps(const LLUUID& normalMapId, const LLUUID& nextNormalMapId);

protected:
    void renderOpaqueLegacyWater();
};

void cgErrorCallback();

#endif // LL_LLDRAWPOOLWATER_H
