/**
 * @file lldatapacker_tut.cpp
 * @author Adroit
 * @date 2007-02
 * @brief LLDataPacker test cases.
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

#include <tut/tut.hpp>
#include "lltut.h"
#include "linden_common.h"
#include "lldatapacker.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "llsdserialize.h"

#define TEST_FILE_NAME	"datapacker_test.txt"
namespace tut
{
	struct datapacker_test
	{
	};
	typedef test_group<datapacker_test> datapacker_test_t;
	typedef datapacker_test_t::object datapacker_test_object_t;
	tut::datapacker_test_t tut_datapacker_test("datapacker_test");

	//*********LLDataPackerBinaryBuffer
	template<> template<>
	void datapacker_test_object_t::test<1>()
	{
		U8 packbuf[128];
		F32 f_val1 = 44.44f, f_unpkval1;
		F32 f_val2 = 12344.443232f, f_unpkval2;
		F32 f_val3 = 44.4456789f, f_unpkval3;
		LLDataPackerBinaryBuffer lldp(packbuf,128);
		lldp.packFixed( f_val1, "linden_lab", FALSE, 8, 8);
		lldp.packFixed( f_val2, "linden_lab", FALSE, 14, 16);
		lldp.packFixed( f_val3, "linden_lab", FALSE, 8, 23);

		LLDataPackerBinaryBuffer lldp1(packbuf, lldp.getCurrentSize());
		lldp1.unpackFixed(f_unpkval1, "linden_lab", FALSE, 8, 8);
		lldp1.unpackFixed(f_unpkval2, "linden_lab", FALSE, 14, 16);
		lldp1.unpackFixed(f_unpkval3, "linden_lab", FALSE, 8, 23);
		ensure_approximately_equals("LLDataPackerBinaryBuffer::packFixed 8 failed", f_val1, f_unpkval1, 8);
		ensure_approximately_equals("LLDataPackerBinaryBuffer::packFixed 16 failed", f_val2, f_unpkval2, 16);
		ensure_approximately_equals("LLDataPackerBinaryBuffer::packFixed 23 failed", f_val3, f_unpkval3, 31);
	}

	template<> template<>
	void datapacker_test_object_t::test<2>()
	{
		U8 packbuf[1024];

		char str[] = "SecondLife is virtual World\0";
		char strBinary[] = "SecondLife is virtual World";
		char strBinaryFixed[] = "Fixed Data";
		S32 sizeBinaryFixed = sizeof(strBinaryFixed);
		U8 valU8 = 'C';
		U16 valU16 = 0xFFFF;
		U32 valU32 = 0xFFFFFFFF;
		S32 valS32 = -94967295;
		F32 valF32 = 4354355.44f ;
		LLColor4 llcol4(3.3f, 0, 4.4f, 5.5f);
		LLColor4U llcol4u(3, 128, 24, 33);
		LLVector2 llvec2(333.33f, 444.44f);
		LLVector3 llvec3(333.33f, 444.44f, 555.55f);
		LLVector4 llvec4(333.33f, 444.44f, 555.55f, 666.66f);
		LLUUID uuid;

		std::string unpkstr;
		char unpkstrBinary[256];
		char unpkstrBinaryFixed[256];
		S32 unpksizeBinary;
		U8 unpkvalU8;
		U16 unpkvalU16;
		U32 unpkvalU32;
		S32 unpkvalS32;
		F32 unpkvalF32;
		LLColor4 unpkllcol4;
		LLColor4U unpkllcol4u;
		LLVector2 unpkllvec2;
		LLVector3 unpkllvec3;
		LLVector4 unpkllvec4;
		LLUUID unpkuuid;

		LLDataPackerBinaryBuffer lldp(packbuf,1024);
		lldp.packString(str , "linden_lab_str");
		lldp.packBinaryData((U8*)strBinary, sizeof(strBinary), "linden_lab_bd");
		lldp.packBinaryDataFixed((U8*)strBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp.packU8(valU8,"linden_lab_u8");
		lldp.packU16(valU16,"linden_lab_u16");
		lldp.packU32(valU32, "linden_lab_u32");
		lldp.packS32(valS32, "linden_lab_s32");
		lldp.packF32(valF32, "linden_lab_f32");
		lldp.packColor4(llcol4, "linden_lab_col4");
		lldp.packColor4U(llcol4u, "linden_lab_col4u");
		lldp.packVector2(llvec2, "linden_lab_vec2");
		lldp.packVector3(llvec3, "linden_lab_vec3");
		lldp.packVector4(llvec4, "linden_lab_vec4");
		uuid.generate();
		lldp.packUUID(uuid, "linden_lab_uuid");

		S32 cur_size = lldp.getCurrentSize();

		LLDataPackerBinaryBuffer lldp1(packbuf, cur_size);
		lldp1.unpackString(unpkstr , "linden_lab_str");
		lldp1.unpackBinaryData((U8*)unpkstrBinary, unpksizeBinary, "linden_lab_bd");
		lldp1.unpackBinaryDataFixed((U8*)unpkstrBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp1.unpackU8(unpkvalU8,"linden_lab_u8");
		lldp1.unpackU16(unpkvalU16,"linden_lab_u16");
		lldp1.unpackU32(unpkvalU32, "linden_lab_u32");
		lldp1.unpackS32(unpkvalS32, "linden_lab_s32");
		lldp1.unpackF32(unpkvalF32, "linden_lab_f32");
		lldp1.unpackColor4(unpkllcol4, "linden_lab_col4");
		lldp1.unpackColor4U(unpkllcol4u, "linden_lab_col4u");
		lldp1.unpackVector2(unpkllvec2, "linden_lab_vec2");
		lldp1.unpackVector3(unpkllvec3, "linden_lab_vec3");
		lldp1.unpackVector4(unpkllvec4, "linden_lab_vec4");
		lldp1.unpackUUID(unpkuuid, "linden_lab_uuid");

		ensure("LLDataPackerBinaryBuffer::packString failed", strcmp(str, unpkstr.c_str())  == 0);
		ensure("LLDataPackerBinaryBuffer::packBinaryData failed", strcmp(strBinary, unpkstrBinary)  == 0);
		ensure("LLDataPackerBinaryBuffer::packBinaryDataFixed failed", strcmp(strBinaryFixed, unpkstrBinaryFixed) == 0);
		ensure_equals("LLDataPackerBinaryBuffer::packU8 failed", valU8, unpkvalU8);
		ensure_equals("LLDataPackerBinaryBuffer::packU16 failed", valU16, unpkvalU16);
		ensure_equals("LLDataPackerBinaryBuffer::packU32 failed", valU32, unpkvalU32);
		ensure_equals("LLDataPackerBinaryBuffer::packS32 failed", valS32, unpkvalS32);
		ensure("LLDataPackerBinaryBuffer::packF32 failed", is_approx_equal(valF32, unpkvalF32));
		ensure_equals("LLDataPackerBinaryBuffer::packColor4 failed", llcol4, unpkllcol4);
		ensure_equals("LLDataPackerBinaryBuffer::packColor4U failed", llcol4u, unpkllcol4u);
		ensure_equals("LLDataPackerBinaryBuffer::packVector2 failed", llvec2, unpkllvec2);
		ensure_equals("LLDataPackerBinaryBuffer::packVector3 failed", llvec3, unpkllvec3);
		ensure_equals("LLDataPackerBinaryBuffer::packVector4 failed", llvec4, unpkllvec4);
		ensure_equals("LLDataPackerBinaryBuffer::packUUID failed", uuid, unpkuuid);
	}

	template<> template<>
	void datapacker_test_object_t::test<3>()
	{
		U8 packbuf[128];
		char str[] = "SecondLife is virtual World";
		S32 strSize = sizeof(str); // include '\0'
		LLDataPackerBinaryBuffer lldp(packbuf, 128);
		lldp.packString(str , "linden_lab");

		ensure("LLDataPackerBinaryBuffer: current size is wrong", strSize == lldp.getCurrentSize());
		ensure("LLDataPackerBinaryBuffer: buffer size is wrong", 128 == lldp.getBufferSize());

		lldp.reset();
		ensure("LLDataPackerBinaryBuffer::reset failed",0 == lldp.getCurrentSize());
	}

	template<> template<>
	void datapacker_test_object_t::test<4>()
	{
		U8* packbuf = new U8[128];
		char str[] = "SecondLife is virtual World";
		LLDataPackerBinaryBuffer lldp(packbuf, 128);
		lldp.packString(str , "linden_lab");
		lldp.freeBuffer();
		ensure("LLDataPackerBinaryBuffer.freeBuffer failed" , 0 == lldp.getBufferSize());

	}

	template<> template<>
	void datapacker_test_object_t::test<5>()
	{
		U8 buf[] = "SecondLife is virtual World";
		S32 size = sizeof(buf);
		LLDataPackerBinaryBuffer lldp(buf, size);
		U8 new_buf[] = "Its Amazing";
		size = sizeof(new_buf);
		lldp.assignBuffer(new_buf, size);
		ensure("LLDataPackerBinaryBuffer::assignBuffer failed" , ((lldp.getBufferSize() == size) && (0 == lldp.getCurrentSize()))) ;
	}

	template<> template<>
	void datapacker_test_object_t::test<6>()
	{
		U8 packbuf[128];
		char str[] = "SecondLife is virtual World";
		LLDataPackerBinaryBuffer lldp(packbuf, 128);
		lldp.packString(str , "linden_lab");
		U8 new_buffer[128];
		std::string unpkbuf;
		LLDataPackerBinaryBuffer lldp1(new_buffer,128);	
		lldp1 = lldp;
		lldp1.unpackString(unpkbuf, "linden_lab");
		ensure("1. LLDataPackerBinaryBuffer::operator= failed" , lldp1.getBufferSize() == lldp.getBufferSize());
		ensure_equals("2.LLDataPackerBinaryBuffer::operator= failed", str,unpkbuf);
	}

	//*********LLDataPackerAsciiBuffer

	template<> template<>
	void datapacker_test_object_t::test<7>()
	{
		char packbuf[128];
		F32 f_val = 44.44f, f_unpkval;
		LLDataPackerAsciiBuffer lldp(packbuf,128);
		lldp.packFixed( f_val, "linden_lab", FALSE, 8, 8);

		LLDataPackerAsciiBuffer lldp1(packbuf, lldp.getCurrentSize());
		lldp1.unpackFixed(f_unpkval, "linden_lab", FALSE, 8, 8);
		ensure_approximately_equals("LLDataPackerAsciiBuffer::packFixed failed", f_val, f_unpkval, 8);
	}

	template<> template<>
	void datapacker_test_object_t::test<8>()
	{
		char packbuf[1024];

		char str[] = "SecondLife is virtual World\0";
		char strBinary[] = "SecondLife is virtual World";
		char strBinaryFixed[] = "Fixed Data";
		S32 sizeBinaryFixed = sizeof(strBinaryFixed);
		U8 valU8 = 'C';
		U16 valU16 = 0xFFFF;
		U32 valU32 = 0xFFFFFFFF;
		S32 valS32 = -94967295;
		F32 valF32 = 4354355.44f ;
		LLColor4 llcol4(3.3f, 0, 4.4f, 5.5f);
		LLColor4U llcol4u(3, 128, 24, 33);
		LLVector2 llvec2(333.33f, 444.44f);
		LLVector3 llvec3(333.33f, 444.44f, 555.55f);
		LLVector4 llvec4(4354355.44f, 444.44f, 555.55f, 666.66f);
		LLUUID uuid;

		std::string unpkstr;
		char unpkstrBinary[256];
		char unpkstrBinaryFixed[256];
		S32 unpksizeBinary;
		U8 unpkvalU8;
		U16 unpkvalU16;
		U32 unpkvalU32;
		S32 unpkvalS32;
		F32 unpkvalF32;
		LLColor4 unpkllcol4;
		LLColor4U unpkllcol4u;
		LLVector2 unpkllvec2;
		LLVector3 unpkllvec3;
		LLVector4 unpkllvec4;
		LLUUID unpkuuid;

		LLDataPackerAsciiBuffer lldp(packbuf,1024);
		lldp.packString(str , "linden_lab_str");
		lldp.packBinaryData((U8*)strBinary, sizeof(strBinary), "linden_lab_bd");
		lldp.packBinaryDataFixed((U8*)strBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp.packU8(valU8,"linden_lab_u8");
		lldp.packU16(valU16,"linden_lab_u16");
		lldp.packU32(valU32, "linden_lab_u32");
		lldp.packS32(valS32, "linden_lab_s32");
		lldp.packF32(valF32, "linden_lab_f32");
		lldp.packColor4(llcol4, "linden_lab_col4");
		lldp.packColor4U(llcol4u, "linden_lab_col4u");
		lldp.packVector2(llvec2, "linden_lab_vec2");
		lldp.packVector3(llvec3, "linden_lab_vec3");
		lldp.packVector4(llvec4, "linden_lab_vec4");
		uuid.generate();
		lldp.packUUID(uuid, "linden_lab_uuid");

		S32 cur_size = lldp.getCurrentSize();

		LLDataPackerAsciiBuffer lldp1(packbuf, cur_size);
		lldp1.unpackString(unpkstr , "linden_lab_str");
		lldp1.unpackBinaryData((U8*)unpkstrBinary, unpksizeBinary, "linden_lab_bd");
		lldp1.unpackBinaryDataFixed((U8*)unpkstrBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp1.unpackU8(unpkvalU8,"linden_lab_u8");
		lldp1.unpackU16(unpkvalU16,"linden_lab_u16");
		lldp1.unpackU32(unpkvalU32, "linden_lab_u32");
		lldp1.unpackS32(unpkvalS32, "linden_lab_s32");
		lldp1.unpackF32(unpkvalF32, "linden_lab_f32");
		lldp1.unpackColor4(unpkllcol4, "linden_lab_col4");
		lldp1.unpackColor4U(unpkllcol4u, "linden_lab_col4u");
		lldp1.unpackVector2(unpkllvec2, "linden_lab_vec2");
		lldp1.unpackVector3(unpkllvec3, "linden_lab_vec3");
		lldp1.unpackVector4(unpkllvec4, "linden_lab_vec4");
		lldp1.unpackUUID(unpkuuid, "linden_lab_uuid");

		ensure("LLDataPackerAsciiBuffer::packString failed", strcmp(str, unpkstr.c_str())  == 0);
		ensure("LLDataPackerAsciiBuffer::packBinaryData failed", strcmp(strBinary, unpkstrBinary)  == 0);
		ensure("LLDataPackerAsciiBuffer::packBinaryDataFixed failed", strcmp(strBinaryFixed, unpkstrBinaryFixed) == 0);
		ensure_equals("LLDataPackerAsciiBuffer::packU8 failed", valU8, unpkvalU8);
		ensure_equals("LLDataPackerAsciiBuffer::packU16 failed", valU16, unpkvalU16);
		ensure_equals("LLDataPackerAsciiBuffer::packU32 failed", valU32, unpkvalU32);
		ensure_equals("LLDataPackerAsciiBuffer::packS32 failed", valS32, unpkvalS32);
		ensure("LLDataPackerAsciiBuffer::packF32 failed", is_approx_equal(valF32, unpkvalF32));
		ensure_equals("LLDataPackerAsciiBuffer::packColor4 failed", llcol4, unpkllcol4);
		ensure_equals("LLDataPackerAsciiBuffer::packColor4U failed", llcol4u, unpkllcol4u);
		ensure_equals("LLDataPackerAsciiBuffer::packVector2 failed", llvec2, unpkllvec2);
		ensure_equals("LLDataPackerAsciiBuffer::packVector3 failed", llvec3, unpkllvec3);
		ensure_equals("LLDataPackerAsciiBuffer::packVector4 failed", llvec4, unpkllvec4);
		ensure_equals("LLDataPackerAsciiBuffer::packUUID failed", uuid, unpkuuid);
	}

	template<> template<>
	void datapacker_test_object_t::test<9>()
	{
		char* packbuf = new char[128];
		char str[] = "SecondLife is virtual World";
		LLDataPackerAsciiBuffer lldp(packbuf, 128);
		lldp.packString(str , "linden_lab");
		lldp.freeBuffer();
		ensure("LLDataPackerAsciiBuffer::freeBuffer failed" , 0 == lldp.getBufferSize());
	}

	template<> template<>
	void datapacker_test_object_t::test<10>()
	{
		char buf[] = "SecondLife is virtual World";
		S32 size = sizeof(buf);
		LLDataPackerAsciiBuffer lldp(buf, size);
		char new_buf[] = "Its Amazing";
		size = sizeof(new_buf);
		lldp.assignBuffer(new_buf, size);
		ensure("LLDataPackerAsciiBuffer::assignBuffer failed" , ((lldp.getBufferSize() == size) && (1 == lldp.getCurrentSize()))) ;
	}

	//*********LLDataPackerAsciiFile

	template<> template<>
	void datapacker_test_object_t::test<11>()
	{
		F32 f_val = 44.44f, f_unpkval;

		LLFILE* fp = LLFile::fopen(TEST_FILE_NAME, "w+");
		if(!fp)
		{
			llerrs << "File couldnt be open" <<llendl;
			return;
		}

		LLDataPackerAsciiFile lldp(fp,2);
		lldp.packFixed( f_val, "linden_lab", FALSE, 8, 8);

		fflush(fp);	
		fseek(fp,0,SEEK_SET);
		LLDataPackerAsciiFile lldp1(fp,2);

		lldp1.unpackFixed(f_unpkval, "linden_lab", FALSE, 8, 8);
		fclose(fp);

		ensure_approximately_equals("LLDataPackerAsciiFile::packFixed failed", f_val, f_unpkval, 8);
	}

	template<> template<>
	void datapacker_test_object_t::test<12>()
	{
		char str[] = "SecondLife is virtual World\0";
		char strBinary[] = "SecondLife is virtual World";
		char strBinaryFixed[] = "Fixed Data";
		S32 sizeBinaryFixed = sizeof(strBinaryFixed);
		U8 valU8 = 'C';
		U16 valU16 = 0xFFFF;
		U32 valU32 = 0xFFFFFFFF;
		S32 valS32 = -94967295;
		F32 valF32 = 4354355.44f ;
		LLColor4 llcol4(3.3f, 0, 4.4f, 5.5f);
		LLColor4U llcol4u(3, 128, 24, 33);
		LLVector2 llvec2(333.33f, 444.44f);
		LLVector3 llvec3(333.33f, 444.44f, 555.55f);
		LLVector4 llvec4(333.33f, 444.44f, 555.55f, 666.66f);
		LLUUID uuid;

		std::string unpkstr;
		char unpkstrBinary[256];
		char unpkstrBinaryFixed[256];
		S32 unpksizeBinary;
		U8 unpkvalU8;
		U16 unpkvalU16;
		U32 unpkvalU32;
		S32 unpkvalS32;
		F32 unpkvalF32;
		LLColor4 unpkllcol4;
		LLColor4U unpkllcol4u;
		LLVector2 unpkllvec2;
		LLVector3 unpkllvec3;
		LLVector4 unpkllvec4;
		LLUUID unpkuuid;

		LLFILE* fp = LLFile::fopen(TEST_FILE_NAME,"w+");
		if(!fp)
		{
			llerrs << "File couldnt be open" <<llendl;
			return;
		}

		LLDataPackerAsciiFile lldp(fp,2);

		lldp.packString(str , "linden_lab_str");
		lldp.packBinaryData((U8*)strBinary, sizeof(strBinary), "linden_lab_bd");
		lldp.packBinaryDataFixed((U8*)strBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp.packU8(valU8,"linden_lab_u8");
		lldp.packU16(valU16,"linden_lab_u16");
		lldp.packU32(valU32, "linden_lab_u32");
		lldp.packS32(valS32, "linden_lab_s32");
		lldp.packF32(valF32, "linden_lab_f32");
		lldp.packColor4(llcol4, "linden_lab_col4");
		lldp.packColor4U(llcol4u, "linden_lab_col4u");
		lldp.packVector2(llvec2, "linden_lab_vec2");
		lldp.packVector3(llvec3, "linden_lab_vec3");
		lldp.packVector4(llvec4, "linden_lab_vec4");
		uuid.generate();
		lldp.packUUID(uuid, "linden_lab_uuid");

		fflush(fp);	
		fseek(fp,0,SEEK_SET);
		LLDataPackerAsciiFile lldp1(fp,2);

		lldp1.unpackString(unpkstr , "linden_lab_str");
		lldp1.unpackBinaryData((U8*)unpkstrBinary, unpksizeBinary, "linden_lab_bd");
		lldp1.unpackBinaryDataFixed((U8*)unpkstrBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp1.unpackU8(unpkvalU8,"linden_lab_u8");
		lldp1.unpackU16(unpkvalU16,"linden_lab_u16");
		lldp1.unpackU32(unpkvalU32, "linden_lab_u32");
		lldp1.unpackS32(unpkvalS32, "linden_lab_s32");
		lldp1.unpackF32(unpkvalF32, "linden_lab_f32");
		lldp1.unpackColor4(unpkllcol4, "linden_lab_col4");
		lldp1.unpackColor4U(unpkllcol4u, "linden_lab_col4u");
		lldp1.unpackVector2(unpkllvec2, "linden_lab_vec2");
		lldp1.unpackVector3(unpkllvec3, "linden_lab_vec3");
		lldp1.unpackVector4(unpkllvec4, "linden_lab_vec4");
		lldp1.unpackUUID(unpkuuid, "linden_lab_uuid");

		fclose(fp);

		ensure("LLDataPackerAsciiFile::packString failed", strcmp(str, unpkstr.c_str())  == 0);
		ensure("LLDataPackerAsciiFile::packBinaryData failed", strcmp(strBinary, unpkstrBinary)  == 0);
		ensure("LLDataPackerAsciiFile::packBinaryDataFixed failed", strcmp(strBinaryFixed, unpkstrBinaryFixed) == 0);
		ensure_equals("LLDataPackerAsciiFile::packU8 failed", valU8, unpkvalU8);
		ensure_equals("LLDataPackerAsciiFile::packU16 failed", valU16, unpkvalU16);
		ensure_equals("LLDataPackerAsciiFile::packU32 failed", valU32, unpkvalU32);
		ensure_equals("LLDataPackerAsciiFile::packS32 failed", valS32, unpkvalS32);
		ensure("LLDataPackerAsciiFile::packF32 failed", is_approx_equal(valF32, unpkvalF32));
		ensure_equals("LLDataPackerAsciiFile::packColor4 failed", llcol4, unpkllcol4);
		ensure_equals("LLDataPackerAsciiFile::packColor4U failed", llcol4u, unpkllcol4u);
		ensure_equals("LLDataPackerAsciiFile::packVector2 failed", llvec2, unpkllvec2);
		ensure_equals("LLDataPackerAsciiFile::packVector3 failed", llvec3, unpkllvec3);
		ensure_equals("LLDataPackerAsciiFile::packVector4 failed", llvec4, unpkllvec4);
		ensure_equals("LLDataPackerAsciiFile::packUUID failed", uuid, unpkuuid);
	}

	template<> template<>
	void datapacker_test_object_t::test<13>()
	{
		F32 f_val = 44.44f, f_unpkval;

		std::ostringstream ostr;
		LLDataPackerAsciiFile lldp(ostr,2);
		lldp.packFixed( f_val, "linden_lab", FALSE, 8, 8);

		std::istringstream istr(ostr.str());
		LLDataPackerAsciiFile lldp1(istr,2);

		lldp1.unpackFixed(f_unpkval, "linden_lab", FALSE, 8, 8);

		ensure_approximately_equals("LLDataPackerAsciiFile::packFixed (iostring) failed", f_val, f_unpkval, 8);
	}

	template<> template<>
	void datapacker_test_object_t::test<14>()
	{
		char str[] = "SecondLife is virtual World\0";
		char strBinary[] = "SecondLife is virtual World";
		char strBinaryFixed[] = "Fixed Data";
		S32 sizeBinaryFixed = sizeof(strBinaryFixed);
		U8 valU8 = 'C';
		U16 valU16 = 0xFFFF;
		U32 valU32 = 0xFFFFFFFF;
		S32 valS32 = -94967295;
		F32 valF32 = 4354355.44f ;
		LLColor4 llcol4(3.3f, 0, 4.4f, 5.5f);
		LLColor4U llcol4u(3, 128, 24, 33);
		LLVector2 llvec2(3333333.33f, 444.333344f);
		LLVector3 llvec3(3323233.33f, 444.4324f, 555.553232f);
		LLVector4 llvec4(333.33233f, 444.4323234f, 55323225.55f, 6323236.66f);
		LLUUID uuid;

		std::string unpkstr;
		char unpkstrBinary[256];
		char unpkstrBinaryFixed[256];
		S32 unpksizeBinary;
		U8 unpkvalU8;
		U16 unpkvalU16;
		U32 unpkvalU32;
		S32 unpkvalS32;
		F32 unpkvalF32;
		LLColor4 unpkllcol4;
		LLColor4U unpkllcol4u;
		LLVector2 unpkllvec2;
		LLVector3 unpkllvec3;
		LLVector4 unpkllvec4;
		LLUUID unpkuuid;

		std::ostringstream ostr;
		LLDataPackerAsciiFile lldp(ostr,2);

		lldp.packString(str , "linden_lab_str");
		lldp.packBinaryData((U8*)strBinary, sizeof(strBinary), "linden_lab_bd");
		lldp.packBinaryDataFixed((U8*)strBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp.packU8(valU8,"linden_lab_u8");
		lldp.packU16(valU16,"linden_lab_u16");
		lldp.packU32(valU32, "linden_lab_u32");
		lldp.packS32(valS32, "linden_lab_s32");
		lldp.packF32(valF32, "linden_lab_f32");
		lldp.packColor4(llcol4, "linden_lab_col4");
		lldp.packColor4U(llcol4u, "linden_lab_col4u");
		lldp.packVector2(llvec2, "linden_lab_vec2");
		lldp.packVector3(llvec3, "linden_lab_vec3");
		lldp.packVector4(llvec4, "linden_lab_vec4");
		uuid.generate();
		lldp.packUUID(uuid, "linden_lab_uuid");

		std::istringstream istr(ostr.str());
		LLDataPackerAsciiFile lldp1(istr,2);

		lldp1.unpackString(unpkstr , "linden_lab_str");
		lldp1.unpackBinaryData((U8*)unpkstrBinary, unpksizeBinary, "linden_lab_bd");
		lldp1.unpackBinaryDataFixed((U8*)unpkstrBinaryFixed, sizeBinaryFixed, "linden_lab_bdf");
		lldp1.unpackU8(unpkvalU8,"linden_lab_u8");
		lldp1.unpackU16(unpkvalU16,"linden_lab_u16");
		lldp1.unpackU32(unpkvalU32, "linden_lab_u32");
		lldp1.unpackS32(unpkvalS32, "linden_lab_s32");
		lldp1.unpackF32(unpkvalF32, "linden_lab_f32");
		lldp1.unpackColor4(unpkllcol4, "linden_lab_col4");
		lldp1.unpackColor4U(unpkllcol4u, "linden_lab_col4u");
		lldp1.unpackVector2(unpkllvec2, "linden_lab_vec2");
		lldp1.unpackVector3(unpkllvec3, "linden_lab_vec3");
		lldp1.unpackVector4(unpkllvec4, "linden_lab_vec4");
		lldp1.unpackUUID(unpkuuid, "linden_lab_uuid");

		ensure("LLDataPackerAsciiFile::packString (iostring) failed", strcmp(str, unpkstr.c_str())  == 0);
		ensure("LLDataPackerAsciiFile::packBinaryData (iostring) failed", strcmp(strBinary, unpkstrBinary)  == 0);
		ensure("LLDataPackerAsciiFile::packBinaryDataFixed (iostring) failed", strcmp(strBinaryFixed, unpkstrBinaryFixed) == 0);
		ensure_equals("LLDataPackerAsciiFile::packU8 (iostring) failed", valU8, unpkvalU8);
		ensure_equals("LLDataPackerAsciiFile::packU16 (iostring) failed", valU16, unpkvalU16);
		ensure_equals("LLDataPackerAsciiFile::packU32 (iostring) failed", valU32, unpkvalU32);
		ensure_equals("LLDataPackerAsciiFile::packS32 (iostring) failed", valS32, unpkvalS32);
		ensure("LLDataPackerAsciiFile::packF32 (iostring) failed", is_approx_equal(valF32, unpkvalF32));
		ensure_equals("LLDataPackerAsciiFile::packColor4 (iostring) failed", llcol4, unpkllcol4);
		ensure_equals("LLDataPackerAsciiFile::packColor4U (iostring) failed", llcol4u, unpkllcol4u);
		ensure_equals("LLDataPackerAsciiFile::packVector2 (iostring) failed", llvec2, unpkllvec2);
		ensure_equals("LLDataPackerAsciiFile::packVector3 (iostring) failed", llvec3, unpkllvec3);
		ensure_equals("LLDataPackerAsciiFile::packVector4 (iostring) failed", llvec4, unpkllvec4);
		ensure_equals("LLDataPackerAsciiFile::packUUID (iostring) failed", uuid, unpkuuid);
	}
}
