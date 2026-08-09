/**
 * @file llversioninfo_test.cpp
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "doctest.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include "../llversioninfo.h"

#include <iostream>

#define ll_viewer_channel LL_TO_STRING(LL_VIEWER_CHANNEL)

namespace tut
{
    using tut_compat::ensure_equals;

    struct versioninfo
    {
        versioninfo()
            : mResetChannel("Reset Channel")
        {
            std::ostringstream stream;
            stream << LL_VIEWER_VERSION_MAJOR << "."
                   << LL_VIEWER_VERSION_MINOR << "."
                   << LL_VIEWER_VERSION_PATCH << "."
                   << LL_VIEWER_VERSION_BUILD;
            mVersion = stream.str();
            stream.str("");

            stream << LL_VIEWER_VERSION_MAJOR << "."
                   << LL_VIEWER_VERSION_MINOR << "."
                   << LL_VIEWER_VERSION_PATCH;
            mShortVersion = stream.str();
            stream.str("");

            stream << ll_viewer_channel
                   << " "
                   << mVersion;
            mVersionAndChannel = stream.str();
            stream.str("");

            stream << mResetChannel
                   << " "
                   << mVersion;
            mResetVersionAndChannel = stream.str();
        }
        std::string mResetChannel;
        std::string mVersion;
        std::string mShortVersion;
        std::string mVersionAndChannel;
        std::string mResetVersionAndChannel;
    };
}

TUT_SUITE("LLVersionInfo")
{
    TUT_CASE("LLVersionInfo::versioninfo_object_t_test_1")
    {
        using namespace tut;
        versioninfo data;
        std::cout << "What we parsed from CMake: " << LL_VIEWER_VERSION_BUILD << std::endl;
        std::cout << "What we get from llversioninfo: " << LLVersionInfo::instance().getBuild() << std::endl;
        ensure_equals("Major version", LLVersionInfo::instance().getMajor(), LL_VIEWER_VERSION_MAJOR);
        ensure_equals("Minor version", LLVersionInfo::instance().getMinor(), LL_VIEWER_VERSION_MINOR);
        ensure_equals("Patch version", LLVersionInfo::instance().getPatch(), LL_VIEWER_VERSION_PATCH);
        ensure_equals("Build version", LLVersionInfo::instance().getBuild(), LL_VIEWER_VERSION_BUILD);
        ensure_equals("Channel version", LLVersionInfo::instance().getChannel(), ll_viewer_channel);
        ensure_equals("Version String", LLVersionInfo::instance().getVersion(), data.mVersion);
        ensure_equals("Short Version String", LLVersionInfo::instance().getShortVersion(), data.mShortVersion);
        ensure_equals("Version and channel String", LLVersionInfo::instance().getChannelAndVersion(), data.mVersionAndChannel);

        LLVersionInfo::instance().resetChannel(data.mResetChannel);
        ensure_equals("Reset channel version", LLVersionInfo::instance().getChannel(), data.mResetChannel);
        ensure_equals("Reset Version and channel String", LLVersionInfo::instance().getChannelAndVersion(), data.mResetVersionAndChannel);
    }
}
