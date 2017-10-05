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

    inline WLColorControl & operator = (const LLColor4 & val)
    {
        mColor = val;
        return *this;
    }

    inline operator LLColor4 (void) const 
    {
        return mColor;
    }

    inline operator LLColor3 (void) const 
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
    std::string mName;			/// name to use to dereference params
    std::string mSliderName;	/// name of the slider in menu
    bool        mHasSliderName;			/// only set slider name for true color types
    bool        mIsSunOrAmbientColor;			/// flag for if it's the sun or ambient color controller
    bool        mIsBlueHorizonOrDensity;		/// flag for if it's the Blue Horizon or Density color controller

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


//-------------------------------------------------------------------------
class LLSkySettingsAdapter
{
public:
    typedef std::shared_ptr<LLSkySettingsAdapter> ptr_t;
    
    LLSkySettingsAdapter();

    WLFloatControl mWLGamma;

    /// Atmospherics
    WLColorControl mBlueHorizon;
    WLFloatControl mHazeDensity;
    WLColorControl mBlueDensity;
    WLFloatControl mDensityMult;
    WLFloatControl mHazeHorizon;
    WLFloatControl mMaxAlt;

    /// Lighting
    WLColorControl mLightnorm;
    WLColorControl mSunlight;
    WLColorControl mAmbient;
    WLColorControl mGlow;

    /// Clouds
    WLColorControl mCloudColor;
    WLColorControl mCloudMain;
    WLFloatControl mCloudCoverage;
    WLColorControl mCloudDetail;
    WLFloatControl mDistanceMult;
    WLFloatControl mCloudScale;


};

#endif // LL_ENVIRONMENT_H

