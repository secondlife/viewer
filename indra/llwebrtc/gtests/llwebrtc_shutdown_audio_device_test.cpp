/**
 * @file llwebrtc_shutdown_audio_device_test.cpp
 * @brief llwebrtc::terminate() tests that open real audio devices.
 *
 * These start real capture and playout, so they need a machine with working
 * audio devices, and they grab the microphone while they run.  They are only
 * built into UNIT_TEST_llwebrtc_shutdown when the build is configured with
 * LL_TESTS_AUDIO_DEVICES=ON, e.g.:
 *
 *   autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON -DLL_TESTS_AUDIO_DEVICES=ON
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

#include "llwebrtc_shutdown_test.h"

using namespace llwebrtc_shutdown_test;

TEST_F(LLWebRTCShutdownTest, TerminateWithVoiceEnabled)
{
    // Stopping capture and playout can each block for up to 2s in WASAPI on
    // Windows, and that comes out of terminate()'s budget.  Playout only
    // runs with voice enabled and a connected call, so use a live one.
    mDevices->setVoiceEnabled(true);
    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);
    mDevices->setMute(false);

    // Let the devices actually start.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateInTuningMode)
{
    mDevices->setVoiceEnabled(true);
    mDevices->setTuningMode(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, TerminateWhileDevicesRedeploying)
{
    // Device changes stop and restart the audio devices via the signaling
    // and worker threads; terminate() blocks new deploys and drains the
    // signaling thread, but one may already be in progress.
    GatedDevicesObserver* observer = newDevicesObserver();
    mDevices->setDevicesObserver(observer);
    mDevices->setVoiceEnabled(true);
    mDevices->refreshDevices();
    ASSERT_TRUE(observer->mCalls.waitFor(1));

    llwebrtc::LLWebRTCVoiceDeviceList capture = observer->captureDevices();
    llwebrtc::LLWebRTCVoiceDeviceList render  = observer->renderDevices();
    for (int i = 0; i < 5; ++i)
    {
        mDevices->setCaptureDevice(capture.empty() ? "Default" : capture[i % capture.size()].mID);
        mDevices->setRenderDevice(render.empty() ? "Default" : render[i % render.size()].mID);
    }

    terminateAndVerify();
}
