/**
 * @file llnamevalue_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief LLNameValue unit test
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
#include "llsdserialize.h"

#include "../llnamevalue.h"

#include "../test/lldoctest.h"


#if LL_WINDOWS
// disable unreachable code warnings
#pragma warning(disable: 4702)
#endif

TEST_SUITE("LLNameValue") {

struct namevalue_test
{

        namevalue_test()
        {
        
};

TEST_CASE_FIXTURE(namevalue_test, "test_1")
{

        // LLNameValue()
        LLNameValue nValue;
        CHECK_MESSAGE(nValue.mName == NULL, "mName should have been NULL");
        CHECK_MESSAGE(nValue.getTypeEnum(, "getTypeEnum failed") == NVT_NULL);
        CHECK_MESSAGE(nValue.getClassEnum(, "getClassEnum failed") == NVC_NULL);
        CHECK_MESSAGE(nValue.getSendtoEnum(, "getSendtoEnum failed") == NVS_NULL);

        LLNameValue nValue1(" SecondLife ASSET RW SIM 232324343");

    
}

TEST_CASE_FIXTURE(namevalue_test, "test_2")
{

        LLNameValue nValue(" SecondLife ASSET RW S 232324343");
        CHECK_MESSAGE((0 == strcmp(nValue.mName,"SecondLife", "mName not set correctly")));
        CHECK_MESSAGE(nValue.getTypeEnum(, "getTypeEnum failed") == NVT_ASSET);
        CHECK_MESSAGE(nValue.getClassEnum(, "getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue.getSendtoEnum(, "getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE((0==strcmp(nValue.getAsset(, "getString failed"),"232324343")));
        CHECK_MESSAGE(!nValue.sendToData(, "sendToData or sendToViewer failed") && !nValue.sendToViewer());

        LLNameValue nValue1("\n\r SecondLife_1 STRING READ_WRITE SIM 232324343");
        CHECK_MESSAGE((0 == strcmp(nValue1.mName,"SecondLife_1", "1. mName not set correctly")));
        CHECK_MESSAGE(nValue1.getTypeEnum(, "1. getTypeEnum failed") == NVT_STRING);
        CHECK_MESSAGE(nValue1.getClassEnum(, "1. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue1.getSendtoEnum(, "1. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE((0==strcmp(nValue1.getString(, "1. getString failed"),"232324343")));
        CHECK_MESSAGE(!nValue1.sendToData(, "1. sendToData or sendToViewer failed") && !nValue1.sendToViewer());

        LLNameValue nValue2("SecondLife", "23.5", "F32", "R", "DS");
        CHECK_MESSAGE(nValue2.getTypeEnum(, "2. getTypeEnum failed") == NVT_F32);
        CHECK_MESSAGE(nValue2.getClassEnum(, "2. getClassEnum failed") == NVC_READ_ONLY);
        CHECK_MESSAGE(nValue2.getSendtoEnum(, "2. getSendtoEnum failed") == NVS_DATA_SIM);
        CHECK_MESSAGE(*nValue2.getF32(, "2. getF32 failed") == 23.5f);
        CHECK_MESSAGE(nValue2.sendToData(, "2. sendToData or sendToViewer failed") && !nValue2.sendToViewer());

        LLNameValue nValue3("SecondLife", "-43456787", "S32", "READ_ONLY", "SIM_SPACE");
        CHECK_MESSAGE(nValue3.getTypeEnum(, "3. getTypeEnum failed") == NVT_S32);
        CHECK_MESSAGE(nValue3.getClassEnum(, "3. getClassEnum failed") == NVC_READ_ONLY);
        CHECK_MESSAGE(nValue3.getSendtoEnum(, "3. getSendtoEnum failed") == NVS_DATA_SIM);
        CHECK_MESSAGE(*nValue3.getS32(, "3. getS32 failed") == -43456787);
        CHECK_MESSAGE(nValue3.sendToData(, "sendToData or sendToViewer failed") && !nValue3.sendToViewer());

        LLNameValue nValue4("SecondLife", "<1.0, 2.0, 3.0>", "VEC3", "RW", "SV");
        LLVector3 llvec4(1.0, 2.0, 3.0);
        CHECK_MESSAGE(nValue4.getTypeEnum(, "4. getTypeEnum failed") == NVT_VEC3);
        CHECK_MESSAGE(nValue4.getClassEnum(, "4. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue4.getSendtoEnum(, "4. getSendtoEnum failed") == NVS_SIM_VIEWER);
        CHECK_MESSAGE(*nValue4.getVec3(, "4. getVec3 failed") == llvec4);
        CHECK_MESSAGE(!nValue4.sendToData(, "4. sendToData or sendToViewer failed") && nValue4.sendToViewer());

        LLNameValue nValue5("SecondLife", "-1.0, 2.4, 3", "VEC3", "RW", "SIM_VIEWER");
        LLVector3 llvec5(-1.0f, 2.4f, 3);
        CHECK_MESSAGE(nValue5.getTypeEnum(, "5. getTypeEnum failed") == NVT_VEC3);
        CHECK_MESSAGE(nValue5.getClassEnum(, "5. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue5.getSendtoEnum(, "5. getSendtoEnum failed") == NVS_SIM_VIEWER);
        CHECK_MESSAGE(*nValue5.getVec3(, "5. getVec3 failed") == llvec5);
        CHECK_MESSAGE(!nValue5.sendToData(, "5. sendToData or sendToViewer failed") && nValue5.sendToViewer());

        LLNameValue nValue6("SecondLife", "89764323", "U32", "RW", "DSV");
        CHECK_MESSAGE(nValue6.getTypeEnum(, "6. getTypeEnum failed") == NVT_U32);
        CHECK_MESSAGE(nValue6.getClassEnum(, "6. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue6.getSendtoEnum(, "6. getSendtoEnum failed") == NVS_DATA_SIM_VIEWER);
        CHECK_MESSAGE(*nValue6.getU32(, "6. getU32 failed") == 89764323);
        CHECK_MESSAGE(nValue6.sendToData(, "6. sendToData or sendToViewer failed") && nValue6.sendToViewer());

        LLNameValue nValue7("SecondLife", "89764323323232", "U64", "RW", "SIM_SPACE_VIEWER");
        U64 u64_7 = U64L(89764323323232);
        CHECK_MESSAGE(nValue7.getTypeEnum(, "7. getTypeEnum failed") == NVT_U64);
        CHECK_MESSAGE(nValue7.getClassEnum(, "7. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue7.getSendtoEnum(, "7. getSendtoEnum failed") == NVS_DATA_SIM_VIEWER);
        CHECK_MESSAGE(*nValue7.getU64(, "7. getU32 failed") == u64_7);
        CHECK_MESSAGE(nValue7.sendToData(, "7. sendToData or sendToViewer failed") && nValue7.sendToViewer());
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_3")
{

        LLNameValue nValue("SecondLife", "232324343", "ASSET", "READ_WRITE");
        CHECK_MESSAGE((0 == strcmp(nValue.mName,"SecondLife", "mName not set correctly")));
        CHECK_MESSAGE(nValue.getTypeEnum(, "getTypeEnum failed") == NVT_ASSET);
        CHECK_MESSAGE(nValue.getClassEnum(, "getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue.getSendtoEnum(, "getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE((0==strcmp(nValue.getAsset(, "getString failed"),"232324343")));

        LLNameValue nValue1("SecondLife", "232324343", "STRING", "READ_WRITE");
        CHECK_MESSAGE((0 == strcmp(nValue1.mName,"SecondLife", "1. mName not set correctly")));
        CHECK_MESSAGE(nValue1.getTypeEnum(, "1. getTypeEnum failed") == NVT_STRING);
        CHECK_MESSAGE(nValue1.getClassEnum(, "1. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue1.getSendtoEnum(, "1. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE((0==strcmp(nValue1.getString(, "1. getString failed"),"232324343")));

        LLNameValue nValue2("SecondLife", "23.5", "F32", "R");
        CHECK_MESSAGE(nValue2.getTypeEnum(, "2. getTypeEnum failed") == NVT_F32);
        CHECK_MESSAGE(nValue2.getClassEnum(, "2. getClassEnum failed") == NVC_READ_ONLY);
        CHECK_MESSAGE(nValue2.getSendtoEnum(, "2. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE(*nValue2.getF32(, "2. getF32 failed") == 23.5f);

        LLNameValue nValue3("SecondLife", "-43456787", "S32", "READ_ONLY");
        CHECK_MESSAGE(nValue3.getTypeEnum(, "3. getTypeEnum failed") == NVT_S32);
        CHECK_MESSAGE(nValue3.getClassEnum(, "3. getClassEnum failed") == NVC_READ_ONLY);
        CHECK_MESSAGE(nValue3.getSendtoEnum(, "3. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE(*nValue3.getS32(, "3. getS32 failed") == -43456787);

        LLNameValue nValue4("SecondLife", "<1.0, 2.0, 3.0>", "VEC3", "RW");
        LLVector3 llvec4(1.0, 2.0, 3.0);
        CHECK_MESSAGE(nValue4.getTypeEnum(, "4. getTypeEnum failed") == NVT_VEC3);
        CHECK_MESSAGE(nValue4.getClassEnum(, "4. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue4.getSendtoEnum(, "4. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE(*nValue4.getVec3(, "4. getVec3 failed") == llvec4);

        LLNameValue nValue5("SecondLife", "-1.0, 2.4, 3", "VEC3", "RW");
        LLVector3 llvec5(-1.0f, 2.4f, 3);
        CHECK_MESSAGE(nValue5.getTypeEnum(, "5. getTypeEnum failed") == NVT_VEC3);
        CHECK_MESSAGE(nValue5.getClassEnum(, "5. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue5.getSendtoEnum(, "5. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE(*nValue5.getVec3(, "5. getVec3 failed") == llvec5);

        LLNameValue nValue6("SecondLife", "89764323", "U32", "RW");
        CHECK_MESSAGE(nValue6.getTypeEnum(, "6. getTypeEnum failed") == NVT_U32);
        CHECK_MESSAGE(nValue6.getClassEnum(, "6. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue6.getSendtoEnum(, "6. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE(*nValue6.getU32(, "6. getU32 failed") == 89764323);

        LLNameValue nValue7("SecondLife", "89764323323232", "U64", "RW");
        U64 u64_7 = U64L(89764323323232);
        CHECK_MESSAGE(nValue7.getTypeEnum(, "7. getTypeEnum failed") == NVT_U64);
        CHECK_MESSAGE(nValue7.getClassEnum(, "7. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue7.getSendtoEnum(, "7. getSendtoEnum failed") == NVS_SIM);
        CHECK_MESSAGE(*nValue7.getU64(, "7. getU32 failed") == u64_7);
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_4")
{

        LLNameValue nValue("SecondLife",  "STRING", "READ_WRITE");
        CHECK_MESSAGE((0 == strcmp(nValue.mName,"SecondLife", "mName not set correctly")));
        CHECK_MESSAGE(nValue.getTypeEnum(, "getTypeEnum failed") == NVT_STRING);
        CHECK_MESSAGE(nValue.getClassEnum(, "getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue.getSendtoEnum(, "getSendtoEnum failed") == NVS_SIM);

        LLNameValue nValue1("SecondLife",  "ASSET", "READ_WRITE");
        CHECK_MESSAGE((0 == strcmp(nValue1.mName,"SecondLife", "1. mName not set correctly")));
        CHECK_MESSAGE(nValue1.getTypeEnum(, "1. getTypeEnum for RW failed") == NVT_ASSET);
        CHECK_MESSAGE(nValue1.getClassEnum(, "1. getClassEnum for RW failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue1.getSendtoEnum(, "1. getSendtoEnum for RW failed") == NVS_SIM);

        LLNameValue nValue2("SecondLife", "F32", "READ_ONLY");
        CHECK_MESSAGE(nValue2.getTypeEnum(, "2. getTypeEnum failed") == NVT_F32);
        CHECK_MESSAGE(nValue2.getClassEnum(, "2. getClassEnum failed") == NVC_READ_ONLY);
        CHECK_MESSAGE(nValue2.getSendtoEnum(, "2. getSendtoEnum failed") == NVS_SIM);

        LLNameValue nValue3("SecondLife", "S32", "READ_ONLY");
        CHECK_MESSAGE(nValue3.getTypeEnum(, "3. getTypeEnum failed") == NVT_S32);
        CHECK_MESSAGE(nValue3.getClassEnum(, "3. getClassEnum failed") == NVC_READ_ONLY);
        CHECK_MESSAGE(nValue3.getSendtoEnum(, "3. getSendtoEnum failed") == NVS_SIM);

        LLNameValue nValue4("SecondLife", "VEC3", "READ_WRITE");
        CHECK_MESSAGE(nValue4.getTypeEnum(, "4. getTypeEnum failed") == NVT_VEC3);
        CHECK_MESSAGE(nValue4.getClassEnum(, "4. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue4.getSendtoEnum(, "4. getSendtoEnum failed") == NVS_SIM);

        LLNameValue nValue6("SecondLife", "U32", "READ_WRITE");
        CHECK_MESSAGE(nValue6.getTypeEnum(, "6. getTypeEnum failed") == NVT_U32);
        CHECK_MESSAGE(nValue6.getClassEnum(, "6. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue6.getSendtoEnum(, "6. getSendtoEnum failed") == NVS_SIM);

        LLNameValue nValue7("SecondLife", "U64", "READ_WRITE");
        CHECK_MESSAGE(nValue7.getTypeEnum(, "7. getTypeEnum failed") == NVT_U64);
        CHECK_MESSAGE(nValue7.getClassEnum(, "7. getClassEnum failed") == NVC_READ_WRITE);
        CHECK_MESSAGE(nValue7.getSendtoEnum(, "7. getSendtoEnum failed") == NVS_SIM);
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_5")
{

        LLNameValue nValue("SecondLife", "This is a test", "STRING", "RW", "SIM");

        CHECK_MESSAGE((0 == strcmp(nValue.getString(, "getString failed"),"This is a test")));
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_6")
{

        LLNameValue nValue("SecondLife", "This is a test", "ASSET", "RW", "S");
        CHECK_MESSAGE((0 == strcmp(nValue.getAsset(, "getAsset failed"),"This is a test")));
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_7")
{

        LLNameValue nValue("SecondLife", "555555", "F32", "RW", "SIM");

        CHECK_MESSAGE(*nValue.getF32(, "getF32 failed") == 555555.f);
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_8")
{

        LLNameValue nValue("SecondLife", "-5555", "S32", "RW", "SIM");

        CHECK_MESSAGE(*nValue.getS32(, "getS32 failed") == -5555);

        S32 sVal = 0x7FFFFFFF;
        nValue.setS32(sVal);
        CHECK_MESSAGE(*nValue.getS32(, "getS32 failed") == sVal);

        sVal = -0x7FFFFFFF;
        nValue.setS32(sVal);
        CHECK_MESSAGE(*nValue.getS32(, "getS32 failed") == sVal);

        sVal = 0;
        nValue.setS32(sVal);
        CHECK_MESSAGE(*nValue.getS32(, "getS32 failed") == sVal);
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_9")
{

        LLNameValue nValue("SecondLife", "<-3, 2, 1>", "VEC3", "RW", "SIM");
        LLVector3 vecExpected(-3, 2, 1);
        LLVector3 vec;
        nValue.getVec3(vec);
        CHECK_MESSAGE(vec == vecExpected, "getVec3 failed");
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_10")
{

        LLNameValue nValue("SecondLife", "12345678", "U32", "RW", "SIM");

        CHECK_MESSAGE(*nValue.getU32(, "getU32 failed") == 12345678);

        U32 val = 0xFFFFFFFF;
        nValue.setU32(val);
        CHECK_MESSAGE(*nValue.getU32(, "U32 max") == val);

        val = 0;
        nValue.setU32(val);
        CHECK_MESSAGE(*nValue.getU32(, "U32 min") == val);
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_11")
{

        //skip_fail("incomplete support for U64.");
        LLNameValue nValue("SecondLife", "44444444444", "U64", "RW", "SIM");

        CHECK_MESSAGE(*nValue.getU64(, "getU64 failed") == U64L(44444444444));

        // there is no LLNameValue::setU64()
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_12")
{

        //skip_fail("incomplete support for U64.");
        LLNameValue nValue("SecondLife U64 RW DSV 44444444444");
        std::string ret_str = nValue.printNameValue();

        CHECK_MESSAGE(ret_str == "SecondLife U64 RW DSV 44444444444", "1:printNameValue failed");

        LLNameValue nValue1(ret_str.c_str());
        ensure_equals("Serialization of printNameValue failed", *nValue.getU64(), *nValue1.getU64());
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_13")
{

        LLNameValue nValue("SecondLife STRING RW DSV 44444444444");
        std::string ret_str = nValue.printData();
        CHECK_MESSAGE(ret_str == "44444444444", "1:printData failed");

        LLNameValue nValue1("SecondLife S32 RW DSV 44444");
        ret_str = nValue1.printData();
        CHECK_MESSAGE(ret_str == "44444", "2:printData failed");
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_14")
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
        //CHECK_MESSAGE(0 == strcmp((stream5.str(, "U64 << failed")).c_str(),"22222"));
    
}

TEST_CASE_FIXTURE(namevalue_test, "test_15")
{

        LLNameValue nValue("SecondLife", "This is a test", "ASSET", "R", "S");

        CHECK_MESSAGE((0 == strcmp(nValue.getAsset(, "getAsset failed"),"This is a test")));
        // this should not have updated as it is read only.
        nValue.setAsset("New Value should not be updated");
        CHECK_MESSAGE((0 == strcmp(nValue.getAsset(, "setAsset on ReadOnly failed"),"This is a test")));

        LLNameValue nValue1("SecondLife", "1234", "U32", "R", "S");
        // this should not have updated as it is read only.
        nValue1.setU32(4567);
        CHECK_MESSAGE(*nValue1.getU32(, "setU32 on ReadOnly failed") == 1234);

        LLNameValue nValue2("SecondLife", "1234", "S32", "R", "S");
        // this should not have updated as it is read only.
        nValue2.setS32(4567);
        CHECK_MESSAGE(*nValue2.getS32(, "setS32 on ReadOnly failed") == 1234);

        LLNameValue nValue3("SecondLife", "1234", "F32", "R", "S");
        // this should not have updated as it is read only.
        nValue3.setF32(4567);
        CHECK_MESSAGE(*nValue3.getF32(, "setF32 on ReadOnly failed") == 1234);

        LLNameValue nValue4("SecondLife", "<1,2,3>", "VEC3", "R", "S");
        // this should not have updated as it is read only.
        LLVector3 vec(4,5,6);
        nValue3.setVec3(vec);
        LLVector3 vec1(1,2,3);
        CHECK_MESSAGE(*nValue4.getVec3(, "setVec3 on ReadOnly failed") == vec1);

        // cant test for U64 as no set64 exists nor any operators support U64 type
    
}

} // TEST_SUITE

