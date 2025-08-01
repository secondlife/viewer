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
// #include "../test/doctest.h"
// #include <tut/tut_reporter.hpp>
// This works:
#include "../test/lldoctest.h"

// Pull in each of the test sets
#include "test_bufferarray.hpp"
#include "test_bufferstream.hpp"
#include "test_httpstatus.hpp"
#include "test_refcounted.hpp"
#include "test_httpoperation.hpp"
// As of 2019-06-28, test_httprequest.hpp consistently crashes on Mac Release
// builds for reasons not yet diagnosed.
#if ! (LL_DARWIN && LL_RELEASE)
#include "test_httprequest.hpp"
#endif
#include "test_httpheaders.hpp"
#include "test_httprequestqueue.hpp"
#include "_httpservice.h"

#include "llproxy.h"
#include "llcleanup.h"

void ssl_thread_id_callback(CRYPTO_THREADID*);
void ssl_locking_callback(int mode, int type, const char * file, int line);

#if 0   // lltut provides main and runner

TEST_SUITE("UnknownSuite") {

} // TEST_SUITE

