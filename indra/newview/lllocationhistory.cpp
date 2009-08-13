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

const char LLLocationHistory::delimiter = '\t';

LLLocationHistory::LLLocationHistory() :
	mFilename("typed_locations.txt")
{
}

void LLLocationHistory::addItem(const std::string & item, const std::string & tooltip) {
	static LLUICachedControl<S32> max_items("LocationHistoryMaxSize", 100);

	// check if this item doesn't duplicate any existing one
	if (touchItem(item)) {
		return;
	}

	mItems.push_back(item);
	mToolTips[item] = tooltip;

	// If the vector size exceeds the maximum, purge the oldest items.
	if ((S32)mItems.size() > max_items) {
		for(std::vector<std::string>::iterator i = mItems.begin(); i != mItems.end()-max_items; ++i) {
			mToolTips.erase(*i);
			mItems.erase(i);
		}
	}
}

bool LLLocationHistory::touchItem(const std::string & item) {
	bool result = false;
	std::vector<std::string>::iterator item_iter = std::find(mItems.begin(), mItems.end(), item);

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
	mToolTips.clear();
}

std::string LLLocationHistory::getToolTip(const std::string & item) const {
	std::map<std::string, std::string>::const_iterator i = mToolTips.find(item);

	return i != mToolTips.end() ? i->second : "";
}

bool LLLocationHistory::getMatchingItems(std::string substring, location_list_t& result) const
{
	// *TODO: an STL algorithm would look nicer
	result.clear();

	std::string needle = substring;
	LLStringUtil::toLower(needle);

	for (location_list_t::const_iterator it = mItems.begin(); it != mItems.end(); ++it)
	{
		std::string haystack = *it;
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
	    llinfos << "#" << std::setw(2) << std::setfill('0') << i << ": " << *it << llendl;
	}
}

void LLLocationHistory::save() const
{
	// build filename for each user
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mFilename);

	// open a file for writing
	llofstream file (resolved_filename);
	if (!file.is_open())
	{
		llwarns << "can't open location history file \"" << mFilename << "\" for writing" << llendl;
		return;
	}

	for (location_list_t::const_iterator it = mItems.begin(); it != mItems.end(); ++it)
		file << (*it) << delimiter << mToolTips.find(*it)->second << std::endl;

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

	while (std::getline(file, line)) {
		size_t dp = line.find(delimiter);

		if (dp != std::string::npos) {
			const std::string reg_name = line.substr(0, dp);
			const std::string tooltip = line.substr(dp + 1, std::string::npos);

			addItem(reg_name, tooltip);
		}
	}

	file.close();
	
	mLoadedSignal();
}
