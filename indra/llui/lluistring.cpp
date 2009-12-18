/** 
 * @file lluistring.cpp
 * @brief LLUIString implementation.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "lluistring.h"
#include "llsd.h"
#include "lltrans.h"

LLFastTimer::DeclareTimer FTM_UI_STRING("UI String");


LLUIString::LLUIString(const std::string& instring, const LLStringUtil::format_map_t& args)
:	mOrig(instring),
	mArgs(args)
{
	dirty();
}

void LLUIString::assign(const std::string& s)
{
	mOrig = s;
	dirty();
}

void LLUIString::setArgList(const LLStringUtil::format_map_t& args)

{
	mArgs = args;
	dirty();
}

void LLUIString::setArgs(const LLSD& sd)
{
	LLFastTimer timer(FTM_UI_STRING);
	
	if (!sd.isMap()) return;
	for(LLSD::map_const_iterator sd_it = sd.beginMap();
		sd_it != sd.endMap();
		++sd_it)
	{
		setArg(sd_it->first, sd_it->second.asString());
	}
	dirty();
}

void LLUIString::setArg(const std::string& key, const std::string& replacement)
{
	mArgs[key] = replacement;
	dirty();
}

void LLUIString::truncate(S32 maxchars)
{
	if (getUpdatedWResult().size() > (size_t)maxchars)
	{
		LLWStringUtil::truncate(getUpdatedWResult(), maxchars);
		mResult = wstring_to_utf8str(getUpdatedWResult());
	}
}

void LLUIString::erase(S32 charidx, S32 len)
{
	getUpdatedWResult().erase(charidx, len);
	mResult = wstring_to_utf8str(getUpdatedWResult());
}

void LLUIString::insert(S32 charidx, const LLWString& wchars)
{
	getUpdatedWResult().insert(charidx, wchars);
	mResult = wstring_to_utf8str(getUpdatedWResult());
}

void LLUIString::replace(S32 charidx, llwchar wc)
{
	getUpdatedWResult()[charidx] = wc;
	mResult = wstring_to_utf8str(getUpdatedWResult());
}

void LLUIString::clear()
{
	// Keep Args
	mOrig.clear();
	mResult.clear();
	mWResult.clear();
}

void LLUIString::dirty()
{
	mNeedsResult = true;
	mNeedsWResult = true;
}

void LLUIString::updateResult() const
{
	mNeedsResult = false;

	LLFastTimer timer(FTM_UI_STRING);
	
	// optimize for empty strings (don't attempt string replacement)
	if (mOrig.empty())
	{
		mResult.clear();
		mWResult.clear();
		return;
	}
	mResult = mOrig;
	
	// get the defailt args + local args
	LLStringUtil::format_map_t combined_args = LLTrans::getDefaultArgs();
	combined_args.insert(mArgs.begin(), mArgs.end());
	LLStringUtil::format(mResult, combined_args);
}

void LLUIString::updateWResult() const
{
	mNeedsWResult = false;

	mWResult = utf8str_to_wstring(getUpdatedResult());
}
