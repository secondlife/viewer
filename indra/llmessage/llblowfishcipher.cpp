/** 
 * @file llblowfishcipher.cpp
 * @brief Wrapper around OpenSSL Blowfish encryption algorithm.
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

#include "linden_common.h"
#include "llblowfishcipher.h"
#include <openssl/evp.h>


LLBlowfishCipher::LLBlowfishCipher(const U8* secret, size_t secret_size)
:	LLCipher()
{
	llassert(secret);

	mSecretSize = secret_size;
	mSecret = new U8[mSecretSize];
	memcpy(mSecret, secret, mSecretSize);
}

LLBlowfishCipher::~LLBlowfishCipher()
{
	delete [] mSecret;
	mSecret = NULL;
}

// virtual
U32 LLBlowfishCipher::encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	if (!src || !src_len || !dst || !dst_len) return 0;
	if (src_len > dst_len) return 0;

	// OpenSSL uses "cipher contexts" to hold encryption parameters.
    EVP_CIPHER_CTX context;
    EVP_CIPHER_CTX_init(&context);

	// We want a blowfish cyclic block chain cipher, but need to set 
	// the key length before we pass in a key, so call EncryptInit 
	// first with NULLs.
	EVP_EncryptInit_ex(&context, EVP_bf_cbc(), NULL, NULL, NULL);
	EVP_CIPHER_CTX_set_key_length(&context, (int)mSecretSize);
	
	// Complete initialization.  Per EVP_EncryptInit man page, the
	// cipher pointer must be NULL.  Apparently initial_vector must
	// be 8 bytes for blowfish, as this is the block size.
    unsigned char initial_vector[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	EVP_EncryptInit_ex(&context, NULL, NULL, mSecret, initial_vector);

    int blocksize = EVP_CIPHER_CTX_block_size(&context);
    int keylen = EVP_CIPHER_CTX_key_length(&context);
    int iv_length = EVP_CIPHER_CTX_iv_length(&context);
    lldebugs << "LLBlowfishCipher blocksize " << blocksize
		<< " keylen " << keylen
		<< " iv_len " << iv_length
		<< llendl;

	int output_len = 0;
	int temp_len = 0;
	if (!EVP_EncryptUpdate(&context,
			dst,
			&output_len,
			src,
			src_len))
	{
		llwarns << "LLBlowfishCipher::encrypt EVP_EncryptUpdate failure" << llendl;
		goto ERROR;
	}

	// There may be some final data left to encrypt if the input is
	// not an exact multiple of the block size.
	if (!EVP_EncryptFinal_ex(&context, (unsigned char*)(dst + output_len), &temp_len))
	{
		llwarns << "LLBlowfishCipher::encrypt EVP_EncryptFinal failure" << llendl;
		goto ERROR;
	}
	output_len += temp_len;

	EVP_CIPHER_CTX_cleanup(&context);
	return output_len;

ERROR:
	EVP_CIPHER_CTX_cleanup(&context);
	return 0;
}

// virtual
U32 LLBlowfishCipher::decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	llerrs << "LLBlowfishCipher decrypt unsupported" << llendl;
	return 0;
}

// virtual
U32 LLBlowfishCipher::requiredEncryptionSpace(U32 len) const
{
	// *HACK: We know blowfish uses an 8 byte block size.
	// Oddly, sometimes EVP_Encrypt produces an extra block
	// if the input is an exact multiple of the block size.
	// So round up.
	const U32 BLOCK_SIZE = 8;
	len += BLOCK_SIZE;
	len -= (len % BLOCK_SIZE);
	return len;
}
