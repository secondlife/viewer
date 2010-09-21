/**
 * @file llpartdata_tut.cpp
 * @author Adroit
 * @date March 2007
 * @brief LLPartData and LLPartSysData test cases.
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
#include "lldatapacker.h"
#include "v3math.h"
#include "llsdserialize.h"
#include "message.h"

#include "../llpartdata.h"

#include "../test/lltut.h"

namespace tut
{
	
	struct partdata_test
	{
	};
	typedef test_group<partdata_test> partdata_test_t;
	typedef partdata_test_t::object partdata_test_object_t;
	tut::partdata_test_t tut_partdata_test("partdata_test");

	template<> template<>
	void partdata_test_object_t::test<1>()
	{
		LLPartData llpdata,llpdata1;
		U8 pkbuf[128];

		llpdata.setFlags(LLPartData::LL_PART_INTERP_COLOR_MASK | LLPartData::LL_PART_INTERP_SCALE_MASK |
		LLPartData::LL_PART_BOUNCE_MASK | LLPartData::LL_PART_WIND_MASK | LLPartData::LL_PART_FOLLOW_SRC_MASK |
		LLPartData::LL_PART_FOLLOW_VELOCITY_MASK | LLPartData::LL_PART_TARGET_POS_MASK | LLPartData::LL_PART_TARGET_LINEAR_MASK |
		LLPartData::LL_PART_EMISSIVE_MASK | LLPartData::LL_PART_BEAM_MASK | LLPartData::LL_PART_DEAD_MASK);

		llpdata.setMaxAge(29.3f);

		LLVector3 llvec1(1.0f, .5f, .25f);
		llpdata.setStartColor(llvec1);
		llpdata.setStartAlpha(.7f);

		LLVector3 llvec2(.2f, .3f, 1.0f);
		llpdata.setEndColor(llvec2);
		llpdata.setEndAlpha(1.0f);

		llpdata.setStartScale(3.23f, 4.0f);
		llpdata.setEndScale(2.4678f, 1.0f);

		LLDataPackerBinaryBuffer dp((U8*)pkbuf, 128);
		llpdata.pack(dp);
		
		S32 cur_size = dp.getCurrentSize();
		
		LLDataPackerBinaryBuffer dp1((U8*)pkbuf, cur_size);
		llpdata1.unpack(dp1);

		ensure("1.mFlags values are different after unpacking", llpdata1.mFlags == llpdata.mFlags);
		ensure_approximately_equals("2.mMaxAge values are different after unpacking", llpdata1.mMaxAge, llpdata.mMaxAge, 8);

		ensure_approximately_equals("3.mStartColor[0] values are different after unpacking", llpdata1.mStartColor.mV[0], llpdata.mStartColor.mV[0], 8);
		ensure_approximately_equals("4.mStartColor[1] values are different after unpacking", llpdata1.mStartColor.mV[1], llpdata.mStartColor.mV[1], 8);
		ensure_approximately_equals("5.mStartColor[2] values are different after unpacking", llpdata1.mStartColor.mV[2], llpdata.mStartColor.mV[2], 8);
		ensure_approximately_equals("6.mStartColor[3] values are different after unpacking", llpdata1.mStartColor.mV[3], llpdata.mStartColor.mV[3], 8);

		ensure_approximately_equals("7.mEndColor[0] values are different after unpacking", llpdata1.mEndColor.mV[0], llpdata.mEndColor.mV[0], 8);
		ensure_approximately_equals("8.mEndColor[1] values are different after unpacking", llpdata1.mEndColor.mV[1], llpdata.mEndColor.mV[1], 8);
		ensure_approximately_equals("9.mEndColor[2] values are different after unpacking", llpdata1.mEndColor.mV[2], llpdata.mEndColor.mV[2], 8);
		ensure_approximately_equals("10.mEndColor[2] values are different after unpacking", llpdata1.mEndColor.mV[3], llpdata.mEndColor.mV[3], 8);

		ensure_approximately_equals("11.mStartScale[0] values are different after unpacking", llpdata1.mStartScale.mV[0], llpdata.mStartScale.mV[0], 5);
		ensure_approximately_equals("12.mStartScale[1] values are different after unpacking", llpdata1.mStartScale.mV[1], llpdata.mStartScale.mV[1], 5);

		ensure_approximately_equals("13.mEndScale[0] values are different after unpacking", llpdata1.mEndScale.mV[0], llpdata.mEndScale.mV[0], 5);
		ensure_approximately_equals("14.mEndScale[1] values are different after unpacking", llpdata1.mEndScale.mV[1], llpdata.mEndScale.mV[1], 5);
	}


	template<> template<>
	void partdata_test_object_t::test<2>()
	{
		LLPartData llpdata,llpdata1;

		llpdata.setFlags(LLPartData::LL_PART_INTERP_COLOR_MASK | LLPartData::LL_PART_INTERP_SCALE_MASK |
		LLPartData::LL_PART_BOUNCE_MASK | LLPartData::LL_PART_WIND_MASK | LLPartData::LL_PART_FOLLOW_SRC_MASK |
		LLPartData::LL_PART_FOLLOW_VELOCITY_MASK | LLPartData::LL_PART_TARGET_POS_MASK | LLPartData::LL_PART_TARGET_LINEAR_MASK |
		LLPartData::LL_PART_EMISSIVE_MASK | LLPartData::LL_PART_BEAM_MASK | LLPartData::LL_PART_DEAD_MASK);
		
		llpdata.setMaxAge(29.3f);

		LLVector3 llvec1(1.0f, .5f, .25f);
		llpdata.setStartColor(llvec1);
		llpdata.setStartAlpha(.7f);

		LLVector3 llvec2(.2f, .3f, 1.0f);
		llpdata.setEndColor(llvec2);
		llpdata.setEndAlpha(1.0f);

		llpdata.setStartScale(3.23f, 4.0f);
		llpdata.setEndScale(2.4678f, 1.0f);

		LLSD llsd = llpdata.asLLSD();

		llpdata1.fromLLSD(llsd);

		ensure("1.mFlags values are different after unpacking", llpdata1.mFlags == llpdata.mFlags);
		ensure_approximately_equals("2.mMaxAge values are different after unpacking", llpdata1.mMaxAge, llpdata.mMaxAge, 8);

		ensure_approximately_equals("3.mStartColor[0] values are different after unpacking", llpdata1.mStartColor.mV[0], llpdata.mStartColor.mV[0], 8);
		ensure_approximately_equals("4.mStartColor[1] values are different after unpacking", llpdata1.mStartColor.mV[1], llpdata.mStartColor.mV[1], 8);
		ensure_approximately_equals("5.mStartColor[2] values are different after unpacking", llpdata1.mStartColor.mV[2], llpdata.mStartColor.mV[2], 8);
		ensure_approximately_equals("6.mStartColor[3] values are different after unpacking", llpdata1.mStartColor.mV[3], llpdata.mStartColor.mV[3], 8);

		ensure_approximately_equals("7.mEndColor[0] values are different after unpacking", llpdata1.mEndColor.mV[0], llpdata.mEndColor.mV[0], 8);
		ensure_approximately_equals("8.mEndColor[1] values are different after unpacking", llpdata1.mEndColor.mV[1], llpdata.mEndColor.mV[1], 8);
		ensure_approximately_equals("9.mEndColor[2] values are different after unpacking", llpdata1.mEndColor.mV[2], llpdata.mEndColor.mV[2], 8);
		ensure_approximately_equals("10.mEndColor[2] values are different after unpacking", llpdata1.mEndColor.mV[3], llpdata.mEndColor.mV[3], 8);

		ensure_approximately_equals("11.mStartScale[0] values are different after unpacking", llpdata1.mStartScale.mV[0], llpdata.mStartScale.mV[0], 5);
		ensure_approximately_equals("12.mStartScale[1] values are different after unpacking", llpdata1.mStartScale.mV[1], llpdata.mStartScale.mV[1], 5);

		ensure_approximately_equals("13.mEndScale[0] values are different after unpacking", llpdata1.mEndScale.mV[0], llpdata.mEndScale.mV[0], 5);
		ensure_approximately_equals("14.mEndScale[1] values are different after unpacking", llpdata1.mEndScale.mV[1], llpdata.mEndScale.mV[1], 5);
	}


//*********llpartsysdata***********

	template<> template<>
	void partdata_test_object_t::test<3>()
	{
		LLPartSysData llpsysdata, llpsysdata1;
		U8 pkbuf[256];
		llpsysdata.setBurstSpeedMin(33.33f);
		ensure("1.mBurstSpeedMin coudnt be set", 33.33f == llpsysdata.mBurstSpeedMin);

		llpsysdata.setBurstSpeedMax(44.44f); 
		ensure("2.mBurstSpeedMax coudnt be set", 44.44f == llpsysdata.mBurstSpeedMax);

		llpsysdata.setBurstRadius(45.55f);
		ensure("3.mBurstRadius coudnt be set", 45.55f == llpsysdata.mBurstRadius);

		LLVector3 llvec(44.44f, 111.11f, -40.4f);
		llpsysdata.setPartAccel(llvec);

		llpsysdata.mCRC = 0xFFFFFFFF;
		llpsysdata.mFlags = 0x20;

		llpsysdata.mPattern = LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE_EMPTY;

		llpsysdata.mMaxAge = 99.99f;
		llpsysdata.mStartAge = 18.5f;
		llpsysdata.mInnerAngle = 4.234f;
		llpsysdata.mOuterAngle = 7.123f;
		llpsysdata.mBurstRate  = 245.53f;
		llpsysdata.mBurstPartCount = 0xFF;
		llpsysdata.mAngularVelocity = llvec;

		llpsysdata.mPartImageID.generate();
		llpsysdata.mTargetUUID.generate();
		
		LLDataPackerBinaryBuffer dp((U8*)pkbuf, 256);
		llpsysdata.pack(dp);
		S32 cur_size = dp.getCurrentSize();
		LLDataPackerBinaryBuffer dp1((U8*)pkbuf, cur_size);
		llpsysdata1.unpack(dp1);

		ensure("1.mCRC's not equal", llpsysdata.mCRC == llpsysdata1.mCRC);
		ensure("2.mFlags's not equal", llpsysdata.mFlags == llpsysdata1.mFlags);
		ensure("3.mPattern's not equal", llpsysdata.mPattern == llpsysdata1.mPattern);
		ensure_approximately_equals("4.mMaxAge's not equal", llpsysdata.mMaxAge , llpsysdata1.mMaxAge, 8);
		ensure_approximately_equals("5.mStartAge's not equal", llpsysdata.mStartAge, llpsysdata1.mStartAge, 8);
		ensure_approximately_equals("6.mOuterAngle's not equal", llpsysdata.mOuterAngle, llpsysdata1.mOuterAngle, 5);
		ensure_approximately_equals("7.mInnerAngles's not equal", llpsysdata.mInnerAngle, llpsysdata1.mInnerAngle, 5);
		ensure_approximately_equals("8.mBurstRate's not equal", llpsysdata.mBurstRate, llpsysdata1.mBurstRate, 8);
		ensure("9.mBurstPartCount's not equal", llpsysdata.mBurstPartCount == llpsysdata1.mBurstPartCount);

		ensure_approximately_equals("10.mBurstSpeedMin's not equal", llpsysdata.mBurstSpeedMin, llpsysdata1.mBurstSpeedMin, 8);
		ensure_approximately_equals("11.mBurstSpeedMax's not equal", llpsysdata.mBurstSpeedMax, llpsysdata1.mBurstSpeedMax, 8);

		ensure_approximately_equals("12.mAngularVelocity's not equal", llpsysdata.mAngularVelocity.mV[0], llpsysdata1.mAngularVelocity.mV[0], 7);
		ensure_approximately_equals("13.mAngularVelocity's not equal", llpsysdata.mAngularVelocity.mV[1], llpsysdata1.mAngularVelocity.mV[1], 7);
		ensure_approximately_equals("14.mAngularVelocity's not equal", llpsysdata.mAngularVelocity.mV[2], llpsysdata1.mAngularVelocity.mV[2], 7);
			
		ensure_approximately_equals("15.mPartAccel's not equal", llpsysdata.mPartAccel.mV[0], llpsysdata1.mPartAccel.mV[0], 7);
		ensure_approximately_equals("16.mPartAccel's not equal", llpsysdata.mPartAccel.mV[1], llpsysdata1.mPartAccel.mV[1], 7);
		ensure_approximately_equals("17.mPartAccel's not equal", llpsysdata.mPartAccel.mV[2], llpsysdata1.mPartAccel.mV[2], 7);

		ensure("18.mPartImageID's not equal", llpsysdata.mPartImageID == llpsysdata1.mPartImageID);
		ensure("19.mTargetUUID's not equal", llpsysdata.mTargetUUID == llpsysdata1.mTargetUUID);
		ensure_approximately_equals("20.mBurstRadius's not equal", llpsysdata.mBurstRadius, llpsysdata1.mBurstRadius, 8);
	}
}
