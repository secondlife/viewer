/**
 * @file llteleporthistorystorage.h
 * @brief Saving/restoring of teleport history.
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

#ifndef LL_LLTELEPORTHISTORYSTORAGE_H
#define LL_LLTELEPORTHISTORYSTORAGE_H

#include <vector>

#include "llsingleton.h"
#include "lldate.h"
#include "v3dmath.h"

class LLSD;

/**
 * An item of the teleport history, stored in file
 *
 * Contains the location's global coordinates, title and date.
 */
class LLTeleportHistoryPersistentItem
{
public:
	LLTeleportHistoryPersistentItem()
	{}

	LLTeleportHistoryPersistentItem(const std::string title, const LLVector3d& global_pos)
		: mTitle(title), mGlobalPos(global_pos), mDate(LLDate::now())
	{}

	LLTeleportHistoryPersistentItem(const std::string title, const LLVector3d& global_pos, const LLDate& date)
		: mTitle(title), mGlobalPos(global_pos), mDate(date)
	{}

	LLTeleportHistoryPersistentItem(const LLSD& val);
	LLSD toLLSD() const;

	std::string	mTitle;
	LLVector3d	mGlobalPos;
	LLDate		mDate;
};

/**
 * Persistent teleport history.
 *
 */
class LLTeleportHistoryStorage: public LLSingleton<LLTeleportHistoryStorage>
{
	LOG_CLASS(LLTeleportHistoryStorage);

public:

	typedef std::vector<LLTeleportHistoryPersistentItem> slurl_list_t;

	// removed_index is index of removed item, which replaced by more recent
	typedef boost::function<void(S32 removed_index)>		history_callback_t;
	typedef boost::signals2::signal<void(S32 removed_index)>	history_signal_t;

	LLTeleportHistoryStorage();
	~LLTeleportHistoryStorage();

	/**
	 * @return history items.
	 */
	const slurl_list_t& getItems() const { return mItems; }
	void			purgeItems();

	void addItem(const std::string title, const LLVector3d& global_pos);
	void addItem(const std::string title, const LLVector3d& global_pos, const LLDate& date);

	void removeItem(S32 idx);

	void save();

	/**
	 * Set a callback to be called upon history changes.
	 * 
	 * Multiple callbacks can be set.
	 */
	boost::signals2::connection	setHistoryChangedCallback(history_callback_t cb);

	/**
	 * Go to specific item in the history.
	 * 
	 * The item is specified by its index (starting from 0).
	 */
	void					goToItem(S32 idx);

private:

	void load();
	void dump() const;
	
	void onTeleportHistoryChange();
	bool compareByTitleAndGlobalPos(const LLTeleportHistoryPersistentItem& a, const LLTeleportHistoryPersistentItem& b);

	slurl_list_t	mItems;
	std::string	mFilename;

	/**
	 * Signal emitted when the history gets changed.
	 * 
	 * Invokes callbacks set with setHistoryChangedCallback().
	 */
	history_signal_t		mHistoryChangedSignal;
};

#endif //LL_LLTELEPORTHISTORYSTORAGE_H
