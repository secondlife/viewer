/** 
 * @file llxorcipher.h
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

#ifndef LLXORCIPHER_H
#define LLXORCIPHER_H

#include "llcipher.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLXORCipher
//
// Implementation of LLCipher which encrypts using a XOR pad.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLXORCipher : public LLCipher
{
public:
    LLXORCipher(const U8* pad, U32 pad_len);
    LLXORCipher(const LLXORCipher& cipher);
    virtual ~LLXORCipher();
    LLXORCipher& operator=(const LLXORCipher& cipher);

    // Cipher functions
    /*virtual*/ U32 encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
    /*virtual*/ U32 decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
    /*virtual*/ U32 requiredEncryptionSpace(U32 src_len) const;

    // special syntactic-sugar since xor can be performed in place.
    BOOL encrypt(U8* buf, U32 len) { return encrypt((const U8*)buf, len, buf, len); }
    BOOL decrypt(U8* buf, U32 len) { return decrypt((const U8*)buf, len, buf, len); }

#ifdef _DEBUG
    // This function runs tests to make sure the crc is
    // working. Returns TRUE if it is.
    static BOOL testHarness();
#endif

protected:
    void init(const U8* pad, U32 pad_len);
    U8* mPad;
    U8* mHead;
    U32 mPadLen;
};

#endif
