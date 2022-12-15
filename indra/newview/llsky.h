/** 
 * @file llsky.h
 * @brief It's, uh, the sky!
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLSKY_H
#define LL_LLSKY_H

#include "llmath.h"
//#include "vmath.h"
#include "v3math.h"
#include "v4math.h"
#include "v4color.h"
#include "v4coloru.h"
#include "llvosky.h"
#include "llvoground.h"

class LLViewerCamera;

class LLVOWLSky;


class LLSky  
{
public:
    LLSky();
    ~LLSky();

    void init();
    void cleanup();

    // These directions should be in CFR coord sys (+x at, +z up, +y right)
    void setSunAndMoonDirectionsCFR(const LLVector3 &sun_direction, const LLVector3 &moon_direction);
    void setSunDirectionCFR(const LLVector3 &sun_direction);
    void setMoonDirectionCFR(const LLVector3 &moon_direction);

    void setSunTextures(const LLUUID& sun_texture, const LLUUID& sun_texture_next);
    void setMoonTextures(const LLUUID& moon_texture, const LLUUID& moon_texture_next);
    void setCloudNoiseTextures(const LLUUID& cloud_noise_texture, const LLUUID& cloud_noise_texture_next);
    void setBloomTextures(const LLUUID& bloom_texture, const LLUUID& bloom_texture_next);

    void setSunScale(F32 sun_scale);
    void setMoonScale(F32 moon_scale);

    LLColor4 getSkyFogColor() const;

    void setCloudDensityAtAgent(F32 cloud_density);
    void setWind(const LLVector3& wind);

    void updateFog(const F32 distance);
    void updateCull();
    void updateSky();

    S32  mLightingGeneration;
    BOOL mUpdatedThisFrame;

    void setFogRatio(const F32 fog_ratio);      // Fog distance as fraction of cull distance.
    F32 getFogRatio() const;
    LLColor4U getFadeColor() const;

    void destroyGL();
    void restoreGL();
    void resetVertexBuffers();

    void addSunMoonBeacons();
    void renderSunMoonBeacons(const LLVector3& pos_agent, const LLVector3& direction, LLColor4 color);

public:
    LLPointer<LLVOSky>      mVOSkyp;    // Pointer to the LLVOSky object (only one, ever!)
    LLPointer<LLVOGround>   mVOGroundp;
    LLPointer<LLVOWLSky>    mVOWLSkyp;

protected:
    LLColor4 mFogColor;
};

extern LLSky gSky;
#endif
