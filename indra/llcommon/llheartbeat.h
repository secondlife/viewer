/** 
 * @file llheartbeat.h
 * @brief Class encapsulating logic for telling a watchdog that we live.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
