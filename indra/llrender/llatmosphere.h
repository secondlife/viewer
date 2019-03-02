/** 
 * @file llatmosphere.h
 * @brief LLAtmosphere class
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

// An atmosphere layer of width 'width' (in m), and whose density is defined as
//   'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
// clamped to [0,1], and where h is the altitude (in m). 'exp_term' and
// 'constant_term' are unitless, while 'exp_scale' and 'linear_term' are in
// m^-1.
class DensityLayer {
 public:
  DensityLayer()
    : width(0.0f)
    , exp_term(0.0f)
    , exp_scale(0.0f)
    , linear_term(0.0f)
    , constant_term(0.0f)
  {
  }

  DensityLayer(float width, float exp_term, float exp_scale, float linear_term, float constant_term)
    : width(width)
    , exp_term(exp_term)
    , exp_scale(exp_scale)
    , linear_term(linear_term)
    , constant_term(constant_term)
  {
  }

  bool operator==(const DensityLayer& rhs) const
  {
      if (width != rhs.width)
      {
         return false;
      }

      if (exp_term != rhs.exp_term)
      {
         return false;
      }

      if (exp_scale != rhs.exp_scale)
      {
         return false;
      }

      if (linear_term != rhs.linear_term)
      {
         return false;
      }

      if (constant_term != rhs.constant_term)
      {
         return false;
      }

      return true;
  }

  float width         = 1024.0f;
  float exp_term      = 1.0f;
  float exp_scale     = 1.0f;
  float linear_term   = 1.0f;
  float constant_term = 0.0f;
};

typedef std::vector<DensityLayer> DensityProfile;

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

    bool configureAtmosphericModel(AtmosphericModelSettings& settings);

protected:    
    LLAtmosphere(const LLAtmosphere& rhs)
    {
        *this = rhs;
    }

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
