/**
 * @file llmd5.h
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLMD5_H
#define LL_LLMD5_H

// LLMD5.CC - source code for the C++/object oriented translation and
//          modification of MD5.

// Translation and modification (c) 1995 by Mordechai T. Abzug

// This translation/ modification is provided "as is," without express or
// implied warranty of any kind.

// The translator/ modifier does not claim (1) that MD5 will do what you think
// it does; (2) that this translation/ modification is accurate; or (3) that
// this software is "merchantible."  (Language for this disclaimer partially
// copied from the disclaimer below).

/* based on:

   MD5.H - header file for MD5C.C
   MDDRIVER.C - test driver for MD2, MD4 and MD5

   Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

*/

#include <cstdint> // uint32_t et al.

// use for the raw digest output
const int MD5RAW_BYTES = 16;

// use for outputting hex digests
const int MD5HEX_STR_SIZE  = 33; // char hex[MD5HEX_STR_SIZE]; with null
const int MD5HEX_STR_BYTES = 32; // message system fixed size

class LL_COMMON_API LLMD5
{
    // how many bytes to grab at a time when checking files
    static const int BLOCK_LEN;

public:
    // methods for controlled operation:
    LLMD5(); // simple initializer
    void update(const uint8_t* input, const size_t input_length);
    void update(std::istream& stream);
    void update(FILE* file);
    void update(const std::string& str);
    void finalize();

    // constructors for special circumstances.  All these constructors finalize
    // the MD5 context.
    LLMD5(const unsigned char* string); // digest string, finalize
    LLMD5(std::istream& stream);        // digest stream, finalize
    LLMD5(FILE* file);                  // digest file, close, finalize
    LLMD5(const unsigned char* string, const unsigned int number);

    // methods to acquire finalized result
    void raw_digest(unsigned char* array) const; // provide 16-byte array for binary data
    void hex_digest(char* string) const;         // provide 33-byte array for ascii-hex string

    friend LL_COMMON_API std::ostream& operator<<(std::ostream&, const LLMD5& context);

private:
    // next, the private data:
    uint32_t state[4];
    uint64_t count;      // number of *bits*, mod 2^64
    uint8_t  buffer[64]; // input buffer
    uint8_t  digest[16];
    bool     finalized;

    // last, the private methods, mostly static:
    void init();                           // called by all constructors
    void transform(const uint8_t* buffer); // does the real update work.  Note
                                           // that length is implied to be 64.

    static void encode(uint8_t* dest, const uint32_t* src, const size_t length);
    static void decode(uint32_t* dest, const uint8_t* src, const size_t length);
};

LL_COMMON_API bool operator==(const LLMD5& a, const LLMD5& b);
LL_COMMON_API bool operator!=(const LLMD5& a, const LLMD5& b);

#endif // LL_LLMD5_H
