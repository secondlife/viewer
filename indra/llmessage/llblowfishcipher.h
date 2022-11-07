/** 
 * @file llblowfishcipher.h
 * @brief A symmetric block cipher, designed in 1993 by Bruce Schneier.
 * We use it because it has an 8 byte block size, allowing encryption of
 * two UUIDs and a timestamp (16x2 + 4 = 36 bytes) with only 40 bytes of
 * output.  AES has a block size of 32 bytes, so this would require 64 bytes.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LLBLOWFISHCIPHER_H
#define LLBLOWFISHCIPHER_H

#include "llcipher.h"


class LLBlowfishCipher : public LLCipher
{
public:
    // Secret may be up to 56 bytes in length per Blowfish spec.
    LLBlowfishCipher(const U8* secret, size_t secret_size);
    virtual ~LLBlowfishCipher();

    // See llcipher.h for documentation.
    /*virtual*/ U32 encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
    /*virtual*/ U32 decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
    /*virtual*/ U32 requiredEncryptionSpace(U32 src_len) const;

#ifdef _DEBUG
    static BOOL testHarness();
#endif

private:
    U8* mSecret;
    size_t mSecretSize;
};

#endif // LL_LLCRYPTO_H
