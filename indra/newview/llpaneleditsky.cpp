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
#include "llsliderctrl.h"
#include "lltexturectrl.h"
#include "llcolorswatch.h"
#include "llvirtualtrackball.h"
#include "llsettingssky.h"
#include "llenvironment.h"
#include "llatmosphere.h"
#include "llviewercontrol.h"

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
    const std::string   FIELD_SKY_CLOUD_VARIANCE("cloud_variance");

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
    const std::string   FIELD_SKY_SUN_AZIMUTH("sun_azimuth");
    const std::string   FIELD_SKY_SUN_ELEVATION("sun_elevation");
    const std::string   FIELD_SKY_SUN_IMAGE("sun_image");
    const std::string   FIELD_SKY_SUN_SCALE("sun_scale");
    const std::string   FIELD_SKY_SUN_BEACON("sunbeacon");
    const std::string   FIELD_SKY_MOON_BEACON("moonbeacon");
    const std::string   FIELD_SKY_MOON_ROTATION("moon_rotation");
    const std::string   FIELD_SKY_MOON_AZIMUTH("moon_azimuth");
    const std::string   FIELD_SKY_MOON_ELEVATION("moon_elevation");
    const std::string   FIELD_SKY_MOON_IMAGE("moon_image");
    const std::string   FIELD_SKY_MOON_SCALE("moon_scale");
    const std::string   FIELD_SKY_MOON_BRIGHTNESS("moon_brightness");

    const std::string   PANEL_SKY_SUN_LAYOUT("sun_layout");
    const std::string   PANEL_SKY_MOON_LAYOUT("moon_layout");

    const std::string   FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL("rayleigh_exponential");
    const std::string   FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE("rayleigh_exponential_scale");
    const std::string   FIELD_SKY_DENSITY_RAYLEIGH_LINEAR("rayleigh_linear");
    const std::string   FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT("rayleigh_constant");
    const std::string   FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE("rayleigh_max_altitude");

    const std::string   FIELD_SKY_DENSITY_MIE_EXPONENTIAL("mie_exponential");
    const std::string   FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE("mie_exponential_scale");
    const std::string   FIELD_SKY_DENSITY_MIE_LINEAR("mie_linear");
    const std::string   FIELD_SKY_DENSITY_MIE_CONSTANT("mie_constant");
    const std::string   FIELD_SKY_DENSITY_MIE_ANISO("mie_aniso_factor");
    const std::string   FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE("mie_max_altitude");

    const std::string   FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL("absorption_exponential");
    const std::string   FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE("absorption_exponential_scale");
    const std::string   FIELD_SKY_DENSITY_ABSORPTION_LINEAR("absorption_linear");
    const std::string   FIELD_SKY_DENSITY_ABSORPTION_CONSTANT("absorption_constant");
    const std::string   FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE("absorption_max_altitude");

    const std::string   FIELD_SKY_DENSITY_MOISTURE_LEVEL("moisture_level");
    const std::string   FIELD_SKY_DENSITY_DROPLET_RADIUS("droplet_radius");
    const std::string   FIELD_SKY_DENSITY_ICE_LEVEL("ice_level");

    const std::string   FIELD_REFLECTION_PROBE_AMBIANCE("probe_ambiance");

    const F32 SLIDER_SCALE_SUN_AMBIENT(3.0f);
    const F32 SLIDER_SCALE_BLUE_HORIZON_DENSITY(2.0f);
    const F32 SLIDER_SCALE_GLOW_R(20.0f);
    const F32 SLIDER_SCALE_GLOW_B(-5.0f);
    const F32 SLIDER_SCALE_DENSITY_MULTIPLIER(0.001f);
}

static LLPanelInjector<LLPanelSettingsSkyAtmosTab> t_settings_atmos("panel_settings_atmos");
static LLPanelInjector<LLPanelSettingsSkyCloudTab> t_settings_cloud("panel_settings_cloud");
static LLPanelInjector<LLPanelSettingsSkySunMoonTab> t_settings_sunmoon("panel_settings_sunmoon");
static LLPanelInjector<LLPanelSettingsSkyDensityTab> t_settings_density("panel_settings_density");

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


