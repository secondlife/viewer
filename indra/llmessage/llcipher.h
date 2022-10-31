/** 
 * @file llcipher.h
 * @brief Abstract base class for encryption ciphers.
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

#ifndef LLCIPHER_H
#define LLCIPHER_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCipher
//
// Abstract base class for a cipher object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLCipher
{
public:
    virtual ~LLCipher() {}

    // encrypt src and place result into dst. returns TRUE if
    // Returns number of bytes written into dst, or 0 on error.
    virtual U32 encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len) = 0;

    // decrypt src and place result into dst. 
    // Returns number of bytes written into dst, or 0 on error.
    virtual U32 decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len) = 0;

    // returns the minimum amount of space required to encrypt for a 
    // unencrypted source buffer of length len.
    // *NOTE: This is estimated space and you should check the return value
    // of the encrypt function.
    virtual U32 requiredEncryptionSpace(U32 src_len) const = 0 ;
};

#endif
