/**
 * @file llenvadapters.h
 * @brief Declaration of classes managing WindLight and water settings.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_ENVADAPTERS_H
#define LL_ENVADAPTERS_H

#include "v3math.h"
#include "v3color.h"
#include "v4math.h"
#include "llsettingsbase.h"
#include "llsettingssky.h"

class WLColorControl 
{
public:
    inline WLColorControl(LLColor4 color, const std::string& n, const std::string& slider_name = std::string()):
        mColor(color), 
        mName(n), 
        mSliderName(slider_name),
        mHasSliderName(false),
        mIsSunOrAmbientColor(false),
        mIsBlueHorizonOrDensity(false)
    {
        // if there's a slider name, say we have one
        mHasSliderName = !mSliderName.empty();

        // if it's the sun controller
        mIsSunOrAmbientColor = (mSliderName == "WLSunlight" || mSliderName == "WLAmbient");
        mIsBlueHorizonOrDensity = (mSliderName == "WLBlueHorizon" || mSliderName == "WLBlueDensity");
    }

    inline void setColor4(const LLColor4 & val)
    {
        mColor = val;
    }

    inline void setColor3(const LLColor3 & val)
    {
        mColor = val;
    }

    inline LLColor4 getColor4() const
    {
        return mColor;
    }

    inline LLColor3 getColor3(void) const 
    {
        return vec4to3(mColor);
    }

    inline void update(const LLSettingsBase::ptr_t &psetting) const 
    {
        psetting->setValue(mName, mColor);
    }

    inline bool getHasSliderName() const
    {
        return mHasSliderName;
    }

    inline std::string getSliderName() const
    {
        return mSliderName;
    }

    inline bool getIsSunOrAmbientColor() const
    {
        return mIsSunOrAmbientColor;
    }

    inline bool getIsBlueHorizonOrDensity() const
    {
        return mIsBlueHorizonOrDensity;
    }

    inline F32 getRed() const
    {
        return mColor[0];
    }

    inline F32 getGreen() const
    {
        return mColor[1];
    }

    inline F32 getBlue() const
    {
        return mColor[2];
    }

    inline F32 getIntensity() const
    {
        return mColor[3];
    }

    inline void setRed(F32 red)
    {
        mColor[0] = red;
    }

    inline void setGreen(F32 green)
    {
        mColor[1] = green;
    }

    inline void setBlue(F32 blue)
    {
        mColor[2] = blue;
    }

    inline void setIntensity(F32 intensity)
    {
        mColor[3] = intensity;
    }

private:
    LLColor4    mColor;         /// [3] is intensity, not alpha
    std::string mName;          /// name to use to dereference params
    std::string mSliderName;    /// name of the slider in menu
    bool        mHasSliderName;         /// only set slider name for true color types
    bool        mIsSunOrAmbientColor;           /// flag for if it's the sun or ambient color controller
    bool        mIsBlueHorizonOrDensity;        /// flag for if it's the Blue Horizon or Density color controller

};

// float slider control
class WLFloatControl 
{
public:
    inline WLFloatControl(F32 val, const std::string& n, F32 m = 1.0f):
        x(val), 
        mName(n), 
        mult(m)
    {
    }

    inline WLFloatControl &operator = (F32 val) 
    {
        x = val;
        return *this;
    }

    inline operator F32 (void) const 
    {
        return x;
    }

    inline void update(const LLSettingsBase::ptr_t &psetting) const 
    {
        psetting->setValue(mName, x);
    }

    inline F32 getMult() const
    {
        return mult;
    }

    inline void setValue(F32 val)
    {
        x = val;
    }

private:
    F32 x;
    std::string mName;
    F32 mult;
};

class WLXFloatControl
{
public:
    inline WLXFloatControl(F32 val, const std::string& n, F32 b): 
        mExp(val),
        mBase(b),
        mName(n)
    {
    }

    inline WLXFloatControl & operator = (F32 val)
    {
        mExp = log(val) / log(mBase);

        return *this;
    }

    inline operator F32 (void) const
    {
        return pow(mBase, mExp);
    }

    inline void update(const LLSettingsBase::ptr_t &psetting) const
    {
        psetting->setValue(mName, pow(mBase, mExp));
    }

    inline F32 getExp() const
    {
        return mExp;
    }

    inline void setExp(F32 val)
    {
        mExp = val;
    }

    inline F32 getBase() const
    {
        return mBase;
    }

    inline void setBase(F32 val)
    {
        mBase = val;
    }

private:
    F32 mExp;
    F32 mBase;
    std::string mName;
};

class WLVect2Control
{
public:
    inline WLVect2Control(LLVector2 val, const std::string& n): 
        mU(val.mV[0]), 
        mV(val.mV[1]), 
        mName(n)
    {
    }

    inline WLVect2Control & operator = (const LLVector2 & val)
    {
        mU = val.mV[0];
        mV = val.mV[1];

        return *this;
    }

    inline void update(const LLSettingsBase::ptr_t &psetting) const
    {
        psetting->setValue(mName, LLVector2(mU, mV));
    }

    inline F32 getU() const
    {
        return mU;
    }

    inline void setU(F32 val)
    {
        mU = val;
    }

    inline F32 getV() const
    {
        return mV;
    }

    inline void setV(F32 val)
    {
        mV = val;
    }

private:
    F32 mU;
    F32 mV;
    std::string mName;
};

class WLVect3Control
{
public:
    inline WLVect3Control(LLVector3 val, const std::string& n):
        mX(val.mV[0]),
        mY(val.mV[1]),
        mZ(val.mV[2]),
        mName(n)
    {
    }

    inline WLVect3Control & operator = (const LLVector3 & val)
    {
        mX = val.mV[0];
        mY = val.mV[1];
        mZ = val.mV[2];

        return *this;
    }

    inline void update(const LLSettingsBase::ptr_t &psetting) const
    {
        psetting->setValue(mName, LLVector3(mX, mY, mZ));
    }

    inline F32 getX() const
    {
        return mX;
    }

    inline void setX(F32 val)
    {
        mX = val;
    }

    inline F32 getY() const
    {
        return mY;
    }

    inline void setY(F32 val)
    {
        mY = val;
    }

    inline F32 getZ() const
    {
        return mZ;
    }

    inline void setZ(F32 val)
    {
        mZ = val;
    }

private:
    F32 mX;
    F32 mY;
    F32 mZ;
    std::string mName;
};

class LLDensityProfileSettingsAdapter
{
public:
    LLDensityProfileSettingsAdapter(const std::string& config, int layerIndex = 0)
    : mConfig(config)
    , mLayerIndex(layerIndex)
    , mLayerWidth(1.0f, LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH)
    , mExpTerm(1.0f, LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM)
    , mExpScale(1.0f, LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR)
    , mLinTerm(1.0f, LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM)
    , mConstantTerm(1.0f, LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM)
    {}

protected:
    std::string     mConfig;
    int             mLayerIndex;
    WLFloatControl  mLayerWidth; // 0.0 -> to top of atmosphere, however big that may be.
    WLFloatControl  mExpTerm;
    WLFloatControl  mExpScale;
    WLFloatControl  mLinTerm;
    WLFloatControl  mConstantTerm;
};

class LLRayleighDensityProfileSettingsAdapter : public LLDensityProfileSettingsAdapter
{
public:
    LLRayleighDensityProfileSettingsAdapter(int layerIndex = 0)
    : LLDensityProfileSettingsAdapter(LLSettingsSky::SETTING_RAYLEIGH_CONFIG, layerIndex)
    {
    }
};

class LLMieDensityProfileSettingsAdapter : public LLDensityProfileSettingsAdapter
{
public:
    LLMieDensityProfileSettingsAdapter(int layerIndex = 0)
    : LLDensityProfileSettingsAdapter(LLSettingsSky::SETTING_MIE_CONFIG, layerIndex)
    , mAnisotropy(0.8f, LLSettingsSky::SETTING_MIE_ANISOTROPY_FACTOR)
    {
    }

protected:
    WLFloatControl  mAnisotropy;
};

class LLAbsorptionDensityProfileSettingsAdapter : public LLDensityProfileSettingsAdapter
{
public:
    LLAbsorptionDensityProfileSettingsAdapter(int layerIndex = 0)
    : LLDensityProfileSettingsAdapter(LLSettingsSky::SETTING_ABSORPTION_CONFIG, layerIndex)
    {
    }
};

//-------------------------------------------------------------------------
class LLSkySettingsAdapter
{
public:
    typedef std::shared_ptr<LLSkySettingsAdapter> ptr_t;
    
    LLSkySettingsAdapter();

    WLFloatControl  mWLGamma;

    /// Lighting
    WLColorControl  mLightnorm;
    WLColorControl  mSunlight;    
    WLColorControl  mGlow;

    /// Clouds
    WLColorControl  mCloudColor;
    WLColorControl  mCloudMain;
    WLFloatControl  mCloudCoverage;
    WLColorControl  mCloudDetail;    
    WLFloatControl  mCloudScale;
};

class LLWatterSettingsAdapter
{
public:
    typedef std::shared_ptr<LLWatterSettingsAdapter> ptr_t;

    LLWatterSettingsAdapter();

    WLColorControl  mFogColor;
    WLXFloatControl mFogDensity;
    WLFloatControl  mUnderWaterFogMod;

    /// wavelet scales and directions
    WLVect3Control  mNormalScale;
    WLVect2Control  mWave1Dir;
    WLVect2Control  mWave2Dir;

    // controls how water is reflected and refracted
    WLFloatControl  mFresnelScale;
    WLFloatControl  mFresnelOffset;
    WLFloatControl  mScaleAbove;
    WLFloatControl  mScaleBelow;
    WLFloatControl  mBlurMultiplier;

};

#endif // LL_ENVIRONMENT_H
