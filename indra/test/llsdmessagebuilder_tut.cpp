/** 
 * @file llsdmessagebuilder_tut.cpp
 * @date   February 2006
 * @brief LLSDMessageBuilder unit tests
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <tut/tut.h>
#include "lltut.h"

#include "llsdmessagebuilder.h"
#include "llsdmessagereader.h"
#include "llsdtraits.h"
#include "llquaternion.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"

namespace tut
{	
	struct LLSDMessageBuilderTestData {
		static LLSDMessageBuilder defaultBuilder()
		{
			LLSDMessageBuilder builder;
			builder.newMessage("name");
			builder.nextBlock("block");
			return builder;
		}

		static LLSDMessageReader setReader(const LLSDMessageBuilder& builder)
		{
			LLSDMessageReader reader;
			reader.setMessage("name", builder.getMessage());
			return reader;
		}
	};
	
	typedef test_group<LLSDMessageBuilderTestData>	LLSDMessageBuilderTestGroup;
	typedef LLSDMessageBuilderTestGroup::object		LLSDMessageBuilderTestObject;
	LLSDMessageBuilderTestGroup llsdMessageBuilderTestGroup("LLSDMessageBuilder");
	
	template<> template<>
	void LLSDMessageBuilderTestObject::test<1>()
		// construction and test of undefined
	{
	  LLSDMessageBuilder builder = defaultBuilder();
	  LLSDMessageReader reader = setReader(builder);
	}
	
	template<> template<>
	void LLSDMessageBuilderTestObject::test<2>()
		 // BOOL
	{
	  BOOL outValue, inValue = TRUE;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addBOOL("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getBOOL("block", "var", outValue);
	  ensure_equals("Ensure BOOL", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<3>()
		 // U8
	{
	  U8 outValue, inValue = 2;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addU8("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getU8("block", "var", outValue);
	  ensure_equals("Ensure U8", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<4>()
		 // S16
	{
	  S16 outValue, inValue = 90;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addS16("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getS16("block", "var", outValue);
	  ensure_equals("Ensure S16", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<5>()
		 // U16
	{
	  U16 outValue, inValue = 3;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addU16("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getU16("block", "var", outValue);
	  ensure_equals("Ensure U16", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<6>()
		 // S32
	{
	  S32 outValue, inValue = 44;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addS32("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getS32("block", "var", outValue);
	  ensure_equals("Ensure S32", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<7>()
		 // F32
	{
	  F32 outValue, inValue = 121.44f;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addF32("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getF32("block", "var", outValue);
	  ensure_equals("Ensure F32", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<8>()
		 // U32
	{
	  U32 outValue, inValue = 88;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addU32("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getU32("block", "var", outValue);
	  ensure_equals("Ensure U32", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<9>()
		 // U64
	{
	  U64 outValue, inValue = 121;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addU64("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getU64("block", "var", outValue);
	  ensure_equals("Ensure U64", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<10>()
		 // F64
	{
	  F64 outValue, inValue = 3232143.33;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addF64("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getF64("block", "var", outValue);
	  ensure_equals("Ensure F64", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<11>()
		 // Vector3
	{
	  LLVector3 outValue, inValue = LLVector3(1,2,3);
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addVector3("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getVector3("block", "var", outValue);
	  ensure_equals("Ensure Vector3", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<12>()
		 // Vector4
	{
	  LLVector4 outValue, inValue = LLVector4(1,2,3,4);
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addVector4("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getVector4("block", "var", outValue);
	  ensure_equals("Ensure Vector4", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<13>()
		 // Vector3d
	{
	  LLVector3d outValue, inValue = LLVector3d(1,2,3);
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addVector3d("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getVector3d("block", "var", outValue);
	  ensure_equals("Ensure Vector3d", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<14>()
		 // Quaternion
	{
	  LLQuaternion outValue, inValue = LLQuaternion(1,2,3,4);
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addQuat("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getQuat("block", "var", outValue);
	  ensure_equals("Ensure Quaternion", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<15>()
		 // UUID
	{
	  LLUUID outValue, inValue;
	  inValue.generate();
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addUUID("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getUUID("block", "var", outValue);
	  ensure_equals("Ensure UUID", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<16>()
		 // IPAddr
	{
	  U32 outValue, inValue = 12344556;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addIPAddr("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getIPAddr("block", "var", outValue);
	  ensure_equals("Ensure IPAddr", inValue, outValue);
	}

	 template<> template<>
	void LLSDMessageBuilderTestObject::test<17>()
		 // IPPort
	{
		 U16 outValue, inValue = 80;
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addIPPort("var", inValue);
	  LLSDMessageReader reader = setReader(builder);
	  reader.getIPPort("block", "var", outValue);
	  ensure_equals("Ensure IPPort", inValue, outValue);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<18>()
	{
		 std::string outValue, inValue = "testing";
	  LLSDMessageBuilder builder = defaultBuilder();
	  builder.addString("var", inValue.c_str());
	  LLSDMessageReader reader = setReader(builder);
	  char buffer[MAX_STRING];
	  reader.getString("block", "var", MAX_STRING, buffer);
	  outValue = buffer;
	  ensure_equals("Ensure String", inValue, outValue);
	}
}

