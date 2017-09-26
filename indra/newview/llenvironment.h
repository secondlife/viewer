/**
 * @file llenvmanager.h
 * @brief Declaration of classes managing WindLight and water settings.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_ENVIRONMENT_H
#define LL_ENVIRONMENT_H

#include "llmemory.h"
#include "llsd.h"

#include "llsettingssky.h"

class LLViewerCamera;
class LLGLSLShader;

//-------------------------------------------------------------------------
class LLEnvironment : public LLSingleton<LLEnvironment>
{
    LLSINGLETON(LLEnvironment);
    LOG_CLASS(LLEnvironment);

public:
    virtual ~LLEnvironment();

    LLSettingsSky::ptr_t    getCurrentSky() const { return mCurrentSky; }

    void                    update(const LLViewerCamera * cam);

    LLVector4               getRotatedLightDir() const { return mRotatedLight; }

    void                    updateGLVariablesForSettings(LLGLSLShader *shader, const LLSettingsBase::ptr_t &psetting);
    void                    updateShaderUniforms(LLGLSLShader *shader);

    void                    addSky(const LLSettingsSky::ptr_t &sky);

    inline LLVector2        getCloudScrollDelta() const { return mCloudScrollDelta; }

private:
    static const F32        SUN_DELTA_YAW;

    typedef std::map<std::string, LLSettingsSky::ptr_t> NamedSkyMap_t;
    typedef std::map<LLUUID, LLSettingsSky::ptr_t> AssetSkyMap_t;

    LLVector4               mRotatedLight;
    LLVector2               mCloudScrollDelta;  // cumulative cloud delta

    LLSettingsSky::ptr_t    mCurrentSky;

    NamedSkyMap_t           mSkysByName;
    AssetSkyMap_t           mSkysById;

    void addSky(const LLUUID &id, const LLSettingsSky::ptr_t &sky);
    void removeSky(const std::string &name);
    void removeSky(const LLUUID &id);
    void clearAllSkys();

    void updateCloudScroll();
};


#endif // LL_ENVIRONMENT_H

