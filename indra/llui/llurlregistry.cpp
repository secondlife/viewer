/** 
 * @file llurlregistry.cpp
 * @author Martin Reddy
 * @brief Contains a set of Url types that can be matched in a string
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llurlregistry.h"

#include <boost/regex.hpp>

// default dummy callback that ignores any label updates from the server
void LLUrlRegistryNullCallback(const std::string &url, const std::string &label, const std::string& icon)
{
}

LLUrlRegistry::LLUrlRegistry()
{
	mUrlEntry.reserve(20);

	// Urls are matched in the order that they were registered
	registerUrl(new LLUrlEntryNoLink());
	registerUrl(new LLUrlEntryIcon());
	registerUrl(new LLUrlEntrySLURL());
	registerUrl(new LLUrlEntryHTTP());
	registerUrl(new LLUrlEntryHTTPLabel());
	registerUrl(new LLUrlEntryAgentCompleteName());
	registerUrl(new LLUrlEntryAgentDisplayName());
	registerUrl(new LLUrlEntryAgentUserName());
	// LLUrlEntryAgent*Name must appear before LLUrlEntryAgent since 
	// LLUrlEntryAgent is a less specific (catchall for agent urls)
	registerUrl(new LLUrlEntryAgent());
	registerUrl(new LLUrlEntryGroup());
	registerUrl(new LLUrlEntryParcel());
	registerUrl(new LLUrlEntryTeleport());
	registerUrl(new LLUrlEntryRegion());
	registerUrl(new LLUrlEntryWorldMap());
	registerUrl(new LLUrlEntryObjectIM());
	registerUrl(new LLUrlEntryPlace());
	registerUrl(new LLUrlEntryInventory());
	registerUrl(new LLUrlEntryObjectIM());
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

void LLUrlRegistry::registerUrl(LLUrlEntryBase *url, bool force_front)
{
	if (url)
	{
		if (force_front)  // IDEVO
			mUrlEntry.insert(mUrlEntry.begin(), url);
		else
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
			text.find("<nolink>") != std::string::npos ||
			text.find("<icon") != std::string::npos);
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
						match_entry->getTooltip(url),
						match_entry->getIcon(url),
						match_entry->getStyle(),
						match_entry->getMenuName(),
						match_entry->getLocation(url),
						match_entry->getID(url),
						match_entry->underlineOnHoverOnly(url));
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
						match.getStyle(),
						match.getMenuName(),
						match.getLocation(),
						match.getID(),
						match.underlineOnHoverOnly());
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
