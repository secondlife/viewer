/** 
 * @file lllocationhistory.cpp
 * @brief Typed locations history
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

#include "lllocationhistory.h"

#include <iomanip> // for std::setw()

#include "llui.h"
#include "llsd.h"
#include "llsdserialize.h"

LLLocationHistory::LLLocationHistory() :
    mFilename("typed_locations.txt")
{
}

void LLLocationHistory::addItem(const LLLocationHistoryItem& item) {
    static LLUICachedControl<S32> max_items("LocationHistoryMaxSize", 100);

    // check if this item doesn't duplicate any existing one
    location_list_t::iterator item_iter = std::find(mItems.begin(), mItems.end(),item);
    if(item_iter != mItems.end()) // if it already exists, erase the old one
    {
        mItems.erase(item_iter);    
    }

    mItems.push_back(item);
    
    // If the vector size exceeds the maximum, purge the oldest items (at the start of the mItems vector).
    if ((S32)mItems.size() > max_items)
    {
        mItems.erase(mItems.begin(), mItems.end()-max_items);
    }
    llassert((S32)mItems.size() <= max_items);
    mChangedSignal(ADD);
}

/*
 * @brief Try to find item in history. 
 * If item has been founded, it will be places into end of history.
 * @return true - item has founded
 */
bool LLLocationHistory::touchItem(const LLLocationHistoryItem& item) {
    bool result = false;
    location_list_t::iterator item_iter = std::find(mItems.begin(), mItems.end(), item);

    // the last used item should be the first in the history
    if (item_iter != mItems.end()) {
        mItems.erase(item_iter);
        mItems.push_back(item);
        result = true;
    }

    return result;
}

void LLLocationHistory::removeItems()
{
    mItems.clear();
    mChangedSignal(CLEAR);
}

bool LLLocationHistory::getMatchingItems(const std::string& substring, location_list_t& result) const
{
    // *TODO: an STL algorithm would look nicer
    result.clear();

    std::string needle = substring;
    LLStringUtil::toLower(needle);

    for (location_list_t::const_iterator it = mItems.begin(); it != mItems.end(); ++it)
    {
        std::string haystack = it->getLocation();
        LLStringUtil::toLower(haystack);

        if (haystack.find(needle) != std::string::npos)
            result.push_back(*it);
    }
    
    return result.size();
}

void LLLocationHistory::dump() const
{
    LL_INFOS() << "Location history dump:" << LL_ENDL;
    int i = 0;
    for (location_list_t::const_iterator it = mItems.begin(); it != mItems.end(); ++it, ++i)
    {
        LL_INFOS() << "#" << std::setw(2) << std::setfill('0') << i << ": " << it->getLocation() << LL_ENDL;
    }
}

void LLLocationHistory::save() const
{
    // build filename for each user
    std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mFilename);

    if (resolved_filename.empty())
    {
        LL_INFOS() << "can't get path to location history filename - probably not logged in yet." << LL_ENDL;
        return;
    }

    // open a file for writing
    llofstream file(resolved_filename.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "can't open location history file \"" << mFilename << "\" for writing" << LL_ENDL;
        return;
    }

    for (location_list_t::const_iterator it = mItems.begin(); it != mItems.end(); ++it)
    {
        file << LLSDOStreamer<LLSDNotationFormatter>((*it).toLLSD()) << std::endl;
    }

    file.close();
}

void LLLocationHistory::load()
{
    LL_INFOS() << "Loading location history." << LL_ENDL;
    
    // build filename for each user
    std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mFilename);
    llifstream file(resolved_filename.c_str());

    if (!file.is_open())
    {
        LL_WARNS() << "can't load location history from file \"" << mFilename << "\"" << LL_ENDL;
        return;
    }
    
    mItems.clear();// need to use a direct call of clear() method to avoid signal invocation
    
    // add each line in the file to the list
    std::string line;
    LLPointer<LLSDParser> parser = new LLSDNotationParser();
    while (std::getline(file, line)) {
        LLSD s_item;
        std::istringstream iss(line);
        if (parser->parse(iss, s_item, line.length()) == LLSDParser::PARSE_FAILURE)
        {
            LL_INFOS()<< "Parsing saved teleport history failed" << LL_ENDL;
            break;
        }

        mItems.push_back(s_item);
    }

    file.close();
    
    mChangedSignal(LOAD);
}
