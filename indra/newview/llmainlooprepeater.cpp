/** 
 * @file llmachineid.cpp
 * @brief retrieves unique machine ids
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llapr.h"
#include "llevents.h"
#include "llmainlooprepeater.h"



// LLMainLoopRepeater
//-----------------------------------------------------------------------------


LLMainLoopRepeater::LLMainLoopRepeater(void):
	mQueue(0)
{
	; // No op.
}


void LLMainLoopRepeater::start(void)
{
	if(mQueue != 0) return;

	mQueue = new LLThreadSafeQueue<LLSD>(gAPRPoolp, 1024);
	mMainLoopConnection = LLEventPumps::instance().
		obtain("mainloop").listen(LLEventPump::inventName(), boost::bind(&LLMainLoopRepeater::onMainLoop, this, _1));
	mRepeaterConnection = LLEventPumps::instance().
		obtain("mainlooprepeater").listen(LLEventPump::inventName(), boost::bind(&LLMainLoopRepeater::onMessage, this, _1));
}


void LLMainLoopRepeater::stop(void)
{
	mMainLoopConnection.release();
	mRepeaterConnection.release();

	delete mQueue;
	mQueue = 0;
}


bool LLMainLoopRepeater::onMainLoop(LLSD const &)
{
	LLSD message;
	while(mQueue->tryPopBack(message)) {
		std::string pump = message["pump"].asString();
		if(pump.length() == 0 ) continue; // No pump.
		LLEventPumps::instance().obtain(pump).post(message["payload"]);
	}
	return false;
}


bool LLMainLoopRepeater::onMessage(LLSD const & event)
{
	try {
		mQueue->pushFront(event);
	} catch(LLThreadSafeQueueError & e) {
		llwarns << "could not repeat message (" << e.what() << ")" << 
			event.asString() << LL_ENDL;
	}
	return false;
}
