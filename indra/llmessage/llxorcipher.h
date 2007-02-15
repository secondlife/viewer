/** 
 * @file llxorcipher.h
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