bool LLPanelSettingsSkyAtmosTab::postBuild()
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
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MOISTURE_LEVEL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoistureLevelChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_DROPLET_RADIUS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onDropletRadiusChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ICE_LEVEL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onIceLevelChanged(); });
    getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setCommitCallback([this](LLUICtrl*, const LLSD&) { onReflectionProbeAmbianceChanged(); });
    refresh();

    return true;
}

//virtual
void LLPanelSettingsSkyAtmosTab::setEnabled(bool enabled)
{
    LLPanelSettingsSky::setEnabled(enabled);

    // Make sure we have initialized children (initialized)
    if (getFirstChild())
    {
        getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_MAX_ALT)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MOISTURE_LEVEL)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_DROPLET_RADIUS)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ICE_LEVEL)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setEnabled(enabled);
    }
}

void LLPanelSettingsSkyAtmosTab::refresh()
{
    if (!mSkySettings)
    {
        setAllChildrenEnabled(false);
        setEnabled(false);
        return;
    }

    setEnabled(getCanChangeSettings());
    setAllChildrenEnabled(getCanChangeSettings());

    getChild<LLColorSwatchCtrl>(FIELD_SKY_AMBIENT_LIGHT)->set(mSkySettings->getAmbientColor() / SLIDER_SCALE_SUN_AMBIENT);
    getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_HORIZON)->set(mSkySettings->getBlueHorizon() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
    getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_DENSITY)->set(mSkySettings->getBlueDensity() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);

    getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->setValue(mSkySettings->getHazeHorizon());
    getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->setValue(mSkySettings->getHazeDensity());
    getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setValue(mSkySettings->getGamma());
    F32 density_mult = mSkySettings->getDensityMultiplier();
    density_mult /= SLIDER_SCALE_DENSITY_MULTIPLIER;
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP)->setValue(density_mult);
    getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP)->setValue(mSkySettings->getDistanceMultiplier());
    getChild<LLUICtrl>(FIELD_SKY_MAX_ALT)->setValue(mSkySettings->getMaxY());

    F32 moisture_level  = mSkySettings->getSkyMoistureLevel();
    F32 droplet_radius  = mSkySettings->getSkyDropletRadius();
    F32 ice_level       = mSkySettings->getSkyIceLevel();

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    F32 rp_ambiance     = mSkySettings->getReflectionProbeAmbiance(should_auto_adjust);

    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MOISTURE_LEVEL)->setValue(moisture_level);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_DROPLET_RADIUS)->setValue(droplet_radius);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ICE_LEVEL)->setValue(ice_level);
    getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setValue(rp_ambiance);

    updateGammaLabel(should_auto_adjust);
}

