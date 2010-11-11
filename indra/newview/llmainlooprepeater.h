/** 
 * @file llmainlooprepeater.h
 * @brief a service for repeating messages on the main loop.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLMAINLOOPREPEATER_H
#define LL_LLMAINLOOPREPEATER_H


#include "llsd.h"
#include "llthreadsafequeue.h"


//
// A service which creates the pump 'mainlooprepeater' to which any thread can
// post a message that will be re-posted on the main loop.
//
// The posted message should contain two map elements: pump and payload.  The
// pump value is a string naming the pump to which the message should be
// re-posted.  The payload value is what will be posted to the designated pump.
//
class LLMainLoopRepeater:
	public LLSingleton<LLMainLoopRepeater>
{
public:
	LLMainLoopRepeater(void);
	
	// Start the repeater service.
	void start(void);
	
	// Stop the repeater service.
	void stop(void);
	
private:
	LLTempBoundListener mMainLoopConnection;
	LLTempBoundListener mRepeaterConnection;
	LLThreadSafeQueue<LLSD> * mQueue;
	
	bool onMainLoop(LLSD const &);
	bool onMessage(LLSD const & event);
};


#endif
