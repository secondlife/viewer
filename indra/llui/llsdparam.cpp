/** 
 * @file llsdparam.cpp
 * @brief parameter block abstraction for creating complex objects and 
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

// Project includes
#include "llsdparam.h"

//
// LLParamSDParser
//
LLParamSDParser::LLParamSDParser()
{
	using boost::bind;

	registerParserFuncs<S32>(bind(&LLParamSDParser::readTypedValue<S32>, this, _1, &LLSD::asInteger),
							bind(&LLParamSDParser::writeTypedValue<S32>, this, _1, _2));
	registerParserFuncs<U32>(bind(&LLParamSDParser::readTypedValue<U32>, this, _1, &LLSD::asInteger),
							bind(&LLParamSDParser::writeU32Param, this, _1, _2));
	registerParserFuncs<F32>(bind(&LLParamSDParser::readTypedValue<F32>, this, _1, &LLSD::asReal),
							bind(&LLParamSDParser::writeTypedValue<F32>, this, _1, _2));
	registerParserFuncs<F64>(bind(&LLParamSDParser::readTypedValue<F64>, this, _1, &LLSD::asReal),
							bind(&LLParamSDParser::writeTypedValue<F64>, this, _1, _2));
	registerParserFuncs<bool>(bind(&LLParamSDParser::readTypedValue<F32>, this, _1, &LLSD::asBoolean),
							bind(&LLParamSDParser::writeTypedValue<F32>, this, _1, _2));
	registerParserFuncs<std::string>(bind(&LLParamSDParser::readTypedValue<std::string>, this, _1, &LLSD::asString),
							bind(&LLParamSDParser::writeTypedValue<std::string>, this, _1, _2));
	registerParserFuncs<LLUUID>(bind(&LLParamSDParser::readTypedValue<LLUUID>, this, _1, &LLSD::asUUID),
							bind(&LLParamSDParser::writeTypedValue<LLUUID>, this, _1, _2));
	registerParserFuncs<LLDate>(bind(&LLParamSDParser::readTypedValue<LLDate>, this, _1, &LLSD::asDate),
							bind(&LLParamSDParser::writeTypedValue<LLDate>, this, _1, _2));
	registerParserFuncs<LLURI>(bind(&LLParamSDParser::readTypedValue<LLURI>, this, _1, &LLSD::asURI),
							bind(&LLParamSDParser::writeTypedValue<LLURI>, this, _1, _2));
	registerParserFuncs<LLSD>(bind(&LLParamSDParser::readSDParam, this, _1),
							bind(&LLParamSDParser::writeTypedValue<LLSD>, this, _1, _2));
}

bool LLParamSDParser::readSDParam(void* value_ptr)
{
	if (!mCurReadSD) return false;
	*((LLSD*)value_ptr) = *mCurReadSD;
	return true;
}

// special case handling of U32 due to ambiguous LLSD::assign overload
bool LLParamSDParser::writeU32Param(const void* val_ptr, const parser_t::name_stack_t& name_stack)
{
	if (!mWriteSD) return false;
	
	LLSD* sd_to_write = getSDWriteNode(name_stack);
	if (!sd_to_write) return false;

	sd_to_write->assign((S32)*((const U32*)val_ptr));
	return true;
}

void LLParamSDParser::readSD(const LLSD& sd, LLInitParam::BaseBlock& block, bool silent)
{
	mCurReadSD = NULL;
	mNameStack.clear();
	setParseSilently(silent);

	readSDValues(sd, block);
}

void LLParamSDParser::writeSD(LLSD& sd, const LLInitParam::BaseBlock& block)
{
	mWriteSD = &sd;
	block.serializeBlock(*this);
}

void LLParamSDParser::readSDValues(const LLSD& sd, LLInitParam::BaseBlock& block)
{
	if (sd.isMap())
	{
		for (LLSD::map_const_iterator it = sd.beginMap();
			it != sd.endMap();
			++it)
		{
			mNameStack.push_back(make_pair(it->first, newParseGeneration()));
			readSDValues(it->second, block);
			mNameStack.pop_back();
		}
	}
	else if (sd.isArray())
	{
		for (LLSD::array_const_iterator it = sd.beginArray();
			it != sd.endArray();
			++it)
		{
			mNameStack.back().second = newParseGeneration();
			readSDValues(*it, block);
		}
	}
	else
	{
		mCurReadSD = &sd;
		block.submitValue(mNameStack, *this);
	}
}

/*virtual*/ std::string LLParamSDParser::getCurrentElementName()
{
	std::string full_name = "sd";
	for (name_stack_t::iterator it = mNameStack.begin();	
		it != mNameStack.end();
		++it)
	{
		full_name += llformat("[%s]", it->first.c_str());
	}

	return full_name;
}

LLSD* LLParamSDParser::getSDWriteNode(const parser_t::name_stack_t& name_stack)
{
	//TODO: implement nested LLSD writing
	return mWriteSD;
}

