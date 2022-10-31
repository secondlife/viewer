/**
* @file lldensityctrl.h
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

#ifndef LLDENSITY_CTRL_H
#define LLDENSITY_CTRL_H

#include "lluictrl.h"
#include "llsettingssky.h"
#include "lltextbox.h"
#include "llsliderctrl.h"

class LLDensityCtrl : public LLUICtrl
{
public:
    static const std::string DENSITY_RAYLEIGH;
    static const std::string DENSITY_MIE;
    static const std::string DENSITY_ABSORPTION;

    // Type of density profile this tab is updating
    enum DensityProfileType
    {
        Rayleigh,
        Mie,
        Absorption
    };

    static const std::string& NameForDensityProfileType(DensityProfileType t);

    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLTextBox::Params>         lbl_exponential,
            lbl_exponential_scale,
            lbl_linear,
            lbl_constant,
            lbl_max_altitude,
            lbl_aniso_factor;

        Optional<LLSliderCtrl::Params> exponential_slider,
        exponential_scale_slider,
        linear_slider,
        constant_slider,
        aniso_factor_slider;

        Optional<LLUIImage*> image_density_feedback;

        DensityProfileType profile_type;
        Params();
    };

    virtual BOOL            postBuild() override;
    virtual void            setEnabled(BOOL enabled) override;

    void setProfileType(DensityProfileType t) { mProfileType = t; }

    void refresh();
    void updateProfile();

    LLSettingsSky::ptr_t    getSky() const                                          { return mSkySettings; }
    void                    setSky(const LLSettingsSky::ptr_t &sky)                 { mSkySettings = sky; refresh(); }

protected:
    friend class LLUICtrlFactory;
    LLDensityCtrl(const Params&);

    LLSD getProfileConfig();
    void updatePreview();

private:
    void onExponentialChanged();
    void onExponentialScaleFactorChanged();
    void onLinearChanged();
    void onConstantChanged();
    void onMaxAltitudeChanged();
    void onAnisoFactorChanged();

    DensityProfileType    mProfileType = Rayleigh;
    LLUIImage*            mImgDensityFeedback = nullptr;
    LLSettingsSky::ptr_t  mSkySettings;
};

#endif LLDENSITY_CTRL_H
