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

// These are not the right way in viewer for some reason:
// #include <tut/tut.hpp>
// #include <tut/tut_reporter.hpp>
// This works:
#include "../test/lltut.h"

// Pull in each of the test sets
#include "test_httpstatus.hpp"
#include "test_refcounted.hpp"
#include "test_httpoperation.hpp"
#include "test_httprequest.hpp"
#include "test_httpheaders.hpp"
#include "test_bufferarray.hpp"
#include "test_httprequestqueue.hpp"

unsigned long ssl_thread_id_callback(void);
void ssl_locking_callback(int mode, int type, const char * file, int line);

#if 0	// lltut provides main and runner

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

int ssl_mutex_count(0);
LLCoreInt::HttpMutex ** ssl_mutex_list = NULL;

void init_curl()
{
	curl_global_init(CURL_GLOBAL_ALL);

	ssl_mutex_count = CRYPTO_num_locks();
	if (ssl_mutex_count > 0)
	{
		ssl_mutex_list = new LLCoreInt::HttpMutex * [ssl_mutex_count];
		
		for (int i(0); i < ssl_mutex_count; ++i)
		{
			ssl_mutex_list[i] = new LLCoreInt::HttpMutex;
		}

		CRYPTO_set_locking_callback(ssl_locking_callback);
		CRYPTO_set_id_callback(ssl_thread_id_callback);
	}
}


void term_curl()
{
	CRYPTO_set_locking_callback(NULL);
	for (int i(0); i < ssl_mutex_count; ++i)
	{
		delete ssl_mutex_list[i];
	}
	delete [] ssl_mutex_list;
}


unsigned long ssl_thread_id_callback(void)
{
#if defined(WIN32)
	return (unsigned long) GetCurrentThread();
#else
	return (unsigned long) pthread_self();
#endif
}


void ssl_locking_callback(int mode, int type, const char * /* file */, int /* line */)
{
	if (type >= 0 && type < ssl_mutex_count)
	{
		if (mode & CRYPTO_LOCK)
		{
			ssl_mutex_list[type]->lock();
		}
		else
		{
			ssl_mutex_list[type]->unlock();
		}
	}
}


