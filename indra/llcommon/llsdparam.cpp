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
#include "boost/bind.hpp"

static 	LLInitParam::Parser::parser_read_func_map_t sReadFuncs;
static 	LLInitParam::Parser::parser_write_func_map_t sWriteFuncs;
static 	LLInitParam::Parser::parser_inspect_func_map_t sInspectFuncs;
static const LLSD NO_VALUE_MARKER;

//
// LLParamSDParser
//
LLParamSDParser::LLParamSDParser()
: Parser(sReadFuncs, sWriteFuncs, sInspectFuncs)
{
	using boost::bind;

	if (sReadFuncs.empty())
	{
		registerParserFuncs<LLInitParam::Flag>(readFlag, &LLParamSDParser::writeFlag);
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
bool LLParamSDParser::writeU32Param(LLParamSDParser::parser_t& parser, const void* val_ptr, parser_t::name_stack_t& name_stack)
{
	LLParamSDParser& sdparser = static_cast<LLParamSDParser&>(parser);
	if (!sdparser.mWriteRootSD) return false;
	
	parser_t::name_stack_range_t range(name_stack.begin(), name_stack.end());
	LLSD& sd_to_write = LLParamSDParserUtilities::getSDWriteNode(*sdparser.mWriteRootSD, range);
	sd_to_write.assign((S32)*((const U32*)val_ptr));

	return true;
}

bool LLParamSDParser::writeFlag(LLParamSDParser::parser_t& parser, const void* val_ptr, parser_t::name_stack_t& name_stack)
{
	LLParamSDParser& sdparser = static_cast<LLParamSDParser&>(parser);
	if (!sdparser.mWriteRootSD) return false;

	parser_t::name_stack_range_t range(name_stack.begin(), name_stack.end());
	LLParamSDParserUtilities::getSDWriteNode(*sdparser.mWriteRootSD, range);

	return true;
}

void LLParamSDParser::submit(LLInitParam::BaseBlock& block, const LLSD& sd, LLInitParam::Parser::name_stack_t& name_stack)
{
	mCurReadSD = &sd;
	block.submitValue(name_stack, *this);
}

void LLParamSDParser::readSD(const LLSD& sd, LLInitParam::BaseBlock& block, bool silent)
{
	mCurReadSD = NULL;
	mNameStack.clear();
	setParseSilently(silent);

	LLParamSDParserUtilities::readSDValues(boost::bind(&LLParamSDParser::submit, this, boost::ref(block), _1, _2), sd, mNameStack);
	//readSDValues(sd, block);
}

void LLParamSDParser::writeSDImpl(LLSD& sd, const LLInitParam::BaseBlock& block, const LLInitParam::predicate_rule_t rules, const LLInitParam::BaseBlock* diff_block)
{
	mNameStack.clear();
	mWriteRootSD = &sd;

	name_stack_t name_stack;
	block.serializeBlock(*this, name_stack, rules, diff_block);
}

/*virtual*/ std::string LLParamSDParser::getCurrentElementName()
{
	std::string full_name = "sd";
	for (name_stack_t::value_type& stack_pair : mNameStack)
	{
		full_name += llformat("[%s]", stack_pair.first.c_str());
	}

	return full_name;
}


bool LLParamSDParser::readFlag(Parser& parser, void* val_ptr)
{
	LLParamSDParser& self = static_cast<LLParamSDParser&>(parser);
	return self.mCurReadSD == &NO_VALUE_MARKER;
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

// static
LLSD& LLParamSDParserUtilities::getSDWriteNode(LLSD& input, LLInitParam::Parser::name_stack_range_t& name_stack_range)
{
	LLSD* sd_to_write = &input;
	
	for (LLInitParam::Parser::name_stack_t::iterator it = name_stack_range.first;
		it != name_stack_range.second;
		++it)
	{
		bool new_traversal = it->second;

		LLSD* child_sd;
		if (it->first.empty())
		{
			child_sd = sd_to_write;
			if (child_sd->isUndefined())
			{
				*child_sd = LLSD::emptyArray();
			}
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
			sd_to_write = &(*sd_to_write)[it->first];
		}
		it->second = false;
	}
	
	return *sd_to_write;
}

//static
void LLParamSDParserUtilities::readSDValues(read_sd_cb_t cb, const LLSD& sd, LLInitParam::Parser::name_stack_t& stack)
{
	if (sd.isMap())
	{
		for (LLSD::map_const_iterator it = sd.beginMap();
			it != sd.endMap();
			++it)
		{
			stack.push_back(make_pair(it->first, true));
			readSDValues(cb, it->second, stack);
			stack.pop_back();
		}
	}
	else if (sd.isArray())
	{
		for (LLSD::array_const_iterator it = sd.beginArray();
			it != sd.endArray();
			++it)
		{
			stack.push_back(make_pair(std::string(), true));
			readSDValues(cb, *it, stack);
			stack.pop_back();
		}
	}
	else if (sd.isUndefined())
	{
		if (!cb.empty())
		{
			cb(NO_VALUE_MARKER, stack);
		}
	}
	else
	{
		if (!cb.empty())
		{
			cb(sd, stack);
		}
	}
}

//static
void LLParamSDParserUtilities::readSDValues(read_sd_cb_t cb, const LLSD& sd)
{
	LLInitParam::Parser::name_stack_t stack = LLInitParam::Parser::name_stack_t();
	readSDValues(cb, sd, stack);
}

namespace LLInitParam
{
	// LLSD specialization
	// block param interface
	bool ParamValue<LLSD, NOT_BLOCK>::deserializeBlock(Parser& p, Parser::name_stack_range_t& name_stack, bool new_name)
	{
		if (name_stack.first == name_stack.second
			&& p.readValue<LLSD>(mValue))
		{
			return true;
		}

		LLSD& sd = LLParamSDParserUtilities::getSDWriteNode(mValue, name_stack);

		LLSD::String string;

		if (p.readValue<LLSD::String>(string))
		{
			sd = string;
			return true;
		}
		return false;
	}

	//static
	void ParamValue<LLSD, NOT_BLOCK>::serializeElement(Parser& p, const LLSD& sd, Parser::name_stack_t& name_stack)
	{
		p.writeValue<LLSD::String>(sd.asString(), name_stack);
	}

	bool ParamValue<LLSD, NOT_BLOCK>::serializeBlock(Parser& p, Parser::name_stack_t& name_stack_range, const predicate_rule_t predicate_rule, const BaseBlock* diff_block) const
	{
		// attempt to write LLSD out directly
		if (!p.writeValue<LLSD>(mValue, name_stack_range))
		{
			// otherwise read from LLSD value and serialize out to parser (which could be LLSD, XUI, etc)
			LLParamSDParserUtilities::readSDValues(boost::bind(&serializeElement, boost::ref(p), _1, _2), mValue, name_stack_range);
		}
		return true;
	}
}
