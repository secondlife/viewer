/**
 * @file llfloatercombatsettings.h
 * @brief LLFloaterCombatSettings class header file
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#pragma once

#include "llregionflags.h"

struct LLRegionCombatSettings
{
    bool mSupportsCombat2 {false};
    bool mAllowDamage {false};
    U32 mCombatFlags {REGION_COMBAT_FLAG_DAMAGE_ADJUST | REGION_COMBAT_FLAG_RESTORE_HEALTH};
    U8 mOnDeath {0};
    F32 mDamageThrottle {0.f};
    F32 mRegenerationRate {0.1666f};
    F32 mInvulnerabilityTime {0.f};
    F32 mDamageLimit {0.f};
};

class LLFloaterCombatSettings : public LLFloater
{
    friend class LLFloaterReg;

public:
    bool postBuild() override;
    void onOpen(const LLSD& key) override;

    // Called when fresh region data arrives.
    void setSettings(const LLRegionCombatSettings& settings);
    void updateControlsEnabled();

private:
    LLFloaterCombatSettings(const LLSD& key);
    ~LLFloaterCombatSettings() {}

    void refreshControls();
    bool canModify() const;
    void onChangeAnything();
    void onChangeCombatEnabled();
    void onClickOk();

    LLRegionCombatSettings mSettings;

    LLUICtrl* mAllowDamageCheck {nullptr};
    LLUICtrl* mRestrictLogCheck {nullptr};
    LLUICtrl* mAllowDamageAdjustCheck {nullptr};
    LLUICtrl* mRestoreHealthCheck {nullptr};
    LLUICtrl* mOnDeathCombo {nullptr};
    LLUICtrl* mDamageThrottleSpin {nullptr};
    LLUICtrl* mHealthRegenSpin {nullptr};
    LLUICtrl* mInvulnerabilitySpin {nullptr};
    LLUICtrl* mDamageLimitSpin {nullptr};
    LLUICtrl* mOkBtn {nullptr};
    LLUICtrl* mCancelBtn {nullptr};
};
