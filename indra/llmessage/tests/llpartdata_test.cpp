/**
 * @file llpartdata_tut.cpp
 * @author Adroit
 * @date March 2007
 * @brief LLPartData and LLPartSysData test cases.
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
#include "lldatapacker.h"
#include "v3math.h"
#include "llsdserialize.h"
#include "message.h"

#include "../llpartdata.h"

#include "../test/lldoctest.h"

TEST_SUITE("LLPartData") {

TEST_CASE("test_1")
{

        LLPartSysData llpsysdata;
        LLDataPackerBinaryBuffer dp1(msg, sizeof(msg));

        CHECK_MESSAGE(llpsysdata.unpack(dp1, "LLPartSysData::unpack failed."));


        //mCRC  1   unsigned int
        CHECK_MESSAGE(llpsysdata.mCRC == (U32, "mCRC different after unpacking") 1);
        //mFlags    0   unsigned int
        CHECK_MESSAGE(llpsysdata.mFlags == (U32, "mFlags different after unpacking") 0);
        //mPattern  1 ''   unsigned char
        CHECK_MESSAGE(llpsysdata.mPattern == (U8, "mPattern different after unpacking") 1);
        //mInnerAngle   0.00000000  float
        ensure_approximately_equals("mInnerAngle different after unpacking", llpsysdata.mInnerAngle, 0.f, 8);
        //mOuterAngle   0.00000000  float
        ensure_approximately_equals("mOuterAngle different after unpacking", llpsysdata.mOuterAngle, 0.f, 8);
        //mAngularVelocity  0,0,0
        ensure_approximately_equals("mAngularVelocity.mV[0] different after unpacking", llpsysdata.mAngularVelocity.mV[0], 0.f, 8);
        ensure_approximately_equals("mAngularVelocity.mV[0] different after unpacking", llpsysdata.mAngularVelocity.mV[1], 0.f, 8);
        ensure_approximately_equals("mAngularVelocity.mV[0] different after unpacking", llpsysdata.mAngularVelocity.mV[2], 0.f, 8);
        //mBurstRate    0.097656250 float
        ensure_approximately_equals("mBurstRate different after unpacking", llpsysdata.mBurstRate, 0.097656250f, 8);
        //mBurstPartCount   1 ''   unsigned char
        CHECK_MESSAGE(llpsysdata.mBurstPartCount == (U8, "mBurstPartCount different after unpacking") 1);
        //mBurstRadius  0.00000000  float
        ensure_approximately_equals("mBurstRadius different after unpacking", llpsysdata.mBurstRadius, 0.f, 8);
        //mBurstSpeedMin    1.0000000   float
        ensure_approximately_equals("mBurstSpeedMin different after unpacking", llpsysdata.mBurstSpeedMin, 1.f, 8);
        //mBurstSpeedMax    1.0000000   float
        ensure_approximately_equals("mBurstSpeedMax different after unpacking", llpsysdata.mBurstSpeedMax, 1.f, 8);
        //mMaxAge   0.00000000  float
        ensure_approximately_equals("mMaxAge different after unpacking", llpsysdata.mMaxAge, 0.f, 8);
        //mStartAge 0.00000000  float
        ensure_approximately_equals("mStartAge different after unpacking", llpsysdata.mStartAge, 0.f, 8);
        //mPartAccel    <0,0,0>
        ensure_approximately_equals("mPartAccel.mV[0] different after unpacking", llpsysdata.mPartAccel.mV[0], 0.f, 7);
        ensure_approximately_equals("mPartAccel.mV[1] different after unpacking", llpsysdata.mPartAccel.mV[1], 0.f, 7);
        ensure_approximately_equals("mPartAccel.mV[2] different after unpacking", llpsysdata.mPartAccel.mV[2], 0.f, 7);

        //mPartData
        LLPartData& data = llpsysdata.mPartData;

        //mFlags    132354  unsigned int
        CHECK_MESSAGE(data.mFlags == (U32, "mPartData.mFlags different after unpacking") 132354);
        //mMaxAge   10.000000   float
        ensure_approximately_equals("mPartData.mMaxAge different after unpacking", data.mMaxAge, 10.f, 8);
        //mStartColor   <1,1,1,1>
        ensure_approximately_equals("mPartData.mStartColor.mV[0] different after unpacking", data.mStartColor.mV[0], 1.f, 8);
        ensure_approximately_equals("mPartData.mStartColor.mV[1] different after unpacking", data.mStartColor.mV[1], 1.f, 8);
        ensure_approximately_equals("mPartData.mStartColor.mV[2] different after unpacking", data.mStartColor.mV[2], 1.f, 8);
        ensure_approximately_equals("mPartData.mStartColor.mV[3] different after unpacking", data.mStartColor.mV[3], 1.f, 8);
        //mEndColor <1,1,0,0>
        ensure_approximately_equals("mPartData.mEndColor.mV[0] different after unpacking", data.mEndColor.mV[0], 1.f, 8);
        ensure_approximately_equals("mPartData.mEndColor.mV[1] different after unpacking", data.mEndColor.mV[1], 1.f, 8);
        ensure_approximately_equals("mPartData.mEndColor.mV[2] different after unpacking", data.mEndColor.mV[2], 0.f, 8);
        ensure_approximately_equals("mPartData.mEndColor.mV[3] different after unpacking", data.mEndColor.mV[3], 0.f, 8);
        //mStartScale   <1,1>
        ensure_approximately_equals("mPartData.mStartScale.mV[0] different after unpacking", data.mStartScale.mV[0], 1.f, 8);
        ensure_approximately_equals("mPartData.mStartScale.mV[1] different after unpacking", data.mStartScale.mV[1], 1.f, 8);
        //mEndScale <0,0>
        ensure_approximately_equals("mPartData.mEndScale.mV[0] different after unpacking", data.mEndScale.mV[0], 0.f, 8);
        ensure_approximately_equals("mPartData.mEndScale.mV[1] different after unpacking", data.mEndScale.mV[1], 0.f, 8);
        //mPosOffset    <0,0,0>
        ensure_approximately_equals("mPartData.mPosOffset.mV[0] different after unpacking", data.mPosOffset.mV[0], 0.f, 8);
        ensure_approximately_equals("mPartData.mPosOffset.mV[1] different after unpacking", data.mPosOffset.mV[1], 0.f, 8);
        ensure_approximately_equals("mPartData.mPosOffset.mV[2] different after unpacking", data.mPosOffset.mV[2], 0.f, 8);
        //mParameter    0.00000000  float
        ensure_approximately_equals("mPartData.mParameter different after unpacking", data.mParameter, 0.f, 8);

        //mStartGlow    0.00000000  float
        ensure_approximately_equals("mPartData.mStartGlow different after unpacking", data.mStartGlow, 0.f, 8);
        //mEndGlow  0.00000000  float
        ensure_approximately_equals("mPartData.mEndGlow different after unpacking", data.mEndGlow, 0.f, 8);
        //mBlendFuncSource  2 ''   unsigned char
        CHECK_MESSAGE(data.mBlendFuncSource == (U8, "mPartData.mBlendFuncSource different after unpacking") 2);
        //mBlendFuncDest    1 ''   unsigned char
        CHECK_MESSAGE(data.mBlendFuncDest == (U8, "mPartData.mBlendFuncDest different after unpacking") 1);
    
}

} // TEST_SUITE

