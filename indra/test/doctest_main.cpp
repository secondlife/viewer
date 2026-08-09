/**
 * @file doctest_main.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for shared doctest entry point
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#define DOCTEST_CONFIG_IMPLEMENT

#include "doctest.h"

#include "lltest_harness.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    lltest_init_apr();

    const char* LOGTEST = std::getenv("LOGTEST");
    const char* LOGFAIL = std::getenv("LOGFAIL");

    std::string app_name(argv[0]);
    std::shared_ptr<LLReplayLog> replayer =
        lltest_init_logging_no_fatal(app_name, LOGTEST, LOGFAIL);

    lltest_init_trace();

    doctest::Context context;
    context.applyCommandLine(argc, argv);

    int result = context.run();

    if (result != 0 && replayer)
    {
        replayer->replay(std::cerr);
    }

    lltest_shutdown_apr();

    return result;
}
