/** 
 * @file lllocationhistory.cpp
 * @brief Typed locations history
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
}

bool LLLocationHistory::getMatchingItems(std::string substring, location_list_t& result) const
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
	llinfos << "Location history dump:" << llendl;
	int i = 0;
	for (location_list_t::const_iterator it = mItems.begin(); it != mItems.end(); ++it, ++i)
	{
	    llinfos << "#" << std::setw(2) << std::setfill('0') << i << ": " << it->getLocation() << llendl;
	}
}

void LLLocationHistory::save() const
{
	// build filename for each user
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mFilename);

	if (resolved_filename.empty())
	{
		llinfos << "can't get path to location history filename - probably not logged in yet." << llendl;
		return;
	}

	// open a file for writing
	llofstream file (resolved_filename);
	if (!file.is_open())
	{
		llwarns << "can't open location history file \"" << mFilename << "\" for writing" << llendl;
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
	llinfos << "Loading location history." << llendl;
	
	// build filename for each user
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mFilename);
	llifstream file(resolved_filename);

	if (!file.is_open())
	{
		llwarns << "can't load location history from file \"" << mFilename << "\"" << llendl;
		return;
	}
	
	removeItems();
	
	// add each line in the file to the list
	std::string line;
	LLPointer<LLSDParser> parser = new LLSDNotationParser();
	while (std::getline(file, line)) {
		LLSD s_item;
		std::istringstream iss(line);
		if (parser->parse(iss, s_item, line.length()) == LLSDParser::PARSE_FAILURE)
		{
			llinfos<< "Parsing saved teleport history failed" << llendl;
			break;
		}

		mItems.push_back(s_item);
	}

	file.close();
	
	mLoadedSignal();
}
