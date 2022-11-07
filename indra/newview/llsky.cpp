/** 
 * @file llsky.cpp
 * @brief IndraWorld sky class 
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

//  Ideas:
//      -haze should be controlled by global query from sims
//      -need secondary optical effects on sun (flare)
//      -stars should be brought down from sims
//      -star intensity should be driven by global ambient level from sims,
//       so that eclipses, etc can be easily done.
//

#include "llviewerprecompiledheaders.h"

#include "llsky.h"

// linden library includes
#include "llerror.h"
#include "llmath.h"
#include "math.h"
#include "v4color.h"

#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llviewercamera.h"
#include "pipeline.h"
#include "lldrawpool.h"

#include "llvosky.h"
#include "llcubemap.h"
#include "llviewercontrol.h"
#include "llenvironment.h"
#include "llvoavatarself.h"
#include "llvowlsky.h"

F32 azimuth_from_vector(const LLVector3 &v);
F32 elevation_from_vector(const LLVector3 &v);

LLSky gSky;

// ---------------- LLSky ----------------
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LLSky::LLSky()
{
    // Set initial clear color to black
    // Set fog color 
    mFogColor.mV[VRED] = mFogColor.mV[VGREEN] = mFogColor.mV[VBLUE] = 0.5f;
    mFogColor.mV[VALPHA] = 0.0f;

    mLightingGeneration = 0;
    mUpdatedThisFrame = TRUE;
}


LLSky::~LLSky()
{
}

void LLSky::cleanup()
{
    mVOSkyp = NULL;
    mVOWLSkyp = NULL;
    mVOGroundp = NULL;
}

void LLSky::destroyGL()
{
    if (!mVOSkyp.isNull() && mVOSkyp->getCubeMap())
    {
        mVOSkyp->cleanupGL();
    }
    if (mVOWLSkyp.notNull())
    {
        mVOWLSkyp->cleanupGL();
    }
}

void LLSky::restoreGL()
{
    if (mVOSkyp)
    {
        mVOSkyp->restoreGL();
    }
    if (mVOWLSkyp)
    {
        mVOWLSkyp->restoreGL();
    }
}

void LLSky::resetVertexBuffers()
{
    if (gSky.mVOSkyp.notNull() && gSky.mVOGroundp.notNull())
    {
        gPipeline.resetVertexBuffers(gSky.mVOSkyp->mDrawable);
        gPipeline.resetVertexBuffers(gSky.mVOGroundp->mDrawable);
        gPipeline.markRebuild(gSky.mVOSkyp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
        gPipeline.markRebuild(gSky.mVOGroundp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
    }
    if (gSky.mVOWLSkyp.notNull())
    {
        gSky.mVOWLSkyp->resetVertexBuffers();
        gPipeline.resetVertexBuffers(gSky.mVOWLSkyp->mDrawable);
        gPipeline.markRebuild(gSky.mVOWLSkyp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
    }
}

void LLSky::setSunScale(F32 sun_scale)
{
    if(mVOSkyp.notNull())
    {
        mVOSkyp->setSunScale(sun_scale);
    }
}

void LLSky::setMoonScale(F32 moon_scale)
{
    if(mVOSkyp.notNull())
    {
        mVOSkyp->setMoonScale(moon_scale);
    }
}

void LLSky::setSunTextures(const LLUUID& sun_texture, const LLUUID& sun_texture_next)
{
    if(mVOSkyp.notNull()) {
        mVOSkyp->setSunTextures(sun_texture, sun_texture_next);
    }
}

void LLSky::setMoonTextures(const LLUUID& moon_texture, const LLUUID& moon_texture_next)
{
    if(mVOSkyp.notNull()) {
        mVOSkyp->setMoonTextures(moon_texture, moon_texture_next);
    }
}

void LLSky::setCloudNoiseTextures(const LLUUID& cloud_noise_texture, const LLUUID& cloud_noise_texture_next)
{
    if(mVOSkyp.notNull()) {
        mVOSkyp->setCloudNoiseTextures(cloud_noise_texture, cloud_noise_texture_next);
    }
}

void LLSky::setBloomTextures(const LLUUID& bloom_texture, const LLUUID& bloom_texture_next)
{
    if(mVOSkyp.notNull()) {
        mVOSkyp->setBloomTextures(bloom_texture, bloom_texture_next);
    }
}

void LLSky::setSunAndMoonDirectionsCFR(const LLVector3 &sun_direction, const LLVector3 &moon_direction)
{
    if(mVOSkyp.notNull()) {
        mVOSkyp->setSunAndMoonDirectionsCFR(sun_direction, moon_direction);
    }
}

void LLSky::setSunDirectionCFR(const LLVector3 &sun_direction)
{
    if(mVOSkyp.notNull()) {
        mVOSkyp->setSunDirectionCFR(sun_direction);
    }
}

void LLSky::setMoonDirectionCFR(const LLVector3 &moon_direction)
{
    if(mVOSkyp.notNull()) {
        mVOSkyp->setMoonDirectionCFR(moon_direction);
    }
}

//////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////

void LLSky::init()
{
    mVOWLSkyp = static_cast<LLVOWLSky*>(gObjectList.createObjectViewer(LLViewerObject::LL_VO_WL_SKY, NULL));
    mVOWLSkyp->init();
    gPipeline.createObject(mVOWLSkyp.get());

    mVOSkyp = (LLVOSky *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_SKY, NULL);
    mVOSkyp->init();
    gPipeline.createObject(mVOSkyp.get());

    mVOGroundp = (LLVOGround*)gObjectList.createObjectViewer(LLViewerObject::LL_VO_GROUND, NULL);
    gPipeline.createObject(mVOGroundp.get());

    gSky.setFogRatio(gSavedSettings.getF32("RenderFogRatio"));

    mUpdatedThisFrame = TRUE;
}


void LLSky::setCloudDensityAtAgent(F32 cloud_density)
{
    if (mVOSkyp)
    {
        mVOSkyp->setCloudDensity(cloud_density);
    }
}


void LLSky::setWind(const LLVector3& average_wind)
{
    if (mVOSkyp)
    {
        mVOSkyp->setWind(average_wind);
    }
}

void LLSky::addSunMoonBeacons()
{   
    if (!gAgentAvatarp || !mVOSkyp) return;

    static LLUICachedControl<bool> show_sun_beacon("sunbeacon", false);
    static LLUICachedControl<bool> show_moon_beacon("moonbeacon", false);

    if (show_sun_beacon)
    {
        renderSunMoonBeacons(gAgentAvatarp->getPositionAgent(), mVOSkyp->getSun().getDirection(), LLColor4(1.f, 0.5f, 0.f, 0.5f));
    }
    if (show_moon_beacon)
    {
        renderSunMoonBeacons(gAgentAvatarp->getPositionAgent(), mVOSkyp->getMoon().getDirection(), LLColor4(1.f, 0.f, 0.8f, 0.5f));
    }
}

//////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////


LLColor4 LLSky::getSkyFogColor() const
{
    if (mVOSkyp)
    {
        return mVOSkyp->getSkyFogColor();
    }

    return LLColor4(1.f, 1.f, 1.f, 1.f);
}

void LLSky::updateFog(const F32 distance)
{
    if (mVOSkyp)
    {
        mVOSkyp->updateFog(distance);
    }
}

void LLSky::updateCull()
{
    // *TODO: do culling for wl sky properly -Brad
}

void LLSky::updateSky()
{
    if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
    {
        return;
    }
    if (mVOSkyp)
    {
        mVOSkyp->updateSky();
    }
}


void LLSky::setFogRatio(const F32 fog_ratio)
{
    if (mVOSkyp)
    {
        mVOSkyp->setFogRatio(fog_ratio);
    }
}


F32 LLSky::getFogRatio() const
{
    if (mVOSkyp)
    {
        return mVOSkyp->getFogRatio();
    }
    else
    {
        return 0.f;
    }
}


// Returns angle (DEGREES) between the horizontal plane and "v", 
// where the angle is negative when v.mV[VZ] < 0.0f
F32 elevation_from_vector(const LLVector3 &v)
{
    F32 elevation = 0.0f;
    F32 xy_component = (F32) sqrt(v.mV[VX] * v.mV[VX] + v.mV[VY] * v.mV[VY]);
    if (xy_component != 0.0f)
    {
        elevation = RAD_TO_DEG * (F32) atan(v.mV[VZ]/xy_component);
    }
    else
    {
        if (v.mV[VZ] > 0.f)
        {
            elevation = 90.f;
        }
        else
        {
            elevation = -90.f;
        }
    }
    return elevation;
}
