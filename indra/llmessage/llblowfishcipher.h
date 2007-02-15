/** 
 * @file llblowfishcipher.h
 * @brief A symmetric block cipher, designed in 1993 by Bruce Schneier.
 * We use it because it has an 8 byte block size, allowing encryption of
 * two UUIDs and a timestamp (16x2 + 4 = 36 bytes) with only 40 bytes of
 * output.  AES has a block size of 32 bytes, so this would require 64 bytes.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
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
