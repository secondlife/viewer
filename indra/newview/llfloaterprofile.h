/**
 * @file llfloaterprofile.h
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

#ifndef LL_LLFLOATERPROFILE_H
#define LL_LLFLOATERPROFILE_H

#include "llavatarnamecache.h"
#include "llavatarpropertiesprocessor.h"
#include "llfloater.h"

class LLPanelProfile;

class LLFloaterProfile : public LLFloater
{
    LOG_CLASS(LLFloaterProfile);
public:
    LLFloaterProfile(const LLSD& key);
    virtual ~LLFloaterProfile();

    bool postBuild() override;

    void onOpen(const LLSD& key) override;
    void onClickCloseBtn(bool app_quitting = false) override;
    void onUnsavedChangesCallback(const LLSD& notification, const LLSD& response, bool can_save);

    void createPick(const LLPickData &data);
    void showPick(const LLUUID& pick_id = LLUUID::null);
    bool isPickTabSelected();
    void refreshName();

    void showClassified(const LLUUID& classified_id = LLUUID::null, bool edit = false);
    void createClassified();

private:
    LLAvatarNameCache::callback_connection_t mNameCallbackConnection;
    void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

    LLPanelProfile* mPanelProfile;

    LLUUID mAvatarId;
};

#endif // LL_LLFLOATERPROFILE_H
