/** 
 * @file llatmosphere.h
 * @brief LLAtmosphere class for integration with libatmosphere
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
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

#ifndef LL_ATMOSPHERE_H
#define LL_ATMOSPHERE_H

#include "llglheaders.h"
#include "llgltexture.h"
#include "libatmosphere/model.h"

typedef std::vector<atmosphere::DensityProfileLayer> DensityProfile;

class AtmosphericModelSettings
{
public:
    AtmosphericModelSettings();

    AtmosphericModelSettings(
        DensityProfile& rayleighProfile,
        DensityProfile& mieProfile,
        DensityProfile& absorptionProfile);

    AtmosphericModelSettings(
        F32             skyBottomRadius,
        F32             skyTopRadius,
        DensityProfile& rayleighProfile,
        DensityProfile& mieProfile,
        DensityProfile& absorptionProfile,
        F32             sunArcRadians,
        F32             mieAniso);

    bool operator==(const AtmosphericModelSettings& rhs) const;

    F32             m_skyBottomRadius;
    F32             m_skyTopRadius;
    DensityProfile  m_rayleighProfile;
    DensityProfile  m_mieProfile;
    DensityProfile  m_absorptionProfile;
    F32             m_sunArcRadians;
    F32             m_mieAnisotropy;
};

class LLAtmosphere
{
public:
     LLAtmosphere();
    ~LLAtmosphere();

    static void initClass();
    static void cleanupClass();

    const LLAtmosphere& operator=(const LLAtmosphere& rhs)
    {
        LL_ERRS() << "Illegal operation!" << LL_ENDL;
        return *this;
    }

    LLGLTexture* getTransmittance();
    LLGLTexture* getScattering();
    LLGLTexture* getMieScattering();
    LLGLTexture* getIlluminance();

    GLhandleARB getAtmosphericShaderForLink() const;

    bool configureAtmosphericModel(AtmosphericModelSettings& settings);

protected:    
    LLAtmosphere(const LLAtmosphere& rhs)
    {
        *this = rhs;
    }

    atmosphere::ModelConfig         m_config;    
    atmosphere::PrecomputedTextures m_textures;
    atmosphere::Model*              m_model = nullptr;

    LLPointer<LLGLTexture> m_transmittance;
    LLPointer<LLGLTexture> m_scattering;
    LLPointer<LLGLTexture> m_mie_scatter_texture;
    LLPointer<LLGLTexture> m_illuminance;

    std::vector<double> m_wavelengths;
    std::vector<double> m_solar_irradiance;
    std::vector<double> m_rayleigh_scattering;
    std::vector<double> m_mie_scattering;
    std::vector<double> m_mie_extinction;
    std::vector<double> m_absorption_extinction;
    std::vector<double> m_ground_albedo;

    AtmosphericModelSettings m_settings;
};

extern LLAtmosphere* gAtmosphere;

#endif // LL_ATMOSPHERE_H
