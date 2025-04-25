/**
 * @file llfloaterfixedenvironment.cpp
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

#include "llfloaterenvironmentadjust.h"

#include "llnotificationsutil.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llcolorswatch.h"
#include "lltexturectrl.h"
#include "llvirtualtrackball.h"
#include "llenvironment.h"
#include "llviewercontrol.h"
#include "pipeline.h"

//=========================================================================
namespace
{
    const std::string FIELD_SKY_AMBIENT_LIGHT("ambient_light");
    const std::string FIELD_SKY_BLUE_HORIZON("blue_horizon");
    const std::string FIELD_SKY_BLUE_DENSITY("blue_density");
    const std::string FIELD_SKY_SUN_COLOR("sun_color");
    const std::string FIELD_SKY_CLOUD_COLOR("cloud_color");
    const std::string FIELD_SKY_HAZE_HORIZON("haze_horizon");
    const std::string FIELD_SKY_HAZE_DENSITY("haze_density");
    const std::string FIELD_SKY_CLOUD_COVERAGE("cloud_coverage");
    const std::string FIELD_SKY_CLOUD_MAP("cloud_map");
    const std::string FIELD_WATER_NORMAL_MAP("water_normal_map");
    const std::string FIELD_SKY_CLOUD_SCALE("cloud_scale");
    const std::string FIELD_SKY_SCENE_GAMMA("scene_gamma");
    const std::string FIELD_SKY_SUN_ROTATION("sun_rotation");
    const std::string FIELD_SKY_SUN_AZIMUTH("sun_azimuth");
    const std::string FIELD_SKY_SUN_ELEVATION("sun_elevation");
    const std::string FIELD_SKY_SUN_SCALE("sun_scale");
    const std::string FIELD_SKY_GLOW_FOCUS("glow_focus");
    const std::string FIELD_SKY_GLOW_SIZE("glow_size");
    const std::string FIELD_SKY_STAR_BRIGHTNESS("star_brightness");
    const std::string FIELD_SKY_MOON_ROTATION("moon_rotation");
    const std::string FIELD_SKY_MOON_AZIMUTH("moon_azimuth");
    const std::string FIELD_SKY_MOON_ELEVATION("moon_elevation");
    const std::string FIELD_REFLECTION_PROBE_AMBIANCE("probe_ambiance");
    const std::string BTN_RESET("btn_reset");

    const F32 SLIDER_SCALE_SUN_AMBIENT(3.0f);
    const F32 SLIDER_SCALE_BLUE_HORIZON_DENSITY(2.0f);
    const F32 SLIDER_SCALE_GLOW_R(20.0f);
    const F32 SLIDER_SCALE_GLOW_B(-5.0f);
    //const F32 SLIDER_SCALE_DENSITY_MULTIPLIER(0.001f);

    const S32 FLOATER_ENVIRONMENT_UPDATE(-2);
}

//=========================================================================
LLFloaterEnvironmentAdjust::LLFloaterEnvironmentAdjust(const LLSD &key):
    LLFloater(key)
{}

LLFloaterEnvironmentAdjust::~LLFloaterEnvironmentAdjust()
{}

//-------------------------------------------------------------------------
bool LLFloaterEnvironmentAdjust::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_AMBIENT_LIGHT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAmbientLightChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_BLUE_HORIZON)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueHorizonChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_BLUE_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeHorizonChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeDensityChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSceneGammaChanged(); });

    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudColorChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudCoverageChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScaleChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunColorChanged(); });

    getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onStarBrightnessChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunRotationChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_AZIMUTH)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunAzimElevChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_ELEVATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunAzimElevChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunScaleChanged(); });

    getChild<LLUICtrl>(FIELD_SKY_MOON_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonRotationChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MOON_AZIMUTH)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonAzimElevChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_MOON_ELEVATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonAzimElevChanged(); });
    getChild<LLUICtrl>(BTN_RESET)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonReset(); });

    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudMapChanged(); });
    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setDefaultImageAssetID(LLSettingsSky::GetDefaultCloudNoiseTextureId());
    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setAllowNoTexture(true);

    getChild<LLTextureCtrl>(FIELD_WATER_NORMAL_MAP)->setDefaultImageAssetID(LLSettingsWater::GetDefaultWaterNormalAssetId());
    getChild<LLTextureCtrl>(FIELD_WATER_NORMAL_MAP)->setBlankImageAssetID(BLANK_OBJECT_NORMAL);
    getChild<LLTextureCtrl>(FIELD_WATER_NORMAL_MAP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onWaterMapChanged(); });

    getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setCommitCallback([this](LLUICtrl*, const LLSD&) { onReflectionProbeAmbianceChanged(); });

    refresh();
    return true;
}

void LLFloaterEnvironmentAdjust::onOpen(const LLSD& key)
{
    if (!mLiveSky)
    {
        LLEnvironment::instance().saveBeaconsState();
    }
    captureCurrentEnvironment();

    mEventConnection = LLEnvironment::instance().setEnvironmentChanged([this](LLEnvironment::EnvSelection_t env, S32 version){ onEnvironmentUpdated(env, version); });

    // HACK -- resume reflection map manager because "setEnvironmentChanged" may pause it (SL-20456)
    gPipeline.mReflectionMapManager.resume();

    LLFloater::onOpen(key);
    refresh();
}

void LLFloaterEnvironmentAdjust::onClose(bool app_quitting)
{
    LLEnvironment::instance().revertBeaconsState();
    mEventConnection.disconnect();
    mLiveSky.reset();
    mLiveWater.reset();
    LLFloater::onClose(app_quitting);
}


//-------------------------------------------------------------------------
void LLFloaterEnvironmentAdjust::refresh()
{
    if (!mLiveSky || !mLiveWater)
    {
        setAllChildrenEnabled(false);
        return;
    }

    setEnabled(true);
    setAllChildrenEnabled(true);

    getChild<LLColorSwatchCtrl>(FIELD_SKY_AMBIENT_LIGHT)->set(mLiveSky->getAmbientColor() / SLIDER_SCALE_SUN_AMBIENT);
    getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_HORIZON)->set(mLiveSky->getBlueHorizon() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
    getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_DENSITY)->set(mLiveSky->getBlueDensity() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
    getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->setValue(mLiveSky->getHazeHorizon());
    getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->setValue(mLiveSky->getHazeDensity());
    getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->setValue(mLiveSky->getGamma());
    getChild<LLColorSwatchCtrl>(FIELD_SKY_CLOUD_COLOR)->set(mLiveSky->getCloudColor());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->setValue(mLiveSky->getCloudShadow());
    getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->setValue(mLiveSky->getCloudScale());
    getChild<LLColorSwatchCtrl>(FIELD_SKY_SUN_COLOR)->set(mLiveSky->getSunlightColor() / SLIDER_SCALE_SUN_AMBIENT);

    getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP)->setValue(mLiveSky->getCloudNoiseTextureId());
    getChild<LLTextureCtrl>(FIELD_WATER_NORMAL_MAP)->setValue(mLiveWater->getNormalMapID());

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setValue(mLiveSky->getReflectionProbeAmbiance(should_auto_adjust));

    LLColor3 glow(mLiveSky->getGlow());

    // takes 40 - 0.2 range -> 0 - 1.99 UI range
    getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->setValue(2.0 - (glow.mV[0] / SLIDER_SCALE_GLOW_R));
    getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->setValue(glow.mV[2] / SLIDER_SCALE_GLOW_B);
    getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->setValue(mLiveSky->getStarBrightness());
    getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE)->setValue(mLiveSky->getSunScale());

    // Sun rotation
    LLQuaternion quat = mLiveSky->getSunRotation();
    F32 azimuth;
    F32 elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);

    getChild<LLUICtrl>(FIELD_SKY_SUN_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_SUN_ELEVATION)->setValue(elevation);
    getChild<LLVirtualTrackball>(FIELD_SKY_SUN_ROTATION)->setRotation(quat);

    // Moon rotation
    quat = mLiveSky->getMoonRotation();
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);

    getChild<LLUICtrl>(FIELD_SKY_MOON_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_MOON_ELEVATION)->setValue(elevation);
    getChild<LLVirtualTrackball>(FIELD_SKY_MOON_ROTATION)->setRotation(quat);

    updateGammaLabel();
}


void LLFloaterEnvironmentAdjust::captureCurrentEnvironment()
{
    LLEnvironment &environment(LLEnvironment::instance());
    bool updatelocal(false);

    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        if (environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
        {   // We have a full day cycle in the local environment.  Freeze the sky
            mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL)->buildClone();
            mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL)->buildClone();
            updatelocal = true;
        }
        else
        {   // otherwise we can just use the sky.
            mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
            mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL);
        }
    }
    else
    {
        mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL, true)->buildClone();
        mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_PARCEL, true)->buildClone();
        updatelocal = true;
    }

    if (updatelocal)
    {
        environment.setEnvironment(LLEnvironment::ENV_LOCAL, mLiveSky, FLOATER_ENVIRONMENT_UPDATE);
        environment.setEnvironment(LLEnvironment::ENV_LOCAL, mLiveWater, FLOATER_ENVIRONMENT_UPDATE);
    }
    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
}

void LLFloaterEnvironmentAdjust::onButtonReset()
{
    LLNotificationsUtil::add("PersonalSettingsConfirmReset", LLSD(), LLSD(),
        [this](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            this->closeFloater();
            LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_LOCAL);
            LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
        }
    });

}
//-------------------------------------------------------------------------
void LLFloaterEnvironmentAdjust::onAmbientLightChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setAmbientColor(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_AMBIENT_LIGHT)->get() * SLIDER_SCALE_SUN_AMBIENT));
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onBlueHorizonChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setBlueHorizon(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_HORIZON)->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onBlueDensityChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setBlueDensity(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_DENSITY)->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onHazeHorizonChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setHazeHorizon((F32)getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON)->getValue().asReal());
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onHazeDensityChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setHazeDensity((F32)getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY)->getValue().asReal());
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onSceneGammaChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setGamma((F32)getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA)->getValue().asReal());
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onCloudColorChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setCloudColor(LLColor3(getChild<LLColorSwatchCtrl>(FIELD_SKY_CLOUD_COLOR)->get()));
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onCloudCoverageChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setCloudShadow((F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE)->getValue().asReal());
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onCloudScaleChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setCloudScale((F32)getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE)->getValue().asReal());
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onGlowChanged()
{
    if (!mLiveSky)
        return;
    LLColor3 glow((F32)getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE)->getValue().asReal(), 0.0f, (F32)getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS)->getValue().asReal());

    // takes 0 - 1.99 UI range -> 40 -> 0.2 range
    glow.mV[0] = (2.0f - glow.mV[0]) * SLIDER_SCALE_GLOW_R;
    glow.mV[2] *= SLIDER_SCALE_GLOW_B;

    mLiveSky->setGlow(glow);
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onStarBrightnessChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setStarBrightness((F32)getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS)->getValue().asReal());
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onSunRotationChanged()
{
    LLQuaternion quat = getChild<LLVirtualTrackball>(FIELD_SKY_SUN_ROTATION)->getRotation();
    F32 azimuth;
    F32 elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);
    getChild<LLUICtrl>(FIELD_SKY_SUN_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_SUN_ELEVATION)->setValue(elevation);
    if (mLiveSky)
    {
        mLiveSky->setSunRotation(quat);
        mLiveSky->update();
    }
}

void LLFloaterEnvironmentAdjust::onSunAzimElevChanged()
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

    if (mLiveSky)
    {
        mLiveSky->setSunRotation(quat);
        mLiveSky->update();
    }
}

void LLFloaterEnvironmentAdjust::onSunScaleChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setSunScale((F32)(getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE)->getValue().asReal()));
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onMoonRotationChanged()
{
    LLQuaternion quat = getChild<LLVirtualTrackball>(FIELD_SKY_MOON_ROTATION)->getRotation();
    F32 azimuth;
    F32 elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);
    getChild<LLUICtrl>(FIELD_SKY_MOON_AZIMUTH)->setValue(azimuth);
    getChild<LLUICtrl>(FIELD_SKY_MOON_ELEVATION)->setValue(elevation);
    if (mLiveSky)
    {
        mLiveSky->setMoonRotation(quat);
        mLiveSky->update();
    }
}

void LLFloaterEnvironmentAdjust::onMoonAzimElevChanged()
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
    az_quat.setAngleAxis(F_TWO_PI - azimuth, 0, 0, 1);
    quat *= az_quat;

    getChild<LLVirtualTrackball>(FIELD_SKY_MOON_ROTATION)->setRotation(quat);

    if (mLiveSky)
    {
        mLiveSky->setMoonRotation(quat);
        mLiveSky->update();
    }
}

void LLFloaterEnvironmentAdjust::onCloudMapChanged()
{
    if (!mLiveSky)
    {
        return;
    }

    LLTextureCtrl* picker_ctrl = getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP);

    LLUUID new_texture_id = picker_ctrl->getValue().asUUID();

    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);

    LLSettingsSky::ptr_t sky_to_set = mLiveSky->buildClone();
    if (!sky_to_set)
    {
        return;
    }

    sky_to_set->setCloudNoiseTextureId(new_texture_id);

    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, sky_to_set);

    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT, true);

    picker_ctrl->setValue(new_texture_id);
}

void LLFloaterEnvironmentAdjust::onWaterMapChanged()
{
    if (!mLiveWater)
        return;
    mLiveWater->setNormalMapID(getChild<LLTextureCtrl>(FIELD_WATER_NORMAL_MAP)->getValue().asUUID());
    mLiveWater->update();
}

void LLFloaterEnvironmentAdjust::onSunColorChanged()
{
    if (!mLiveSky)
        return;
    LLColor3 color(getChild<LLColorSwatchCtrl>(FIELD_SKY_SUN_COLOR)->get());

    color *= SLIDER_SCALE_SUN_AMBIENT;

    mLiveSky->setSunlightColor(color);
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onReflectionProbeAmbianceChanged()
{
    if (!mLiveSky) return;
    F32 ambiance = (F32)getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->getValue().asReal();
    mLiveSky->setReflectionProbeAmbiance(ambiance);

    updateGammaLabel();
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::updateGammaLabel()
{
    if (!mLiveSky) return;

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    F32 ambiance = mLiveSky->getReflectionProbeAmbiance(should_auto_adjust);
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

void LLFloaterEnvironmentAdjust::onEnvironmentUpdated(LLEnvironment::EnvSelection_t env, S32 version)
{
    if (env == LLEnvironment::ENV_LOCAL)
    {   // a new local environment has been applied
        if (version != FLOATER_ENVIRONMENT_UPDATE)
        {   // not by this floater
            captureCurrentEnvironment();
            refresh();
        }
    }
}