//-------------------------------------------------------------------------
void LLPanelSettingsSkyAtmosTab::onAmbientLightChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setAmbientColor(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_AMBIENT_LIGHT)->get() * SLIDER_SCALE_SUN_AMBIENT));
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onBlueHorizonChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setBlueHorizon(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_HORIZON)->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onBlueDensityChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setBlueDensity(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_DENSITY)->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onHazeHorizonChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setHazeHorizon((F32)getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->getValue().asReal());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onHazeDensityChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setHazeDensity((F32)getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->getValue().asReal());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onSceneGammaChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setGamma((F32)getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->getValue().asReal());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onDensityMultipChanged()
{
    if (!mSkySettings) return;
    F32 density_mult = (F32)getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP)->getValue().asReal();
    density_mult *= SLIDER_SCALE_DENSITY_MULTIPLIER;
    mSkySettings->setDensityMultiplier(density_mult);
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onDistanceMultipChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setDistanceMultiplier((F32)getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP)->getValue().asReal());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onMaxAltChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setMaxY((F32)getChild<LLUICtrl>(FIELD_SKY_MAX_ALT)->getValue().asReal());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onMoistureLevelChanged()
{
    if (!mSkySettings) return;
    F32 moisture_level = (F32)getChild<LLUICtrl>(FIELD_SKY_DENSITY_MOISTURE_LEVEL)->getValue().asReal();
    mSkySettings->setSkyMoistureLevel(moisture_level);
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onDropletRadiusChanged()
{
    if (!mSkySettings) return;
    F32 droplet_radius = (F32)getChild<LLUICtrl>(FIELD_SKY_DENSITY_DROPLET_RADIUS)->getValue().asReal();
    mSkySettings->setSkyDropletRadius(droplet_radius);
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onIceLevelChanged()
{
    if (!mSkySettings) return;
    F32 ice_level = (F32)getChild<LLUICtrl>(FIELD_SKY_DENSITY_ICE_LEVEL)->getValue().asReal();
    mSkySettings->setSkyIceLevel(ice_level);
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onReflectionProbeAmbianceChanged()
{
    if (!mSkySettings) return;
    F32 ambiance = (F32)getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->getValue().asReal();

    mSkySettings->setReflectionProbeAmbiance(ambiance);
    mSkySettings->update();
    setIsDirty();

    updateGammaLabel();
}


void LLPanelSettingsSkyAtmosTab::updateGammaLabel(bool auto_adjust)
{
    if (!mSkySettings) return;
    F32 ambiance = mSkySettings->getReflectionProbeAmbiance(auto_adjust);
    if (ambiance != 0.f)
    {
        childSetValue("scene_gamma_label", getString("hdr_string"));
        getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setToolTip(getString("hdr_tooltip"));
    }
    else
    {
        childSetValue("scene_gamma_label", getString("brightness_string"));
        getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setToolTip(std::string());
    }

}
//==========================================================================
LLPanelSettingsSkyCloudTab::LLPanelSettingsSkyCloudTab() :
    LLPanelSettingsSky()
{
}

bool LLPanelSettingsSkyCloudTab::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudColorChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudCoverageChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScaleChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_VARIANCE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudVarianceChanged(); });

    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_XY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScrollChanged(); });
    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudMapChanged(); });
    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setDefaultImageAssetID(LLSettingsSky::GetDefaultCloudNoiseTextureId());
    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setAllowNoTexture(true);

    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });

    refresh();

    return true;
}

//virtual
void LLPanelSettingsSkyCloudTab::setEnabled(bool enabled)
{
    LLPanelSettingsSky::setEnabled(enabled);

    // Make sure we have children (initialized)
    if (getFirstChild())
    {
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_VARIANCE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->setEnabled(enabled);
    }
}

