/** 
 * @file llsearchhistory.h
 * @brief Search history container definition
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

#ifndef LL_LLSEARCHHISTORY_H
#define LL_LLSEARCHHISTORY_H

#include "llsingleton.h"
#include "llui.h"

/**
 * Search history container able to save and load history from file.
 * History is stored in chronological order, most recent at the beginning.
 */
class LLSearchHistory : public LLSingleton<LLSearchHistory>, private LLDestroyClass<LLSearchHistory>
{
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

	LLSearchHistory();

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
		bool operator < (const LLSearchHistory::LLSearchHistoryItem& right);

		/**
		 * Allows std::list sorting
		 */
		bool operator > (const LLSearchHistory::LLSearchHistoryItem& right);

		bool operator==(const LLSearchHistoryItem& right);

		bool operator==(const std::string& right);

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
