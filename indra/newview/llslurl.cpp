/** 
 * @file llslurl.cpp
 * @brief SLURL manipulation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llslurl.h"

#include "llweb.h"

const std::string LLSLURL::PREFIX_SL_HELP		= "secondlife://app.";
const std::string LLSLURL::PREFIX_SL			= "sl://";
const std::string LLSLURL::PREFIX_SECONDLIFE	= "secondlife://";
const std::string LLSLURL::PREFIX_SLURL			= "http://slurl.com/secondlife/";

const std::string LLSLURL::APP_TOKEN = "app/";

// static
std::string LLSLURL::stripProtocol(const std::string& url)
{
	std::string stripped = url;
	if (matchPrefix(stripped, PREFIX_SL_HELP))
	{
		stripped.erase(0, PREFIX_SL_HELP.length());
	}
	else if (matchPrefix(stripped, PREFIX_SL))
	{
		stripped.erase(0, PREFIX_SL.length());
	}
	else if (matchPrefix(stripped, PREFIX_SECONDLIFE))
	{
		stripped.erase(0, PREFIX_SECONDLIFE.length());
	}
	else if (matchPrefix(stripped, PREFIX_SLURL))
	{
		stripped.erase(0, PREFIX_SLURL.length());
	}
	
	return stripped;
}

// static
bool LLSLURL::isSLURL(const std::string& url)
{
	if (matchPrefix(url, PREFIX_SL_HELP))		return true;
	if (matchPrefix(url, PREFIX_SL))			return true;
	if (matchPrefix(url, PREFIX_SECONDLIFE))	return true;
	if (matchPrefix(url, PREFIX_SLURL))			return true;
	
	return false;
}

// static
bool LLSLURL::isSLURLCommand(const std::string& url)
{ 
	if (matchPrefix(url, PREFIX_SL + APP_TOKEN) ||
		matchPrefix(url, PREFIX_SECONDLIFE + "/" + APP_TOKEN) ||
		matchPrefix(url, PREFIX_SLURL + APP_TOKEN) )
	{
		return true;
	}

	return false;
}

// static
bool LLSLURL::isSLURLHelp(const std::string& url)
{
	return matchPrefix(url, PREFIX_SL_HELP);
}

// static
std::string LLSLURL::buildSLURL(const std::string& regionname, S32 x, S32 y, S32 z)
{
	std::string slurl = PREFIX_SLURL + regionname + llformat("/%d/%d/%d",x,y,z); 
	slurl = LLWeb::escapeURL( slurl );
	return slurl;
}

// static
std::string LLSLURL::buildUnescapedSLURL(const std::string& regionname, S32 x, S32 y, S32 z)
{
	std::string unescapedslurl = PREFIX_SLURL + regionname + llformat("/%d/%d/%d",x,y,z);
	return unescapedslurl;
}

// static
bool LLSLURL::matchPrefix(const std::string& url, const std::string& prefix)
{
	std::string test_prefix = url.substr(0, prefix.length());
	LLStringUtil::toLower(test_prefix);
	return test_prefix == prefix;
}
