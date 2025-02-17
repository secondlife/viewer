 /**
 * @file lldrawpoolavatar.h
 * @brief LLDrawPoolAvatar class definition
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

#ifndef LL_LLDRAWPOOLAVATAR_H
#define LL_LLDRAWPOOLAVATAR_H

#include "lldrawpool.h"
#include "llmodel.h"

#include <unordered_map>

class LLVOAvatar;
class LLVOVolume;
class LLGLSLShader;
class LLFace;
class LLVolume;
class LLVolumeFace;

extern U32 gFrameCount;

class LLDrawPoolAvatar : public LLFacePool
{
public:
    enum
    {
        SHADER_LEVEL_BUMP = 2,
        SHADER_LEVEL_CLOTH = 3
    };

    enum
    {
        VERTEX_DATA_MASK =  LLVertexBuffer::MAP_VERTEX |
                            LLVertexBuffer::MAP_NORMAL |
                            LLVertexBuffer::MAP_TEXCOORD0 |
                            LLVertexBuffer::MAP_WEIGHT |
                            LLVertexBuffer::MAP_CLOTHWEIGHT
    };

    ~LLDrawPoolAvatar();
    /*virtual*/ bool isDead() override;

typedef enum
    {
        SHADOW_PASS_AVATAR_OPAQUE,
        SHADOW_PASS_AVATAR_ALPHA_BLEND,
        SHADOW_PASS_AVATAR_ALPHA_MASK,
        NUM_SHADOW_PASSES
    } eShadowPass;

    U32 getVertexDataMask() override;

    virtual S32 getShaderLevel() const override;

    LLDrawPoolAvatar(U32 type);

    static LLMatrix4& getModelView();

    /*virtual*/ S32  getNumPasses() override;
    /*virtual*/ void beginRenderPass(S32 pass) override;
    /*virtual*/ void endRenderPass(S32 pass) override;
    /*virtual*/ void prerender() override;
    /*virtual*/ void render(S32 pass = 0) override;

    /*virtual*/ S32 getNumDeferredPasses() override;
    /*virtual*/ void beginDeferredPass(S32 pass) override;
    /*virtual*/ void endDeferredPass(S32 pass) override;
    /*virtual*/ void renderDeferred(S32 pass) override;

    /*virtual*/ S32 getNumPostDeferredPasses() override;
    /*virtual*/ void beginPostDeferredPass(S32 pass) override;
    /*virtual*/ void endPostDeferredPass(S32 pass) override;
    /*virtual*/ void renderPostDeferred(S32 pass) override;

    /*virtual*/ S32 getNumShadowPasses() override;
    /*virtual*/ void beginShadowPass(S32 pass) override;
    /*virtual*/ void endShadowPass(S32 pass) override;
    /*virtual*/ void renderShadow(S32 pass) override;

    void beginRigid();
    void beginImpostor();
    void beginSkinned();

    void endRigid();
    void endImpostor();
    void endSkinned();

    void beginDeferredRigid();
    void beginDeferredImpostor();
    void beginDeferredSkinned();

    void endDeferredRigid();
    void endDeferredImpostor();
    void endDeferredSkinned();

    /*virtual*/ LLViewerTexture *getDebugTexture() override;
    /*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

    void renderAvatars(LLVOAvatar *single_avatar, S32 pass = -1); // renders only one avatar if single_avatar is not null.

    static bool sSkipOpaque;
    static bool sSkipTransparent;
    static S32  sShadowPass;
    static S32 sDiffuseChannel;
    static F32 sMinimumAlpha;

    static LLGLSLShader* sVertexProgram;
};

extern S32 AVATAR_OFFSET_POS;
extern S32 AVATAR_OFFSET_NORMAL;
extern S32 AVATAR_OFFSET_TEX0;
extern S32 AVATAR_OFFSET_TEX1;
extern S32 AVATAR_VERTEX_BYTES;
const S32 AVATAR_BUFFER_ELEMENTS = 8192; // Needs to be enough to store all avatar vertices.

extern bool gAvatarEmbossBumpMap;
#endif // LL_LLDRAWPOOLAVATAR_H
