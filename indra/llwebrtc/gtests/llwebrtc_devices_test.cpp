/**
 * @file llwebrtc_devices_test.cpp
 * @brief Tests for llwebrtc's device interface.
 *
 * These deliberately avoid anything that opens an audio device (voice
 * enable, tuning mode, unmute), so they run on build machines with no sound
 * hardware and never trigger an OS microphone-permission prompt.  Device
 * enumeration may legitimately return empty lists there.
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

#include "llwebrtc_test_support.h"

#include <cmath>

using llwebrtc_test::RecordingDevicesObserver;

namespace
{

class LLWebRTCDevicesTest : public testing::Test
{
protected:
    void SetUp() override
    {
        mDevices = llwebrtc::getDeviceInterface();
        ASSERT_NE(mDevices, nullptr);
    }

    llwebrtc::LLWebRTCDeviceInterface* mDevices{ nullptr };
};

} // namespace

TEST_F(LLWebRTCDevicesTest, RefreshNotifiesObserver)
{
    RecordingDevicesObserver observer;
    mDevices->setDevicesObserver(&observer);

    mDevices->refreshDevices();
    bool notified = observer.mCalls.waitFor(1);

    // Unregister before 'observer' goes out of scope, even if we failed.
    mDevices->unsetDevicesObserver(&observer);

    ASSERT_TRUE(notified) << "OnDevicesChanged was not called after refreshDevices()";

    // Every device the viewer is told about must be selectable by id.
    for (const auto& device : observer.renderDevices())
    {
        EXPECT_FALSE(device.mID.empty()) << "render device '" << device.mDisplayName << "' has no id";
    }
    for (const auto& device : observer.captureDevices())
    {
        EXPECT_FALSE(device.mID.empty()) << "capture device '" << device.mDisplayName << "' has no id";
    }
}

TEST_F(LLWebRTCDevicesTest, AllObserversAreNotified)
{
    RecordingDevicesObserver first;
    RecordingDevicesObserver second;
    mDevices->setDevicesObserver(&first);
    mDevices->setDevicesObserver(&second);

    mDevices->refreshDevices();
    bool first_notified  = first.mCalls.waitFor(1);
    bool second_notified = second.mCalls.waitFor(1);

    mDevices->unsetDevicesObserver(&first);
    mDevices->unsetDevicesObserver(&second);

    EXPECT_TRUE(first_notified);
    EXPECT_TRUE(second_notified);
}

TEST_F(LLWebRTCDevicesTest, UnsetObserverIsNoLongerNotified)
{
    RecordingDevicesObserver removed;
    RecordingDevicesObserver kept;
    mDevices->setDevicesObserver(&removed);
    mDevices->setDevicesObserver(&kept);

    mDevices->refreshDevices();
    ASSERT_TRUE(removed.mCalls.waitFor(1));
    ASSERT_TRUE(kept.mCalls.waitFor(1));

    mDevices->unsetDevicesObserver(&removed);
    const int removed_calls = removed.mCalls.count();
    const int kept_calls    = kept.mCalls.count();

    // Observers are notified in a single pass on the worker thread, so once
    // 'kept' has seen the next refresh, 'removed' would have too.
    mDevices->refreshDevices();
    bool kept_notified = kept.mCalls.waitFor(kept_calls + 1);

    mDevices->unsetDevicesObserver(&kept);

    ASSERT_TRUE(kept_notified);
    EXPECT_EQ(removed.mCalls.count(), removed_calls);
}

TEST_F(LLWebRTCDevicesTest, UnsetUnknownObserverIsHarmless)
{
    RecordingDevicesObserver never_registered;
    mDevices->unsetDevicesObserver(&never_registered);
    SUCCEED();
}

TEST_F(LLWebRTCDevicesTest, RefreshIsLogged)
{
    RecordingDevicesObserver observer;
    mDevices->setDevicesObserver(&observer);
    mDevices->refreshDevices();
    bool notified = observer.mCalls.waitFor(1);
    mDevices->unsetDevicesObserver(&observer);

    ASSERT_TRUE(notified);
    // updateDevices() logs the device counts before notifying observers.
    EXPECT_TRUE(llwebrtc_test::logCallback().contains("updateDevices"))
        << "expected llwebrtc's own log output to reach the log callback";
}

TEST_F(LLWebRTCDevicesTest, AudioLevelIsNeverNaN)
{
    // With no audio flowing the level is computed from zero energy; it must
    // come back as a usable number (+inf means silence), never NaN.
    EXPECT_FALSE(std::isnan(mDevices->getPeerConnectionAudioLevel()));
    EXPECT_FALSE(std::isnan(mDevices->getTuningAudioLevel()));
}
