/** 
 * @file llteleporthistory.h
 * @brief Teleport history
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

#ifndef LL_LLTELEPORTHISTORY_H
#define LL_LLTELEPORTHISTORY_H

#include "llsingleton.h" // for LLSingleton

#include <vector>
#include <string>
#include <boost/function.hpp>
#include <boost/signals/connection.hpp>


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
	
	LLTeleportHistoryItem(const LLSD& val);
	LLSD toLLSD() const;

	std::string	mTitle;		// human-readable location title
	LLVector3d	mGlobalPos; // global position
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
	 */
	void					setHistoryChangedCallback(history_callback_t cb);

	/**
	 * Save the history to a file, so that it can be restored upon next logon.
	 */
	void					save() const;
	
	/**
	 * Load previously saved history from a file.
	 */
	void					load();
	
	/**
	 * Save history to a file so that we can restore it on startup.
	 * 
	 * @see load()
	 */
	void					dump() const;

private:
	
	/**
	 * Update current location.
	 * 
	 * Called when a teleport finishes.
	 *
	 * Takes mHistoryTeleportInProgress into consideration: if it's false
	 * (i.e. user is teleporting to an arbitrary location, not to a history item)
	 * we purge forward items.
	 * 
	 * @see mHistoryTeleportInProgress
	 * @see mGotInitialUpdate
	 */
	void					updateCurrentLocation();
	
	/**
	 * Invokes mHistoryChangedCallback.
	 */
	void					onHistoryChanged();
	
	static std::string		getCurrentLocationTitle();
	
	/**
	 * Actually, the teleport history.
	 */
	slurl_list_t			mItems;
	
	/**
	 * Current position within the history.
	 */
	int						mCurrentItem;
	
	/**
	 * Indicates whether teleport back/forward is currently in progress.
	 * 
	 * Helps to make sure we don't purge forward items
	 * when a teleport within history finishes. 
	 * 
	 * @see updateCurrentLocation()
	 */ 
	bool					mHistoryTeleportInProgress;
	
	/**
	 * Have we received the initial location update?
	 * 
	 * @see updateCurrentLocation()
	 */
	bool					mGotInitialUpdate;
	
	/**
	 * File to store the history to.
	 */
	std::string				mFilename;
	
	/**
	 * Callback to be called when the history gets changed.
	 */
	history_callback_t		mHistoryChangedCallback;
	
	/**
	 * Teleport notification connection.
	 * 
	 * Using this connection we get notified when a teleport finishes
	 * or initial location update occurs.
	 */
	boost::signals::connection	mTeleportFinishedConn;
};

#endif
