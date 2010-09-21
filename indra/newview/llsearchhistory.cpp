/**
 * @file llsearchhistory.cpp
 * @brief Search history container implementation
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
#include "llsearchhistory.h"

#include "llfile.h"
#include "llsdserialize.h"
#include "llxmlnode.h"

std::string LLSearchHistory::SEARCH_QUERY = "search_query";
std::string LLSearchHistory::SEARCH_HISTORY_FILE_NAME = "search_history.txt";

LLSearchHistory::LLSearchHistory()
{

}

bool LLSearchHistory::load()
{
	// build filename for each user
	std::string resolved_filename = getHistoryFilePath();
	llifstream file(resolved_filename);
	if (!file.is_open())
	{
		return false;
	}

	clearHistory();

	// add each line in the file to the list
	std::string line;
	LLPointer<LLSDParser> parser = new LLSDNotationParser();
	while (std::getline(file, line)) 
	{
		LLSD s_item;
		std::istringstream iss(line);
		if (parser->parse(iss, s_item, line.length()) == LLSDParser::PARSE_FAILURE)
		{
			break;
		}

		mSearchHistory.push_back(s_item);
	}

	file.close();

	return true;
}

bool LLSearchHistory::save()
{
	// build filename for each user
	std::string resolved_filename = getHistoryFilePath();
	// open a file for writing
	llofstream file (resolved_filename);
	if (!file.is_open())
	{
		return false;
	}

	search_history_list_t::const_iterator it = mSearchHistory.begin();
	for (; mSearchHistory.end() != it; ++it)
	{
		file << LLSDOStreamer<LLSDNotationFormatter>((*it).toLLSD()) << std::endl;
	}

	file.close();
	return true;
}

std::string LLSearchHistory::getHistoryFilePath()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SEARCH_HISTORY_FILE_NAME);
}

void LLSearchHistory::addEntry(const std::string& search_query)
{
	if(search_query.empty())
	{
		return;
	}

	search_history_list_t::iterator it = 
		find(mSearchHistory.begin(), mSearchHistory.end(), search_query);

	if(mSearchHistory.end() != it)
	{
		mSearchHistory.erase(it);
	}

	LLSearchHistoryItem item(search_query);
	mSearchHistory.push_front(item);
}

bool LLSearchHistory::LLSearchHistoryItem::operator < (const LLSearchHistory::LLSearchHistoryItem& right)
{
	S32 result = LLStringUtil::compareInsensitive(search_query, right.search_query);

	return result < 0;
}

bool LLSearchHistory::LLSearchHistoryItem::operator > (const LLSearchHistory::LLSearchHistoryItem& right)
{
	S32 result = LLStringUtil::compareInsensitive(search_query, right.search_query);

	return result > 0;
}

bool LLSearchHistory::LLSearchHistoryItem::operator==(const LLSearchHistory::LLSearchHistoryItem& right)
{
	return 0 == LLStringUtil::compareInsensitive(search_query, right.search_query);
}

bool LLSearchHistory::LLSearchHistoryItem::operator==(const std::string& right)
{
	return 0 == LLStringUtil::compareInsensitive(search_query, right);
}

LLSD LLSearchHistory::LLSearchHistoryItem::toLLSD() const
{
	LLSD ret;
	ret[SEARCH_QUERY] = search_query;
	return ret;
}
