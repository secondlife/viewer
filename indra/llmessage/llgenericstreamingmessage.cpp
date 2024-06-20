/**
 * @file llgenericstreamingmessage.cpp
 * @brief Generic Streaming Message helpers.  Shared between viewer and simulator.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#include "linden_common.h"

#include "llgenericstreamingmessage.h"

#include "message.h"

void LLGenericStreamingMessage::send(LLMessageSystem* msg)
{
#if 0 // viewer cannot send GenericStreamingMessage
    msg->newMessageFast(_PREHASH_GenericStreamingMessage);

    if (mData.size() < 1024 * 7)
    { // disable warning about big messages unless we're sending a REALLY big message
        msg->tempDisableWarnAboutBigMessage();
    }
    else
    {
        LL_WARNS("Messaging") << "Attempted to send too large GenericStreamingMessage, dropping." << LL_ENDL;
        return;
    }

    msg->nextBlockFast(_PREHASH_MethodData);
    msg->addU16Fast(_PREHASH_Method, mMethod);
    msg->nextBlockFast(_PREHASH_DataBlock);
    msg->addStringFast(_PREHASH_Data, mData.c_str());
#endif
}

void LLGenericStreamingMessage::unpack(LLMessageSystem* msg)
{
    U16* m = (U16*)&mMethod; // squirrely pass enum as U16 by reference
    msg->getU16Fast(_PREHASH_MethodData, _PREHASH_Method, *m);

    constexpr int MAX_SIZE = 7 * 1024;

    char buffer[MAX_SIZE];

    // NOTE: don't use getStringFast to avoid 1200 byte truncation
    U32 size = msg->getSizeFast(_PREHASH_DataBlock, _PREHASH_Data);
    msg->getBinaryDataFast(_PREHASH_DataBlock, _PREHASH_Data, buffer, size, 0, MAX_SIZE);

    mData.assign(buffer, size);
}



