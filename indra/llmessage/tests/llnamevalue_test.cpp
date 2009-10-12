/** 
 * @file llnamevalue_tut.cpp
 * @author Adroit
 * @date 2007-02
 * @brief LLNameValue unit test
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
#include "llsdserialize.h"

#include "../llnamevalue.h"

#include "../test/lltut.h"


#if LL_WINDOWS
// disable unreachable code warnings
#pragma warning(disable: 4702)
#endif

namespace tut
{
	struct namevalue_test
	{
		namevalue_test() 
		{			
		}
 	};
	typedef test_group<namevalue_test> namevalue_t;
	typedef namevalue_t::object namevalue_object_t;
	tut::namevalue_t tut_namevalue("namevalue_test");

	
	template<> template<>
	void namevalue_object_t::test<1>()
	{
		// LLNameValue()
		LLNameValue nValue;
		ensure("mName should have been NULL", nValue.mName == NULL);
		ensure("getTypeEnum failed",nValue.getTypeEnum() == NVT_NULL);
		ensure("getClassEnum failed",nValue.getClassEnum() == NVC_NULL);
		ensure("getSendtoEnum failed",nValue.getSendtoEnum() == NVS_NULL);

		LLNameValue nValue1(" SecondLife ASSET RW SIM 232324343");

	}

	// LLNameValue(const char* data);
	// 	LLNameValue(const char* name, const char* data, const char* type, const char* nvclass, const char* nvsendto, 
	//				TNameValueCallback nvcb = NULL, void** user_data = NULL);
	template<> template<>
	void namevalue_object_t::test<2>()
	{
		LLNameValue nValue(" SecondLife ASSET RW S 232324343");
		ensure("mName not set correctly", (0 == strcmp(nValue.mName,"SecondLife")));
		ensure("getTypeEnum failed", nValue.getTypeEnum() == NVT_ASSET);
		ensure("getClassEnum failed", nValue.getClassEnum() == NVC_READ_WRITE);
		ensure("getSendtoEnum failed", nValue.getSendtoEnum() == NVS_SIM);
		ensure("getString failed", (0==strcmp(nValue.getAsset(),"232324343")));
		ensure("sendToData or sendToViewer failed", !nValue.sendToData() && !nValue.sendToViewer());
		
		LLNameValue nValue1("\n\r SecondLife_1 STRING READ_WRITE SIM 232324343");
		ensure("1. mName not set correctly", (0 == strcmp(nValue1.mName,"SecondLife_1")));
		ensure("1. getTypeEnum failed", nValue1.getTypeEnum() == NVT_STRING);
		ensure("1. getClassEnum failed", nValue1.getClassEnum() == NVC_READ_WRITE);
		ensure("1. getSendtoEnum failed", nValue1.getSendtoEnum() == NVS_SIM);
		ensure("1. getString failed", (0==strcmp(nValue1.getString(),"232324343")));
		ensure("1. sendToData or sendToViewer failed", !nValue1.sendToData() && !nValue1.sendToViewer());

		LLNameValue nValue2("SecondLife", "23.5", "F32", "R", "DS");
		ensure("2. getTypeEnum failed", nValue2.getTypeEnum() == NVT_F32);
		ensure("2. getClassEnum failed", nValue2.getClassEnum() == NVC_READ_ONLY);
		ensure("2. getSendtoEnum failed", nValue2.getSendtoEnum() == NVS_DATA_SIM);
		ensure("2. getF32 failed", *nValue2.getF32() == 23.5f);
		ensure("2. sendToData or sendToViewer failed", nValue2.sendToData() && !nValue2.sendToViewer());

		LLNameValue nValue3("SecondLife", "-43456787", "S32", "READ_ONLY", "SIM_SPACE");
		ensure("3. getTypeEnum failed", nValue3.getTypeEnum() == NVT_S32);
		ensure("3. getClassEnum failed", nValue3.getClassEnum() == NVC_READ_ONLY);
		ensure("3. getSendtoEnum failed", nValue3.getSendtoEnum() == NVS_DATA_SIM);
		ensure("3. getS32 failed", *nValue3.getS32() == -43456787);
		ensure("sendToData or sendToViewer failed", nValue3.sendToData() && !nValue3.sendToViewer());

		LLNameValue nValue4("SecondLife", "<1.0, 2.0, 3.0>", "VEC3", "RW", "SV");
		LLVector3 llvec4(1.0, 2.0, 3.0);
		ensure("4. getTypeEnum failed", nValue4.getTypeEnum() == NVT_VEC3);
		ensure("4. getClassEnum failed", nValue4.getClassEnum() == NVC_READ_WRITE);
		ensure("4. getSendtoEnum failed", nValue4.getSendtoEnum() == NVS_SIM_VIEWER);
		ensure("4. getVec3 failed", *nValue4.getVec3() == llvec4);
		ensure("4. sendToData or sendToViewer failed", !nValue4.sendToData() && nValue4.sendToViewer());

		LLNameValue nValue5("SecondLife", "-1.0, 2.4, 3", "VEC3", "RW", "SIM_VIEWER");
		LLVector3 llvec5(-1.0f, 2.4f, 3);
		ensure("5. getTypeEnum failed", nValue5.getTypeEnum() == NVT_VEC3);
		ensure("5. getClassEnum failed", nValue5.getClassEnum() == NVC_READ_WRITE);
		ensure("5. getSendtoEnum failed", nValue5.getSendtoEnum() == NVS_SIM_VIEWER);
		ensure("5. getVec3 failed", *nValue5.getVec3() == llvec5);
		ensure("5. sendToData or sendToViewer failed", !nValue5.sendToData() && nValue5.sendToViewer());

		LLNameValue nValue6("SecondLife", "89764323", "U32", "RW", "DSV");
		ensure("6. getTypeEnum failed", nValue6.getTypeEnum() == NVT_U32);
		ensure("6. getClassEnum failed", nValue6.getClassEnum() == NVC_READ_WRITE);
		ensure("6. getSendtoEnum failed", nValue6.getSendtoEnum() == NVS_DATA_SIM_VIEWER);
		ensure("6. getU32 failed", *nValue6.getU32() == 89764323);
		ensure("6. sendToData or sendToViewer failed", nValue6.sendToData() && nValue6.sendToViewer());

		LLNameValue nValue7("SecondLife", "89764323323232", "U64", "RW", "SIM_SPACE_VIEWER");
		U64 u64_7 = U64L(89764323323232);
		ensure("7. getTypeEnum failed", nValue7.getTypeEnum() == NVT_U64);
		ensure("7. getClassEnum failed", nValue7.getClassEnum() == NVC_READ_WRITE);
		ensure("7. getSendtoEnum failed", nValue7.getSendtoEnum() == NVS_DATA_SIM_VIEWER);
		ensure("7. getU32 failed", *nValue7.getU64() == u64_7);
		ensure("7. sendToData or sendToViewer failed", nValue7.sendToData() && nValue7.sendToViewer());
	}

	// LLNameValue(const char* name, const char* data, const char* type, const char* nvclass, 
	//				TNameValueCallback nvcb = NULL, void** user_data = NULL);
	template<> template<>
	void namevalue_object_t::test<3>()
	{
		LLNameValue nValue("SecondLife", "232324343", "ASSET", "READ_WRITE");
		ensure("mName not set correctly", (0 == strcmp(nValue.mName,"SecondLife")));
		ensure("getTypeEnum failed", nValue.getTypeEnum() == NVT_ASSET);
		ensure("getClassEnum failed", nValue.getClassEnum() == NVC_READ_WRITE);
		ensure("getSendtoEnum failed", nValue.getSendtoEnum() == NVS_SIM);
		ensure("getString failed", (0==strcmp(nValue.getAsset(),"232324343")));

		LLNameValue nValue1("SecondLife", "232324343", "STRING", "READ_WRITE");
		ensure("1. mName not set correctly", (0 == strcmp(nValue1.mName,"SecondLife")));
		ensure("1. getTypeEnum failed", nValue1.getTypeEnum() == NVT_STRING);
		ensure("1. getClassEnum failed", nValue1.getClassEnum() == NVC_READ_WRITE);
		ensure("1. getSendtoEnum failed", nValue1.getSendtoEnum() == NVS_SIM);
		ensure("1. getString failed", (0==strcmp(nValue1.getString(),"232324343")));

		LLNameValue nValue2("SecondLife", "23.5", "F32", "R");
		ensure("2. getTypeEnum failed", nValue2.getTypeEnum() == NVT_F32);
		ensure("2. getClassEnum failed", nValue2.getClassEnum() == NVC_READ_ONLY);
		ensure("2. getSendtoEnum failed", nValue2.getSendtoEnum() == NVS_SIM);
		ensure("2. getF32 failed", *nValue2.getF32() == 23.5f);

		LLNameValue nValue3("SecondLife", "-43456787", "S32", "READ_ONLY");
		ensure("3. getTypeEnum failed", nValue3.getTypeEnum() == NVT_S32);
		ensure("3. getClassEnum failed", nValue3.getClassEnum() == NVC_READ_ONLY);
		ensure("3. getSendtoEnum failed", nValue3.getSendtoEnum() == NVS_SIM);
		ensure("3. getS32 failed", *nValue3.getS32() == -43456787);

		LLNameValue nValue4("SecondLife", "<1.0, 2.0, 3.0>", "VEC3", "RW");
		LLVector3 llvec4(1.0, 2.0, 3.0);
		ensure("4. getTypeEnum failed", nValue4.getTypeEnum() == NVT_VEC3);
		ensure("4. getClassEnum failed", nValue4.getClassEnum() == NVC_READ_WRITE);
		ensure("4. getSendtoEnum failed", nValue4.getSendtoEnum() == NVS_SIM);
		ensure("4. getVec3 failed", *nValue4.getVec3() == llvec4);

		LLNameValue nValue5("SecondLife", "-1.0, 2.4, 3", "VEC3", "RW");
		LLVector3 llvec5(-1.0f, 2.4f, 3);
		ensure("5. getTypeEnum failed", nValue5.getTypeEnum() == NVT_VEC3);
		ensure("5. getClassEnum failed", nValue5.getClassEnum() == NVC_READ_WRITE);
		ensure("5. getSendtoEnum failed", nValue5.getSendtoEnum() == NVS_SIM);
		ensure("5. getVec3 failed", *nValue5.getVec3() == llvec5);

		LLNameValue nValue6("SecondLife", "89764323", "U32", "RW");
		ensure("6. getTypeEnum failed", nValue6.getTypeEnum() == NVT_U32);
		ensure("6. getClassEnum failed", nValue6.getClassEnum() == NVC_READ_WRITE);
		ensure("6. getSendtoEnum failed", nValue6.getSendtoEnum() == NVS_SIM);
		ensure("6. getU32 failed", *nValue6.getU32() == 89764323);

		LLNameValue nValue7("SecondLife", "89764323323232", "U64", "RW");
		U64 u64_7 = U64L(89764323323232);
		ensure("7. getTypeEnum failed", nValue7.getTypeEnum() == NVT_U64);
		ensure("7. getClassEnum failed", nValue7.getClassEnum() == NVC_READ_WRITE);
		ensure("7. getSendtoEnum failed", nValue7.getSendtoEnum() == NVS_SIM);
		ensure("7. getU32 failed", *nValue7.getU64() == u64_7);
	}

	// LLNameValue(const char* name, const char* type, const char* nvclass, 
	//				TNameValueCallback nvcb = NULL, void** user_data = NULL);
	template<> template<>
	void namevalue_object_t::test<4>()
	{
		LLNameValue nValue("SecondLife",  "STRING", "READ_WRITE");
		ensure("mName not set correctly", (0 == strcmp(nValue.mName,"SecondLife")));
		ensure("getTypeEnum failed", nValue.getTypeEnum() == NVT_STRING);
		ensure("getClassEnum failed", nValue.getClassEnum() == NVC_READ_WRITE);
		ensure("getSendtoEnum failed", nValue.getSendtoEnum() == NVS_SIM);

		LLNameValue nValue1("SecondLife",  "ASSET", "READ_WRITE");
		ensure("1. mName not set correctly", (0 == strcmp(nValue1.mName,"SecondLife")));
		ensure("1. getTypeEnum for RW failed", nValue1.getTypeEnum() == NVT_ASSET);
		ensure("1. getClassEnum for RW failed", nValue1.getClassEnum() == NVC_READ_WRITE);
		ensure("1. getSendtoEnum for RW failed", nValue1.getSendtoEnum() == NVS_SIM);

		LLNameValue nValue2("SecondLife", "F32", "READ_ONLY");
		ensure("2. getTypeEnum failed", nValue2.getTypeEnum() == NVT_F32);
		ensure("2. getClassEnum failed", nValue2.getClassEnum() == NVC_READ_ONLY);
		ensure("2. getSendtoEnum failed", nValue2.getSendtoEnum() == NVS_SIM);

		LLNameValue nValue3("SecondLife", "S32", "READ_ONLY");
		ensure("3. getTypeEnum failed", nValue3.getTypeEnum() == NVT_S32);
		ensure("3. getClassEnum failed", nValue3.getClassEnum() == NVC_READ_ONLY);
		ensure("3. getSendtoEnum failed", nValue3.getSendtoEnum() == NVS_SIM);

		LLNameValue nValue4("SecondLife", "VEC3", "READ_WRITE");
		ensure("4. getTypeEnum failed", nValue4.getTypeEnum() == NVT_VEC3);
		ensure("4. getClassEnum failed", nValue4.getClassEnum() == NVC_READ_WRITE);
		ensure("4. getSendtoEnum failed", nValue4.getSendtoEnum() == NVS_SIM);

		LLNameValue nValue6("SecondLife", "U32", "READ_WRITE");
		ensure("6. getTypeEnum failed", nValue6.getTypeEnum() == NVT_U32);
		ensure("6. getClassEnum failed", nValue6.getClassEnum() == NVC_READ_WRITE);
		ensure("6. getSendtoEnum failed", nValue6.getSendtoEnum() == NVS_SIM);

		LLNameValue nValue7("SecondLife", "U64", "READ_WRITE");
		ensure("7. getTypeEnum failed", nValue7.getTypeEnum() == NVT_U64);
		ensure("7. getClassEnum failed", nValue7.getClassEnum() == NVC_READ_WRITE);
		ensure("7. getSendtoEnum failed", nValue7.getSendtoEnum() == NVS_SIM);
	}

	template<> template<>
	void namevalue_object_t::test<5>()
	{
		LLNameValue nValue("SecondLife", "This is a test", "STRING", "RW", "SIM");

		ensure("getString failed", (0 == strcmp(nValue.getString(),"This is a test")));
	}

	template<> template<>
	void namevalue_object_t::test<6>()
	{
		LLNameValue nValue("SecondLife", "This is a test", "ASSET", "RW", "S");
		ensure("getAsset failed", (0 == strcmp(nValue.getAsset(),"This is a test")));
	}
         	
	template<> template<>
	void namevalue_object_t::test<7>()
	{
		LLNameValue nValue("SecondLife", "555555", "F32", "RW", "SIM");
		
		ensure("getF32 failed",*nValue.getF32() == 555555.f);
	}

	template<> template<>
	void namevalue_object_t::test<8>()
	{
		LLNameValue nValue("SecondLife", "-5555", "S32", "RW", "SIM");

		ensure("getS32 failed", *nValue.getS32() == -5555);

		S32 sVal = 0x7FFFFFFF;
		nValue.setS32(sVal);
		ensure("getS32 failed", *nValue.getS32() == sVal);

		sVal = -0x7FFFFFFF;
		nValue.setS32(sVal);
		ensure("getS32 failed", *nValue.getS32() == sVal);

		sVal = 0;
		nValue.setS32(sVal);
		ensure("getS32 failed", *nValue.getS32() == sVal);
	}
	
	template<> template<>
	void namevalue_object_t::test<9>()
	{
		LLNameValue nValue("SecondLife", "<-3, 2, 1>", "VEC3", "RW", "SIM");
		LLVector3 vecExpected(-3, 2, 1);
		LLVector3 vec;
		nValue.getVec3(vec);
		ensure("getVec3 failed", vec == vecExpected);
	}
	
	template<> template<>
	void namevalue_object_t::test<10>()
	{
		LLNameValue nValue("SecondLife", "12345678", "U32", "RW", "SIM");

		ensure("getU32 failed",*nValue.getU32() == 12345678);

		U32 val = 0xFFFFFFFF;
		nValue.setU32(val);
		ensure("U32 max", *nValue.getU32() == val);

		val = 0;
		nValue.setU32(val);		
		ensure("U32 min", *nValue.getU32() == val);
	}
	
	template<> template<>
	void namevalue_object_t::test<11>()
	{
		//skip_fail("incomplete support for U64.");
		LLNameValue nValue("SecondLife", "44444444444", "U64", "RW", "SIM");

		ensure("getU64 failed",*nValue.getU64() == U64L(44444444444));

		// there is no LLNameValue::setU64()  
	}
	
	
	template<> template<>
	void namevalue_object_t::test<12>()
	{
		//skip_fail("incomplete support for U64.");
		LLNameValue nValue("SecondLife U64 RW DSV 44444444444");	
		std::string ret_str = nValue.printNameValue();
		
		ensure_equals("1:printNameValue failed",ret_str,"SecondLife U64 RW DSV 44444444444");
		
		LLNameValue nValue1(ret_str.c_str());
		ensure_equals("Serialization of printNameValue failed", *nValue.getU64(), *nValue1.getU64());		
	}

	template<> template<>
	void namevalue_object_t::test<13>()
	{
		LLNameValue nValue("SecondLife STRING RW DSV 44444444444");	
		std::string ret_str = nValue.printData();
		ensure_equals("1:printData failed",ret_str,"44444444444");

		LLNameValue nValue1("SecondLife S32 RW DSV 44444");	
		ret_str = nValue1.printData();
		ensure_equals("2:printData failed",ret_str,"44444");		
	}
	
	template<> template<>
	void namevalue_object_t::test<14>()
	{
		LLNameValue nValue("SecodLife STRING RW SIM 22222");
		std::ostringstream stream1,stream2,stream3, stream4, stream5;
		stream1 << nValue;
		ensure_equals("STRING << failed",stream1.str(),"22222");

		LLNameValue nValue1("SecodLife F32 RW SIM 22222");
		stream2 << nValue1;
		ensure_equals("F32 << failed",stream2.str(),"22222");

		LLNameValue nValue2("SecodLife S32 RW SIM 22222");
		stream3<< nValue2;
		ensure_equals("S32 << failed",stream3.str(),"22222");

		LLNameValue nValue3("SecodLife U32 RW SIM 122222");
		stream4<< nValue3;
		ensure_equals("U32 << failed",stream4.str(),"122222");

		// I don't think we use U64 name value pairs.  JC
		//skip_fail("incomplete support for U64.");
		//LLNameValue nValue4("SecodLife U64 RW SIM 22222");
		//stream5<< nValue4;
		//ensure("U64 << failed",0 == strcmp((stream5.str()).c_str(),"22222"));
	}

	template<> template<>
	void namevalue_object_t::test<15>()
	{
		LLNameValue nValue("SecondLife", "This is a test", "ASSET", "R", "S");

		ensure("getAsset failed", (0 == strcmp(nValue.getAsset(),"This is a test")));
		// this should not have updated as it is read only.
		nValue.setAsset("New Value should not be updated");
		ensure("setAsset on ReadOnly failed", (0 == strcmp(nValue.getAsset(),"This is a test")));

		LLNameValue nValue1("SecondLife", "1234", "U32", "R", "S");
		// this should not have updated as it is read only.
		nValue1.setU32(4567);
		ensure("setU32 on ReadOnly failed", *nValue1.getU32() == 1234);

		LLNameValue nValue2("SecondLife", "1234", "S32", "R", "S");
		// this should not have updated as it is read only.
		nValue2.setS32(4567);
		ensure("setS32 on ReadOnly failed", *nValue2.getS32() == 1234);

		LLNameValue nValue3("SecondLife", "1234", "F32", "R", "S");
		// this should not have updated as it is read only.
		nValue3.setF32(4567);
		ensure("setF32 on ReadOnly failed", *nValue3.getF32() == 1234);

		LLNameValue nValue4("SecondLife", "<1,2,3>", "VEC3", "R", "S");
		// this should not have updated as it is read only.
		LLVector3 vec(4,5,6);
		nValue3.setVec3(vec);
		LLVector3 vec1(1,2,3);
		ensure("setVec3 on ReadOnly failed", *nValue4.getVec3() == vec1);

		// cant test for U64 as no set64 exists nor any operators support U64 type
	}         
}
