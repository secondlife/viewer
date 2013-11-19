/** 
 * @file llsdparam.h
 * @brief parameter block abstraction for creating complex objects and 
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLSDPARAM_H
#define LL_LLSDPARAM_H

#include "llinitparam.h"
#include "boost/function.hpp"

struct LL_COMMON_API LLParamSDParserUtilities
{
	static LLSD& getSDWriteNode(LLSD& input, LLInitParam::Parser::name_stack_range_t& name_stack_range);

	typedef boost::function<void (const LLSD&, LLInitParam::Parser::name_stack_t&)> read_sd_cb_t;
	static void readSDValues(read_sd_cb_t cb, const LLSD& sd, LLInitParam::Parser::name_stack_t& stack);
	static void readSDValues(read_sd_cb_t cb, const LLSD& sd);
};

class LL_COMMON_API LLParamSDParser 
:	public LLInitParam::Parser
{
LOG_CLASS(LLParamSDParser);

typedef LLInitParam::Parser parser_t;

public:
	LLParamSDParser();
	void readSD(const LLSD& sd, LLInitParam::BaseBlock& block, bool silent = false);
	void writeSD(LLSD& sd, const LLInitParam::BaseBlock& block);

	/*virtual*/ std::string getCurrentElementName();

private:
	void submit(LLInitParam::BaseBlock& block, const LLSD& sd, LLInitParam::Parser::name_stack_t& name_stack);

	template<typename T>
	static bool writeTypedValue(Parser& parser, const void* val_ptr, parser_t::name_stack_t& name_stack)
	{
		LLParamSDParser& sdparser = static_cast<LLParamSDParser&>(parser);
		if (!sdparser.mWriteRootSD) return false;
		
		LLInitParam::Parser::name_stack_range_t range(name_stack.begin(), name_stack.end());
		LLSD& sd_to_write = LLParamSDParserUtilities::getSDWriteNode(*sdparser.mWriteRootSD, range);

		sd_to_write.assign(*((const T*)val_ptr));
		return true;
	}

	static bool writeU32Param(Parser& parser, const void* value_ptr, parser_t::name_stack_t& name_stack);
	static bool writeFlag(Parser& parser, const void* value_ptr, parser_t::name_stack_t& name_stack);

	static bool readFlag(Parser& parser, void* val_ptr);
	static bool readS32(Parser& parser, void* val_ptr);
	static bool readU32(Parser& parser, void* val_ptr);
	static bool readF32(Parser& parser, void* val_ptr);
	static bool readF64(Parser& parser, void* val_ptr);
	static bool readBool(Parser& parser, void* val_ptr);
	static bool readString(Parser& parser, void* val_ptr);
	static bool readUUID(Parser& parser, void* val_ptr);
	static bool readDate(Parser& parser, void* val_ptr);
	static bool readURI(Parser& parser, void* val_ptr);
	static bool readSD(Parser& parser, void* val_ptr);

	Parser::name_stack_t	mNameStack;
	const LLSD*				mCurReadSD;
	LLSD*					mWriteRootSD;
	LLSD*					mCurWriteSD;
};


extern LL_COMMON_API LLFastTimer::DeclareTimer FTM_SD_PARAM_ADAPTOR;
template<typename T>
class LLSDParamAdapter : public T
{
public:
	LLSDParamAdapter() {}
	LLSDParamAdapter(const LLSD& sd)
	{
		LLFastTimer _(FTM_SD_PARAM_ADAPTOR);
		LLParamSDParser parser;
		// don't spam for implicit parsing of LLSD, as we want to allow arbitrary freeform data and ignore most of it
		bool parse_silently = true;
		parser.readSD(sd, *this, parse_silently);
	}

	operator LLSD() const
	{
		LLParamSDParser parser;
		LLSD sd;
		parser.writeSD(sd, *this);
		return sd;
	}
		
	LLSDParamAdapter(const T& val)
	: T(val)
	{
		T::operator=(val);
	}
};

#endif // LL_LLSDPARAM_H

