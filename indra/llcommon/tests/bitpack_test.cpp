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

#include "../bitpack.h"

#include "../test/lltut.h"


namespace tut
{
	struct bit_pack
	{
	};
	typedef test_group<bit_pack> bit_pack_t;
	typedef bit_pack_t::object bit_pack_object_t;
	tut::bit_pack_t tut_bit_pack("LLBitPack");

	// pack -> unpack
	template<> template<>
	void bit_pack_object_t::test<1>()
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
		ensure("bitPack: unpack size should be same as string size prior to pack", len == unpack_bufsize);
		ensure_memory_matches("str->bitPack->bitUnpack should be equal to string", str, len, unpackbuffer, unpack_bufsize); 
	}

	// pack large, unpack in individual bytes
	template<> template<>
	void bit_pack_object_t::test<2>()
	{
		U8 packbuffer[255];
		U8 unpackbuffer[255];
		int pack_bufsize = 0;
		int unpack_bufsize = 0;

		LLBitPack bitpack(packbuffer, 255);

		char str[] = "SecondLife";
		int len = sizeof(str);
		pack_bufsize = bitpack.bitPack((U8*) str, len*8);
		pack_bufsize = bitpack.flushBitPack();

		LLBitPack bitunpack(packbuffer, pack_bufsize*8);
		unpack_bufsize = bitunpack.bitUnpack(&unpackbuffer[0], 8);
		ensure("bitPack: individual unpack: 0", unpackbuffer[0] == (U8) str[0]);
		unpack_bufsize = bitunpack.bitUnpack(&unpackbuffer[0], 8);
		ensure("bitPack: individual unpack: 1", unpackbuffer[0] == (U8) str[1]);
		unpack_bufsize = bitunpack.bitUnpack(&unpackbuffer[0], 8);
		ensure("bitPack: individual unpack: 2", unpackbuffer[0] == (U8) str[2]);
		unpack_bufsize = bitunpack.bitUnpack(&unpackbuffer[0], 8);
		ensure("bitPack: individual unpack: 3", unpackbuffer[0] == (U8) str[3]);
		unpack_bufsize = bitunpack.bitUnpack(&unpackbuffer[0], 8);
		ensure("bitPack: individual unpack: 4", unpackbuffer[0] == (U8) str[4]);
		unpack_bufsize = bitunpack.bitUnpack(&unpackbuffer[0], 8);
		ensure("bitPack: individual unpack: 5", unpackbuffer[0] == (U8) str[5]);
		unpack_bufsize = bitunpack.bitUnpack(unpackbuffer, 8*4); // Life
		ensure_memory_matches("bitPack: 4 bytes unpack:", unpackbuffer, 4, str+6, 4);
		ensure("keep compiler quiet", unpack_bufsize == unpack_bufsize);
	}

	// U32 packing
	template<> template<>
	void bit_pack_object_t::test<3>()
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
		ensure("U32->bitPack->bitUnpack->U32 should be equal", num == res); 
	}
}
