/**
 * @file lldrawpooltree.h
 * @brief LLDrawPoolTree class definition
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

#ifndef LL_LLDRAWPOOLTREE_H
#define LL_LLDRAWPOOLTREE_H

#include "lldrawpool.h"

class LLDrawPoolTree : public LLFacePool
{
    LLPointer<LLViewerTexture> mTexturep;
public:
    enum
    {
        VERTEX_DATA_MASK =  LLVertexBuffer::MAP_VERTEX |
                            LLVertexBuffer::MAP_NORMAL |
                            LLVertexBuffer::MAP_COLOR  |
                            LLVertexBuffer::MAP_TEXCOORD0
    };

    U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

    LLDrawPoolTree(LLViewerTexture *texturep);

    S32 getNumDeferredPasses() override { return 1; }
    void beginDeferredPass(S32 pass) override;
    void endDeferredPass(S32 pass) override;
    void renderDeferred(S32 pass) override;

    S32 getNumShadowPasses() override { return 1; }
    void beginShadowPass(S32 pass) override;
    void endShadowPass(S32 pass) override;
    void renderShadow(S32 pass) override;

    S32 getNumVelocityPasses() override;
    void beginVelocityPass(S32 pass) override;
    void endVelocityPass(S32 pass) override;
    void renderVelocity(S32 pass) override;

    bool verify() const override;
    LLViewerTexture *getTexture() override;
    LLViewerTexture *getDebugTexture() override;
    /*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

    static S32 sDiffTex;
};

#endif // LL_LLDRAWPOOLTREE_H
