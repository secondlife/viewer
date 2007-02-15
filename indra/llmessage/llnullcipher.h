/** 
 * @file llnullcipher.h
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLNULLCIPHER_H
#define LLNULLCIPHER_H

#include "llcipher.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLNullCipher
//
// A class which implements LLCipher, but does not transform src
// during encryption.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLNullCipher : public LLCipher
{
public:
	LLNullCipher() {}
	virtual ~LLNullCipher() {}
	virtual U32 encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	virtual U32 decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	virtual U32 requiredEncryptionSpace(U32 src_len) const;
};

#endif
