/**
* @file llpaneleditwater.h
* @brief Panels for water settings
*
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#ifndef LLPANEL_EDIT_WATER_H
#define LLPANEL_EDIT_WATER_H

#include "llpanel.h"
#include "llsettingswater.h"

#include "llfloatereditenvironmentbase.h"

//=========================================================================
class LLSlider;
class LLColorSwatchCtrl;
class LLTextureCtrl;
class LLXYVector;

//=========================================================================
class LLPanelSettingsWater : public LLSettingsEditPanel
{
    LOG_CLASS(LLPanelSettingsWater);

public:
                            LLPanelSettingsWater();

    virtual void            setSettings(const LLSettingsBase::ptr_t &settings) override   { setWater(std::static_pointer_cast<LLSettingsWater>(settings)); }

    LLSettingsWater::ptr_t  getWater() const                                        { return mWaterSettings; }
    void                    setWater(const LLSettingsWater::ptr_t &water)           { mWaterSettings = water; clearIsDirty(); refresh(); }

protected:
    LLSettingsWater::ptr_t  mWaterSettings;
};

// *RIDER* In this case this split is unecessary since there is only a single 
// tab page for water settings at this point.  However more may be added in the 
// future and I want to reinforce the pattern used for sky/atmosphere tabs.
class LLPanelSettingsWaterMainTab : public LLPanelSettingsWater
{
    LOG_CLASS(LLPanelSettingsWaterMainTab);

public:
                            LLPanelSettingsWaterMainTab();

    virtual BOOL            postBuild() override;
    virtual void            setEnabled(BOOL enabled) override;

protected:
    virtual void            refresh() override;

private:

    LLColorSwatchCtrl *     mClrFogColor;
//     LLSlider *              mSldFogDensity;
//     LLSlider *              mSldUnderWaterMod;
    LLTextureCtrl *         mTxtNormalMap;

    void                    onFogColorChanged();
    void                    onFogDensityChanged();
    void                    onFogUnderWaterChanged();
    void                    onNormalMapChanged();

    void                    onLargeWaveChanged();
    void                    onSmallWaveChanged();

    void                    onNormalScaleChanged();
    void                    onFresnelScaleChanged();
    void                    onFresnelOffsetChanged();
    void                    onScaleAboveChanged();
    void                    onScaleBelowChanged();
    void                    onBlurMultipChanged();
};


#endif // LLPANEL_EDIT_WATER_H