void LLPanelSettingsSkyCloudTab::refresh()
{
    if (!mSkySettings)
    {
        setAllChildrenEnabled(false);
        setEnabled(false);
        return;
    }

    setEnabled(getCanChangeSettings());
    setAllChildrenEnabled(getCanChangeSettings());

    getChild<LLColorSwatchCtrl>(FIELD_SKY_CLOUD_COLOR)->set(mSkySettings->getCloudColor());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setValue(mSkySettings->getCloudShadow());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setValue(mSkySettings->getCloudScale());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_VARIANCE)->setValue(mSkySettings->getCloudVariance());

    LLVector2 cloudScroll(mSkySettings->getCloudScrollRate());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_XY)->setValue(cloudScroll.getValue());

    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setValue(mSkySettings->getCloudNoiseTextureId());

    LLVector3 cloudDensity(mSkySettings->getCloudPosDensity1().getValue());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->setValue(cloudDensity[0]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->setValue(cloudDensity[1]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->setValue(cloudDensity[2]);

    LLVector3 cloudDetail(mSkySettings->getCloudPosDensity2().getValue());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->setValue(cloudDetail[0]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->setValue(cloudDetail[1]);
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->setValue(cloudDetail[2]);
}

//-------------------------------------------------------------------------
void LLPanelSettingsSkyCloudTab::onCloudColorChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setCloudColor(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_CLOUD_COLOR)->get()));
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudCoverageChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setCloudShadow((F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->getValue().asReal());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudScaleChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setCloudScale((F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudVarianceChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setCloudVariance((F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_VARIANCE)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudScrollChanged()
{
    if (!mSkySettings) return;
    LLVector2 scroll(getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_XY)->getValue());
    mSkySettings->setCloudScrollRate(scroll);
    setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudMapChanged()
{
    if (!mSkySettings) return;
    LLTextureCtrl* ctrl = getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP);
    mSkySettings->setCloudNoiseTextureId(ctrl->getValue().asUUID());
    setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudDensityChanged()
{
    if (!mSkySettings) return;
    LLColor3 density((F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X)->getValue().asReal(),
        (F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y)->getValue().asReal(),
        (F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D)->getValue().asReal());

    mSkySettings->setCloudPosDensity1(density);
    setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudDetailChanged()
{
    if (!mSkySettings) return;
    LLColor3 detail((F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X)->getValue().asReal(),
        (F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y)->getValue().asReal(),
        (F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D)->getValue().asReal());

    mSkySettings->setCloudPosDensity2(detail);
    setIsDirty();
}

//==========================================================================
LLPanelSettingsSkySunMoonTab::LLPanelSettingsSkySunMoonTab() :
    LLPanelSettingsSky()
{
}


bool LLPanelSettingsSkySunMoonTab::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_SUN_MOON_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunMoonColorChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onStarBrightnessChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunRotationChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_AZIMUTH)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunAzimElevChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_ELEVATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunAzimElevChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_IMAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunImageChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunScaleChanged(); });
    getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->setBlankImageAssetID(LLSettingsSky::GetBlankSunTextureId());
    getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->setDefaultImageAssetID(LLSettingsSky::GetBlankSunTextureId());
    getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->setAllowNoTexture(true);
    getChild<LLUICtrl>(FIELD_SKY_MOON_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonRotationChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MOON_AZIMUTH)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonAzimElevChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MOON_ELEVATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonAzimElevChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MOON_IMAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonImageChanged(); });
    getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->setDefaultImageAssetID(LLSettingsSky::GetDefaultMoonTextureId());
    getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->setBlankImageAssetID(LLSettingsSky::GetDefaultMoonTextureId());
    getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->setAllowNoTexture(true);
    getChild<LLUICtrl>(FIELD_SKY_MOON_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonScaleChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MOON_BRIGHTNESS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonBrightnessChanged(); });

    refresh();

    return true;
}

//virtual
void LLPanelSettingsSkySunMoonTab::setEnabled(bool enabled)
{
    LLPanelSettingsSky::setEnabled(enabled);

    // Make sure we have children
    if (getFirstChild())
    {
        getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_MOON_SCALE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_MOON_BRIGHTNESS)->setEnabled(enabled);
        getChildView(PANEL_SKY_SUN_LAYOUT)->setAllChildrenEnabled(true);
        getChildView(PANEL_SKY_MOON_LAYOUT)->setAllChildrenEnabled(true);
    }
}

void LLPanelSettingsSkySunMoonTab::refresh()
{
    if (!mSkySettings || !getCanChangeSettings())
    {
        getChildView(PANEL_SKY_SUN_LAYOUT)->setAllChildrenEnabled(false);
        getChildView(PANEL_SKY_MOON_LAYOUT)->setAllChildrenEnabled(false);
        getChildView(FIELD_SKY_SUN_BEACON)->setEnabled(true);
        getChildView(FIELD_SKY_MOON_BEACON)->setEnabled(true);

        if (!mSkySettings)
            return;
    }
    else
    {
        setEnabled(true);
        setAllChildrenEnabled(true);
    }

    getChild<LLColorSwatchCtrl>(FIELD_SKY_SUN_MOON_COLOR)->set(mSkySettings->getSunlightColor() / SLIDER_SCALE_SUN_AMBIENT);

    LLColor3 glow(mSkySettings->getGlow());

    // takes 40 - 0.2 range -> 0 - 1.99 UI range
    getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setValue(2.0 - (glow.mV[0] / SLIDER_SCALE_GLOW_R));
    getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setValue(glow.mV[2] / SLIDER_SCALE_GLOW_B);

    getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setValue(mSkySettings->getStarBrightness());
    getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->setValue(mSkySettings->getSunTextureId());
    getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE)->setValue(mSkySettings->getSunScale());
    getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->setValue(mSkySettings->getMoonTextureId());
    getChild<LLUICtrl>(FIELD_SKY_MOON_SCALE)->setValue(mSkySettings->getMoonScale());
    getChild<LLUICtrl>(FIELD_SKY_MOON_BRIGHTNESS)->setValue(mSkySettings->getMoonBrightness());

    // Sun rotation values
    F32 azimuth, elevation;
    LLQuaternion quat = mSkySettings->getSunRotation();
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);

    getChild<LLVirtualTrackball>(FIELD_SKY_SUN_ROTATION)->setRotation(quat);
    getChild<LLUICtrl>(FIELD_SKY_SUN_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_SUN_ELEVATION)->setValue(elevation);

    // Moon rotation values
    quat = mSkySettings->getMoonRotation();
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);

    getChild<LLVirtualTrackball>(FIELD_SKY_MOON_ROTATION)->setRotation(quat);
    getChild<LLUICtrl>(FIELD_SKY_MOON_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_MOON_ELEVATION)->setValue(elevation);

}

