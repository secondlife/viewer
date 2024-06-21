/**
 * @file llinstantmessage.cpp
 * @author Phoenix
 * @date 2005-08-29
 * @brief Constants and functions used in IM.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "linden_common.h"

#include "lldbstrings.h"
#include "llinstantmessage.h"
#include "llhost.h"
#include "lluuid.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil_math.h"
#include "llpointer.h"
#include "message.h"

#include "message.h"

const U8 IM_ONLINE = 0;
const U8 IM_OFFLINE = 1;

const char EMPTY_BINARY_BUCKET[] = "";
const S32 EMPTY_BINARY_BUCKET_SIZE = 1;
const U32 NO_TIMESTAMP = 0;
const std::string SYSTEM_FROM("Second Life");
const std::string INTERACTIVE_SYSTEM_FROM("F387446C-37C4-45f2-A438-D99CBDBB563B");
const S32 IM_TTL = 1;


void pack_instant_message(
    LLMessageSystem* msg,
    const LLUUID& from_id,
    bool from_group,
    const LLUUID& session_id,
    const LLUUID& to_id,
    const std::string& name,
    const std::string& message,
    U8 offline,
    EInstantMessage dialog,
    const LLUUID& id,
    U32 parent_estate_id,
    const LLUUID& region_id,
    const LLVector3& position,
    U32 timestamp,
    const U8* binary_bucket,
    S32 binary_bucket_size)
{
    LL_DEBUGS() << "pack_instant_message()" << LL_ENDL;
    msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
    pack_instant_message_block(
        msg,
        from_id,
        from_group,
        session_id,
        to_id,
        name,
        message,
        offline,
        dialog,
        id,
        parent_estate_id,
        region_id,
        position,
        timestamp,
        binary_bucket,
        binary_bucket_size);
}

void pack_instant_message_block(
    LLMessageSystem* msg,
    const LLUUID& from_id,
    bool from_group,
    const LLUUID& session_id,
    const LLUUID& to_id,
    const std::string& name,
    const std::string& message,
    U8 offline,
    EInstantMessage dialog,
    const LLUUID& id,
    U32 parent_estate_id,
    const LLUUID& region_id,
    const LLVector3& position,
    U32 timestamp,
    const U8* binary_bucket,
    S32 binary_bucket_size)
{
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, from_id);
    msg->addUUIDFast(_PREHASH_SessionID, session_id);
    msg->nextBlockFast(_PREHASH_MessageBlock);
    msg->addBOOLFast(_PREHASH_FromGroup, from_group);
    msg->addUUIDFast(_PREHASH_ToAgentID, to_id);
    msg->addU32Fast(_PREHASH_ParentEstateID, parent_estate_id);
    msg->addUUIDFast(_PREHASH_RegionID, region_id);
    msg->addVector3Fast(_PREHASH_Position, position);
    msg->addU8Fast(_PREHASH_Offline, offline);
    msg->addU8Fast(_PREHASH_Dialog, (U8) dialog);
    msg->addUUIDFast(_PREHASH_ID, id);
    msg->addU32Fast(_PREHASH_Timestamp, timestamp);
    msg->addStringFast(_PREHASH_FromAgentName, name);
    S32 bytes_left = MTUBYTES;
    if(!message.empty())
    {
        char buffer[MTUBYTES];
        int num_written = snprintf(buffer, MTUBYTES, "%s", message.c_str());    /* Flawfinder: ignore */
        // snprintf returns number of bytes that would have been written
        // had the output not being truncated. In that case, it will
        // return either -1 or value >= passed in size value . So a check needs to be added
        // to detect truncation, and if there is any, only account for the
        // actual number of bytes written..and not what could have been
        // written.
        if (num_written < 0 || num_written >= MTUBYTES)
        {
            num_written = MTUBYTES - 1;
            LL_WARNS() << "pack_instant_message_block: message truncated: " << message << LL_ENDL;
        }

        bytes_left -= num_written;
        bytes_left = llmax(0, bytes_left);
        msg->addStringFast(_PREHASH_Message, buffer);
    }
    else
    {
        msg->addStringFast(_PREHASH_Message, NULL);
    }
    const U8* bb;
    if(binary_bucket)
    {
        bb = binary_bucket;
        binary_bucket_size = llmin(bytes_left, binary_bucket_size);
    }
    else
    {
        bb = (const U8*)EMPTY_BINARY_BUCKET;
        binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE;
    }
    msg->addBinaryDataFast(_PREHASH_BinaryBucket, bb, binary_bucket_size);
}


