/**
 * @file llcorehttp_test
 * @brief Main test runner
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llcorehttp_test.h"

#include <iostream>
#include <sstream>

// These are not the right way in viewer for some reason:
// #include <tut/tut.hpp>
// #include <tut/tut_reporter.hpp>
// This works:
#include "../test/lltut.h"

// Pull in each of the test sets
#include "test_bufferarray.hpp"
#include "test_bufferstream.hpp"
#include "test_httpstatus.hpp"
#include "test_refcounted.hpp"
#include "test_httpoperation.hpp"
#include "test_httprequest.hpp"

#include "test_httpheaders.hpp"
#include "test_httprequestqueue.hpp"
#include "_httpservice.h"

#include "llproxy.h"
#include "llcleanup.h"

#if 0   // lltut provides main and runner

namespace tut
{
    test_runner_singleton runner;
}

int main()
{
    curl_global_init(CURL_GLOBAL_ALL);

    // *FIXME:  Need threaded/SSL curl setup here.

    tut::reporter reporter;

    tut::runner.get().set_callback(&reporter);
    tut::runner.get().run_tests();
    return !reporter.all_ok();

    curl_global_cleanup();
}

#endif // 0

void init_curl()
{
    curl_global_init(CURL_GLOBAL_ALL);

    LLProxy::getInstance();
}


void term_curl()
{
    SUBSYSTEM_CLEANUP(LLProxy);
}


std::string get_base_url()
{
    const char * env(getenv("LL_TEST_PORT"));

    if (! env)
    {
        std::cerr << "LL_TEST_PORT environment variable missing." << std::endl;
        std::cerr << "Test expects to run in test_llcorehttp_peer.py script." << std::endl;
        tut::ensure("LL_TEST_PORT set in environment", NULL != env);
    }

    int port(atoi(env));
    std::ostringstream out;
    out << "http://localhost:" << port << "/";
    return out.str();
}


void stop_thread(LLCore::HttpRequest * req)
{
    if (req)
    {
        req->requestStopThread(LLCore::HttpHandler::ptr_t());

        int count = 0;
        int limit = 10;
        while (count++ < limit && ! HttpService::isStopped())
        {
            req->update(1000);
            usleep(100000);
        }
    }
}
