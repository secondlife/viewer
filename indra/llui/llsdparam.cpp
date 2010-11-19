/** 
 * @file llsdparam.cpp
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

#include "linden_common.h"

// Project includes
#include "llsdparam.h"
#include "llsdutil.h"

static 	LLInitParam::Parser::parser_read_func_map_t sReadFuncs;
static 	LLInitParam::Parser::parser_write_func_map_t sWriteFuncs;
static 	LLInitParam::Parser::parser_inspect_func_map_t sInspectFuncs;

//
// LLParamSDParser
//
LLParamSDParser::LLParamSDParser()
: Parser(sReadFuncs, sWriteFuncs, sInspectFuncs)
{
	using boost::bind;

	if (sReadFuncs.empty())
	{
		registerParserFuncs<S32>(readS32, &LLParamSDParser::writeTypedValue<S32>);
		registerParserFuncs<U32>(readU32, &LLParamSDParser::writeU32Param);
		registerParserFuncs<F32>(readF32, &LLParamSDParser::writeTypedValue<F32>);
		registerParserFuncs<F64>(readF64, &LLParamSDParser::writeTypedValue<F64>);
		registerParserFuncs<bool>(readBool, &LLParamSDParser::writeTypedValue<bool>);
		registerParserFuncs<std::string>(readString, &LLParamSDParser::writeTypedValue<std::string>);
		registerParserFuncs<LLUUID>(readUUID, &LLParamSDParser::writeTypedValue<LLUUID>);
		registerParserFuncs<LLDate>(readDate, &LLParamSDParser::writeTypedValue<LLDate>);
		registerParserFuncs<LLURI>(readURI, &LLParamSDParser::writeTypedValue<LLURI>);
		registerParserFuncs<LLSD>(readSD, &LLParamSDParser::writeTypedValue<LLSD>);
	}
}

// special case handling of U32 due to ambiguous LLSD::assign overload
bool LLParamSDParser::writeU32Param(LLParamSDParser::parser_t& parser, const void* val_ptr, const parser_t::name_stack_t& name_stack)
{
	LLParamSDParser& sdparser = static_cast<LLParamSDParser&>(parser);
	if (!sdparser.mWriteRootSD) return false;
	
	LLSD* sd_to_write = sdparser.getSDWriteNode(name_stack);
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
	mNameStack.clear();
	mWriteRootSD = &sd;
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
	LLSD* sd_to_write = mWriteRootSD;
	bool new_traversal = false;
	for (name_stack_t::const_iterator it = name_stack.begin(), prev_it = mNameStack.begin();
		it != name_stack.end();
		++it)
	{
		bool new_array_entry = false;
		if (prev_it == mNameStack.end())
		{
			new_traversal = true;
		}
		else
		{
			if (!new_traversal						// have not diverged yet from previous trace
				&& prev_it->first == it->first		// names match
				&& prev_it->second != it->second)	// versions differ
			{
				// name stacks match, but version numbers differ in last place.
				// create a different entry at this point using an LLSD array
				new_array_entry = true;
			}
			if (prev_it->first != it->first			// names differ
				|| prev_it->second != it->second)	// versions differ
			{
				// at this point we have diverged from our last trace
				// so any elements referenced here are new
				new_traversal = true;
			}
		}

		LLSD* child_sd = &(*sd_to_write)[it->first];

		if (child_sd->isArray())
		{
			if (new_traversal)
			{
				// write to new element at end
				sd_to_write = &(*child_sd)[child_sd->size()];
			}
			else
			{
				// write to last of existing elements, or first element if empty
				sd_to_write = &(*child_sd)[llmax(0, child_sd->size() - 1)];
			}
		}
		else
		{
			if (new_array_entry && !child_sd->isArray())
			{
				// copy child contents into first element of an array
				LLSD new_array = LLSD::emptyArray();
				new_array.append(*child_sd);
				// assign array to slot that previously held the single value
				*child_sd = new_array;
				// return next element in that array
				sd_to_write = &((*child_sd)[1]);
			}
			else
			{
				sd_to_write = child_sd;
			}
		}
		if (prev_it != mNameStack.end())
		{
			++prev_it;
		}
	}
	mNameStack = name_stack;
	
	//llinfos << ll_pretty_print_sd(*mWriteRootSD) << llendl;
	return sd_to_write;
}

bool LLParamSDParser::readS32(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

    *((S32*)val_ptr) = self.mCurReadSD->asInteger();
    return true;
}

bool LLParamSDParser::readU32(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

    *((U32*)val_ptr) = self.mCurReadSD->asInteger();
    return true;
}

bool LLParamSDParser::readF32(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

    *((F32*)val_ptr) = self.mCurReadSD->asReal();
    return true;
}

bool LLParamSDParser::readF64(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

    *((F64*)val_ptr) = self.mCurReadSD->asReal();
    return true;
}

bool LLParamSDParser::readBool(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

    *((bool*)val_ptr) = self.mCurReadSD->asBoolean();
    return true;
}

bool LLParamSDParser::readString(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

	*((std::string*)val_ptr) = self.mCurReadSD->asString();
    return true;
}

bool LLParamSDParser::readUUID(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

	*((LLUUID*)val_ptr) = self.mCurReadSD->asUUID();
    return true;
}

bool LLParamSDParser::readDate(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

	*((LLDate*)val_ptr) = self.mCurReadSD->asDate();
    return true;
}

bool LLParamSDParser::readURI(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

	*((LLURI*)val_ptr) = self.mCurReadSD->asURI();
    return true;
}

bool LLParamSDParser::readSD(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);

	*((LLSD*)val_ptr) = *self.mCurReadSD;
    return true;
}
