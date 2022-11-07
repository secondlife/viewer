/**
 * @file llxorcipher_tut.cpp
 * @author Adroit
 * @date 2007-03
 * @brief llxorcipher, llnullcipher test cases.
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
