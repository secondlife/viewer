/** 
 * @file llurlregistry.cpp
 * @author Martin Reddy
 * @brief Contains a set of Url types that can be matched in a string
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

#include "linden_common.h"
#include "llurlregistry.h"

#include <boost/regex.hpp>

// default dummy callback that ignores any label updates from the server
void LLUrlRegistryNullCallback(const std::string &url, const std::string &label)
{
}

LLUrlRegistry::LLUrlRegistry()
{
	// Urls are matched in the order that they were registered
	registerUrl(new LLUrlEntryNoLink());
	registerUrl(new LLUrlEntrySLURL());
	registerUrl(new LLUrlEntryHTTP());
	registerUrl(new LLUrlEntryHTTPLabel());
	registerUrl(new LLUrlEntryAgent());
	registerUrl(new LLUrlEntryGroup());
	registerUrl(new LLUrlEntryParcel());
	registerUrl(new LLUrlEntryTeleport());
	registerUrl(new LLUrlEntryWorldMap());
	registerUrl(new LLUrlEntryPlace());
	registerUrl(new LLUrlEntryInventory());
	//LLUrlEntrySL and LLUrlEntrySLLabel have more common pattern, 
	//so it should be registered in the end of list
	registerUrl(new LLUrlEntrySL());
	registerUrl(new LLUrlEntrySLLabel());
	// most common pattern is a URL without any protocol,
	// e.g., "secondlife.com"
	registerUrl(new LLUrlEntryHTTPNoProtocol());	
}

LLUrlRegistry::~LLUrlRegistry()
{
	// free all of the LLUrlEntryBase objects we are holding
	std::vector<LLUrlEntryBase *>::iterator it;
	for (it = mUrlEntry.begin(); it != mUrlEntry.end(); ++it)
	{
		delete *it;
	}
}

void LLUrlRegistry::registerUrl(LLUrlEntryBase *url)
{
	if (url)
	{
		mUrlEntry.push_back(url);
	}
}

static bool matchRegex(const char *text, boost::regex regex, U32 &start, U32 &end)
{
	boost::cmatch result;
	bool found;

	// regex_search can potentially throw an exception, so check for it
	try
	{
		found = boost::regex_search(text, result, regex);
	}
	catch (std::runtime_error &)
	{
		return false;
	}

	if (! found)
	{
		return false;
	}

	// return the first/last character offset for the matched substring
	start = static_cast<U32>(result[0].first - text);
	end = static_cast<U32>(result[0].second - text) - 1;

	// we allow certain punctuation to terminate a Url but not match it,
	// e.g., "http://foo.com/." should just match "http://foo.com/"
	if (text[end] == '.' || text[end] == ',')
	{
		end--;
	}
	// ignore a terminating ')' when Url contains no matching '('
	// see DEV-19842 for details
	else if (text[end] == ')' && std::string(text+start, end-start).find('(') == std::string::npos)
	{
		end--;
	}

	return true;
}

static bool stringHasUrl(const std::string &text)
{
	// fast heuristic test for a URL in a string. This is used
	// to avoid lots of costly regex calls, BUT it needs to be
	// kept in sync with the LLUrlEntry regexes we support.
	return (text.find("://") != std::string::npos ||
			text.find("www.") != std::string::npos ||
			text.find(".com") != std::string::npos ||
			text.find(".net") != std::string::npos ||
			text.find(".edu") != std::string::npos ||
			text.find(".org") != std::string::npos ||
			text.find("<nolink>") != std::string::npos);
}

bool LLUrlRegistry::findUrl(const std::string &text, LLUrlMatch &match, const LLUrlLabelCallback &cb)
{
	// avoid costly regexes if there is clearly no URL in the text
	if (! stringHasUrl(text))
	{
		return false;
	}

	// find the first matching regex from all url entries in the registry
	U32 match_start = 0, match_end = 0;
	LLUrlEntryBase *match_entry = NULL;

	std::vector<LLUrlEntryBase *>::iterator it;
	for (it = mUrlEntry.begin(); it != mUrlEntry.end(); ++it)
	{
		LLUrlEntryBase *url_entry = *it;

		U32 start = 0, end = 0;
		if (matchRegex(text.c_str(), url_entry->getPattern(), start, end))
		{
			// does this match occur in the string before any other match
			if (start < match_start || match_entry == NULL)
			{
				match_start = start;
				match_end = end;
				match_entry = url_entry;
			}
		}
	}
	
	// did we find a match? if so, return its details in the match object
	if (match_entry)
	{
		// fill in the LLUrlMatch object and return it
		std::string url = text.substr(match_start, match_end - match_start + 1);
		match.setValues(match_start, match_end,
						match_entry->getUrl(url),
						match_entry->getLabel(url, cb),
						match_entry->getTooltip(),
						match_entry->getIcon(),
						match_entry->getColor(),
						match_entry->getMenuName(),
						match_entry->getLocation(url),
						match_entry->isLinkDisabled());
		return true;
	}

	return false;
}

bool LLUrlRegistry::findUrl(const LLWString &text, LLUrlMatch &match, const LLUrlLabelCallback &cb)
{
	// boost::regex_search() only works on char or wchar_t
	// types, but wchar_t is only 2-bytes on Win32 (not 4).
	// So we use UTF-8 to make this work the same everywhere.
	std::string utf8_text = wstring_to_utf8str(text);
	if (findUrl(utf8_text, match, cb))
	{
		// we cannot blindly return the start/end offsets from
		// the UTF-8 string because it is a variable-length
		// character encoding, so we need to update the start
		// and end values to be correct for the wide string.
		LLWString wurl = utf8str_to_wstring(match.getUrl());
		S32 start = text.find(wurl);
		if (start == std::string::npos)
		{
			return false;
		}
		S32 end = start + wurl.size() - 1;

		match.setValues(start, end, match.getUrl(), 
						match.getLabel(),
						match.getTooltip(),
						match.getIcon(),
						match.getColor(),
						match.getMenuName(),
						match.getLocation(),
						match.isLinkDisabled());
		return true;
	}
	return false;
}

bool LLUrlRegistry::hasUrl(const std::string &text)
{
	LLUrlMatch match;
	return findUrl(text, match);
}

bool LLUrlRegistry::hasUrl(const LLWString &text)
{
	LLUrlMatch match;
	return findUrl(text, match);
}

bool LLUrlRegistry::isUrl(const std::string &text)
{
	LLUrlMatch match;
	if (findUrl(text, match))
	{
		return (match.getStart() == 0 && match.getEnd() >= text.size()-1);
	}
	return false;
}

bool LLUrlRegistry::isUrl(const LLWString &text)
{
	LLUrlMatch match;
	if (findUrl(text, match))
	{
		return (match.getStart() == 0 && match.getEnd() >= text.size()-1);
	}
	return false;
}
