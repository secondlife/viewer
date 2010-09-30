/** 
 * @file llsdparam.h
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

