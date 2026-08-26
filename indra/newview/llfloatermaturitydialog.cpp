/**
 * @file llfloatermaturitydialog.cpp
 * @brief LLFloaterMaturityDialog class implementation
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

#include "llfloatermaturitydialog.h"

#include "llbutton.h"
#include "llcombobox.h"
#include "llagent.h"
#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"

LLFloaterMaturityDialog::LLFloaterMaturityDialog(const LLSD& key)
    : LLModalDialog(key, true)
{
}

bool LLFloaterMaturityDialog::postBuild()
{
    setCanDrag(false);
    LLComboBox* combo = getChild<LLComboBox>("maturity_combo");
    combo->setCommitCallback([this](LLUICtrl*, const LLSD&) { updateContinueButton(); });
    // Ensure the dropdown list renders on top of buttons and labels below the combo
    sendChildToFront(combo);
    getChild<LLButton>("continue_btn")->setCommitCallback(
        [this](LLUICtrl*, const LLSD&) { onContinue(); });
    getChild<LLButton>("cancel_btn")->setCommitCallback(
        [this](LLUICtrl*, const LLSD&) { onCancel(); });

    return true;
}

void LLFloaterMaturityDialog::onOpen(const LLSD& key)
{
    LLModalDialog::onOpen(key);

    mPreviousMaturity = gSavedSettings.getU32("PreferredMaturity");
    mRegionAccess = static_cast<U8>(key.asInteger());

    LLStringUtil::format_map_t args;
    args["[MATURITY]"] = LLViewerRegion::accessToString(mRegionAccess);
    getChild<LLTextBox>("location_rated_lbl")->setText(getString("location_rated_string", args));
    getChild<LLTextBox>("include_adult_lbl")->setText(getString("update_maturity_string", args));

    updateContinueButton();
    centerOnScreen();
}

void LLFloaterMaturityDialog::onContinue()
{
    gAgent.restartFailedTeleportRequest();
    closeFloater();
}

void LLFloaterMaturityDialog::onCancel()
{
    gSavedSettings.setU32("PreferredMaturity", mPreviousMaturity);
    gAgent.clearTeleportRequest();
    closeFloater();
}

void LLFloaterMaturityDialog::updateContinueButton()
{
    U32 current = gSavedSettings.getU32("PreferredMaturity");
    getChild<LLButton>("continue_btn")->setEnabled(current >= mRegionAccess);
}
