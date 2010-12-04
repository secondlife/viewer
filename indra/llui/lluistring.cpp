/** 
 * @file lluistring.cpp
 * @brief LLUIString implementation.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#include "lluistring.h"
#include "llsd.h"
#include "lltrans.h"

LLFastTimer::DeclareTimer FTM_UI_STRING("UI String");


LLUIString::LLUIString(const std::string& instring, const LLStringUtil::format_map_t& args)
:	mOrig(instring),
	mArgs(new LLStringUtil::format_map_t(args))
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
	getArgs() = args;
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
	getArgs()[key] = replacement;
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
	if (!mArgs || mArgs->empty())
	{
		LLStringUtil::format(mResult, LLTrans::getDefaultArgs());
	}
	else
	{
		LLStringUtil::format_map_t combined_args = LLTrans::getDefaultArgs();
		combined_args.insert(mArgs->begin(), mArgs->end());
		LLStringUtil::format(mResult, combined_args);
	}
}

void LLUIString::updateWResult() const
{
	mNeedsWResult = false;

	mWResult = utf8str_to_wstring(getUpdatedResult());
}

LLStringUtil::format_map_t& LLUIString::getArgs()
{
	if (!mArgs)
	{
		mArgs = new LLStringUtil::format_map_t;
	}
	return *mArgs;
}
