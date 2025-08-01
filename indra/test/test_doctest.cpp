/**
 * @file test_doctest.cpp
 * @author Converted from test.cpp for doctest migration
 * @date 2025-08-01
 * @brief Entry point for the doctest-based test app.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "linden_common.h"
#include "llerrorcontrol.h"

// Define DOCTEST_CONFIG_IMPLEMENT to provide main() function
#define DOCTEST_CONFIG_IMPLEMENT
#include "lldoctest.h"

#include "stringize.h"
#include "namedtempfile.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"

#include "apr_pools.h"

// the CTYPE_WORKAROUND is needed for linux dev stations that don't
// have the broken libc6 packages needed by our out-of-date static
// libs (such as libcrypto and libcurl). -- Leviathan 20060113
#ifdef CTYPE_WORKAROUND
#   include "ctype_workaround.h"
#endif

#include <fstream>

void wouldHaveCrashed(const std::string& message);

static LLTrace::ThreadRecorder* sMasterThreadRecorder = NULL;

int main(int argc, char **argv)
{
    ll_init_apr();

    // set up logging
    LLError::initForApplication(".", ".", false /* do not log to stderr */);
    LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
    LLError::setFatalFunction(wouldHaveCrashed);
    
    std::string test_app_name(argv[0]);
    std::string test_log = test_app_name + ".log";
    LLFile::remove(test_log);
    LLError::logToFile(test_log);

#ifdef CTYPE_WORKAROUND
    ctype_workaround();
#endif

    if (!sMasterThreadRecorder)
    {
        sMasterThreadRecorder = new LLTrace::ThreadRecorder();
        LLTrace::set_master_thread_recorder(sMasterThreadRecorder);
    }

    // Create doctest context and run tests
    doctest::Context context;
    
    // Apply command line arguments to doctest
    context.applyCommandLine(argc, argv);
    
    // Run the tests
    int result = context.run();
    
    // Clean up
    if (sMasterThreadRecorder)
    {
        delete sMasterThreadRecorder;
        sMasterThreadRecorder = NULL;
    }
    
    return result;
}

void wouldHaveCrashed(const std::string& message)
{
    FAIL("llerrs message: " + message);
}

