/** 
 * @file llhasheduniqueid.cpp
 * @brief retrieves an obfuscated unique id for the system
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#include "llhasheduniqueid.h"
#include "llviewernetwork.h"
#include "lluuid.h"
#include "llmachineid.h"

bool llHashedUniqueID(unsigned char id[MD5HEX_STR_SIZE])
{
    bool idIsUnique = true;
    LLMD5 hashed_unique_id;
    unsigned char unique_id[MAC_ADDRESS_BYTES];
    if ( LLMachineID::getUniqueID(unique_id, sizeof(unique_id))
         || LLUUID::getNodeID(unique_id)
        )
    {
        hashed_unique_id.update(unique_id, MAC_ADDRESS_BYTES);
        hashed_unique_id.finalize();
        hashed_unique_id.hex_digest((char*)id);
        LL_INFOS_ONCE("AppInit") << "System ID " << id << LL_ENDL;
    }
    else
    {
        idIsUnique = false;
        memcpy(id,"00000000000000000000000000000000", MD5HEX_STR_SIZE);
        LL_WARNS_ONCE("AppInit") << "Failed to get an id; cannot uniquely identify this machine." << LL_ENDL;
    }
    return idIsUnique;
}

