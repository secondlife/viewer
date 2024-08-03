/**
 * @file llfloaterregionrestartschedule.h
 * @author Andrii Kleshchev
 * @brief LLFloaterRegionRestartSchedule class header file
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#ifndef LL_LLFLOATERREGIONRESTARTSCHEDULE_H
#define LL_LLFLOATERREGIONRESTARTSCHEDULE_H

#include "llfloater.h"

class LLLineEditor;
class LLButton;

class LLFloaterRegionRestartSchedule : public LLFloater
{
public:
    LLFloaterRegionRestartSchedule(LLView* owner);

    virtual ~LLFloaterRegionRestartSchedule();

    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void draw() override;

    void onPMAMButtonClicked();
    void onSaveButtonClicked();

    void onCommitHours(const LLSD& value);
    void onCommitMinutes(const LLSD& value);

    void resetUI(bool enable_ui);
    void updateAMPM();

    static bool canUse();

protected:
    static void requestRegionShcheduleCoro(std::string url, LLHandle<LLFloater> handle);
    static void setRegionShcheduleCoro(std::string url, LLSD body, LLHandle<LLFloater> handle);

    LLHandle<LLView> mOwnerHandle;
    F32 mContextConeOpacity{ 0.f };

    LLLineEditor* mHoursLineEditor{nullptr};
    LLLineEditor* mMinutesLineEditor{ nullptr };
    LLButton* mPMAMButton{ nullptr };
    LLButton* mSaveButton{ nullptr };
    LLButton* mCancelButton{ nullptr };

    bool mTimeAM{ true };
};

#endif  // LL_LLFLOATERREGIONRESTARTSCHEDULE_H