//-------------------------------------------------------------------------
void LLPanelSettingsSkySunMoonTab::onSunMoonColorChanged()
{
    if (!mSkySettings) return;
    LLColor3 color(getChild<LLColorSwatchCtrl>(FIELD_SKY_SUN_MOON_COLOR)->get());

    color *= SLIDER_SCALE_SUN_AMBIENT;

    mSkySettings->setSunlightColor(color);
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onGlowChanged()
{
    if (!mSkySettings) return;
    LLColor3 glow((F32)getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->getValue().asReal(), 0.0f, (F32)getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->getValue().asReal());

    // takes 0 - 1.99 UI range -> 40 -> 0.2 range
    glow.mV[0] = (2.0f - glow.mV[0]) * SLIDER_SCALE_GLOW_R;
    glow.mV[2] *= SLIDER_SCALE_GLOW_B;

    mSkySettings->setGlow(glow);
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onStarBrightnessChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setStarBrightness((F32)getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->getValue().asReal());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onSunRotationChanged()
{
    LLQuaternion quat = getChild<LLVirtualTrackball>(FIELD_SKY_SUN_ROTATION)->getRotation();

    F32 azimuth, elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);
    getChild<LLUICtrl>(FIELD_SKY_SUN_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_SUN_ELEVATION)->setValue(elevation);
    if (mSkySettings)
    {
        mSkySettings->setSunRotation(quat);
        mSkySettings->update();
        setIsDirty();
    }
}

void LLPanelSettingsSkySunMoonTab::onSunAzimElevChanged()
{
    F32 azimuth = (F32)getChild<LLUICtrl>(FIELD_SKY_SUN_AZIMUTH)->getValue().asReal();
    F32 elevation = (F32)getChild<LLUICtrl>(FIELD_SKY_SUN_ELEVATION)->getValue().asReal();
    LLQuaternion quat;

    azimuth *= DEG_TO_RAD;
    elevation *= DEG_TO_RAD;

    if (is_approx_zero(elevation))
    {
        elevation = F_APPROXIMATELY_ZERO;
    }

    quat.setAngleAxis(-elevation, 0, 1, 0);
    LLQuaternion az_quat;
    az_quat.setAngleAxis(F_TWO_PI - azimuth, 0, 0, 1);
    quat *= az_quat;

    getChild<LLVirtualTrackball>(FIELD_SKY_SUN_ROTATION)->setRotation(quat);

    if (mSkySettings)
    {
        mSkySettings->setSunRotation(quat);
        mSkySettings->update();
        setIsDirty();
    }
}

void LLPanelSettingsSkySunMoonTab::onSunScaleChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setSunScale((F32)(getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE)->getValue().asReal()));
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onSunImageChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setSunTextureId(getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE)->getValue().asUUID());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onMoonRotationChanged()
{
    LLQuaternion quat = getChild<LLVirtualTrackball>(FIELD_SKY_MOON_ROTATION)->getRotation();

    F32 azimuth, elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);
    getChild<LLUICtrl>(FIELD_SKY_MOON_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_MOON_ELEVATION)->setValue(elevation);

    if (mSkySettings)
    {
        mSkySettings->setMoonRotation(quat);
        mSkySettings->update();
        setIsDirty();
    }
}

