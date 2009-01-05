/** 
 * @file lluistring.cpp
 * @brief LLUIString implementation.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "lluistring.h"
#include "llsd.h"

const LLStringUtil::format_map_t LLUIString::sNullArgs;


LLUIString::LLUIString(const std::string& instring, const LLStringUtil::format_map_t& args)
	: mOrig(instring),
	  mArgs(args)
{
	format();
}

void LLUIString::assign(const std::string& s)
{
	mOrig = s;
	format();
}

void LLUIString::setArgList(const LLStringUtil::format_map_t& args)
{
	mArgs = args;
	format();
}

void LLUIString::setArgs(const LLSD& sd)
{
	if (!sd.isMap()) return;
	for(LLSD::map_const_iterator sd_it = sd.beginMap();
		sd_it != sd.endMap();
		++sd_it)
	{
		setArg(sd_it->first, sd_it->second.asString());
	}
	format();
}

void LLUIString::setArg(const std::string& key, const std::string& replacement)
{
	mArgs[key] = replacement;
	format();
}

void LLUIString::truncate(S32 maxchars)
{
	if (mWResult.size() > (size_t)maxchars)
	{
		LLWStringUtil::truncate(mWResult, maxchars);
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

void LLUIString::format()
{
	mResult = mOrig;
	LLStringUtil::format(mResult, mArgs);
	mWResult = utf8str_to_wstring(mResult);
}
