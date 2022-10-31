/** 
 * @file llnullcipher.cpp
 * @brief Implementation of a cipher which does not encrypt.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llnullcipher.h"

///----------------------------------------------------------------------------
/// Class LLNullCipher
///----------------------------------------------------------------------------

U32  LLNullCipher::encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
    if((src_len == dst_len) && src && dst)
    {
        memmove(dst, src, src_len);
        return src_len;
    }
    return 0;
}

U32 LLNullCipher::decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
    if((src_len == dst_len) && src && dst)
    {
        memmove(dst, src, src_len);
        return src_len;
    }
    return 0;
}

U32 LLNullCipher::requiredEncryptionSpace(U32 len) const
{
    return len;
}
