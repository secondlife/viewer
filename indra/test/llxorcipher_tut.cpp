/**
 * @file llxorcipher_tut.cpp
 * @author Adroit
 * @date 2007-03
 * @brief llxorcipher, llnullcipher test cases.
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
#include "lltut.h"
#include "llxorcipher.h"
#include "llnullcipher.h"

namespace tut
{
	struct cipher
	{
	};
	typedef test_group<cipher> cipher_t;
	typedef cipher_t::object cipher_object_t;
	tut::cipher_t tut_cipher("cipher");

	//encrypt->decrypt
	template<> template<>
	void cipher_object_t::test<1>()
	{
		const U32 len = 3;
		const U8 pad[] = "abc";
		const char str[] = "SecondLife";
		const S32 str_len = sizeof(str);
		U8 encrypted[str_len];
		U8 decrypted[str_len];
		LLXORCipher xorCipher(pad, len);
		LLXORCipher xorCipher1(pad, len);

		U32 length = xorCipher.requiredEncryptionSpace(50);
		ensure("requiredEncryptionSpace() function failed", (length == 50));

		U32 lenEncrypted = xorCipher.encrypt((U8 *) str, str_len, encrypted, str_len);
		ensure("Encryption failed", (lenEncrypted == str_len));
		U32 lenDecrypted = xorCipher1.decrypt(encrypted, str_len, decrypted, str_len);
		ensure("Decryption failed", (lenDecrypted == str_len));
		ensure_memory_matches("LLXORCipher Encrypt/Decrypt failed", str, str_len, decrypted, lenDecrypted);	
	}

	// operator=
	template<> template<>
	void cipher_object_t::test<2>()
	{
		const U8 pad[] = "ABCDEFGHIJKLMNOPQ"; // pad len longer than data to be ciphered
		const U32 pad_len = sizeof(pad);
		const U8 pad1[] = "SecondLife";
		const U32 pad_len1 = sizeof(pad1);
		const char str[] = "To Be Ciphered";
		const S32 str_len = sizeof(str);
		U8 encrypted[str_len];
		U8 decrypted[str_len];

		LLXORCipher xorCipher(pad, pad_len);
		LLXORCipher xorCipher1(pad1, pad_len1);

		xorCipher.encrypt((U8 *) str, str_len, encrypted, str_len);
		// make xorCipher1 same as xorCipher..so that xorCipher1 can decrypt what was 
		// encrypted using xorCipher
		xorCipher1 = xorCipher;
		U32 lenDecrypted = xorCipher1.decrypt(encrypted, str_len, decrypted, str_len);
		ensure_memory_matches("LLXORCipher operator= failed", str, str_len, decrypted, lenDecrypted);	
	}
	
	//in place encrypt->decrypt
	template<> template<>
	void cipher_object_t::test<3>()
	{
		U32 padNum = 0x12349087;
		const U8* pad = (U8*) &padNum;
		const U32 pad_len = sizeof(U32);
		char str[] = "To Be Ciphered a long string.........!!!.";
		char str1[] = "To Be Ciphered a long string.........!!!."; // same as str
		const S32 str_len = sizeof(str);

		LLXORCipher xorCipher(pad, pad_len);
		LLXORCipher xorCipher1(pad, pad_len);
		xorCipher.encrypt((U8 *) str, str_len);
		// it should not be the same as original data!
		ensure("LLXORCipher: In Place encrypt failed", memcmp(str, str1, str_len) != 0);
		xorCipher1.decrypt((U8 *) str, str_len);
		// it should not be the same as original data!
		ensure_memory_matches("LLXORCipher: In Place decrypt failed", str, str_len, str1, str_len);
	}

	//LLNullCipher encrypt->decrypt
	template<> template<>
	void cipher_object_t::test<4>()
	{
		const char str[] = "SecondLife";
		const S32 str_len = sizeof(str);
		U8 encrypted[str_len];
		U8 decrypted[str_len];
		LLNullCipher nullCipher;
		LLNullCipher nullCipher1;

		U32 length = nullCipher.requiredEncryptionSpace(50);
		ensure("LLNullCipher::requiredEncryptionSpace() function failed", (length == 50));

		U32 len1 = nullCipher.encrypt((U8 *) str, str_len, encrypted, str_len);
		ensure_memory_matches("LLNullCipher - Source transformed during encryption.", encrypted, len1, str, str_len);
		
		U32 len2 = nullCipher1.decrypt(encrypted, str_len, decrypted, str_len);
		ensure_memory_matches("LLNullCipher - Decryption failed", decrypted, len2, str, str_len);
	}
}
