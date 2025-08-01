/**
 * @file bitpack_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief llstreamtools test cases.
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

#include "../llbitpack.h"

#include "../test/lldoctest.h"


TEST_SUITE("LLBitPack") {

TEST_CASE("test_1")
{

        U8 packbuffer[255];
        U8 unpackbuffer[255];
        int pack_bufsize = 0;
        int unpack_bufsize = 0;

        LLBitPack bitpack(packbuffer, 255);

        char str[] = "SecondLife is a 3D virtual world";
        int len = sizeof(str);
        pack_bufsize = bitpack.bitPack((U8*) str, len*8);
        pack_bufsize = bitpack.flushBitPack();

        LLBitPack bitunpack(packbuffer, pack_bufsize*8);
        unpack_bufsize = bitunpack.bitUnpack(unpackbuffer, len*8);
        CHECK_MESSAGE(len == unpack_bufsize, "bitPack: unpack size should be same as string size prior to pack");
        ensure_memory_matches("str->bitPack->bitUnpack should be equal to string", str, len, unpackbuffer, unpack_bufsize);
    
}

TEST_CASE("test_2")
{

        U8 packbuffer[255];
        U8 unpackbuffer[255];
        int pack_bufsize = 0;

        LLBitPack bitpack(packbuffer, 255);

        char str[] = "SecondLife";
        int len = sizeof(str);
        pack_bufsize = bitpack.bitPack((U8*) str, len*8);
        pack_bufsize = bitpack.flushBitPack();

        LLBitPack bitunpack(packbuffer, pack_bufsize*8);
        bitunpack.bitUnpack(&unpackbuffer[0], 8);
        CHECK_MESSAGE(unpackbuffer[0] == (U8, "bitPack: individual unpack: 0") str[0]);
        bitunpack.bitUnpack(&unpackbuffer[0], 8);
        CHECK_MESSAGE(unpackbuffer[0] == (U8, "bitPack: individual unpack: 1") str[1]);
        bitunpack.bitUnpack(&unpackbuffer[0], 8);
        CHECK_MESSAGE(unpackbuffer[0] == (U8, "bitPack: individual unpack: 2") str[2]);
        bitunpack.bitUnpack(&unpackbuffer[0], 8);
        CHECK_MESSAGE(unpackbuffer[0] == (U8, "bitPack: individual unpack: 3") str[3]);
        bitunpack.bitUnpack(&unpackbuffer[0], 8);
        CHECK_MESSAGE(unpackbuffer[0] == (U8, "bitPack: individual unpack: 4") str[4]);
        bitunpack.bitUnpack(&unpackbuffer[0], 8);
        CHECK_MESSAGE(unpackbuffer[0] == (U8, "bitPack: individual unpack: 5") str[5]);
        bitunpack.bitUnpack(unpackbuffer, 8*4); // Life
        ensure_memory_matches("bitPack: 4 bytes unpack:", unpackbuffer, 4, str+6, 4);
    
}

TEST_CASE("test_3")
{

        U8 packbuffer[255];
        int pack_bufsize = 0;

        LLBitPack bitpack(packbuffer, 255);
        U32 num = 0x41fab67a;
        pack_bufsize = bitpack.bitPack((U8*)&num, 8*sizeof(U32));
        pack_bufsize = bitpack.flushBitPack();

        LLBitPack bitunpack(packbuffer, pack_bufsize*8);
        U32 res = 0;
        // since packing and unpacking is done on same machine in the unit test run,
        // endianness should not matter
        bitunpack.bitUnpack((U8*) &res, sizeof(res)*8);
        CHECK_MESSAGE(num == res, "U32->bitPack->bitUnpack->U32 should be equal");
    
}

} // TEST_SUITE

