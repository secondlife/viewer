/**
 * @file llappcorehttp.h
 * @brief Singleton initialization/shutdown class for llcorehttp library
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

#ifndef	_LL_APP_COREHTTP_H_
#define	_LL_APP_COREHTTP_H_


#include "httprequest.h"
#include "httphandler.h"
#include "httpresponse.h"


// This class manages the lifecyle of the core http library.
// Slightly different style than traditional code but reflects
// the use of handler classes and light-weight interface
// object instances of the new libraries.  To be used
// as a singleton and static construction is fine.
class LLAppCoreHttp : public LLCore::HttpHandler
{
public:
	LLAppCoreHttp();
	~LLAppCoreHttp();
	
	// Initialize the LLCore::HTTP library creating service classes
	// and starting the servicing thread.  Caller is expected to do
	// other initializations (SSL mutex, thread hash function) appropriate
	// for the application.
	void init();

	// Request that the servicing thread stop servicing requests,
	// release resource references and stop.  Request is asynchronous
	// and @see cleanup() will perform a limited wait loop for this
	// request to stop the thread.
	void requestStop();
	
	// Terminate LLCore::HTTP library services.  Caller is expected
	// to have made a best-effort to shutdown the servicing thread
	// by issuing a requestThreadStop() and waiting for completion
	// notification that the stop has completed.
	void cleanup();

	// Notification when the stop request is complete.
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

	// Retrieve the policy class for default operations.
	int getPolicyDefault() const
		{
			return mPolicyDefault;
		}
	
private:
	static const F64			MAX_THREAD_WAIT_TIME;
	
private:
	LLCore::HttpRequest *		mRequest;
	LLCore::HttpHandle			mStopHandle;
	F64							mStopRequested;
	bool						mStopped;
	int							mPolicyDefault;
};


#endif	// _LL_APP_COREHTTP_H_
