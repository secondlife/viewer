/** 
 * @file llblowfishcipher.h
 * @brief A symmetric block cipher, designed in 1993 by Bruce Schneier.
 * We use it because it has an 8 byte block size, allowing encryption of
 * two UUIDs and a timestamp (16x2 + 4 = 36 bytes) with only 40 bytes of
 * output.  AES has a block size of 32 bytes, so this would require 64 bytes.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
