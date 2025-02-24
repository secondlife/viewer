/**
 * @file lldrawpoolwater.cpp
 * @brief LLDrawPoolWater class implementation
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
#include "llfeaturemanager.h"
#include "lldrawpoolwater.h"

#include "llviewercontrol.h"
#include "lldir.h"
#include "llerror.h"
#include "m3math.h"
#include "llrender.h"

#include "llagent.h"        // for gAgent for getRegion for getWaterHeight
#include "llcubemap.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llvowater.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llenvironment.h"
#include "llsettingssky.h"
#include "llsettingswater.h"

bool LLDrawPoolWater::sSkipScreenCopy = false;
bool LLDrawPoolWater::sNeedsReflectionUpdate = true;
bool LLDrawPoolWater::sNeedsDistortionUpdate = true;
F32 LLDrawPoolWater::sWaterFogEnd = 0.f;

extern bool gCubeSnapshot;

LLDrawPoolWater::LLDrawPoolWater() : LLFacePool(POOL_WATER)
{
}

LLDrawPoolWater::~LLDrawPoolWater()
{
}

void LLDrawPoolWater::setTransparentTextures(const LLUUID& transparentTextureId, const LLUUID& nextTransparentTextureId)
{
    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    mWaterImagep[0] = LLViewerTextureManager::getFetchedTexture(!transparentTextureId.isNull() ? transparentTextureId : pwater->GetDefaultTransparentTextureAssetId());
    mWaterImagep[1] = LLViewerTextureManager::getFetchedTexture(!nextTransparentTextureId.isNull() ? nextTransparentTextureId : (!transparentTextureId.isNull() ? transparentTextureId : pwater->GetDefaultTransparentTextureAssetId()));
    mWaterImagep[0]->addTextureStats(1024.f*1024.f);
    mWaterImagep[1]->addTextureStats(1024.f*1024.f);
}

void LLDrawPoolWater::setOpaqueTexture(const LLUUID& opaqueTextureId)
{
    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    mOpaqueWaterImagep = LLViewerTextureManager::getFetchedTexture(opaqueTextureId);
    mOpaqueWaterImagep->addTextureStats(1024.f*1024.f);
}

void LLDrawPoolWater::setNormalMaps(const LLUUID& normalMapId, const LLUUID& nextNormalMapId)
{
    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    mWaterNormp[0] = LLViewerTextureManager::getFetchedTexture(!normalMapId.isNull() ? normalMapId : pwater->GetDefaultWaterNormalAssetId());
    mWaterNormp[1] = LLViewerTextureManager::getFetchedTexture(!nextNormalMapId.isNull() ? nextNormalMapId : (!normalMapId.isNull() ? normalMapId : pwater->GetDefaultWaterNormalAssetId()));
    mWaterNormp[0]->addTextureStats(1024.f*1024.f);
    mWaterNormp[1]->addTextureStats(1024.f*1024.f);
}

void LLDrawPoolWater::prerender()
{
    mShaderLevel = LLCubeMap::sUseCubeMaps ? LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_WATER) : 0;
}

S32 LLDrawPoolWater::getNumPostDeferredPasses()
{
    if (LLViewerCamera::getInstance()->getOrigin().mV[2] < 1024.f)
    {
        return 1;
    }

    return 0;
}

void LLDrawPoolWater::beginPostDeferredPass(S32 pass)
{
    LL_PROFILE_GPU_ZONE("water beginPostDeferredPass")
    gGL.setColorMask(true, true);

    if (LLPipeline::sRenderTransparentWater)
    {
        // copy framebuffer contents so far to a texture to be used for
        // reflections and refractions
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

        LLRenderTarget& src = gPipeline.mRT->screen;
        LLRenderTarget& depth_src = gPipeline.mRT->deferredScreen;
        LLRenderTarget& dst = gPipeline.mWaterDis;

        dst.bindTarget();
        gCopyDepthProgram.bind();

        S32 diff_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
        S32 depth_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);

        gGL.getTexUnit(diff_map)->bind(&src);
        gGL.getTexUnit(depth_map)->bind(&depth_src, true);

        gPipeline.mScreenTriangleVB->setBuffer();
        gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        dst.flush();
    }
}

void LLDrawPoolWater::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLGLDisable blend(GL_BLEND);

    gGL.setColorMask(true, true);

    LLColor3 light_diffuse(0, 0, 0);
    F32      light_exp = 0.0f;

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsWater::ptr_t pwater = environment.getCurrentWater();
    LLSettingsSky::ptr_t   psky   = environment.getCurrentSky();
    LLVector3              light_dir       = environment.getLightDirection();
    bool                   sun_up          = environment.getIsSunUp();
    bool                   moon_up         = environment.getIsMoonUp();
    bool                   has_normal_mips = gSavedSettings.getBOOL("RenderWaterMipNormal");
    bool                   underwater      = LLViewerCamera::getInstance()->cameraUnderWater();
    LLColor4               fog_color       = LLColor4(pwater->getWaterFogColor(), 0.f);
    LLColor3               fog_color_linear = linearColor3(fog_color);

    if (sun_up)
    {
        light_diffuse += psky->getSunlightColor();
    }
    // moonlight is several orders of magnitude less bright than sunlight,
    // so only use this color when the moon alone is showing
    else if (moon_up)
    {
        light_diffuse += psky->getMoonlightColor();
    }

    // Apply magic numbers translating light direction into intensities
    light_dir.normalize();
    F32 ground_proj_sq = light_dir.mV[0] * light_dir.mV[0] + light_dir.mV[1] * light_dir.mV[1];
    light_exp          = llmax(32.f, 256.f * powf(ground_proj_sq, 16.0f));
    if (0.f < light_diffuse.normalize())  // Normalizing a color? Puzzling...
    {
        light_diffuse *= (1.5f + (6.f * ground_proj_sq));
    }

    LLTexUnit::eTextureFilterOptions filter_mode = has_normal_mips ? LLTexUnit::TFO_ANISOTROPIC : LLTexUnit::TFO_POINT;

    LLColor4      specular(sun_up ? psky->getSunlightColor() : psky->getMoonlightColor());
    F32           phase_time = (F32) LLFrameTimer::getElapsedSeconds() * 0.5f;
    LLGLSLShader *shader     = nullptr;

    // One pass, one of two shaders.  Void water and region water share state.
    // There isn't a good reason anymore to really have void water run in a separate pass.
    // It also just introduced a bunch of weird state consistency stuff that we really don't need.
    // Not to mention, re-binding the the same shader and state for that shader is kind of wasteful.
    // - Geenz 2025-02-11
    // select shader
    if (underwater)
    {
        shader = &gUnderWaterProgram;
    }
    else
    {
        shader = &gWaterProgram;
    }

    gPipeline.bindDeferredShader(*shader, nullptr, &gPipeline.mWaterDis);

    LLViewerTexture* tex_a = mWaterNormp[0];
    LLViewerTexture* tex_b = mWaterNormp[1];

    F32 blend_factor = (F32)pwater->getBlendFactor();

    if (tex_a && (!tex_b || (tex_a == tex_b)))
    {
        shader->bindTexture(LLViewerShaderMgr::BUMP_MAP, tex_a);
        tex_a->setFilteringOption(filter_mode);
        blend_factor = 0; // only one tex provided, no blending
    }
    else if (tex_b && !tex_a)
    {
        shader->bindTexture(LLViewerShaderMgr::BUMP_MAP, tex_b);
        tex_b->setFilteringOption(filter_mode);
        blend_factor = 0; // only one tex provided, no blending
    }
    else if (tex_b != tex_a)
    {
        shader->bindTexture(LLViewerShaderMgr::BUMP_MAP, tex_a);
        tex_a->setFilteringOption(filter_mode);
        shader->bindTexture(LLViewerShaderMgr::BUMP_MAP2, tex_b);
        tex_b->setFilteringOption(filter_mode);
    }

    shader->bindTexture(LLShaderMgr::WATER_EXCLUSIONTEX, &gPipeline.mWaterExclusionMask);

    shader->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);

    F32      fog_density = pwater->getModifiedWaterFogDensity(underwater);

    shader->bindTexture(LLShaderMgr::WATER_SCREENTEX, &gPipeline.mWaterDis);

    if (mShaderLevel == 1)
    {
        fog_color.mV[VALPHA] = (F32)(log(fog_density) / log(2));
    }

    F32 water_height = environment.getWaterHeight();
    F32 camera_height = LLViewerCamera::getInstance()->getOrigin().mV[2];
    shader->uniform1f(LLShaderMgr::WATER_WATERHEIGHT, camera_height - water_height);
    shader->uniform1f(LLShaderMgr::WATER_TIME, phase_time);
    shader->uniform3fv(LLShaderMgr::WATER_EYEVEC, 1, LLViewerCamera::getInstance()->getOrigin().mV);

    shader->uniform3fv(LLShaderMgr::WATER_SPECULAR, 1, light_diffuse.mV);

    shader->uniform2fv(LLShaderMgr::WATER_WAVE_DIR1, 1, pwater->getWave1Dir().mV);
    shader->uniform2fv(LLShaderMgr::WATER_WAVE_DIR2, 1, pwater->getWave2Dir().mV);

    shader->uniform3fv(LLShaderMgr::WATER_LIGHT_DIR, 1, light_dir.mV);

    shader->uniform3fv(LLShaderMgr::WATER_NORM_SCALE, 1, pwater->getNormalScale().mV);
    shader->uniform1f(LLShaderMgr::WATER_FRESNEL_SCALE, pwater->getFresnelScale());
    shader->uniform1f(LLShaderMgr::WATER_FRESNEL_OFFSET, pwater->getFresnelOffset());
    shader->uniform1f(LLShaderMgr::WATER_BLUR_MULTIPLIER, fmaxf(0, pwater->getBlurMultiplier()) * 2);

    static LLStaticHashedString s_exposure("exposure");
    static LLStaticHashedString tonemap_mix("tonemap_mix");
    static LLStaticHashedString tonemap_type("tonemap_type");

    static LLCachedControl<F32> exposure(gSavedSettings, "RenderExposure", 1.f);

    F32 e = llclamp(exposure(), 0.5f, 4.f);

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);

    shader->uniform1f(s_exposure, e);
    static LLCachedControl<U32> tonemap_type_setting(gSavedSettings, "RenderTonemapType", 0U);
    shader->uniform1i(tonemap_type, tonemap_type_setting);
    shader->uniform1f(tonemap_mix, psky->getTonemapMix(should_auto_adjust()));

    F32 sunAngle = llmax(0.f, light_dir.mV[1]);
    F32 scaledAngle = 1.f - sunAngle;

    shader->uniform1i(LLShaderMgr::SUN_UP_FACTOR, sun_up ? 1 : 0);

    // SL-15861 This was changed from getRotatedLightNorm() as it was causing
    // lightnorm in shaders\class1\windlight\atmosphericsFuncs.glsl in have inconsistent additive lighting for 180 degrees of the FOV.
    LLVector4 rotated_light_direction = LLEnvironment::instance().getClampedLightNorm();
    shader->uniform3fv(LLViewerShaderMgr::LIGHTNORM, 1, rotated_light_direction.mV);

    shader->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1, LLViewerCamera::getInstance()->getOrigin().mV);

    if (LLViewerCamera::getInstance()->cameraUnderWater())
    {
        shader->uniform1f(LLShaderMgr::WATER_REFSCALE, pwater->getScaleBelow());
    }
    else
    {
        shader->uniform1f(LLShaderMgr::WATER_REFSCALE, pwater->getScaleAbove());
    }

    LLGLDisable cullface(GL_CULL_FACE);

    // Only push the water planes once.
    // Previously we did this twice: once for void water and one for region water.
    // However, the void water and region water shaders are the same exact shader.
    // They also had the same exact state with the sole exception setting an edge water flag.
    // That flag was not actually used anywhere in the shaders.
    // - Geenz 2025-02-11
    pushWaterPlanes(0);

    // clean up
    gPipeline.unbindDeferredShader(*shader);

    gGL.setColorMask(true, false);
}

void LLDrawPoolWater::pushWaterPlanes(int pass)
{
    LLVOWater* water = nullptr;
    for (LLFace* const& face : mDrawFace)
    {
        water = static_cast<LLVOWater*>(face->getViewerObject());

        face->renderIndexed();

        // Note non-void water being drawn, updates required
        // Previously we had some logic to determine if this pass was also our water edge pass.
        // Now we only have one pass.  Check if we're doing a region water plane or void water plane.
        // - Geenz 2025-02-11
        if (!water->getIsEdgePatch())
        {
            sNeedsReflectionUpdate = true;
            sNeedsDistortionUpdate = true;
        }
    }
}

LLViewerTexture *LLDrawPoolWater::getDebugTexture()
{
    return LLViewerTextureManager::getFetchedTexture(IMG_SMOKE);
}

LLColor3 LLDrawPoolWater::getDebugColor() const
{
    return LLColor3(0.f, 1.f, 1.f);
}
