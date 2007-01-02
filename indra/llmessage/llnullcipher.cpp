/** 
 * @file llnullcipher.cpp
 * @brief Implementation of a cipher which does not encrypt.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llcrypto.h"

///----------------------------------------------------------------------------
/// Class LLNullCipher
///----------------------------------------------------------------------------

BOOL LLNullCipher::encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	if((src_len == dst_len) && src && dst)
	{
		memmove(dst, src, src_len);
		return TRUE;
	}
	return FALSE;
}

BOOL LLNullCipher::decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	if((src_len == dst_len) && src && dst)
	{
		memmove(dst, src, src_len);
		return TRUE;
	}
	return FALSE;
}

U32 LLNullCipher::requiredEncryptionSpace(U32 len)
{
	return len;
}
