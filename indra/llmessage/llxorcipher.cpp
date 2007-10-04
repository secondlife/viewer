/** 
 * @file llxorcipher.cpp
 * @brief Implementation of LLXORCipher
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
BOOL LLXORCipher::testHarness()
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

	if(0 != memcmp((void*)MESSAGE, decrypted, MSG_LENGTH)) return FALSE;
	return TRUE;
}
#endif
