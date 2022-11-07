/** 
 * @file lllegacyatmospherics.h
 * @brief LLVOSky class header file
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

#ifndef LL_LLLEGACYATMOSPHERICS_H
#define LL_LLLEGACYATMOSPHERICS_H

#include "stdtypes.h"
#include "v3color.h"
#include "v4coloru.h"
#include "llviewertexture.h"
#include "llviewerobject.h"
#include "llframetimer.h"
#include "v3colorutil.h"
#include "llsettingssky.h"

//////////////////////////////////
//
// Lots of constants
//
// Will clean these up at some point...
//

const F32 HORIZON_DIST          = 1024.0f;
const F32 ATM_EXP_FALLOFF       = 0.000126f;
const F32 ATM_SEA_LEVEL_NDENS   = 2.55e25f;
const F32 ATM_HEIGHT            = 100000.f;

// constants used in calculation of scattering coeff of clear air
const F32 sigma     = 0.035f;
const F32 fsigma    = (6.f + 3.f * sigma) / (6.f-7.f*sigma);
const F64 Ndens     = 2.55e25;
const F64 Ndens2    = Ndens*Ndens;

class LLFace;
class LLHaze;

LL_FORCE_INLINE LLColor3 refr_ind_calc(const LLColor3 &wave_length)
{
    LLColor3 refr_ind;
    for (S32 i = 0; i < 3; ++i)
    {
        const F32 wl2 = wave_length.mV[i] * wave_length.mV[i] * 1e-6f;
        refr_ind.mV[i] = 6.43e3f + ( 2.95e6f / ( 146.0f - 1.f/wl2 ) ) + ( 2.55e4f / ( 41.0f - 1.f/wl2 ) );
        refr_ind.mV[i] *= 1.0e-8f;
        refr_ind.mV[i] += 1.f;
    }
    return refr_ind;
}


class LLHaze
{
public:
    LLHaze() : mG(0), mFalloff(1), mAbsCoef(0.f) {mSigSca.setToBlack();}
    LLHaze(const F32 g, const LLColor3& sca, const F32 fo = 2.f) : 
            mG(g), mSigSca(0.25f/F_PI * sca), mFalloff(fo), mAbsCoef(0.f)
    {
        mAbsCoef = color_intens(mSigSca) / sAirScaIntense;
    }

    LLHaze(const F32 g, const F32 sca, const F32 fo = 2.f) : mG(g),
            mSigSca(0.25f/F_PI * LLColor3(sca, sca, sca)), mFalloff(fo)
    {
        mAbsCoef = 0.01f * sca / sAirScaAvg;
    }

/* Proportion of light that is scattered into 'path' from 'in' over distance dt. */
/* assumes that vectors 'path' and 'in' are normalized. Scattering coef / 2pi */

    LL_FORCE_INLINE LLColor3 calcAirSca(const F32 h)
    {
       return calcFalloff(h) * sAirScaSeaLevel;
    }

    LL_FORCE_INLINE void calcAirSca(const F32 h, LLColor3 &result)
    {
        result = sAirScaSeaLevel;
        result *= calcFalloff(h);
    }

    F32 getG() const                { return mG; }

    void setG(const F32 g)
    {
        mG = g;
    }

    const LLColor3& getSigSca() const // sea level
    {
        return mSigSca;
    } 

    void setSigSca(const LLColor3& s)
    {
        mSigSca = s;
        mAbsCoef = 0.01f * color_intens(mSigSca) / sAirScaIntense;
    }

    void setSigSca(const F32 s0, const F32 s1, const F32 s2)
    {
        mSigSca = sAirScaAvg * LLColor3 (s0, s1, s2);
        mAbsCoef = 0.01f * (s0 + s1 + s2) / 3;
    }

    F32 getFalloff() const
    {
        return mFalloff;
    }

    void setFalloff(const F32 fo)
    {
        mFalloff = fo;
    }

    F32 getAbsCoef() const
    {
        return mAbsCoef;
    }

    inline static F32 calcFalloff(const F32 h)
    {
        return (h <= 0) ? 1.0f : (F32)LL_FAST_EXP(-ATM_EXP_FALLOFF * h);
    }

    inline LLColor3 calcSigSca(const F32 h) const
    {
        return calcFalloff(h * mFalloff) * mSigSca;
    }

    inline void calcSigSca(const F32 h, LLColor3 &result) const
    {
        result = mSigSca;
        result *= calcFalloff(h * mFalloff);
    }

    LLColor3 calcSigExt(const F32 h) const
    {
        return calcFalloff(h * mFalloff) * (1 + mAbsCoef) * mSigSca;
    }

    F32 calcPhase(const F32 cos_theta) const;

