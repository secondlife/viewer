// ---------------------------------------------------------------------------
// Auto-generated from bitpack_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "../llbitpack.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("bitpack_test::bit_pack_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert bitpack_test.cpp::bit_pack_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void bit_pack_object_t::test<1>()
        //     {
        //         U8 packbuffer[255];
        //         U8 unpackbuffer[255];
        //         int pack_bufsize = 0;
        //         int unpack_bufsize = 0;

        //         LLBitPack bitpack(packbuffer, 255);

        //         char str[] = "SecondLife is a 3D virtual world";
        //         int len = sizeof(str);
        //         pack_bufsize = bitpack.bitPack((U8*) str, len*8);
        //         pack_bufsize = bitpack.flushBitPack();

        //         LLBitPack bitunpack(packbuffer, pack_bufsize*8);
        //         unpack_bufsize = bitunpack.bitUnpack(unpackbuffer, len*8);
        //         ensure("bitPack: unpack size should be same as string size prior to pack", len == unpack_bufsize);
        //         ensure_memory_matches("str->bitPack->bitUnpack should be equal to string", str, len, unpackbuffer, unpack_bufsize);
        //     }
    }

    TUT_CASE("bitpack_test::bit_pack_object_t_test_2")
    {
        DOCTEST_FAIL("TODO: convert bitpack_test.cpp::bit_pack_object_t::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void bit_pack_object_t::test<2>()
        //     {
        //         U8 packbuffer[255];
        //         U8 unpackbuffer[255];
        //         int pack_bufsize = 0;

        //         LLBitPack bitpack(packbuffer, 255);

        //         char str[] = "SecondLife";
        //         int len = sizeof(str);
        //         pack_bufsize = bitpack.bitPack((U8*) str, len*8);
        //         pack_bufsize = bitpack.flushBitPack();

        //         LLBitPack bitunpack(packbuffer, pack_bufsize*8);
        //         bitunpack.bitUnpack(&unpackbuffer[0], 8);
        //         ensure("bitPack: individual unpack: 0", unpackbuffer[0] == (U8) str[0]);
        //         bitunpack.bitUnpack(&unpackbuffer[0], 8);
        //         ensure("bitPack: individual unpack: 1", unpackbuffer[0] == (U8) str[1]);
        //         bitunpack.bitUnpack(&unpackbuffer[0], 8);
        //         ensure("bitPack: individual unpack: 2", unpackbuffer[0] == (U8) str[2]);
        //         bitunpack.bitUnpack(&unpackbuffer[0], 8);
        //         ensure("bitPack: individual unpack: 3", unpackbuffer[0] == (U8) str[3]);
        //         bitunpack.bitUnpack(&unpackbuffer[0], 8);
        //         ensure("bitPack: individual unpack: 4", unpackbuffer[0] == (U8) str[4]);
        //         bitunpack.bitUnpack(&unpackbuffer[0], 8);
        //         ensure("bitPack: individual unpack: 5", unpackbuffer[0] == (U8) str[5]);
        //         bitunpack.bitUnpack(unpackbuffer, 8*4); // Life
        //         ensure_memory_matches("bitPack: 4 bytes unpack:", unpackbuffer, 4, str+6, 4);
        //     }
    }

    TUT_CASE("bitpack_test::bit_pack_object_t_test_3")
    {
        DOCTEST_FAIL("TODO: convert bitpack_test.cpp::bit_pack_object_t::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void bit_pack_object_t::test<3>()
        //     {
        //         U8 packbuffer[255];
        //         int pack_bufsize = 0;

        //         LLBitPack bitpack(packbuffer, 255);
        //         U32 num = 0x41fab67a;
        //         pack_bufsize = bitpack.bitPack((U8*)&num, 8*sizeof(U32));
        //         pack_bufsize = bitpack.flushBitPack();

        //         LLBitPack bitunpack(packbuffer, pack_bufsize*8);
        //         U32 res = 0;
        //         // since packing and unpacking is done on same machine in the unit test run,
        //         // endianness should not matter
        //         bitunpack.bitUnpack((U8*) &res, sizeof(res)*8);
        //         ensure("U32->bitPack->bitUnpack->U32 should be equal", num == res);
        //     }
    }

}

