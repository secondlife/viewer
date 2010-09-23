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

class LLParamSDParser 
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
	void readSDValues(const LLSD& sd, LLInitParam::BaseBlock& block);

	template<typename T>
	static bool writeTypedValue(Parser& parser, const void* val_ptr, const parser_t::name_stack_t& name_stack)
	{
		LLParamSDParser& sdparser = static_cast<LLParamSDParser&>(parser);
		if (!sdparser.mWriteRootSD) return false;
		
		LLSD* sd_to_write = sdparser.getSDWriteNode(name_stack);
		if (!sd_to_write) return false;

		sd_to_write->assign(*((const T*)val_ptr));
		return true;
	}

	LLSD* getSDWriteNode(const parser_t::name_stack_t& name_stack);

	static bool writeU32Param(Parser& parser, const void* value_ptr, const parser_t::name_stack_t& name_stack);

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

template<typename T>
class LLSDParamAdapter : public T
	{
	public:
		LLSDParamAdapter() {}
		LLSDParamAdapter(const LLSD& sd)
		{
			LLParamSDParser parser;
			parser.readSD(sd, *this);
		}
		
		LLSDParamAdapter(const T& val)
		{
			T::operator=(val);
		}
	};

#endif // LL_LLSDPARAM_H

