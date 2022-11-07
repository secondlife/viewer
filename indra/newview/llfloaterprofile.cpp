/**
 * @file llfloaterprofile.cpp
 * @brief Avatar profile floater.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "llfloaterprofile.h"

#include "llagent.h" //gAgent
#include "llnotificationsutil.h"
#include "llpanelavatar.h"
#include "llpanelprofile.h"

static const std::string PANEL_PROFILE_VIEW = "panel_profile_view";

LLFloaterProfile::LLFloaterProfile(const LLSD& key)
 : LLFloater(key),
 mAvatarId(key["id"].asUUID()),
 mNameCallbackConnection()
{
    mDefaultRectForGroup = false;
}

LLFloaterProfile::~LLFloaterProfile()
{
    if (mNameCallbackConnection.connected())
    {
        mNameCallbackConnection.disconnect();
    }
}

void LLFloaterProfile::onOpen(const LLSD& key)
{
    mPanelProfile->onOpen(key);

    // Update the avatar name.
    mNameCallbackConnection = LLAvatarNameCache::get(mAvatarId, boost::bind(&LLFloaterProfile::onAvatarNameCache, this, _1, _2));
}

BOOL LLFloaterProfile::postBuild()
{
    mPanelProfile = findChild<LLPanelProfile>(PANEL_PROFILE_VIEW);

    return TRUE;
}

void LLFloaterProfile::onClickCloseBtn(bool app_quitting)
{
    if (!app_quitting)
    {
        if (mPanelProfile->hasUnpublishedClassifieds())
        {
            LLNotificationsUtil::add("ProfileUnpublishedClassified", LLSD(), LLSD(),
                boost::bind(&LLFloaterProfile::onUnsavedChangesCallback, this, _1, _2, false));
        }
        else if (mPanelProfile->hasUnsavedChanges())
        {
            LLNotificationsUtil::add("ProfileUnsavedChanges", LLSD(), LLSD(),
                boost::bind(&LLFloaterProfile::onUnsavedChangesCallback, this, _1, _2, true));
        }
        else
        {
            closeFloater();
        }
    }
    else
    {
        closeFloater();
    }
}

void LLFloaterProfile::onUnsavedChangesCallback(const LLSD& notification, const LLSD& response, bool can_save)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (can_save)
    {
        // savable content

        if (option == 0) // Save
        {
            mPanelProfile->commitUnsavedChanges();
            closeFloater();
        }
        if (option == 1) // Discard
        {
            closeFloater();
        }
        // else cancel
    }
    else
    {
        // classifieds

        if (option == 0) // Ok
        {
            closeFloater();
        }
        // else cancel
    }

}

void LLFloaterProfile::createPick(const LLPickData &data)
{
    mPanelProfile->createPick(data);
}

void LLFloaterProfile::showPick(const LLUUID& pick_id)
{
    mPanelProfile->showPick(pick_id);
}

bool LLFloaterProfile::isPickTabSelected()
{
    return mPanelProfile->isPickTabSelected();
}

void LLFloaterProfile::refreshName()
{
    if (!mNameCallbackConnection.connected())
    {
        mNameCallbackConnection = LLAvatarNameCache::get(mAvatarId, boost::bind(&LLFloaterProfile::onAvatarNameCache, this, _1, _2));
    }

    LLPanelProfileSecondLife *panel = findChild<LLPanelProfileSecondLife>("panel_profile_secondlife");
    if (panel)
    {
        panel->refreshName();
    }
}

void LLFloaterProfile::showClassified(const LLUUID& classified_id, bool edit)
{
    mPanelProfile->showClassified(classified_id, edit);
}

void LLFloaterProfile::createClassified()
{
    mPanelProfile->createClassified();
}

void LLFloaterProfile::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    mNameCallbackConnection.disconnect();
    setTitle(av_name.getCompleteName());
}

// eof
