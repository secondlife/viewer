/** 
 * @file llteleporthistory.h
 * @brief Teleport history
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

#ifndef LL_LLTELEPORTHISTORY_H
#define LL_LLTELEPORTHISTORY_H

#include "llsingleton.h" // for LLSingleton

#include <vector>
#include <string>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include "llteleporthistorystorage.h"


/**
 * An item of the teleport history.
 * 
 * Contains the location's global coordinates and its title.
 */
class LLTeleportHistoryItem
{
public:
	LLTeleportHistoryItem()
	{}

	LLTeleportHistoryItem(std::string title, LLVector3d global_pos)
		: mTitle(title), mGlobalPos(global_pos)
	{}

	/**
	 * @return title formatted according to the current value of the 
	 * NavBarShowCoordinates setting.
	 */
	const std::string& getTitle() const;
	
	std::string	mTitle;		// human-readable location title
	std::string mFullTitle; // human-readable location title including coordinates
	LLVector3d	mGlobalPos; // global position
	LLUUID		mRegionID;	// region ID for getting the region info 
};

/**
 * Teleport history.
 * 
 * Along with the navigation bar "Back" and "Forward" buttons
 * implements web browser-like navigation functionality.
 * 
 * @see LLNavigationBar
 */
class LLTeleportHistory: public LLSingleton<LLTeleportHistory>
{
	LOG_CLASS(LLTeleportHistory);

public:
	
	typedef std::vector<LLTeleportHistoryItem>	slurl_list_t;
	typedef boost::function<void()>				history_callback_t;
	typedef boost::signals2::signal<void()>		history_signal_t;
	
	LLTeleportHistory();
	~LLTeleportHistory();

	/**
	 * Go back in the history.
	 */
	void					goBack() { goToItem(getCurrentItemIndex() - 1); }
	
	/**
	 * Go forward in the history.
	 */
	void					goForward() { goToItem(getCurrentItemIndex() + 1); }
	
	/**
	 * Go to specific item in the history.
	 * 
	 * The item is specified by its index (starting from 0).
	 */ 
	void					goToItem(int idx);
	
	/**
	 * @return history items.
	 */
	const slurl_list_t&		getItems() const { return mItems; }
	void                    purgeItems();
	/**
	 * Is the history empty?
	 * 
	 * History containing single item is treated as empty
	 * because the item points to the current location.
	 */ 
	bool					isEmpty() const { return mItems.size() <= 1; }
	
	/**
	 * Get index of the current location in the history.
	 */
	int						getCurrentItemIndex() const { return mCurrentItem; }
	/**
	 * Set a callback to be called upon history changes.
	 * 
	 * Multiple callbacks can be set.
	 */
	boost::signals2::connection	setHistoryChangedCallback(history_callback_t cb);
	
	/**
	 * Save history to a file so that we can restore it on startup.
	 * 
	 * @see load()
	 */
	void					dump() const;
	/**
	 * Process login complete event. Basically put current location into history
	 */
	void					handleLoginComplete();

private:
	
	/**
	 * Called by when a teleport fails.
	 * 
	 * Called via callback set on the LLViewerParcelMgr "teleport failed" signal.
	 * 
	 * @see mTeleportFailedConn
	 */
	void onTeleportFailed();

	/**
	 * Update current location.
	 * 
	 * @param new_pos Current agent global position. After local teleports we
	 *                cannot rely on gAgent.getPositionGlobal(),
	 *                so the new position gets passed explicitly.
	 * 
	 * Called when a teleport finishes.
	 * Called via callback set on the LLViewerParcelMgr "teleport finished" signal.
	 *
	 * Takes mRequestedItem into consideration: if it's not -1
	 * (i.e. user is teleporting to an arbitrary location, not to a history item)
	 * we purge forward items and append a new one, making it current. Otherwise
	 * we just modify mCurrentItem.
	 * 
	 * @see mRequestedItem
	 * @see mGotInitialUpdate
	 */
	void					updateCurrentLocation(const LLVector3d& new_pos);
	
	/**
	 * Invokes the "history changed" callback(s).
	 */
	void					onHistoryChanged();

	/**
	 * Format current agent location in a human-readable manner.
	 * 
	 * @param full whether to include coordinates
	 * @param local_pos_override hack: see description of updateCurrentLocation()
	 * @return
	 */
	static std::string		getCurrentLocationTitle(bool full, const LLVector3& local_pos_override);
	
	/**
	 * Actually, the teleport history.
	 */
	slurl_list_t			mItems;
	
	/**
	 * Current position within the history.
	 */
	int						mCurrentItem;
	
	/**
	 * Requested position within the history.
	 * 
	 * When a teleport succeeds, this is checked by updateCurrentLocation() to tell
	 * if this is a teleport within the history (mRequestedItem >=0) or not (-1).
	 * 
	 * Set by goToItem(); reset by onTeleportFailed() (if teleport fails).
	 * 
	 * @see goToItem()
	 * @see updateCurrentLocation()
	 */
	int						mRequestedItem;
	
	/**
	 * Have we received the initial location update?
	 * 
	 * @see updateCurrentLocation()
	 */
	bool					mGotInitialUpdate;
	
	LLTeleportHistoryStorage*	mTeleportHistoryStorage;

	/**
	 * Signal emitted when the history gets changed.
	 * 
	 * Invokes callbacks set with setHistoryChangedCallback().
	 */
	history_signal_t		mHistoryChangedSignal;
	
	/**
	 * Teleport success notification connection.
	 * 
	 * Using this connection we get notified when a teleport finishes
	 * or initial location update occurs.
	 */
	boost::signals2::connection	mTeleportFinishedConn;
	
	/**
	 * Teleport failure notification connection.
	 * 
	 * Using this connection we get notified when a teleport fails.
	 */
	boost::signals2::connection	mTeleportFailedConn;
};

#endif
