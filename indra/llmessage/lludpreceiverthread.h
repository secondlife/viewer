/**
 * @file lludpreceiverthread.h
 * @brief Background thread that pulls raw UDP packets off the socket
 *        and hands them to the main thread via a thread-safe queue.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#pragma once

#include <memory>
#include "llthread.h"
#include "llthreadsafequeue.h"
#include "llpacketbuffer.h"

// flowchart TB
// B1["Background thread<br/>(runs independently, all the time)"] -- > B2["Read incoming network data from the socket"]
// B2-- > B3["Drop packets into a waiting-line (queue)"]
//
// C1["Game frame starts"] -- > C2["Pick up packets already waiting in the queue"]
// C2-- > C3["Decode and apply updates<br/>(objects, avatars, chat, etc.)"]
// C3-- > C4["Render the frame"]
// C4-- > C1
// end

class LLUDPReceiverThread : public LLThread
{
public:
    using PacketQueue = LLThreadSafeQueue<LLPacketBuffer>;

    // hSocket must remain valid for the lifetime of this thread.
    LLUDPReceiverThread(S32 hSocket, std::shared_ptr<PacketQueue> queue);
    ~LLUDPReceiverThread() override;

protected:
    void run() override;

private:
    S32 mSocket;
    std::shared_ptr<PacketQueue> mQueue;
};
