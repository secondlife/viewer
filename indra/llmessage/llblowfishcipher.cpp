/** 
 * @file llblowfishcipher.cpp
 * @brief Wrapper around OpenSSL Blowfish encryption algorithm.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
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
