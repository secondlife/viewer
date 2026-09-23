/**
 * @file lldecodedmessage.h
 * @brief Declaration of the decoded message structure.
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

#include "llhost.h"
#include <memory>

class LLMessageTemplate;
class LLMsgData;

// A single fully-decoded, not-yet-dispatched message, ready to hand off
// from the receiver thread to the main thread's dispatch step.
struct LLDecodedMessage
{
public:
    // Destructor neds to be in .cpp where LLMsgData is complete,
    // so unique_ptr's deleter can be instantiated
    ~LLDecodedMessage();

    const LLMessageTemplate*        mTemplate = nullptr;   // stable pointer; templates never change after load
    std::unique_ptr<LLMsgData> mData;                // this message's OWN decoded data, not shared/reused
    LLHost                    mSender;
    bool                      mTrusted = false;
    S32                       mReceiveSize = -1;
};
