/**
* @file llpaneleditsky.h
* @brief Panels for sky settings
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

#ifndef LLPANEL_EDIT_SKY_H
#define LLPANEL_EDIT_SKY_H

#include "llpanel.h"
#include "llsettingssky.h"
#include "llfloatereditenvironmentbase.h"

//=========================================================================
class LLSlider;
class LLColorSwatchCtrl;
class LLTextureCtrl;

//=========================================================================
class LLPanelSettingsSky : public LLSettingsEditPanel
{
    LOG_CLASS(LLPanelSettingsSky);

public:
                            LLPanelSettingsSky();

    virtual void            setSettings(const LLSettingsBase::ptr_t &settings) override   { setSky(std::static_pointer_cast<LLSettingsSky>(settings)); }

    LLSettingsSky::ptr_t    getSky() const                                          { return mSkySettings; }
    void                    setSky(const LLSettingsSky::ptr_t &sky)                 { mSkySettings = sky; clearIsDirty(); refresh(); }

protected:
    LLSettingsSky::ptr_t  mSkySettings;
};

class LLPanelSettingsSkyAtmosTab : public LLPanelSettingsSky
{
    LOG_CLASS(LLPanelSettingsSkyAtmosTab);

public:
                            LLPanelSettingsSkyAtmosTab();

    virtual BOOL            postBuild() override;
    virtual void            setEnabled(BOOL enabled) override;

protected:
    virtual void            refresh() override;

private:
    void                    onAmbientLightChanged();
    void                    onBlueHorizonChanged();
    void                    onBlueDensityChanged();
    void                    onHazeHorizonChanged();
    void                    onHazeDensityChanged();
    void                    onSceneGammaChanged();
    void                    onDensityMultipChanged();
    void                    onDistanceMultipChanged();
    void                    onMaxAltChanged();
    void                    onMoistureLevelChanged();
    void                    onDropletRadiusChanged();
    void                    onIceLevelChanged();

};

class LLPanelSettingsSkyCloudTab : public LLPanelSettingsSky
{
    LOG_CLASS(LLPanelSettingsSkyCloudTab);

public:
                            LLPanelSettingsSkyCloudTab();

    virtual BOOL            postBuild() override;
    void                    setEnabled(BOOL enabled) override;

protected:
    virtual void            refresh() override;

private:
    void                    onCloudColorChanged();
    void                    onCloudCoverageChanged();
    void                    onCloudScaleChanged();
    void                    onCloudVarianceChanged();
    void                    onCloudScrollChanged();
    void                    onCloudMapChanged();
    void                    onCloudDensityChanged();
    void                    onCloudDetailChanged();
};

class LLPanelSettingsSkySunMoonTab : public LLPanelSettingsSky
{
    LOG_CLASS(LLPanelSettingsSkySunMoonTab);

public:
                            LLPanelSettingsSkySunMoonTab();

    virtual BOOL            postBuild() override;
    virtual void            setEnabled(BOOL enabled) override;

protected:
    virtual void            refresh() override;

private:
    void                    onSunMoonColorChanged();
    void                    onGlowChanged();
    void                    onStarBrightnessChanged();
    void                    onSunRotationChanged();
    void                    onSunAzimElevChanged();
    void                    onSunScaleChanged();
    void                    onSunImageChanged();
    void                    onMoonRotationChanged();
    void                    onMoonAzimElevChanged();
    void                    onMoonScaleChanged();
    void                    onMoonBrightnessChanged();
    void                    onMoonImageChanged();
};

// single subtab of the density settings tab
class LLPanelSettingsSkyDensityTab : public LLPanelSettingsSky
{
    LOG_CLASS(LLPanelSettingsSkyDensityTab);

public:
    LLPanelSettingsSkyDensityTab();

    virtual BOOL postBuild() override;
    virtual void setEnabled(BOOL enabled) override;

protected:
    virtual void refresh() override;

    void onRayleighExponentialChanged();
    void onRayleighExponentialScaleChanged();
    void onRayleighLinearChanged();
    void onRayleighConstantChanged();
    void onRayleighMaxAltitudeChanged();

    void onMieExponentialChanged();
    void onMieExponentialScaleChanged();
    void onMieLinearChanged();
    void onMieConstantChanged();
    void onMieAnisoFactorChanged();
    void onMieMaxAltitudeChanged();

    void onAbsorptionExponentialChanged();
    void onAbsorptionExponentialScaleChanged();
    void onAbsorptionLinearChanged();
    void onAbsorptionConstantChanged();
    void onAbsorptionMaxAltitudeChanged();

    // update the settings for our profile type
    void updateProfile();
};
#endif // LLPANEL_EDIT_SKY_H
