/** 
 * @file llfloaterenvironmentadjust.h
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

#ifndef LL_FLOATERENVIRONMENTADJUST_H
#define LL_FLOATERENVIRONMENTADJUST_H

#include "llfloater.h"
#include "llsettingsbase.h"
#include "llsettingssky.h"
#include "llenvironment.h"

#include "boost/signals2.hpp"

class LLButton;
class LLLineEditor;

/**
 * Floater container for taking a snapshot of the current environment and making minor adjustments.
 */
class LLFloaterEnvironmentAdjust : public LLFloater
{
    LOG_CLASS(LLFloaterEnvironmentAdjust);

public:
                                LLFloaterEnvironmentAdjust(const LLSD &key);
    virtual                     ~LLFloaterEnvironmentAdjust();


    virtual BOOL                postBuild() override;
    virtual void                onOpen(const LLSD& key) override;
    virtual void                onClose(bool app_quitting) override;

    virtual void                refresh() override;

private:
    void                        captureCurrentEnvironment();

    void                        onAmbientLightChanged();
    void                        onBlueHorizonChanged();
    void                        onBlueDensityChanged();
    void                        onHazeHorizonChanged();
    void                        onHazeDensityChanged();
    void                        onSceneGammaChanged();

    void                        onCloudColorChanged();
    void                        onCloudCoverageChanged();
    void                        onCloudScaleChanged();
    void                        onSunColorChanged();

    void                        onGlowChanged();
    void                        onStarBrightnessChanged();
    void                        onSunRotationChanged();
    void                        onSunScaleChanged();

    void                        onMoonRotationChanged();

    void                        onCloudMapChanged();
    void                        onWaterMapChanged();

    void                        onButtonReset();

    void                        onEnvironmentUpdated(LLEnvironment::EnvSelection_t env, S32 version);

    LLSettingsSky::ptr_t        mLiveSky;
    LLSettingsWater::ptr_t      mLiveWater;
    LLEnvironment::connection_t mEventConnection;
};

#endif // LL_FLOATERFIXEDENVIRONMENT_H
