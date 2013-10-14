/** 
 * @file llcorehttp_test.h
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


#ifndef _LLCOREHTTP_TEST_H_
#define	_LLCOREHTTP_TEST_H_

#include "linden_common.h"		// Modifies curl interfaces

#include <curl/curl.h>
#include <openssl/crypto.h>
#include <string>

#include "httprequest.h"

// Initialization and cleanup for libcurl.  Mainly provides
// a mutex callback for SSL and a thread ID hash for libcurl.
// If you don't use these (or equivalent) and do use libcurl,
// you'll see stalls and other anomalies when performing curl
// operations.
extern void init_curl();
extern void term_curl();
extern std::string get_base_url();
extern void stop_thread(LLCore::HttpRequest * req);

class ScopedCurlInit
{
public:
	ScopedCurlInit()
		{
			init_curl();
		}

	~ScopedCurlInit()
		{
			term_curl();
		}
};
	

#endif	// _LLCOREHTTP_TEST_H_