private:
    static LLColor3 const sAirScaSeaLevel;
    static F32 const sAirScaIntense;
    static F32 const sAirScaAvg;

protected:
    F32         mG;
    LLColor3    mSigSca;
    F32         mFalloff;   // 1 - slow, >1 - faster
    F32         mAbsCoef;
};


class LLCubeMap;

class AtmosphericsVars
{
public:
    AtmosphericsVars()
    : hazeColor(0,0,0)
    , hazeColorBelowCloud(0,0,0)
    , cloudColorSun(0,0,0)
    , cloudColorAmbient(0,0,0)
    , cloudDensity(0.0f)
    , blue_density()
    , blue_horizon()
    , haze_density(0.0f)
    , haze_horizon(0.0f)
    , density_multiplier(0.0f)
    , max_y(0.0f)
    , gamma(1.0f)
    , sun_norm(0.0f, 1.0f, 0.0f, 1.0f)
    , sunlight()
    , ambient()
    , glow()
    , cloud_shadow(1.0f)
    , dome_radius(1.0f)
    , dome_offset(1.0f)
    , light_atten()
    , light_transmittance()
    {
    }

    friend bool operator==(const AtmosphericsVars& a, const AtmosphericsVars& b);
    // returns true if values are within treshold of each other.
    friend bool approximatelyEqual(const AtmosphericsVars& a, const AtmosphericsVars& b, const F32 fraction_treshold);

    LLColor3  hazeColor;
    LLColor3  hazeColorBelowCloud;
    LLColor3  cloudColorSun;
    LLColor3  cloudColorAmbient;
    F32       cloudDensity;
    LLColor3  blue_density;
    LLColor3  blue_horizon;
    F32       haze_density;
    F32       haze_horizon;
    F32       density_multiplier;
    F32       distance_multiplier;
    F32       max_y;
    F32       gamma;
    LLVector4 sun_norm;
    LLColor3  sunlight;
    LLColor3  ambient;
    LLColor3  glow;
    F32       cloud_shadow;
    F32       dome_radius;
    F32       dome_offset;
    LLColor3 light_atten;
    LLColor3 light_transmittance;
    LLColor3 total_density;
};

class LLAtmospherics
{
public:    
    LLAtmospherics();
    ~LLAtmospherics();

    void init();
    void updateFog(const F32 distance, const LLVector3& tosun);

    const LLHaze& getHaze() const                    { return mHaze; }
    LLHaze& getHaze()                                { return mHaze; }
    F32 getHazeConcentration() const                 { return mHazeConcentration; }
    void setHaze(const LLHaze& h)                    { mHaze = h; }
    void setFogRatio(const F32 fog_ratio)            { mFogRatio = fog_ratio; }

    F32      getFogRatio() const                     { return mFogRatio; }
    LLColor4 getFogColor() const                     { return mFogColor; }
    LLColor4 getGLFogColor() const                   { return mGLFogCol; }

    void setCloudDensity(F32 cloud_density)          { mCloudDensity = cloud_density; }
    void setWind ( const LLVector3& wind )           { mWind = wind.length(); }

    LLColor4 calcSkyColorInDir(const LLSettingsSky::ptr_t &psky, AtmosphericsVars& vars, const LLVector3& dir, bool isShiny = false, const bool low_end = false);

protected:

    void     calcSkyColorWLVert(const LLSettingsSky::ptr_t &psky, LLVector3 & Pn, AtmosphericsVars& vars);

    LLHaze              mHaze;
    F32                 mHazeConcentration;
    F32                 mCloudDensity;
    F32                 mWind;
    BOOL                mInitialized;
    LLVector3           mLastLightingDirection;
    LLColor3            mLastTotalAmbient;
    F32                 mAmbientScale;
    LLColor3            mNightColorShift;
    F32                 mInterpVal;
    LLColor4            mFogColor;
    LLColor4            mGLFogCol;
    F32                 mFogRatio;
    F32                 mWorldScale;
};

#endif
