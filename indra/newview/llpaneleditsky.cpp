/**
* @file llpaneleditsky.cpp
* @brief Floaters to create and edit fixed settings for sky and water.
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

#include "llviewerprecompiledheaders.h"

#include "llpaneleditsky.h"

#include "llslider.h"
#include "lltexturectrl.h"
#include "llcolorswatch.h"
#include "lljoystickbutton.h"

namespace
{   
    // Atmosphere Tab
    const std::string   FIELD_SKY_AMBIENT_LIGHT("ambient_light");
    const std::string   FIELD_SKY_BLUE_HORIZON("blue_horizon");
    const std::string   FIELD_SKY_BLUE_DENSITY("blue_density");
    const std::string   FIELD_SKY_HAZE_HORIZON("haze_horizon");
    const std::string   FIELD_SKY_HAZE_DENSITY("haze_density");
    const std::string   FIELD_SKY_SCENE_GAMMA("scene_gamma");
    const std::string   FIELD_SKY_DENSITY_MULTIP("density_multip");
    const std::string   FIELD_SKY_DISTANCE_MULTIP("distance_multip");
    const std::string   FIELD_SKY_MAX_ALT("max_alt");

    const std::string   FIELD_SKY_CLOUD_COLOR("cloud_color");
    const std::string   FIELD_SKY_CLOUD_COVERAGE("cloud_coverage");
    const std::string   FIELD_SKY_CLOUD_SCALE("cloud_scale");
    const std::string   FIELD_SKY_CLOUD_SCROLL_XY("cloud_scroll_xy");
    const std::string   FIELD_SKY_CLOUD_MAP("cloud_map");
    const std::string   FIELD_SKY_CLOUD_DENSITY_X("cloud_density_x");
    const std::string   FIELD_SKY_CLOUD_DENSITY_Y("cloud_density_y");
    const std::string   FIELD_SKY_CLOUD_DENSITY_D("cloud_density_d");
    const std::string   FIELD_SKY_CLOUD_DETAIL_X("cloud_detail_x");
    const std::string   FIELD_SKY_CLOUD_DETAIL_Y("cloud_detail_y");
    const std::string   FIELD_SKY_CLOUD_DETAIL_D("cloud_detail_d");

    const std::string   FIELD_SKY_SUN_MOON_COLOR("sun_moon_color");
    const std::string   FIELD_SKY_GLOW_FOCUS("glow_focus");
    const std::string   FIELD_SKY_GLOW_SIZE("glow_size");
    const std::string   FIELD_SKY_STAR_BRIGHTNESS("star_brightness");
    const std::string   FIELD_SKY_SUN_ROTATION("sun_rotation");
    const std::string   FIELD_SKY_SUN_IMAGE("sun_image");
    const std::string   FIELD_SKY_MOON_ROTATION("moon_rotation");
    const std::string   FIELD_SKY_MOON_IMAGE("moon_image");

    const F32 SLIDER_SCALE_SUN_AMBIENT(3.0f);
    const F32 SLIDER_SCALE_BLUE_HORIZON_DENSITY(2.0f);
    const F32 SLIDER_SCALE_GLOW_R(20.0f);
    const F32 SLIDER_SCALE_GLOW_B(-5.0f);

    const LLVector2     CLOUD_SCROLL_ADJUST(10, 10);
}

static LLPanelInjector<LLPanelSettingsSkyAtmosTab> t_settings_atmos("panel_settings_atmos");
static LLPanelInjector<LLPanelSettingsSkyCloudTab> t_settings_cloud("panel_settings_cloud");
static LLPanelInjector<LLPanelSettingsSkySunMoonTab> t_settings_sunmoon("panel_settings_sunmoon");

//==========================================================================
LLPanelSettingsSky::LLPanelSettingsSky() :
    LLSettingsEditPanel(),
    mSkySettings()
{

}


//==========================================================================
LLPanelSettingsSkyAtmosTab::LLPanelSettingsSkyAtmosTab() :
    LLPanelSettingsSky()
{
}


BOOL LLPanelSettingsSkyAtmosTab::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_AMBIENT_LIGHT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAmbientLightChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_BLUE_HORIZON)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueHorizonChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_BLUE_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeHorizonChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSceneGammaChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onDensityMultipChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onDistanceMultipChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MAX_ALT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMaxAltChanged(); });

    refresh();

    return TRUE;
}

//virtual
void LLPanelSettingsSkyAtmosTab::setEnabled(BOOL enabled)
{
    LLPanelSettingsSky::setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_MAX_ALT)->setEnabled(enabled);
}

void LLPanelSettingsSkyAtmosTab::refresh()
{
    if (!mSkySettings)
    {
        setAllChildrenEnabled(FALSE);
        setEnabled(FALSE);
        return;
    }

    setEnabled(TRUE);
    setAllChildrenEnabled(TRUE);

    getChild<LLColorSwatchCtrl>(FIELD_SKY_AMBIENT_LIGHT)->set(mSkySettings->getAmbientColor() / SLIDER_SCALE_SUN_AMBIENT);
    getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_HORIZON)->set(mSkySettings->getBlueHorizon() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
    getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_DENSITY)->set(mSkySettings->getBlueDensity() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);

    getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->setValue(mSkySettings->getHazeHorizon());
    getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->setValue(mSkySettings->getHazeDensity());
    getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setValue(mSkySettings->getGamma());
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP)->setValue(mSkySettings->getDensityMultiplier());
    getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP)->setValue(mSkySettings->getDistanceMultiplier());
    getChild<LLUICtrl>(FIELD_SKY_MAX_ALT)->setValue(mSkySettings->getMaxY());

}

//-------------------------------------------------------------------------
void LLPanelSettingsSkyAtmosTab::onAmbientLightChanged()
{
    mSkySettings->setAmbientColor(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_AMBIENT_LIGHT)->get() * SLIDER_SCALE_SUN_AMBIENT));
}

void LLPanelSettingsSkyAtmosTab::onBlueHorizonChanged()
{
    mSkySettings->setBlueHorizon(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_HORIZON)->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
}

void LLPanelSettingsSkyAtmosTab::onBlueDensityChanged()
{
    mSkySettings->setBlueDensity(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_DENSITY)->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
}

void LLPanelSettingsSkyAtmosTab::onHazeHorizonChanged()
{
    mSkySettings->setHazeHorizon(getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->getValue().asReal());
}

void LLPanelSettingsSkyAtmosTab::onHazeDensityChanged()
{
    mSkySettings->setHazeDensity(getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->getValue().asReal());
}

void LLPanelSettingsSkyAtmosTab::onSceneGammaChanged()
{
    mSkySettings->setGamma(getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->getValue().asReal());
}

void LLPanelSettingsSkyAtmosTab::onDensityMultipChanged()
{
    mSkySettings->setDensityMultiplier(getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP)->getValue().asReal());
}

void LLPanelSettingsSkyAtmosTab::onDistanceMultipChanged()
{
    mSkySettings->setDistanceMultiplier(getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP)->getValue().asReal());
}

void LLPanelSettingsSkyAtmosTab::onMaxAltChanged()
{
    mSkySettings->setMaxY(getChild<LLUICtrl>(FIELD_SKY_MAX_ALT)->getValue().asReal());
}

//==========================================================================
LLPanelSettingsSkyCloudTab::LLPanelSettingsSkyCloudTab() :
    LLPanelSettingsSky()
{
}


BOOL LLPanelSettingsSkyCloudTab::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudColorChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudCoverageChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScaleChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_XY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScrollChanged(); });
    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudMapChanged(); });
//    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setDefaultImageAssetID(LLSettingsSky::DEFAULT_CLOUD_TEXTURE_ID);

    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });

    refresh();

    return TRUE;
}

//virtual
void LLPanelSettingsSkyCloudTab::setEnabled(BOOL enabled)
{
    LLPanelSettingsSky::setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->setEnabled(enabled);
}

void LLPanelSettingsSkyCloudTab::refresh()
{
    if (!mSkySettings)
    {
        setAllChildrenEnabled(FALSE);
        setEnabled(FALSE);
        return;
    }

    setEnabled(TRUE);
    setAllChildrenEnabled(TRUE);

    getChild<LLColorSwatchCtrl>(FIELD_SKY_CLOUD_COLOR)->set(mSkySettings->getCloudColor());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setValue(mSkySettings->getCloudShadow());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setValue(mSkySettings->getCloudScale());

    LLVector2 cloudScroll(mSkySettings->getCloudScrollRate());
    cloudScroll -= CLOUD_SCROLL_ADJUST;
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_XY)->setValue(cloudScroll.getValue());

    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setValue(mSkySettings->getCloudNoiseTextureId());

    LLVector3 cloudDensity(mSkySettings->getCloudPosDensity1().getValue());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->setValue(cloudDensity[0]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->setValue(cloudDensity[1]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->setValue(cloudDensity[2]);

    LLVector3 cloudDetail(mSkySettings->getCloudPosDensity1().getValue());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->setValue(cloudDetail[0]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->setValue(cloudDetail[1]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->setValue(cloudDetail[2]);
}

//-------------------------------------------------------------------------
void LLPanelSettingsSkyCloudTab::onCloudColorChanged()
{
    mSkySettings->setCloudColor(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_CLOUD_COLOR)->get()));
}

void LLPanelSettingsSkyCloudTab::onCloudCoverageChanged()
{
    mSkySettings->setCloudShadow(getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->getValue().asReal());
}

void LLPanelSettingsSkyCloudTab::onCloudScaleChanged()
{
    mSkySettings->setCloudScale(getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->getValue().asReal());
}

void LLPanelSettingsSkyCloudTab::onCloudScrollChanged()
{
    LLVector2 scroll(getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_XY)->getValue());
    scroll += CLOUD_SCROLL_ADJUST;
    mSkySettings->setCloudScrollRate(scroll);
}

void LLPanelSettingsSkyCloudTab::onCloudMapChanged()
{
    mSkySettings->setCloudNoiseTextureId(getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->getValue().asUUID());
}

void LLPanelSettingsSkyCloudTab::onCloudDensityChanged()
{
    LLColor3 density(getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->getValue().asReal(), 
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->getValue().asReal(), 
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->getValue().asReal());

    mSkySettings->setCloudPosDensity1(density);
}

void LLPanelSettingsSkyCloudTab::onCloudDetailChanged()
{
    LLColor3 detail(getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->getValue().asReal(),
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->getValue().asReal(),
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->getValue().asReal());

    mSkySettings->setCloudPosDensity2(detail);
}

//==========================================================================
LLPanelSettingsSkySunMoonTab::LLPanelSettingsSkySunMoonTab() :
    LLPanelSettingsSky()
{
}


BOOL LLPanelSettingsSkySunMoonTab::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_SUN_MOON_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunMoonColorChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onStarBrightnessChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunRotationChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_IMAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunImageChanged(); });
//    getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->setDefaultImageAssetID(LLSettingsSky:: );
    getChild<LLUICtrl>(FIELD_SKY_MOON_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonRotationChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MOON_IMAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonImageChanged(); });
//    getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->setDefaultImageAssetID(LLSettingsSky:: );

    refresh();

    return TRUE;
}

//virtual
void LLPanelSettingsSkySunMoonTab::setEnabled(BOOL enabled)
{
    LLPanelSettingsSky::setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setEnabled(enabled);
}

void LLPanelSettingsSkySunMoonTab::refresh()
{
    if (!mSkySettings)
    {
        setAllChildrenEnabled(FALSE);
        setEnabled(FALSE);
        return;
    }

    setEnabled(TRUE);
    setAllChildrenEnabled(TRUE);

    getChild<LLColorSwatchCtrl>(FIELD_SKY_SUN_MOON_COLOR)->set(mSkySettings->getSunlightColor() / SLIDER_SCALE_SUN_AMBIENT);

    LLColor3 glow(mSkySettings->getGlow());
    
    getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setValue(2.0 - (glow.mV[0] / SLIDER_SCALE_GLOW_R));
    getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setValue(glow.mV[2] / SLIDER_SCALE_GLOW_B);
    getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setValue(mSkySettings->getStarBrightness());
    getChild<LLJoystickQuaternion>(FIELD_SKY_SUN_ROTATION)->setRotation(mSkySettings->getSunRotation());
    getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->setValue(mSkySettings->getSunTextureId());
    getChild<LLJoystickQuaternion>(FIELD_SKY_MOON_ROTATION)->setRotation(mSkySettings->getMoonRotation());
    getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->setValue(mSkySettings->getMoonTextureId());
}

//-------------------------------------------------------------------------
void LLPanelSettingsSkySunMoonTab::onSunMoonColorChanged()
{
    LLColor3 color(getChild<LLColorSwatchCtrl>(FIELD_SKY_SUN_MOON_COLOR)->get());

    color *= SLIDER_SCALE_SUN_AMBIENT;

    mSkySettings->setSunlightColor(color);
}

void LLPanelSettingsSkySunMoonTab::onGlowChanged()
{
    LLColor3 glow(getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->getValue().asReal(), 0.0f, getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->getValue().asReal());

    glow.mV[0] = (2.0f - glow.mV[0]) * SLIDER_SCALE_GLOW_R; 
    glow.mV[2] *= SLIDER_SCALE_GLOW_B;

    mSkySettings->setGlow(glow);
}

void LLPanelSettingsSkySunMoonTab::onStarBrightnessChanged()
{
    mSkySettings->setStarBrightness(getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->getValue().asReal());
}

void LLPanelSettingsSkySunMoonTab::onSunRotationChanged()
{
    mSkySettings->setSunRotation(getChild<LLJoystickQuaternion>(FIELD_SKY_SUN_ROTATION)->getRotation());
    mSkySettings->update();
}

void LLPanelSettingsSkySunMoonTab::onSunImageChanged()
{
    mSkySettings->setSunTextureId(getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->getValue().asUUID());
    mSkySettings->update();
}

void LLPanelSettingsSkySunMoonTab::onMoonRotationChanged()
{
    mSkySettings->setMoonRotation(getChild<LLJoystickQuaternion>(FIELD_SKY_MOON_ROTATION)->getRotation());
    mSkySettings->update();
}

void LLPanelSettingsSkySunMoonTab::onMoonImageChanged()
{
    mSkySettings->setMoonTextureId(getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->getValue().asUUID());
    mSkySettings->update();
}
