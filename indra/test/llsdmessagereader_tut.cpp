/** 
 * @file llsdmessagereader_tut.cpp
 * @date   February 2006
 * @brief LLSDMessageReader unit tests
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
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"
#include "llquaternion.h"

#include "message.h"
#include "llsdmessagereader.h"
#include "llsdutil.h"
#include "llsdutil_math.h"

namespace tut
{	
	struct LLSDMessageReaderTestData {
		static void ensureMessageName(const std::string& msg_name,
									const LLSD& msg_data,
									const std::string& expected_name)
		{
			LLSDMessageReader msg;
			msg.setMessage(LLMessageStringTable::getInstance()->getString(msg_name.c_str()), msg_data);
			ensure_equals("Ensure name", std::string(msg.getMessageName()), 
						  expected_name);
		}

		static void ensureNumberOfBlocks(const LLSD& msg_data,
									const std::string& block,
									S32 expected_number)
		{
			LLSDMessageReader msg;
			msg.setMessage("fakename", msg_data);
			ensure_equals("Ensure number of blocks", msg.getNumberOfBlocks(block.c_str()), 
						  expected_number);
		}

		static void ensureMessageSize(const LLSD& msg_data,
									S32 expected_size)
		{
			LLSDMessageReader msg;
			msg.setMessage("fakename", msg_data);
			ensure_equals(	"Ensure size",	msg.getMessageSize(), expected_size);
		}

		static void ensureBool(const LLSD& msg_data,
								const std::string& block,
								const std::string& var,
								S32 blocknum,
								bool expected)
		{
			LLSDMessageReader msg;
			msg.setMessage("fakename", msg_data);
			bool test_data;
			msg.getBOOL(block.c_str(), var.c_str(), test_data, blocknum);
 			ensure_equals(	"Ensure bool field", test_data, expected);
 		}
 	};
	
 	typedef test_group<LLSDMessageReaderTestData>	LLSDMessageReaderTestGroup;
 	typedef LLSDMessageReaderTestGroup::object		LLSDMessageReaderTestObject;
 	LLSDMessageReaderTestGroup llsdMessageReaderTestGroup("LLSDMessageReader");
	
 	template<> template<>
 	void LLSDMessageReaderTestObject::test<1>()
 		// construction and test of empty LLSD
 	{
 		LLSD message = LLSD::emptyMap();

 		ensureMessageName("", message, "");
 		ensureNumberOfBlocks(message, "Fakeblock", 0);
 		ensureMessageSize(message, 0);
 	}
	
 	template<> template<>
 	void LLSDMessageReaderTestObject::test<2>()
 		// construction and test of simple message with one block
 	{
 		LLSD message = LLSD::emptyMap();
 		message["block1"] = LLSD::emptyArray();
 		message["block1"][0] = LLSD::emptyMap();
 		message["block1"][0]["Field1"] = 0;

 		ensureMessageName("name2", message, "name2");
 		ensureNumberOfBlocks(message, "block1", 1);
 		ensureMessageSize(message, 0);
 	}
	
 	template<> template<>
 	void LLSDMessageReaderTestObject::test<3>()
 		// multiple blocks
 	{
 		LLSD message = LLSD::emptyMap();
 		message["block1"] = LLSD::emptyArray();
 		bool bool_true = true;
 		bool bool_false = false;
 		message["block1"][0] = LLSD::emptyMap();
 		message["block1"][0]["BoolField1"] = bool_true;
		message["block1"][1] = LLSD::emptyMap();
 		message["block1"][1]["BoolField1"] = bool_false;
 		message["block1"][1]["BoolField2"] = bool_true;

 		ensureMessageName("name3", message, "name3");
 		ensureBool(message, "block1", "BoolField1", 0, true);
 		ensureBool(message, "block1", "BoolField1", 1, false);
 		ensureBool(message, "block1", "BoolField2", 1, true);
 		ensureNumberOfBlocks(message, "block1", 2);
 		ensureMessageSize(message, 0);
 	}
	
 	template<typename T>
 	LLSDMessageReader testType(const T& value)
 	{
 		LLSD message = LLSD::emptyMap();
 		message["block"][0]["var"] = value;
 		LLSDMessageReader msg;
 		msg.setMessage("fakename", message);
 		return msg;
 	}

 	template<> template<>
 	void LLSDMessageReaderTestObject::test<4>()
 		// S8
 	{
 		S8 outValue, inValue = -3;
 		LLSDMessageReader msg = testType(inValue);
 		msg.getS8("block", "var", outValue);
 		ensure_equals("Ensure S8", outValue, inValue);
 	}
 	template<> template<>
 	void 
	LLSDMessageReaderTestObject::test<5>()
		// U8
	{
		U8 outValue, inValue = 2;
		LLSDMessageReader msg = testType(inValue);
		msg.getU8("block", "var", outValue);
		ensure_equals("Ensure U8", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<6>()
		// S16
	{
		S16 outValue, inValue = 90;
		LLSDMessageReader msg = testType(inValue);
		msg.getS16("block", "var", outValue);
		ensure_equals("Ensure S16", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<7>()
		// U16
	{
		U16 outValue, inValue = 3;
		LLSDMessageReader msg = testType(inValue);
		msg.getU16("block", "var", outValue);
		ensure_equals("Ensure S16", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<8>()
		// S32
	{
		S32 outValue, inValue = 44;
		LLSDMessageReader msg = testType(inValue);
		msg.getS32("block", "var", outValue);
		ensure_equals("Ensure S32", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<9>()
		// F32
	{
		F32 outValue, inValue = 121.44f;
		LLSDMessageReader msg = testType(inValue);
		msg.getF32("block", "var", outValue);
		ensure_equals("Ensure F32", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<10>()
		// U32
	{
		U32 outValue, inValue = 88;
		LLSD sdValue = ll_sd_from_U32(inValue);
		LLSDMessageReader msg = testType(sdValue);
		msg.getU32("block", "var", outValue);
		ensure_equals("Ensure U32", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<11>()
		// U64
	{
		U64 outValue, inValue = 121;
		LLSD sdValue = ll_sd_from_U64(inValue);
		LLSDMessageReader msg = testType(sdValue);
		msg.getU64("block", "var", outValue);
		ensure_equals("Ensure U64", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<12>()
		// F64
	{
		F64 outValue, inValue = 3232143.33;
		LLSDMessageReader msg = testType(inValue);
		msg.getF64("block", "var", outValue);
		ensure_equals("Ensure F64", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<13>()
		// String
	{
		 std::string outValue, inValue = "testing";
		LLSDMessageReader msg = testType<std::string>(inValue.c_str());
		
		char buffer[MAX_STRING];
		msg.getString("block", "var", MAX_STRING, buffer);
		outValue = buffer;
		ensure_equals("Ensure String", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<14>()
		// Vector3
	{
		 LLVector3 outValue, inValue = LLVector3(1,2,3);
		LLSD sdValue = ll_sd_from_vector3(inValue);
		LLSDMessageReader msg = testType(sdValue);
		msg.getVector3("block", "var", outValue);
		ensure_equals("Ensure Vector3", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<15>()
		// Vector4
	{
		LLVector4 outValue, inValue = LLVector4(1,2,3,4);
		LLSD sdValue = ll_sd_from_vector4(inValue);
		LLSDMessageReader msg = testType(sdValue);
		msg.getVector4("block", "var", outValue);
		ensure_equals("Ensure Vector4", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<16>()
		// Vector3d
	{
		 LLVector3d outValue, inValue = LLVector3d(1,2,3);
		 LLSD sdValue = ll_sd_from_vector3d(inValue);
		LLSDMessageReader msg = testType(sdValue);
		msg.getVector3d("block", "var", outValue);
		ensure_equals("Ensure Vector3d", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<17>()
		// Quaternion
	{
		LLQuaternion outValue, inValue = LLQuaternion(1,LLVector3(2,3,4));
		LLSD sdValue = ll_sd_from_quaternion(inValue);
		LLSDMessageReader msg = testType(sdValue);
		msg.getQuat("block", "var", outValue);
		ensure_equals("Ensure Quaternion", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<18>()
		// UUID
	{
		LLUUID outValue, inValue;
		inValue.generate();
		LLSDMessageReader msg = testType(inValue);
		msg.getUUID("block", "var", outValue);
		ensure_equals("Ensure UUID", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<19>()
		// IPAddr
	{
		U32 outValue, inValue = 12344556;
		LLSD sdValue = ll_sd_from_ipaddr(inValue);
		LLSDMessageReader msg = testType(sdValue);
		msg.getIPAddr("block", "var", outValue);
		ensure_equals("Ensure IPAddr", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<20>()
		// IPPort
	{
		U16 outValue, inValue = 80;
		LLSDMessageReader msg = testType(inValue);
		msg.getIPPort("block", "var", outValue);
		ensure_equals("Ensure IPPort", outValue, inValue);
	}
	template<> template<>
	void LLSDMessageReaderTestObject::test<21>()
		// Binary 
	{
		std::vector<U8> outValue(2), inValue(2);
		inValue[0] = 0;
		inValue[1] = 1;
	  
		LLSDMessageReader msg = testType(inValue);
		msg.getBinaryData("block", "var", &(outValue[0]), inValue.size());
		ensure_equals("Ensure Binary", outValue, inValue);
	}
}
