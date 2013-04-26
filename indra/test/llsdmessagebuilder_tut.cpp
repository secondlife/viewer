/** 
 * @file llsdmessagebuilder_tut.cpp
 * @date   February 2006
 * @brief LLSDMessageBuilder unit tests
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "linden_common.h"
#include "lltut.h"
#include "llmessagetemplate.h"
#include "llsdmessagebuilder.h"
#include "llsdmessagereader.h"
#include "llsdtraits.h"
#include "llmath.h"
#include "llquaternion.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"
#include "llsdutil.h"
//#include "llsdutil.cpp"
#include "llsdutil_math.cpp"
#include "lltemplatemessagebuilder.h"

namespace tut
{	
	static LLTemplateMessageBuilder::message_template_name_map_t templateNameMap;

    LLMsgData* messageData = NULL;
    LLMsgBlkData* messageBlockData = NULL;

	struct LLSDMessageBuilderTestData {

		LLSDMessageBuilderTestData()
		{
			messageData = new LLMsgData("testMessage");
			messageBlockData = new LLMsgBlkData("testBlock", 0);
		}

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

		static void addValue(LLMsgBlkData* mbd, char* name, void* v, EMsgVariableType type, int size, int data_size = -1)
		{
			LLMsgVarData tmp(name, type);
			tmp.addData(v, size, type, data_size);
			mbd->mMemberVarData[name] = tmp;
		}


		static LLMessageBlock* defaultTemplateBlock(const EMsgVariableType type = MVT_NULL, const S32 size = 0, EMsgBlockType block = MBT_VARIABLE)
		{
			return createTemplateBlock(_PREHASH_Test0, type, size, block);
		}

		static LLMessageBlock* createTemplateBlock(const char* name, const EMsgVariableType type = MVT_NULL, const S32 size = 0, EMsgBlockType block = MBT_VARIABLE)
		{
			LLMessageBlock* result = new LLMessageBlock(name, block);
			if(type != MVT_NULL)
			{
				result->addVariable(const_cast<char*>(_PREHASH_Test0), type, size);
			}
			return result;
		}

		static LLTemplateMessageBuilder* defaultTemplateBuilder(LLMessageTemplate& messageTemplate, char* name = const_cast<char*>(_PREHASH_Test0))
		{
			templateNameMap[_PREHASH_TestMessage] = &messageTemplate;
			LLTemplateMessageBuilder* builder = new LLTemplateMessageBuilder(templateNameMap);
			builder->newMessage(_PREHASH_TestMessage);
			builder->nextBlock(name);
			return builder;
		}

		static LLMessageTemplate defaultTemplate()
		{
			return LLMessageTemplate(_PREHASH_TestMessage, 1, MFT_HIGH);
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

	template<> template<>
	void LLSDMessageBuilderTestObject::test<19>()
	{
	  LLMsgBlkData* mbd = new LLMsgBlkData("testBlock", 0);
	  LLMsgData* md = new LLMsgData("testMessage");
	  md->addBlock(mbd);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*md);
	  LLSD output = builder.getMessage();

	  ensure("Ensure message block created when copied from legacy message to llsd", output["testBlock"].isDefined());
	}

	// MVT_FIXED
	template<> template<>
	void LLSDMessageBuilderTestObject::test<20>()
	{
	  char binData[] = "abcdefghijklmnop";

	  addValue(messageBlockData, (char *)"testBinData", &binData, MVT_FIXED, sizeof(binData));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  std::vector<U8> v = output["testBlock"][0]["testBinData"].asBinary();
	  ensure("Ensure MVT_S16Array data copied from legacy to llsd give a valid vector", v.size() > 0);

	  ensure_memory_matches("Ensure fixed binary data works in a message copied from legacy to llsd",
		  &v[0], sizeof(binData), binData, sizeof(binData));
	}

	// MVT_VARIABLE data_size 1 (U8's)
	template<> template<>
	void LLSDMessageBuilderTestObject::test<21>()
	{
	 /* U8 binData[] = "abcdefghijklmnop";

	  addValue(messageBlockData, "testBinData", &binData, MVT_VARIABLE, sizeof(binData), 1);
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  std::vector<U8> v = output["testBlock"][0]["testBinData"].asBinary();
	  ensure("Ensure MVT_S16Array data copied from legacy to llsd give a valid vector", v.size() > 0);

	  ensure_memory_matches("Ensure MVT_VARIABLE U8 binary data works in a message copied from legacy to llsd",
		  &v[0], sizeof(binData), binData, sizeof(binData));*/
	}

	// MVT_VARIABLE data_size 2 (U16's)
	template<> template<>
	void LLSDMessageBuilderTestObject::test<22>()
	{
	  U16 binData[] = {1,2,3,4,5,6,7,8,9}; //9 shorts

	  addValue(messageBlockData, (char *)"testBinData", &binData, MVT_VARIABLE, sizeof(binData) >> 1, 2);
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  std::vector<U8> v = output["testBlock"][0]["testBinData"].asBinary();
	  ensure("Ensure MVT_S16Array data copied from legacy to llsd give a valid vector", v.size() > 0);

	  ensure_memory_matches("Ensure MVT_VARIABLE U16 binary data works in a message copied from legacy to llsd",
		  &v[0], sizeof(binData) >> 1, binData, sizeof(binData) >> 1);
	}

	// MVT_VARIABLE data_size 4 (S32's)
	template<> template<>
	void LLSDMessageBuilderTestObject::test<23>()
	{
	  U32 binData[] = {9,8,7,6,5,4,3,2,1};

	  addValue(messageBlockData, (char *)"testBinData", &binData, MVT_VARIABLE, sizeof(binData) >> 2, 4);
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  std::vector<U8> v = output["testBlock"][0]["testBinData"].asBinary();
	  ensure("Ensure MVT_S16Array data copied from legacy to llsd give a valid vector", v.size() > 0);

	  ensure_memory_matches("Ensure MVT_VARIABLE S32 binary data works in a message copied from legacy to llsd",
		  &v[0], sizeof(binData) >> 2, binData, sizeof(binData) >> 2);
	}

	// MVT_U8
	template<> template<>
	void LLSDMessageBuilderTestObject::test<24>()
	{
	  U8 data = 0xa5;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_U8, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_U8 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asInteger(), data);
	}

	// MVT_U16
	template<> template<>
	void LLSDMessageBuilderTestObject::test<25>()
	{
	  U16 data = 0xa55a;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_U16, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_U16 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asInteger(), data);
	}

	// MVT_U32
	template<> template<>
	void LLSDMessageBuilderTestObject::test<26>()
	{
	  U32 data = 0xa55a7117;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_U32, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_U32 data works in a message copied from legacy to llsd",
		  ll_U32_from_sd(output["testBlock"][0]["testBinData"]), data);
	}

	// MVT_U64 - crush into an s32: LLSD does not support 64 bit values
	template<> template<>
	void LLSDMessageBuilderTestObject::test<27>()
	{
	  U64 data = U64L(0xa55a711711223344);
	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_U64, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_U64 data works in a message copied from legacy to llsd",
		  ll_U64_from_sd(output["testBlock"][0]["testBinData"]), data);
	}

	// MVT_S8
	template<> template<>
	void LLSDMessageBuilderTestObject::test<28>()
	{
	  S8 data = -31;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_S8, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_S8 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asInteger(), data);
	}

	// MVT_S16
	template<> template<>
	void LLSDMessageBuilderTestObject::test<29>()
	{
	  S16 data = -31;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_S16, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_S16 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asInteger(), data);
	}

	// MVT_S32
	template<> template<>
	void LLSDMessageBuilderTestObject::test<30>()
	{
	  S32 data = -3100;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_S32, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_S32 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asInteger(), data);
	}

	// MVT_S64 - crush into an s32: LLSD does not support 64 bit values
	template<> template<>
	void LLSDMessageBuilderTestObject::test<31>()
	{
	  S64 data = -31003100;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_S64, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_S64 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asInteger(), (S32)data);
	}

	// MVT_F32
	template<> template<>
	void LLSDMessageBuilderTestObject::test<32>()
	{
	  F32 data = 1234.1234f;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_F32, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_F32 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asReal(), data);
	}

	// MVT_F64
	template<> template<>
	void LLSDMessageBuilderTestObject::test<33>()
	{
	  F64 data = 1234.1234;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_F64, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_F64 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asReal(), data);
	}

	// MVT_LLVector3
	template<> template<>
	void LLSDMessageBuilderTestObject::test<34>()
	{
	  LLVector3 data(1,2,3);

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_LLVector3, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_LLVector3 data works in a message copied from legacy to llsd",
		  ll_vector3_from_sd(output["testBlock"][0]["testBinData"]), data);
	}

	// MVT_LLVector3d
	template<> template<>
	void LLSDMessageBuilderTestObject::test<35>()
	{
	  LLVector3d data(1,2,3);

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_LLVector3d, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_LLVector3 data works in a message copied from legacy to llsd",
		  ll_vector3d_from_sd(output["testBlock"][0]["testBinData"]), data);
	}

	// MVT_LLVector4
	template<> template<>
	void LLSDMessageBuilderTestObject::test<36>()
	{
	  LLVector4 data(1,2,3,4);
	  LLSD v = ll_sd_from_vector4(data);

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_LLVector4, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_LLVector4 data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"], v);
	}

	// MVT_LLQuaternion
	template<> template<>
	void LLSDMessageBuilderTestObject::test<37>()
	{
	  LLQuaternion data(1,2,3,0);

	  //we send a quaternion packed into a vec3 (w is infered) - so sizeof(vec) == 12 bytes not 16.
	  LLVector3 vec = data.packToVector3();

	  addValue(messageBlockData, (char *)"testBinData", &vec, MVT_LLQuaternion, sizeof(vec));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_LLQuaternion data works in a message copied from legacy to llsd",
		  ll_quaternion_from_sd(output["testBlock"][0]["testBinData"]), data);
	}

	// MVT_LLUUID
	template<> template<>
	void LLSDMessageBuilderTestObject::test<38>()
	{
	  LLUUID data("01234567-0123-0123-0123-234567abcdef");

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_LLUUID, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();
	 
	  std::string v = output["testBlock"][0]["testBinData"].asUUID().asString();

	  ensure_equals("Ensure MVT_LLUUID data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asUUID(), data);
	}

	// MVT_BOOL
	template<> template<>
	void LLSDMessageBuilderTestObject::test<39>()
	{
	  BOOL valueTrue = true;
	  BOOL valueFalse = false;

	  LLMsgData* md = new LLMsgData("testMessage");
	  LLMsgBlkData* mbd = new LLMsgBlkData("testBlock", 0);
	  addValue(mbd, (char *)"testBoolFalse", &valueFalse, MVT_BOOL, sizeof(BOOL));
	  addValue(mbd, (char *)"testBoolTrue", &valueTrue, MVT_BOOL, sizeof(BOOL));
	  md->addBlock(mbd);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*md);
	  LLSD output = builder.getMessage();

	  ensure("Ensure bools work in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBoolTrue"].asBoolean() && !output["testBlock"][0]["testBoolFalse"].asBoolean());
	}

	// MVT_IP_ADDR
	template<> template<>
	void LLSDMessageBuilderTestObject::test<40>()
	{
	  U32 data(0xff887766);
	  LLSD v = ll_sd_from_ipaddr(data);

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_IP_ADDR, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_IP_ADDR data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"], v);
	}

	// MVT_IP_PORT
	template<> template<>
	void LLSDMessageBuilderTestObject::test<41>()
	{
	  U16 data = 0xff88;

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_IP_PORT, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  ensure_equals("Ensure MVT_IP_PORT data works in a message copied from legacy to llsd",
		  output["testBlock"][0]["testBinData"].asInteger(), data);
	}

	// MVT_U16Vec3
	template<> template<>
	void LLSDMessageBuilderTestObject::test<42>()
	{
	  U16 data[3] = {0,1,2};

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_U16Vec3, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  std::vector<U8> v = output["testBlock"][0]["testBinData"].asBinary();
	  ensure("Ensure MVT_U16Vec3 data copied from legacy to llsd give a valid vector", v.size() > 0);

	  ensure_memory_matches("Ensure MVT_U16Vec3 data works in a message copied from legacy to llsd",
		  (U16*)&v[0], 6, data, 6);
	}

	// MVT_U16Quat
	template<> template<>
	void LLSDMessageBuilderTestObject::test<43>()
	{
	  U16 data[4] = {0,1,2,4};

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_U16Quat, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  std::vector<U8> v = output["testBlock"][0]["testBinData"].asBinary();
	  ensure("Ensure MVT_U16Quat data copied from legacy to llsd give a valid vector", v.size() > 0);

	  ensure_memory_matches("Ensure MVT_U16Quat data works in a message copied from legacy to llsd",
		  (U16*)&v[0], 8, data, 8);
	}

	// MVT_S16Array
	template<> template<>
	void LLSDMessageBuilderTestObject::test<44>()
	{
	  S16 data[19] = {0,-1,2,-4,5,-6,7,-8,9,-10,11,-12,13,-14,15,16,17,18};

	  addValue(messageBlockData, (char *)"testBinData", &data, MVT_S16Array, sizeof(data));
	  messageData->addBlock(messageBlockData);
	  LLSDMessageBuilder builder = defaultBuilder();
	  
	  builder.copyFromMessageData(*messageData);
	  LLSD output = builder.getMessage();

	  std::vector<U8> v = output["testBlock"][0]["testBinData"].asBinary();
	  ensure("Ensure MVT_S16Array data copied from legacy to llsd give a valid vector", v.size() > 0);

	  ensure_memory_matches("Ensure MVT_S16Array data works in a message copied from legacy to llsd",
		  (U16*)&v[0], 19, data, 19);
	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<45>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultTemplateBlock(MVT_U8, 1));
		U8 inValue = 2;
		LLTemplateMessageBuilder* template_builder = defaultTemplateBuilder(messageTemplate);
		template_builder->addU8(_PREHASH_Test0, inValue);

		LLSDMessageBuilder builder;
		builder.copyFromMessageData(*template_builder->getCurrentMessage());
		LLSD output = builder.getMessage();
		
		ensure_equals(output["Test0"][0]["Test0"].asInteger(), 2);

	}

	template<> template<>
	void LLSDMessageBuilderTestObject::test<46>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultTemplateBlock(MVT_VARIABLE, 1));
		std::string inValue = "testing";
		LLTemplateMessageBuilder* builder = defaultTemplateBuilder(messageTemplate);
		builder->addString(_PREHASH_Test0, inValue.c_str());

		LLSDMessageBuilder sd_builder;
		sd_builder.copyFromMessageData(*builder->getCurrentMessage());
		LLSD output = sd_builder.getMessage();
		
		ensure_equals(output["Test0"][0]["Test0"].asString(), std::string("testing"));
	}

}

