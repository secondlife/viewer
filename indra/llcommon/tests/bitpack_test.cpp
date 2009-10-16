/**
 * @file bitpack_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief llstreamtools test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "../bitpack.h"

#include "../test/lltut.h"


namespace tut
{
	struct bit_pack
	{
	};
	typedef test_group<bit_pack> bit_pack_t;
	typedef bit_pack_t::object bit_pack_object_t;
	tut::bit_pack_t tut_bit_pack("bitpack");

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
