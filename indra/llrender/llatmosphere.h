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

    LLGLTexture* getTransmittance() const;
    LLGLTexture* getScattering() const;
    LLGLTexture* getMieScattering() const;

    GLhandleARB getAtmosphericShaderForLink() const;

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
    LLPointer<LLGLTexture> m_mie_scattering;
};

extern LLAtmosphere* gAtmosphere;

#endif // LL_ATMOSPHERE_H
