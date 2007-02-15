/** 
 * @file llcipher.h
 * @brief Abstract base class for encryption ciphers.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
