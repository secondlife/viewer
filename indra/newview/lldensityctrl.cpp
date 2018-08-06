/**
* @file lldensityctrl.cpp
* @brief Control for specifying density over a height range for sky settings.
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

#include "lldensityctrl.h"

#include "llslider.h"
#include "llsliderctrl.h"
#include "llsettingssky.h"

static LLDefaultChildRegistry::Register<LLDensityCtrl> register_density_control("densityctrl");

const std::string LLDensityCtrl::DENSITY_RAYLEIGH("density_rayleigh");
const std::string LLDensityCtrl::DENSITY_MIE("density_mie");
const std::string LLDensityCtrl::DENSITY_ABSORPTION("density_absorption");

namespace
{   
    const std::string   FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL("level_exponential");
    const std::string   FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL_SCALE("exponential_scale");
    const std::string   FIELD_SKY_DENSITY_PROFILE_LINEAR("level_linear");
    const std::string   FIELD_SKY_DENSITY_PROFILE_CONSTANT("level_constant");
    const std::string   FIELD_SKY_DENSITY_MAX_ALTITUDE("max_altitude");
    const std::string   FIELD_SKY_DENSITY_ANISO_FACTOR("aniso_factor");
    const std::string   FIELD_SKY_DENSITY_ANISO_FACTOR_LABEL("aniso_factor_label");
}

const std::string& LLDensityCtrl::NameForDensityProfileType(DensityProfileType t)
{
    switch (t)
    {
        case Rayleigh:   return DENSITY_RAYLEIGH;
        case Mie:        return DENSITY_MIE;
        case Absorption: return DENSITY_ABSORPTION;
        default:
            break;
    }

    llassert(false);
    return DENSITY_RAYLEIGH;
}

LLDensityCtrl::Params::Params()
: image_density_feedback("image_density_feedback")
, lbl_exponential("label_exponential")
, lbl_exponential_scale("label_exponential_scale")
, lbl_linear("label_linear")
, lbl_constant("label_constant")
, lbl_max_altitude("label_max_altitude")
, lbl_aniso_factor("label_aniso_factor")
, profile_type(LLDensityCtrl::Rayleigh)
{
}

LLDensityCtrl::LLDensityCtrl(const Params& params)
: mProfileType(params.profile_type)
, mImgDensityFeedback(params.image_density_feedback)
{

}

LLSD LLDensityCtrl::getProfileConfig()
{
    LLSD config;
    switch (mProfileType)
    {
        case Rayleigh:   return mSkySettings->getRayleighConfigs();
        case Mie:        return mSkySettings->getMieConfigs();
        case Absorption: return mSkySettings->getAbsorptionConfigs();
        default:
            break;
    }
    llassert(false);
    return config;
}

BOOL LLDensityCtrl::postBuild()
{
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onExponentialChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onExponentialScaleFactorChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_LINEAR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onLinearChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_CONSTANT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onConstantChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MAX_ALTITUDE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMaxAltitudeChanged(); });
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_ANISO_FACTOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAnisoFactorChanged(); });

    if (mProfileType != Mie)
    {
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ANISO_FACTOR_LABEL)->setValue(false);
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ANISO_FACTOR)->setVisible(false);
    }

    return TRUE;
}

void LLDensityCtrl::setEnabled(BOOL enabled)
{
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL_SCALE)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_LINEAR)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_PROFILE_CONSTANT)->setEnabled(enabled);
    getChild<LLUICtrl>(FIELD_SKY_DENSITY_MAX_ALTITUDE)->setEnabled(enabled);

    if (mProfileType == Mie)
    {
        getChild<LLUICtrl>(FIELD_SKY_DENSITY_ANISO_FACTOR)->setEnabled(enabled);
    }
}

void LLDensityCtrl::refresh()
{
    if (!mSkySettings)
    {
        setAllChildrenEnabled(FALSE);
        setEnabled(FALSE);
        return;
    }

    setEnabled(TRUE);
    setAllChildrenEnabled(TRUE);

    LLSD config = getProfileConfig();

    getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL)->setValue(config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM]);
    getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL_SCALE)->setValue(config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR]);
    getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_LINEAR)->setValue(config[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM]);
    getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_CONSTANT)->setValue(config[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM]);
    getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MAX_ALTITUDE)->setValue(config[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH]);

    if (mProfileType == Mie)
    {        
        getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ANISO_FACTOR)->setValue(config[LLSettingsSky::SETTING_MIE_ANISOTROPY_FACTOR]);
    }
}

void LLDensityCtrl::updateProfile()
{
    F32 exponential_term  = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL)->getValueF32();
    F32 exponential_scale = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_EXPONENTIAL_SCALE)->getValueF32();
    F32 linear_term       = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_LINEAR)->getValueF32();
    F32 constant_term     = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_PROFILE_CONSTANT)->getValueF32();
    F32 max_alt           = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MAX_ALTITUDE)->getValueF32();
    F32 aniso_factor      = 0.0f;

    if (mProfileType == Mie)
    {
        aniso_factor = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ANISO_FACTOR)->getValueF32();
    }

    LLSD profile = LLSettingsSky::createSingleLayerDensityProfile(max_alt, exponential_term, exponential_scale, linear_term, constant_term, aniso_factor);

    switch (mProfileType)
    {
        case Rayleigh: mSkySettings->setRayleighConfigs(profile); break;
        case Mie: mSkySettings->setMieConfigs(profile); break;
        case Absorption: mSkySettings->setAbsorptionConfigs(profile); break;
        default:
            break;
    }
}

void LLDensityCtrl::onExponentialChanged()
{
    updateProfile();
    updatePreview();
}

void LLDensityCtrl::onExponentialScaleFactorChanged()
{
    updateProfile();
    updatePreview();
}

void LLDensityCtrl::onLinearChanged()
{
    updateProfile();
    updatePreview();
}

void LLDensityCtrl::onConstantChanged()
{
    updateProfile();
    updatePreview();
}

void LLDensityCtrl::onMaxAltitudeChanged()
{
    updateProfile();
    updatePreview();
}

void LLDensityCtrl::onAnisoFactorChanged()
{
    updateProfile();
}

void LLDensityCtrl::updatePreview()
{
    // AdvancedAtmospherics TODO
    // Generate image according to current density profile
}

