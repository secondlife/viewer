/** 
 * @file llxorcipher.cpp
 * @brief Implementation of LLXORCipher
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

#include "llxorcipher.h"

#include "llerror.h"

///----------------------------------------------------------------------------
/// Class LLXORCipher
///----------------------------------------------------------------------------

LLXORCipher::LLXORCipher(const U8* pad, U32 pad_len) :
	mPad(NULL),
	mHead(NULL),
	mPadLen(0)
{
	init(pad, pad_len);
}

// Destroys the object
LLXORCipher::~LLXORCipher()
{
	init(NULL, 0);
}

LLXORCipher::LLXORCipher(const LLXORCipher& cipher) :
	mPad(NULL),
	mHead(NULL),
	mPadLen(0)
{
	init(cipher.mPad, cipher.mPadLen);
}

LLXORCipher& LLXORCipher::operator=(const LLXORCipher& cipher)
{
	if(this == &cipher) return *this;
	init(cipher.mPad, cipher.mPadLen);
	return *this;
}

U32 LLXORCipher::encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	if(!src || !src_len || !dst || !dst_len || !mPad) return 0;
	U8* pad_end = mPad + mPadLen;
	U32 count = src_len;
	while(count--)
	{
		*dst++ = *src++ ^ *mHead++;
		if(mHead >= pad_end) mHead = mPad;
	}
	return src_len;
}

U32 LLXORCipher::decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	// xor is a symetric cipher, thus, just call the other function.
	return encrypt(src, src_len, dst, dst_len);
}

U32 LLXORCipher::requiredEncryptionSpace(U32 len) const
{
	return len;
}

void LLXORCipher::init(const U8* pad, U32 pad_len)
{
	if(mPad)
	{
		delete [] mPad;
		mPad = NULL;
		mPadLen = 0;
	}
	if(pad && pad_len)
	{
		mPadLen = pad_len;
		mPad = new U8[mPadLen];
		if (mPad != NULL)
		{
			memcpy(mPad, pad, mPadLen); /* Flawfinder : ignore */
		}
	}
	mHead = mPad;
}

#ifdef _DEBUG
// static
bool LLXORCipher::testHarness()
{
	const U32 PAD_LEN = 3;
	const U8 PAD[] = "abc";
	const S32 MSG_LENGTH = 12;
	const char MESSAGE[MSG_LENGTH+1] = "gesundheight";		/* Flawfinder : ignore */
	U8 encrypted[MSG_LENGTH];
	U8 decrypted[MSG_LENGTH];

	LLXORCipher cipher(PAD, PAD_LEN);
	cipher.encrypt((U8*)MESSAGE, MSG_LENGTH, encrypted, MSG_LENGTH);
	cipher.decrypt(encrypted, MSG_LENGTH, decrypted, MSG_LENGTH);

	if(0 != memcmp((void*)MESSAGE, decrypted, MSG_LENGTH)) return false;
	return true;
}
#endif