void LLPanelSettingsSkySunMoonTab::onMoonAzimElevChanged()
{
    F32 azimuth = (F32)getChild<LLUICtrl>(FIELD_SKY_MOON_AZIMUTH)->getValue().asReal();
    F32 elevation = (F32)getChild<LLUICtrl>(FIELD_SKY_MOON_ELEVATION)->getValue().asReal();
    LLQuaternion quat;

    azimuth *= DEG_TO_RAD;
    elevation *= DEG_TO_RAD;

    if (is_approx_zero(elevation))
    {
        elevation = F_APPROXIMATELY_ZERO;
    }

    quat.setAngleAxis(-elevation, 0, 1, 0);
    LLQuaternion az_quat;
    az_quat.setAngleAxis(F_TWO_PI- azimuth, 0, 0, 1);
    quat *= az_quat;

    getChild<LLVirtualTrackball>(FIELD_SKY_MOON_ROTATION)->setRotation(quat);

    if (mSkySettings)
    {
        mSkySettings->setMoonRotation(quat);
        mSkySettings->update();
        setIsDirty();
    }
}

void LLPanelSettingsSkySunMoonTab::onMoonImageChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setMoonTextureId(getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE)->getValue().asUUID());
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onMoonScaleChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setMoonScale((F32)(getChild<LLUICtrl>(FIELD_SKY_MOON_SCALE)->getValue().asReal()));
    mSkySettings->update();
    setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onMoonBrightnessChanged()
{
    if (!mSkySettings) return;
    mSkySettings->setMoonBrightness((F32)(getChild<LLUICtrl>(FIELD_SKY_MOON_BRIGHTNESS)->getValue().asReal()));
    mSkySettings->update();
    setIsDirty();
}

LLPanelSettingsSkyDensityTab::LLPanelSettingsSkyDensityTab()
{
}

bool LLPanelSettingsSkyDensityTab::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighExponentialChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighExponentialScaleChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_LINEAR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighLinearChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighConstantChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighMaxAltitudeChanged(); });

    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieExponentialChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieExponentialScaleChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_LINEAR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieLinearChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_CONSTANT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieConstantChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_ANISO)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieAnisoFactorChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieMaxAltitudeChanged(); });

    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionExponentialChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionExponentialScaleChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_LINEAR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionLinearChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_CONSTANT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionConstantChanged(); });

    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionMaxAltitudeChanged(); });

    refresh();
    return true;
}

void LLPanelSettingsSkyDensityTab::setEnabled(bool enabled)
{
    LLPanelSettingsSky::setEnabled(enabled);

    // Make sure we have children
    if (getFirstChild())
    {
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_LINEAR)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE)->setEnabled(enabled);

        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_LINEAR)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_CONSTANT)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_ANISO)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE)->setEnabled(enabled);

        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_LINEAR)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_CONSTANT)->setEnabled(enabled);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE)->setEnabled(enabled);
    }
}

void LLPanelSettingsSkyDensityTab::refresh()
{
    if (!mSkySettings)
    {
        setAllChildrenEnabled(false);
        setEnabled(false);
        return;
    }

    setEnabled(getCanChangeSettings());
    setAllChildrenEnabled(getCanChangeSettings());

    // Get first (only) profile layer of each type for editing
    LLSD rayleigh_config    = mSkySettings->getRayleighConfig();
    LLSD mie_config         = mSkySettings->getMieConfig();
    LLSD absorption_config  = mSkySettings->getAbsorptionConfig();

    F32 rayleigh_exponential_term    = (F32)rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
    F32 rayleigh_exponential_scale   = (F32)rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
    F32 rayleigh_linear_term         = (F32)rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
    F32 rayleigh_constant_term       = (F32)rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
    F32 rayleigh_max_alt             = (F32)rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();

    F32 mie_exponential_term         = (F32)mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
    F32 mie_exponential_scale        = (F32)mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
    F32 mie_linear_term              = (F32)mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
    F32 mie_constant_term            = (F32)mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
    F32 mie_aniso_factor             = (F32)mie_config[LLSettingsSky::SETTING_MIE_ANISOTROPY_FACTOR].asReal();
    F32 mie_max_alt                  = (F32)mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();

    F32 absorption_exponential_term  = (F32)absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
    F32 absorption_exponential_scale = (F32)absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
    F32 absorption_linear_term       = (F32)absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
    F32 absorption_constant_term     = (F32)absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
    F32 absorption_max_alt           = (F32)absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();

    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL)->setValue(rayleigh_exponential_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE)->setValue(rayleigh_exponential_scale);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_LINEAR)->setValue(rayleigh_linear_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT)->setValue(rayleigh_constant_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE)->setValue(rayleigh_max_alt);

    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL)->setValue(mie_exponential_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE)->setValue(mie_exponential_scale);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_LINEAR)->setValue(mie_linear_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_CONSTANT)->setValue(mie_constant_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_ANISO)->setValue(mie_aniso_factor);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE)->setValue(mie_max_alt);

    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL)->setValue(absorption_exponential_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE)->setValue(absorption_exponential_scale);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_LINEAR)->setValue(absorption_linear_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_CONSTANT)->setValue(absorption_constant_term);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE)->setValue(absorption_max_alt);
}

