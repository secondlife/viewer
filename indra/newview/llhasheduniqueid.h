/** 
 * @file llhasheduniqueid.h
 * @brief retrieves obfuscated but unique id for the system
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
#ifndef LL_LLHASHEDUNIQUEID_H
#define LL_LLHASHEDUNIQUEID_H
#include "llmd5.h"

/// Get an obfuscated identifier for this system 
bool llHashedUniqueID(unsigned char id[MD5HEX_STR_SIZE]);
///< @returns true if the id is considered valid (if false, the id is all zeros)

#endif // LL_LLHASHEDUNIQUEID_H
