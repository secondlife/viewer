/**
 * @file _httppolicy.cpp
 * @brief Internal definitions of the Http policy thread
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

#include "_httppolicy.h"

#include "_httpoprequest.h"
#include "_httpservice.h"


namespace LLCore
{


HttpPolicy::HttpPolicy(HttpService * service)
	: mService(service)
{}


HttpPolicy::~HttpPolicy()
{
	for (ready_queue_t::reverse_iterator i(mReadyQueue.rbegin());
		 mReadyQueue.rend() != i;)
	{
		ready_queue_t::reverse_iterator cur(i++);

		(*cur)->cancel();
		(*cur)->release();
	}

	mService = NULL;
}


void HttpPolicy::addOp(HttpOpRequest * op)
{
	mReadyQueue.push_back(op);
}


void HttpPolicy::processReadyQueue()
{
	while (! mReadyQueue.empty())
	{
		HttpOpRequest * op(mReadyQueue.front());
		mReadyQueue.erase(mReadyQueue.begin());

		op->stageFromReady(mService);
		op->release();
	}
}


}  // end namespace LLCore
