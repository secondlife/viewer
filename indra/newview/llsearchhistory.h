/** 
 * @file llsearchhistory.h
 * @brief Search history container definition
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

#ifndef LL_LLSEARCHHISTORY_H
#define LL_LLSEARCHHISTORY_H

#include "llsingleton.h"
#include "llinitdestroyclass.h"
#include "llui.h"

/**
 * Search history container able to save and load history from file.
 * History is stored in chronological order, most recent at the beginning.
 */
class LLSearchHistory : public LLSingleton<LLSearchHistory>, private LLDestroyClass<LLSearchHistory>
{
	LLSINGLETON(LLSearchHistory);
	friend class LLDestroyClass<LLSearchHistory>;
public:

	// Forward declaration
	class LLSearchHistoryItem;

	// Search history container
	typedef std::list<LLSearchHistoryItem>	search_history_list_t;

	/**
	 * Saves search history to file
	 */
	bool save();

	/**
	 * loads search history from file
	 */
	bool load();

	/**
	 * Returns search history list
	 */
	search_history_list_t& getSearchHistoryList() { return mSearchHistory; }

	/**
	 * Deletes all search history queries from list.
	 */
	void clearHistory() { mSearchHistory.clear(); }

	/**
	 * Adds unique entry to front of search history list, case insensitive
	 * If entry is already in list, it will be deleted and added to front.
	 */
	void addEntry(const std::string& search_text);


	/**
	 * Class for storing data about single search request.
	 */
	class LLSearchHistoryItem
	{
	public:

		LLSearchHistoryItem()
		{}

		LLSearchHistoryItem(const std::string& query)
			: search_query(query)
		{}

		LLSearchHistoryItem(const LLSD& item)
		{
			if(item.has(SEARCH_QUERY))
				search_query = item[SEARCH_QUERY].asString();
		}

		std::string search_query;

		/**
		 * Allows std::list sorting
		 */
		bool operator < (const LLSearchHistory::LLSearchHistoryItem& right) const;

		/**
		 * Allows std::list sorting
		 */
		bool operator > (const LLSearchHistory::LLSearchHistoryItem& right) const;

		bool operator==(const LLSearchHistoryItem& right) const;

		bool operator==(const std::string& right) const;

		/**
		 * Serializes search history item to LLSD
		 */
		LLSD toLLSD() const;
	};

protected:

	/**
	 * Returns path to search history file.
	 */
	std::string getHistoryFilePath();

	static std::string SEARCH_HISTORY_FILE_NAME;
	static std::string SEARCH_QUERY;

private:

	// Implementation of LLDestroyClass<LLSearchHistory>
	static void destroyClass()
	{
		LLSearchHistory::getInstance()->save();
	}

	search_history_list_t mSearchHistory;
};

class LLSearchComboBox;

#endif //LL_LLSEARCHHISTORY_H
