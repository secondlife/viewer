/**
 * @file llfloatercombatsettings.cpp
 * @brief LLFloaterCombatSettings class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llfloatercombatsettings.h"

#include "llagent.h"
#include "llcorehttputil.h"
#include "llviewerregion.h"

LLFloaterCombatSettings::LLFloaterCombatSettings(const LLSD& key)
:   LLFloater(key)
{
}

bool LLFloaterCombatSettings::postBuild()
{
    mAllowDamageCheck = getChild<LLUICtrl>("allow_damage_check");
    mRestrictLogCheck = getChild<LLUICtrl>("combat_restrict_log");
    mAllowDamageAdjustCheck = getChild<LLUICtrl>("combat_allow_damage_adjust");
    mRestoreHealthCheck = getChild<LLUICtrl>("combat_restore_health");
    mOnDeathCombo = getChild<LLUICtrl>("combat_on_death");
    mDamageThrottleSpin = getChild<LLUICtrl>("combat_dps_spin");
    mHealthRegenSpin = getChild<LLUICtrl>("combat_hps_spin");
    mInvulnerabilitySpin = getChild<LLUICtrl>("combat_invuln_spin");
    mDamageLimitSpin = getChild<LLUICtrl>("combat_damage_limit");
    mOkBtn = getChild<LLUICtrl>("ok_btn");
    mCancelBtn = getChild<LLUICtrl>("cancel_btn");

    mAllowDamageCheck->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeCombatEnabled, this));
    mRestrictLogCheck->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));
    mAllowDamageAdjustCheck->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));
    mRestoreHealthCheck->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));
    mOnDeathCombo->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));
    mDamageThrottleSpin->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));
    mHealthRegenSpin->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));
    mInvulnerabilitySpin->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));
    mDamageLimitSpin->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onChangeAnything, this));

    mOkBtn->setCommitCallback(boost::bind(&LLFloaterCombatSettings::onClickOk, this));
    mCancelBtn->setCommitCallback(boost::bind(&LLFloater::closeFloater, this, false));

    return true;
}

void LLFloaterCombatSettings::onOpen(const LLSD& key)
{
    refreshControls();
}

void LLFloaterCombatSettings::setSettings(const LLRegionCombatSettings& settings)
{
    mSettings = settings;
    refreshControls();
}

bool LLFloaterCombatSettings::canModify() const
{
    LLViewerRegion* region = gAgent.getRegion();
    return gAgent.isGodlike() || (region && region->canManageEstate());
}

void LLFloaterCombatSettings::refreshControls()
{
    mAllowDamageCheck->setValue(mSettings.mAllowDamage);
    mRestrictLogCheck->setValue(is_flag_set(mSettings.mCombatFlags, REGION_COMBAT_FLAG_RESTRICT_LOG));
    mAllowDamageAdjustCheck->setValue(is_flag_set(mSettings.mCombatFlags, REGION_COMBAT_FLAG_DAMAGE_ADJUST));
    mRestoreHealthCheck->setValue(is_flag_set(mSettings.mCombatFlags, REGION_COMBAT_FLAG_RESTORE_HEALTH));
    mOnDeathCombo->setValue((S32)mSettings.mOnDeath);
    mDamageThrottleSpin->setValue(mSettings.mDamageThrottle);
    mHealthRegenSpin->setValue(mSettings.mRegenerationRate);
    mInvulnerabilitySpin->setValue(mSettings.mInvulnerabilityTime);
    mDamageLimitSpin->setValue(mSettings.mDamageLimit);

    updateControlsEnabled();
    mOkBtn->setEnabled(false);
}

void LLFloaterCombatSettings::updateControlsEnabled()
{
    bool allow_modify = canModify();
    bool combat2_enabled = allow_modify
        && mAllowDamageCheck->getValue().asBoolean()
        && mSettings.mSupportsCombat2;

    mAllowDamageCheck->setEnabled(allow_modify);
    mRestrictLogCheck->setEnabled(combat2_enabled);
    mAllowDamageAdjustCheck->setEnabled(combat2_enabled);
    mRestoreHealthCheck->setEnabled(combat2_enabled);
    mOnDeathCombo->setEnabled(combat2_enabled);
    mDamageThrottleSpin->setEnabled(combat2_enabled);
    mHealthRegenSpin->setEnabled(combat2_enabled);
    mInvulnerabilitySpin->setEnabled(combat2_enabled);
    mDamageLimitSpin->setEnabled(combat2_enabled);
}

void LLFloaterCombatSettings::onChangeCombatEnabled()
{
    updateControlsEnabled();
    onChangeAnything();
}

void LLFloaterCombatSettings::onChangeAnything()
{
    mOkBtn->setEnabled(canModify());
}

void LLFloaterCombatSettings::onClickOk()
{
    LLRegionCombatSettings settings = mSettings;

    settings.mAllowDamage = mAllowDamageCheck->getValue().asBoolean();

    U32 flags = 0;
    if (mRestrictLogCheck->getValue().asBoolean())
    {
        flags |= REGION_COMBAT_FLAG_RESTRICT_LOG;
    }
    if (mAllowDamageAdjustCheck->getValue().asBoolean())
    {
        flags |= REGION_COMBAT_FLAG_DAMAGE_ADJUST;
    }
    if (mRestoreHealthCheck->getValue().asBoolean())
    {
        flags |= REGION_COMBAT_FLAG_RESTORE_HEALTH;
    }
    settings.mCombatFlags = flags;
    settings.mOnDeath = (U8)mOnDeathCombo->getValue().asInteger();
    settings.mDamageThrottle = (F32)mDamageThrottleSpin->getValue().asReal();
    settings.mRegenerationRate = (F32)mHealthRegenSpin->getValue().asReal();
    settings.mInvulnerabilityTime = (F32)mInvulnerabilitySpin->getValue().asReal();
    settings.mDamageLimit = (F32)mDamageLimitSpin->getValue().asReal();

    std::string url = gAgent.getRegionCapability("DispatchRegionInfo");
    if (!url.empty())
    {
        LLSD body;
        body["allow_damage"] = settings.mAllowDamage;
        if (settings.mSupportsCombat2)
        {
            body["combat_flags"] = LLSD::Integer(settings.mCombatFlags);
            body["combat_on_death"] = LLSD::Integer(settings.mOnDeath);
            body["combat_dps_throttle"] = settings.mDamageThrottle;
            body["combat_hps_rate"] = settings.mRegenerationRate;
            body["combat_invuln_time"] = settings.mInvulnerabilityTime;
            body["combat_damage_limit"] = settings.mDamageLimit;
        }

        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, body,
            "Combat settings update posted.", "Combat settings update not posted.");
    }
    closeFloater();
}
