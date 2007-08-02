/** 
 * @file lluistring.cpp
 * @brief LLUIString base class
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lluistring.h"

const LLString::format_map_t LLUIString::sNullArgs;


// public

LLUIString::LLUIString(const LLString& instring, const LLString::format_map_t& args)
	: mOrig(instring),
	  mArgs(args)
{
	format();
}

void LLUIString::assign(const LLString& s)
{
	mOrig = s;
	format();
}

void LLUIString::setArgList(const LLString::format_map_t& args)
{
	mArgs = args;
	format();
}

void LLUIString::setArg(const LLString& key, const LLString& replacement)
{
	mArgs[key] = replacement;
	format();
}

void LLUIString::truncate(S32 maxchars)
{
	if (mWResult.size() > (size_t)maxchars)
	{
		LLWString::truncate(mWResult, maxchars);
		mResult = wstring_to_utf8str(mWResult);
	}
}

void LLUIString::erase(S32 charidx, S32 len)
{
	mWResult.erase(charidx, len);
	mResult = wstring_to_utf8str(mWResult);
}

void LLUIString::insert(S32 charidx, const LLWString& wchars)
{
	mWResult.insert(charidx, wchars);
	mResult = wstring_to_utf8str(mWResult);
}

void LLUIString::replace(S32 charidx, llwchar wc)
{
	mWResult[charidx] = wc;
	mResult = wstring_to_utf8str(mWResult);
}

void LLUIString::clear()
{
	// Keep Args
	mOrig.clear();
	mResult.clear();
	mWResult.clear();
}

void LLUIString::clearArgs()
{
	mArgs.clear();
}

// private

void LLUIString::format()
{
	mResult = mOrig;
	LLString::format(mResult, mArgs);
	mWResult = utf8str_to_wstring(mResult);
}
