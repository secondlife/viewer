/**
 * @file llappcorehttp.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llappcorehttp.h"

#include "llviewercontrol.h"


const F64 LLAppCoreHttp::MAX_THREAD_WAIT_TIME(10.0);

LLAppCoreHttp::LLAppCoreHttp()
	: mRequest(NULL),
	  mStopHandle(LLCORE_HTTP_HANDLE_INVALID),
	  mStopRequested(0.0),
	  mStopped(false)
{
	for (int i(0); i < LL_ARRAY_SIZE(mPolicies); ++i)
	{
		mPolicies[i] = LLCore::HttpRequest::DEFAULT_POLICY_ID;
	}
}


LLAppCoreHttp::~LLAppCoreHttp()
{
	delete mRequest;
	mRequest = NULL;
}


void LLAppCoreHttp::init()
{
	static const struct
	{
		EAppPolicy		mPolicy;
		U32				mDefault;
		U32				mMin;
		U32				mMax;
		U32				mDivisor;
		std::string		mKey;
		const char *	mUsage;
	} init_data[] =					//  Default and dynamic values for classes
		  {
			  {
				  AP_TEXTURE,			8,		1,		12,		1,
				  "TextureFetchConcurrency",
				  "texture fetch"
			  },
			  {
				  AP_MESH,				8,		1,		32,		4,
				  "MeshMaxConcurrentRequests",
				  "mesh fetch"
			  },
			  {
				  AP_LARGE_MESH,		2,		1,		8,		1,
				  "",
				  "large mesh fetch"
			  },
			  {
				  AP_UPLOADS,			2,		1,		8,		1,
				  "",
				  "asset upload"
			  }
		  };
		
	LLCore::HttpStatus status = LLCore::HttpRequest::createService();
	if (! status)
	{
		LL_ERRS("Init") << "Failed to initialize HTTP services.  Reason:  " << status.toString()
						<< LL_ENDL;
	}

	// Point to our certs or SSH/https: will fail on connect
	status = LLCore::HttpRequest::setPolicyGlobalOption(LLCore::HttpRequest::GP_CA_FILE,
														gDirUtilp->getCAFile());
	if (! status)
	{
		LL_ERRS("Init") << "Failed to set CA File for HTTP services.  Reason:  " << status.toString()
						<< LL_ENDL;
	}

	// Establish HTTP Proxy.  "LLProxy" is a special string which directs
	// the code to use LLProxy::applyProxySettings() to establish any
	// HTTP or SOCKS proxy for http operations.
	status = LLCore::HttpRequest::setPolicyGlobalOption(LLCore::HttpRequest::GP_LLPROXY, 1);
	if (! status)
	{
		LL_ERRS("Init") << "Failed to set HTTP proxy for HTTP services.  Reason:  " << status.toString()
						<< LL_ENDL;
	}

	// Tracing levels for library & libcurl (note that 2 & 3 are beyond spammy):
	// 0 - None
	// 1 - Basic start, stop simple transitions
	// 2 - libcurl CURLOPT_VERBOSE mode with brief lines
	// 3 - with partial data content
	static const std::string http_trace("QAModeHttpTrace");
	if (gSavedSettings.controlExists(http_trace))
	{
		long trace_level(0L);
		trace_level = long(gSavedSettings.getU32(http_trace));
		status = LLCore::HttpRequest::setPolicyGlobalOption(LLCore::HttpRequest::GP_TRACE, trace_level);
	}
	
	// Setup default policy and constrain if directed to
	mPolicies[AP_DEFAULT] = LLCore::HttpRequest::DEFAULT_POLICY_ID;

	// Setup additional policies based on table and some special rules
	// *TODO:  Make these configurations dynamic later
	for (int i(0); i < LL_ARRAY_SIZE(init_data); ++i)
	{
		const EAppPolicy policy(init_data[i].mPolicy);

		// Create a policy class but use default for texture for now.
		// This also has the side-effect of initializing the default
		// class to desired values.
		if (AP_TEXTURE == policy)
		{
			mPolicies[policy] = mPolicies[AP_DEFAULT];
		}
		else
		{
			mPolicies[policy] = LLCore::HttpRequest::createPolicyClass();
			if (! mPolicies[policy])
			{
				// Use default policy (but don't accidentally modify default)
				LL_WARNS("Init") << "Failed to create HTTP policy class for " << init_data[i].mUsage
								 << ".  Using default policy."
								 << LL_ENDL;
				mPolicies[policy] = mPolicies[AP_DEFAULT];
				continue;
			}
		}

		// Get target connection concurrency value
		U32 setting(init_data[i].mDefault);
		if (! init_data[i].mKey.empty() && gSavedSettings.controlExists(init_data[i].mKey))
		{
			U32 new_setting(gSavedSettings.getU32(init_data[i].mKey));
			if (new_setting)
			{
				// Treat zero settings as an ask for default
				setting = new_setting / init_data[i].mDivisor;
				setting = llclamp(setting, init_data[i].mMin, init_data[i].mMax);
			}
		}

		// Set it and report
		LLCore::HttpStatus status;
		status = LLCore::HttpRequest::setPolicyClassOption(mPolicies[policy],
														   LLCore::HttpRequest::CP_CONNECTION_LIMIT,
														   setting);
		if (! status)
		{
			LL_WARNS("Init") << "Unable to set " << init_data[i].mUsage
							 << " concurrency.  Reason:  " << status.toString()
							 << LL_ENDL;
		}
		else if (setting != init_data[i].mDefault)
		{
			LL_INFOS("Init") << "Application settings overriding default " << init_data[i].mUsage
							 << " concurrency.  New value:  " << setting
							 << LL_ENDL;
		}
	}
	
	// Kick the thread
	status = LLCore::HttpRequest::startThread();
	if (! status)
	{
		LL_ERRS("Init") << "Failed to start HTTP servicing thread.  Reason:  " << status.toString()
						<< LL_ENDL;
	}

	mRequest = new LLCore::HttpRequest;
}


void LLAppCoreHttp::requestStop()
{
	llassert_always(mRequest);

	mStopHandle = mRequest->requestStopThread(this);
	if (LLCORE_HTTP_HANDLE_INVALID != mStopHandle)
	{
		mStopRequested = LLTimer::getTotalSeconds();
	}
}


void LLAppCoreHttp::cleanup()
{
	if (LLCORE_HTTP_HANDLE_INVALID == mStopHandle)
	{
		// Should have been started already...
		requestStop();
	}
	
	if (LLCORE_HTTP_HANDLE_INVALID == mStopHandle)
	{
		LL_WARNS("Cleanup") << "Attempting to cleanup HTTP services without thread shutdown"
							<< LL_ENDL;
	}
	else
	{
		while (! mStopped && LLTimer::getTotalSeconds() < (mStopRequested + MAX_THREAD_WAIT_TIME))
		{
			mRequest->update(200000);
			ms_sleep(50);
		}
		if (! mStopped)
		{
			LL_WARNS("Cleanup") << "Attempting to cleanup HTTP services with thread shutdown incomplete"
								<< LL_ENDL;
		}
	}

	delete mRequest;
	mRequest = NULL;

	LLCore::HttpStatus status = LLCore::HttpRequest::destroyService();
	if (! status)
	{
		LL_WARNS("Cleanup") << "Failed to shutdown HTTP services, continuing.  Reason:  "
							<< status.toString()
							<< LL_ENDL;
	}
}


void LLAppCoreHttp::onCompleted(LLCore::HttpHandle, LLCore::HttpResponse *)
{
	mStopped = true;
}
