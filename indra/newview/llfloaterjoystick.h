/**
 * @file llfloaterjoystick.h
 * @brief Joystick preferences panel
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include <array>
#include "llfloater.h"
#include "llstatview.h"

class LLCheckBoxCtrl;
class LLComboBox;

class LLFloaterJoystick : public LLFloater
{
    friend class LLFloaterReg;

public:

    virtual bool postBuild();
    virtual void refresh();
    virtual void apply();   // Apply the changed values.
    virtual void cancel();  // Cancel the changed values.
    virtual void draw();
    static  void setSNDefaults();

    static bool addDeviceCallback(std::string &name, LLSD& value, void* userdata);
    void addDevice(std::string &name, LLSD& value);

protected:

    void refreshListOfDevices();
    void onClose(bool app_quitting);
    void onClickCloseBtn(bool app_quitting);

private:

    explicit LLFloaterJoystick(const LLSD& data);
    virtual ~LLFloaterJoystick();

    void initFromSettings();

    static void onCommitJoystickEnabled(LLUICtrl*, void*);
    static void onClickRestoreSNDefaults(void*);
    static void onClickCancel(void*);
    static void onClickOK(void*);

private:
    // Device prefs
    bool mJoystickEnabled;
    LLSD mJoystickId;
    std::array<S32, 7> mJoystickAxis;
    bool m3DCursor;
    bool mAutoLeveling;
    bool mZoomDirect;

    // Modes prefs
    bool mAvatarEnabled;
    bool mBuildEnabled;
    bool mFlycamEnabled;
    std::array<F32, 6> mAvatarAxisScale;
    std::array<F32, 6> mBuildAxisScale;
    std::array<F32, 7> mFlycamAxisScale;
    std::array<F32, 6> mAvatarAxisDeadZone;
    std::array<F32, 6> mBuildAxisDeadZone;
    std::array<F32, 7> mFlycamAxisDeadZone;
    F32 mAvatarFeathering;
    F32 mBuildFeathering;
    F32 mFlycamFeathering;

    // Controls that can disable the flycam
    LLCheckBoxCtrl  *mCheckFlycamEnabled;
    LLComboBox      *mJoysticksCombo;

    bool mHasDeviceList;
    bool mJoystickInitialized;
    LLUUID mCurrentDeviceId;

    // stats view
    std::array<LLStatBar*, 6> mAxisStatsBar;
};

