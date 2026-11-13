/**
 * @file llwebrtc_types_test.cpp
 * @brief Tests for the plain value types declared in llwebrtc.h.
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

#include "llgtest.h"
#include "llwebrtc.h"

using llwebrtc::LLWebRTCDeviceInterface;
using llwebrtc::LLWebRTCPeerConnectionInterface;
using llwebrtc::LLWebRTCVoiceDevice;

TEST(LLWebRTCVoiceDevice, KeepsExplicitId)
{
    LLWebRTCVoiceDevice device("Headset Microphone", "{0.0.1.00000000}.{abcd}");

    EXPECT_EQ(device.mDisplayName, "Headset Microphone");
    EXPECT_EQ(device.mID, "{0.0.1.00000000}.{abcd}");
}

TEST(LLWebRTCVoiceDevice, FallsBackToDisplayNameWhenIdIsEmpty)
{
    // Some platforms report devices without a GUID; the display name must
    // then double as the id so the device can still be selected.
    LLWebRTCVoiceDevice device("Built-in Output", "");

    EXPECT_EQ(device.mDisplayName, "Built-in Output");
    EXPECT_EQ(device.mID, "Built-in Output");
}

TEST(LLWebRTCAudioConfig, DefaultsEnableFullProcessing)
{
    LLWebRTCDeviceInterface::AudioConfig config;

    EXPECT_TRUE(config.mAGC);
    EXPECT_TRUE(config.mEchoCancellation);
    EXPECT_EQ(config.mNoiseSuppressionLevel,
              LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_VERY_HIGH);
}

TEST(LLWebRTCInitOptions, DefaultsToNoIceServers)
{
    LLWebRTCPeerConnectionInterface::InitOptions options;

    EXPECT_TRUE(options.mServers.empty());
}
