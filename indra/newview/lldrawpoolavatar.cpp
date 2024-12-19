/**
 * @file lldrawpoolavatar.cpp
 * @brief LLDrawPoolAvatar class implementation
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

#include "lldrawpoolavatar.h"
#include "llskinningutil.h"
#include "llrender.h"

#include "llvoavatar.h"
#include "m3math.h"
#include "llmatrix4a.h"

#include "llagent.h" //for gAgent.needsRenderAvatar()
#include "lldrawable.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llmeshrepository.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "noise.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llvovolume.h"
#include "llvolume.h"
#include "llappviewer.h"
#include "llrendersphere.h"
#include "llviewerpartsim.h"
#include "llviewercontrol.h" // for gSavedSettings
#include "llviewertexturelist.h"

static U32 sShaderLevel = 0;

LLGLSLShader* LLDrawPoolAvatar::sVertexProgram = NULL;
bool    LLDrawPoolAvatar::sSkipOpaque = false;
bool    LLDrawPoolAvatar::sSkipTransparent = false;
S32     LLDrawPoolAvatar::sShadowPass = -1;
S32 LLDrawPoolAvatar::sDiffuseChannel = 0;
F32 LLDrawPoolAvatar::sMinimumAlpha = 0.2f;

static bool is_deferred_render = false;
static bool is_post_deferred_render = false;

extern bool gUseGLPick;

F32 CLOTHING_GRAVITY_EFFECT = 0.7f;
F32 CLOTHING_ACCEL_FORCE_FACTOR = 0.2f;

// Format for gAGPVertices
// vertex format for bumpmapping:
//  vertices   12
//  pad         4
//  normals    12
//  pad         4
//  texcoords0  8
//  texcoords1  8
// total       48
//
// for no bumpmapping
//  vertices       12
//  texcoords   8
//  normals    12
// total       32
//

S32 AVATAR_OFFSET_POS = 0;
S32 AVATAR_OFFSET_NORMAL = 16;
S32 AVATAR_OFFSET_TEX0 = 32;
S32 AVATAR_OFFSET_TEX1 = 40;
S32 AVATAR_VERTEX_BYTES = 48;

bool gAvatarEmbossBumpMap = false;
static bool sRenderingSkinned = false;
S32 normal_channel = -1;
S32 specular_channel = -1;
S32 cube_channel = -1;

LLDrawPoolAvatar::LLDrawPoolAvatar(U32 type) :
    LLFacePool(type)
{
}

LLDrawPoolAvatar::~LLDrawPoolAvatar()
{
    if (!isDead())
    {
        LL_WARNS() << "Destroying avatar drawpool that still contains faces" << LL_ENDL;
    }
}

// virtual
bool LLDrawPoolAvatar::isDead()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (!LLFacePool::isDead())
    {
        return false;
    }

    return true;
}

S32 LLDrawPoolAvatar::getShaderLevel() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    return (S32) LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_AVATAR);
}

void LLDrawPoolAvatar::prerender()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_AVATAR);

    sShaderLevel = mShaderLevel;
}

LLMatrix4& LLDrawPoolAvatar::getModelView()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    static LLMatrix4 ret;

    ret.initRows(LLVector4(gGLModelView+0),
                 LLVector4(gGLModelView+4),
                 LLVector4(gGLModelView+8),
                 LLVector4(gGLModelView+12));

    return ret;
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------



void LLDrawPoolAvatar::beginDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sSkipTransparent = true;
    is_deferred_render = true;

    if (LLPipeline::sImpostorRender)
    { //impostor pass does not have impostor rendering
        ++pass;
    }

    switch (pass)
    {
    case 0:
        beginDeferredImpostor();
        break;
    case 1:
        beginDeferredRigid();
        break;
    case 2:
        beginDeferredSkinned();
        break;
    }
}

void LLDrawPoolAvatar::endDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sSkipTransparent = false;
    is_deferred_render = false;

    if (LLPipeline::sImpostorRender)
    {
        ++pass;
    }

    switch (pass)
    {
    case 0:
        endDeferredImpostor();
        break;
    case 1:
        endDeferredRigid();
        break;
    case 2:
        endDeferredSkinned();
        break;
    }
}

void LLDrawPoolAvatar::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    render(pass);
}

S32 LLDrawPoolAvatar::getNumPostDeferredPasses()
{
    return 1;
}

void LLDrawPoolAvatar::beginPostDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sSkipOpaque = true;
    sShaderLevel = mShaderLevel;
    sVertexProgram = &gDeferredAvatarAlphaProgram;
    sRenderingSkinned = true;

    gPipeline.bindDeferredShader(*sVertexProgram);

    sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);

    sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
}

void LLDrawPoolAvatar::endPostDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    // if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
    sRenderingSkinned = false;
    sSkipOpaque = false;

    gPipeline.unbindDeferredShader(*sVertexProgram);
    sDiffuseChannel = 0;
    sShaderLevel = mShaderLevel;
}

void LLDrawPoolAvatar::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    is_post_deferred_render = true;
    if (LLPipeline::sImpostorRender)
    { //HACK for impostors so actual pass ends up being proper pass
        render(0);
    }
    else
    {
        render(2);
    }
    is_post_deferred_render = false;
}


S32 LLDrawPoolAvatar::getNumShadowPasses()
{
    // avatars opaque, avatar alpha, avatar alpha mask, alpha attachments, alpha mask attachments, opaque attachments...
    return NUM_SHADOW_PASSES;
}

void LLDrawPoolAvatar::beginShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (pass == SHADOW_PASS_AVATAR_OPAQUE)
    {
        sVertexProgram = &gDeferredAvatarShadowProgram;

        if ((sShaderLevel > 0))  // for hardware blending
        {
            sRenderingSkinned = true;
            sVertexProgram->bind();
        }

        gGL.diffuseColor4f(1, 1, 1, 1);
    }
    else if (pass == SHADOW_PASS_AVATAR_ALPHA_BLEND)
    {
        sVertexProgram = &gDeferredAvatarAlphaShadowProgram;

        // bind diffuse tex so we can reference the alpha channel...
        S32 loc = sVertexProgram->getUniformLocation(LLViewerShaderMgr::DIFFUSE_MAP);
        sDiffuseChannel = 0;
        if (loc != -1)
        {
            sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
        }

        if ((sShaderLevel > 0))  // for hardware blending
        {
            sRenderingSkinned = true;
            sVertexProgram->bind();
        }

        gGL.diffuseColor4f(1, 1, 1, 1);
    }
    else if (pass == SHADOW_PASS_AVATAR_ALPHA_MASK)
    {
        sVertexProgram = &gDeferredAvatarAlphaMaskShadowProgram;

        // bind diffuse tex so we can reference the alpha channel...
        S32 loc = sVertexProgram->getUniformLocation(LLViewerShaderMgr::DIFFUSE_MAP);
        sDiffuseChannel = 0;
        if (loc != -1)
        {
            sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
        }

        if ((sShaderLevel > 0))  // for hardware blending
        {
            sRenderingSkinned = true;
            sVertexProgram->bind();
        }

        gGL.diffuseColor4f(1, 1, 1, 1);
    }
}

void LLDrawPoolAvatar::endShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (sShaderLevel > 0)
    {
        sVertexProgram->unbind();
    }
    sVertexProgram = NULL;
    sRenderingSkinned = false;
    LLDrawPoolAvatar::sShadowPass = -1;
}

void LLDrawPoolAvatar::renderShadow(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (mDrawFace.empty())
    {
        return;
    }

    const LLFace *facep = mDrawFace[0];
    if (!facep->getDrawable())
    {
        return;
    }
    LLVOAvatar *avatarp = (LLVOAvatar *)facep->getDrawable()->getVObj().get();

    if (avatarp->isDead() || avatarp->isUIAvatar() || avatarp->mDrawable.isNull())
    {
        return;
    }

    static LLCachedControl<bool> friends_only(gSavedSettings, "RenderAvatarFriendsOnly", false);
    if (friends_only()
        && !avatarp->isControlAvatar()
        && !avatarp->isSelf()
        && !avatarp->isBuddy())
    {
        return;
    }

    LLVOAvatar::AvatarOverallAppearance oa = avatarp->getOverallAppearance();
    bool impostor = !LLPipeline::sImpostorRender && avatarp->isImpostor();
    // no shadows if the shadows are causing this avatar to breach the limit.
    if (avatarp->isTooSlow() || impostor || (oa == LLVOAvatar::AOA_INVISIBLE))
    {
        // No shadows for impostored (including jellydolled) or invisible avs.
        return;
    }

    LLDrawPoolAvatar::sShadowPass = pass;

    if (pass == SHADOW_PASS_AVATAR_OPAQUE)
    {
        LLDrawPoolAvatar::sSkipTransparent = true;
        avatarp->renderSkinned();
        LLDrawPoolAvatar::sSkipTransparent = false;
    }
    else if (pass == SHADOW_PASS_AVATAR_ALPHA_BLEND)
    {
        LLDrawPoolAvatar::sSkipOpaque = true;
        avatarp->renderSkinned();
        LLDrawPoolAvatar::sSkipOpaque = false;
    }
    else if (pass == SHADOW_PASS_AVATAR_ALPHA_MASK)
    {
        LLDrawPoolAvatar::sSkipOpaque = true;
        avatarp->renderSkinned();
        LLDrawPoolAvatar::sSkipOpaque = false;
    }
}

S32 LLDrawPoolAvatar::getNumPasses()
{
    return 3;
}


S32 LLDrawPoolAvatar::getNumDeferredPasses()
{
    return 3;
}


void LLDrawPoolAvatar::render(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    if (LLPipeline::sImpostorRender)
    {
        renderAvatars(NULL, ++pass);
        return;
    }

    renderAvatars(NULL, pass); // render all avatars
}

void LLDrawPoolAvatar::beginRenderPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    //reset vertex buffer mappings
    LLVertexBuffer::unbind();

    if (LLPipeline::sImpostorRender)
    { //impostor render does not have impostors or rigid rendering
        ++pass;
    }

    switch (pass)
    {
    case 0:
        beginImpostor();
        break;
    case 1:
        beginRigid();
        break;
    case 2:
        beginSkinned();
        break;
    }

    if (pass == 0)
    { //make sure no stale colors are left over from a previous render
        gGL.diffuseColor4f(1,1,1,1);
    }
}

void LLDrawPoolAvatar::endRenderPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (LLPipeline::sImpostorRender)
    {
        ++pass;
    }

    switch (pass)
    {
    case 0:
        endImpostor();
        break;
    case 1:
        endRigid();
        break;
    case 2:
        endSkinned();
        break;
    }
}

void LLDrawPoolAvatar::beginImpostor()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (!LLPipeline::sReflectionRender)
    {
        LLVOAvatar::sRenderDistance = llclamp(LLVOAvatar::sRenderDistance, 16.f, 256.f);
        LLVOAvatar::sNumVisibleAvatars = 0;
    }

        gImpostorProgram.bind();
        gImpostorProgram.setMinimumAlpha(0.01f);

    gPipeline.enableLightsFullbright();
    sDiffuseChannel = 0;
}

void LLDrawPoolAvatar::endImpostor()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

        gImpostorProgram.unbind();
    gPipeline.enableLightsDynamic();
}

void LLDrawPoolAvatar::beginRigid()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (gPipeline.shadersLoaded())
    {
        sVertexProgram = &gObjectAlphaMaskNoColorProgram;

        if (sVertexProgram != NULL)
        {   //eyeballs render with the specular shader
            sVertexProgram->bind();
            sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
        }
    }
    else
    {
        sVertexProgram = NULL;
    }
}

void LLDrawPoolAvatar::endRigid()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sShaderLevel = mShaderLevel;
    if (sVertexProgram != NULL)
    {
        sVertexProgram->unbind();
    }
}

void LLDrawPoolAvatar::beginDeferredImpostor()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (!LLPipeline::sReflectionRender)
    {
        LLVOAvatar::sRenderDistance = llclamp(LLVOAvatar::sRenderDistance, 16.f, 256.f);
        LLVOAvatar::sNumVisibleAvatars = 0;
    }

    sVertexProgram = &gDeferredImpostorProgram;
    specular_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::SPECULAR_MAP);
    normal_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::NORMAL_MAP);
    sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
    sVertexProgram->bind();
    sVertexProgram->setMinimumAlpha(0.01f);
}

void LLDrawPoolAvatar::endDeferredImpostor()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sShaderLevel = mShaderLevel;
    sVertexProgram->disableTexture(LLViewerShaderMgr::NORMAL_MAP);
    sVertexProgram->disableTexture(LLViewerShaderMgr::SPECULAR_MAP);
    sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
    gPipeline.unbindDeferredShader(*sVertexProgram);
   sVertexProgram = NULL;
   sDiffuseChannel = 0;
}

void LLDrawPoolAvatar::beginDeferredRigid()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sVertexProgram = &gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
    sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
    sVertexProgram->bind();
    sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
}

void LLDrawPoolAvatar::endDeferredRigid()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sShaderLevel = mShaderLevel;
    sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
    sVertexProgram->unbind();
    gGL.getTexUnit(0)->activate();
}


void LLDrawPoolAvatar::beginSkinned()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    // used for preview only

    sVertexProgram = &gAvatarProgram;

    sRenderingSkinned = true;

    sVertexProgram->bind();
    sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
}

void LLDrawPoolAvatar::endSkinned()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    // if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
    if (sShaderLevel > 0)
    {
        sRenderingSkinned = false;
        sVertexProgram->disableTexture(LLViewerShaderMgr::BUMP_MAP);
        gGL.getTexUnit(0)->activate();
        sVertexProgram->unbind();
        sShaderLevel = mShaderLevel;
    }
    else
    {
        if(gPipeline.shadersLoaded())
        {
            // software skinning, use a basic shader for windlight.
            // TODO: find a better fallback method for software skinning.
            sVertexProgram->unbind();
        }
    }

    gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::beginDeferredSkinned()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    sShaderLevel = mShaderLevel;
    sVertexProgram = &gDeferredAvatarProgram;
    sRenderingSkinned = true;

    sVertexProgram->bind();
    sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
    sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
    gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::endDeferredSkinned()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    // if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
    sRenderingSkinned = false;
    sVertexProgram->unbind();

    sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);

    sShaderLevel = mShaderLevel;

    gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::renderAvatars(LLVOAvatar* single_avatar, S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR; //LL_RECORD_BLOCK_TIME(FTM_RENDER_CHARACTERS);

    if (pass == -1)
    {
        for (S32 i = 1; i < getNumPasses(); i++)
        { //skip impostor pass
            prerender();
            beginRenderPass(i);
            renderAvatars(single_avatar, i);
            endRenderPass(i);
        }

        return;
    }

    if (mDrawFace.empty() && !single_avatar)
    {
        return;
    }

    LLVOAvatar *avatarp = NULL;

    if (single_avatar)
    {
        avatarp = single_avatar;
    }
    else
    {
        const LLFace *facep = mDrawFace[0];
        if (!facep->getDrawable())
        {
            return;
        }
        avatarp = (LLVOAvatar *)facep->getDrawable()->getVObj().get();
    }

    if (avatarp->isDead() || avatarp->mDrawable.isNull())
    {
        return;
    }

    if (!single_avatar && !avatarp->isFullyLoaded() )
    {
        if (pass==0 && (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) || LLViewerPartSim::getMaxPartCount() <= 0))
        {
            // debug code to draw a sphere in place of avatar
            gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
            gGL.setColorMask(true, true);
            LLVector3 pos = avatarp->getPositionAgent();
            gGL.color4f(1.0f, 1.0f, 1.0f, 0.7f);

            gGL.pushMatrix();
            gGL.translatef((F32)(pos.mV[VX]),
                           (F32)(pos.mV[VY]),
                            (F32)(pos.mV[VZ]));
             gGL.scalef(0.15f, 0.15f, 0.3f);

             gSphere.renderGGL();

             gGL.popMatrix();
             gGL.setColorMask(true, false);
        }
        // don't render please
        return;
    }

    static LLCachedControl<bool> friends_only(gSavedSettings, "RenderAvatarFriendsOnly", false);
    if (!single_avatar
        && friends_only()
        && !avatarp->isUIAvatar()
        && !avatarp->isControlAvatar()
        && !avatarp->isSelf()
        && !avatarp->isBuddy())
    {
        return;
    }

    bool impostor = !LLPipeline::sImpostorRender && avatarp->isImpostor() && !single_avatar;

    if (( avatarp->isInMuteList()
          || impostor
          || (LLVOAvatar::AOA_NORMAL != avatarp->getOverallAppearance() && !avatarp->needsImpostorUpdate()) ) && pass != 0)
//        || (LLVOAvatar::AV_DO_NOT_RENDER == avatarp->getVisualMuteSettings() && !avatarp->needsImpostorUpdate()) ) && pass != 0)
    { //don't draw anything but the impostor for impostored avatars
        return;
    }

    if (pass == 0 && !impostor && LLPipeline::sUnderWaterRender)
    { //don't draw foot shadows under water
        return;
    }

    LLVOAvatar *attached_av = avatarp->getAttachedAvatar();
    if (attached_av && (LLVOAvatar::AOA_NORMAL != attached_av->getOverallAppearance() || !gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_AVATAR)))
    {
        // Animesh attachment of a jellydolled or invisible parent - don't show
        return;
    }

    if (pass == 0)
    {
        if (!LLPipeline::sReflectionRender)
        {
            LLVOAvatar::sNumVisibleAvatars++;
        }

//      if (impostor || (LLVOAvatar::AV_DO_NOT_RENDER == avatarp->getVisualMuteSettings() && !avatarp->needsImpostorUpdate()))
        if (impostor || (LLVOAvatar::AOA_NORMAL != avatarp->getOverallAppearance() && !avatarp->needsImpostorUpdate()))
        {
            if (LLPipeline::sRenderDeferred && !LLPipeline::sReflectionRender && avatarp->mImpostor.isComplete())
            {
                if (normal_channel > -1)
                {
                    avatarp->mImpostor.bindTexture(2, normal_channel);
                }
                if (specular_channel > -1)
                {
                    avatarp->mImpostor.bindTexture(1, specular_channel);
                }
            }
            avatarp->renderImpostor(avatarp->getMutedAVColor(), sDiffuseChannel);
        }
        return;
    }

    if (pass == 1)
    {
        // render rigid meshes (eyeballs) first
        avatarp->renderRigid();
        return;
    }

    if ((sShaderLevel >= SHADER_LEVEL_CLOTH))
    {
        LLMatrix4 rot_mat;
        LLViewerCamera::getInstance()->getMatrixToLocal(rot_mat);
        LLMatrix4 cfr(OGL_TO_CFR_ROTATION);
        rot_mat *= cfr;

        LLVector4 wind;
        wind.setVec(avatarp->mWindVec);
        wind.mV[VW] = 0;
        wind = wind * rot_mat;
        wind.mV[VW] = avatarp->mWindVec.mV[VW];

        sVertexProgram->uniform4fv(LLViewerShaderMgr::AVATAR_WIND, 1, wind.mV);
        F32 phase = -1.f * (avatarp->mRipplePhase);

        F32 freq = 7.f + (noise1(avatarp->mRipplePhase) * 2.f);
        LLVector4 sin_params(freq, freq, freq, phase);
        sVertexProgram->uniform4fv(LLViewerShaderMgr::AVATAR_SINWAVE, 1, sin_params.mV);

        LLVector4 gravity(0.f, 0.f, -CLOTHING_GRAVITY_EFFECT, 0.f);
        gravity = gravity * rot_mat;
        sVertexProgram->uniform4fv(LLViewerShaderMgr::AVATAR_GRAVITY, 1, gravity.mV);
    }

    if( !single_avatar || (avatarp == single_avatar) )
    {
        avatarp->renderSkinned();
    }
}

static LLTrace::BlockTimerStatHandle FTM_RIGGED_VBO("Rigged VBO");

//-----------------------------------------------------------------------------
// getDebugTexture()
//-----------------------------------------------------------------------------
LLViewerTexture *LLDrawPoolAvatar::getDebugTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (mReferences.empty())
    {
        return NULL;
    }
    LLFace *face = mReferences[0];
    if (!face->getDrawable())
    {
        return NULL;
    }
    const LLViewerObject *objectp = face->getDrawable()->getVObj();

    // Avatar should always have at least 1 (maybe 3?) TE's.
    return objectp->getTEImage(0);
}


LLColor3 LLDrawPoolAvatar::getDebugColor() const
{
    return LLColor3(0.f, 1.f, 0.f);
}