void LLPanelSettingsSkyDensityTab::updateProfile()
{
    F32 rayleigh_exponential_term    = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL)->getValueF32();
    F32 rayleigh_exponential_scale   = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE)->getValueF32();
    F32 rayleigh_linear_term         = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_LINEAR)->getValueF32();
    F32 rayleigh_constant_term       = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT)->getValueF32();
    F32 rayleigh_max_alt             = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE)->getValueF32();

    F32 mie_exponential_term         = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL)->getValueF32();
    F32 mie_exponential_scale        = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE)->getValueF32();
    F32 mie_linear_term              = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_LINEAR)->getValueF32();
    F32 mie_constant_term            = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_CONSTANT)->getValueF32();
    F32 mie_aniso_factor             = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_ANISO)->getValueF32();
    F32 mie_max_alt                  = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE)->getValueF32();

    F32 absorption_exponential_term  = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL)->getValueF32();
    F32 absorption_exponential_scale = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE)->getValueF32();
    F32 absorption_linear_term       = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_LINEAR)->getValueF32();
    F32 absorption_constant_term     = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_CONSTANT)->getValueF32();
    F32 absorption_max_alt           = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE)->getValueF32();

    LLSD rayleigh_config    = LLSettingsSky::createSingleLayerDensityProfile(rayleigh_max_alt, rayleigh_exponential_term, rayleigh_exponential_scale, rayleigh_linear_term, rayleigh_constant_term);
    LLSD mie_config         = LLSettingsSky::createSingleLayerDensityProfile(mie_max_alt, mie_exponential_term, mie_exponential_scale, mie_linear_term, mie_constant_term, mie_aniso_factor);
    LLSD absorption_layer   = LLSettingsSky::createSingleLayerDensityProfile(absorption_max_alt, absorption_exponential_term, absorption_exponential_scale, absorption_linear_term, absorption_constant_term);

    static LLSD absorption_layer_ozone = LLSettingsSky::createDensityProfileLayer(0.0f, 0.0f, 0.0f, -1.0f / 15000.0f, 8.0f / 3.0f);

    LLSD absorption_config;
    absorption_config.append(absorption_layer);
    absorption_config.append(absorption_layer_ozone);

    mSkySettings->setRayleighConfigs(rayleigh_config);
    mSkySettings->setMieConfigs(mie_config);
    mSkySettings->setAbsorptionConfigs(absorption_config);
    mSkySettings->update();
    setIsDirty();

    if (gAtmosphere)
    {
        AtmosphericModelSettings atmospheric_settings;
        LLEnvironment::getAtmosphericModelSettings(atmospheric_settings, mSkySettings);
        gAtmosphere->configureAtmosphericModel(atmospheric_settings);
    }
}

void LLPanelSettingsSkyDensityTab::onRayleighExponentialChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighExponentialScaleChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighLinearChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighConstantChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighMaxAltitudeChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieExponentialChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieExponentialScaleChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieLinearChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieConstantChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieAnisoFactorChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieMaxAltitudeChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionExponentialChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionExponentialScaleChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionLinearChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionConstantChanged()
{
    updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionMaxAltitudeChanged()
{
    updateProfile();
}
