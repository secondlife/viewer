/**
 * @file llsearchhistory.cpp
 * @brief Search history container implementation
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
    llifstream file(resolved_filename.c_str());
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
    llofstream file(resolved_filename.c_str());
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

bool LLSearchHistory::LLSearchHistoryItem::operator < (const LLSearchHistory::LLSearchHistoryItem& right) const
{
    S32 result = LLStringUtil::compareInsensitive(search_query, right.search_query);

    return result < 0;
}

bool LLSearchHistory::LLSearchHistoryItem::operator > (const LLSearchHistory::LLSearchHistoryItem& right) const
{
    S32 result = LLStringUtil::compareInsensitive(search_query, right.search_query);

    return result > 0;
}

bool LLSearchHistory::LLSearchHistoryItem::operator==(const LLSearchHistory::LLSearchHistoryItem& right) const
{
    return 0 == LLStringUtil::compareInsensitive(search_query, right.search_query);
}

bool LLSearchHistory::LLSearchHistoryItem::operator==(const std::string& right) const
{
    return 0 == LLStringUtil::compareInsensitive(search_query, right);
}

LLSD LLSearchHistory::LLSearchHistoryItem::toLLSD() const
{
    LLSD ret;
    ret[SEARCH_QUERY] = search_query;
    return ret;
}
