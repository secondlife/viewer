/**
 * @file llwebrtc_test_environment.cpp
 * @brief Initializes llwebrtc once for the whole UNIT_TEST_llwebrtc run.
 *
 * Not linked into UNIT_TEST_llwebrtc_shutdown, which manages init/terminate
 * itself in every test.
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

namespace
{

class LLWebRTCEnvironment : public testing::Environment
{
public:
    void SetUp() override
    {
        ASSERT_TRUE(llwebrtc_test::startNetworking()) << "failed to initialize the socket layer";
        mNetworkingStarted = true;

        llwebrtc::init(&llwebrtc_test::logCallback());
        ASSERT_NE(llwebrtc::getDeviceInterface(), nullptr) << "llwebrtc::init() did not create a device interface";
    }

    void TearDown() override
    {
        llwebrtc::terminate();
        if (mNetworkingStarted)
        {
            llwebrtc_test::stopNetworking();
        }
    }

private:
    bool mNetworkingStarted{ false };
};

// Registered at static-initialization time so it's in place before
// RUN_ALL_TESTS() in llgtest_main.cpp; GoogleTest takes ownership.
testing::Environment* const sLLWebRTCEnvironment = testing::AddGlobalTestEnvironment(new LLWebRTCEnvironment);

} // namespace
