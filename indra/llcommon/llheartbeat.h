/** 
 * @file llheartbeat.h
 * @brief Class encapsulating logic for telling a watchdog that we live.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLHEARTBEAT_H
#define LL_LLHEARTBEAT_H

#include "linden_common.h"

#include "lltimer.h"

// Note: Win32 does not support the heartbeat/smackdown system;
//   heartbeat-delivery turns into a no-op there.

class LL_COMMON_API LLHeartbeat
{
public:
	// secs_between_heartbeat: after a heartbeat is successfully delivered,
	//   we suppress sending more for this length of time.
	// aggressive_heartbeat_panic_secs: if we've been failing to
	//   successfully deliver heartbeats for this length of time then
	//   we block for a while until we're really sure we got one delivered.
	// aggressive_heartbeat_max_blocking_secs: this is the length of
	//   time we block for when we're aggressively ensuring that a 'panic'
	//   heartbeat was delivered.
	LLHeartbeat(F32 secs_between_heartbeat = 5.0f,
		    F32 aggressive_heartbeat_panic_secs = 10.0f,
		    F32 aggressive_heartbeat_max_blocking_secs = 4.0f);
	~LLHeartbeat();

	bool send(F32 timeout_sec = 0.0f);
	void setSuppressed(bool is_suppressed);

private:
	int rawSend();
	int rawSendWithTimeout(F32 timeout_sec);
	F32 mSecsBetweenHeartbeat;
	F32 mAggressiveHeartbeatPanicSecs;
	F32 mAggressiveHeartbeatMaxBlockingSecs;
	bool mSuppressed;
	LLTimer mBeatTimer;
	LLTimer mPanicTimer;
	LLTimer mTimeoutTimer;
};

#endif // LL_HEARTBEAT_H
