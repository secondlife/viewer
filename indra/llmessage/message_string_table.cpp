/** 
 * @file message_string_table.cpp
 * @brief static string table for message template
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llerror.h"
#include "message.h"

inline U32  message_hash_my_string(const char *str)
{
    U32 retval = 0;
    while (*str++)
    {
        retval += *str;
        retval <<= 1;
    }
    return (retval % MESSAGE_NUMBER_OF_HASH_BUCKETS);
}



LLMessageStringTable::LLMessageStringTable()
:   mUsed(0)
{
    for (U32 i = 0; i < MESSAGE_NUMBER_OF_HASH_BUCKETS; i++)
    {
        mEmpty[i] = TRUE;
        mString[i][0] = 0;
    }
}


LLMessageStringTable::~LLMessageStringTable()
{ }


char* LLMessageStringTable::getString(const char *str)
{
    U32 hash_value = message_hash_my_string(str);
    while (!mEmpty[hash_value])
    {
        if (!strncmp(str, mString[hash_value], MESSAGE_MAX_STRINGS_LENGTH))
        {
            return mString[hash_value];
        }
        else
        {
            hash_value++;
            hash_value %= MESSAGE_NUMBER_OF_HASH_BUCKETS;
        }
    }
    // not found, so add!
    strncpy(mString[hash_value], str, MESSAGE_MAX_STRINGS_LENGTH);  /* Flawfinder: ignore */
    mString[hash_value][MESSAGE_MAX_STRINGS_LENGTH - 1] = 0;
    mEmpty[hash_value] = FALSE;
    mUsed++;
    if (mUsed >= MESSAGE_NUMBER_OF_HASH_BUCKETS - 1)
    {
        U32 i;
        LL_INFOS() << "Dumping string table before crashing on HashTable full!" << LL_ENDL;
        for (i = 0; i < MESSAGE_NUMBER_OF_HASH_BUCKETS; i++)
        {
            LL_INFOS() << "Entry #" << i << ": " << mString[i] << LL_ENDL;
        }
    }
    return mString[hash_value];
}

