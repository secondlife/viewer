/** 
 * @file llfloateravatarpicker.h
 * @brief was llavatarpicker.h
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLFLOATERAVATARPICKER_H
#define LLFLOATERAVATARPICKER_H

#include "llfloater.h"

#include <vector>

class LLAvatarName;
class LLScrollListCtrl;

class LLFloaterAvatarPicker : public LLFloater
{
public:
	typedef boost::signals2::signal<bool(const uuid_vec_t&), boost_boolean_combiner> validate_signal_t;
	typedef validate_signal_t::slot_type validate_callback_t;

	// The callback function will be called with an avatar name and UUID.
	typedef boost::function<void (const uuid_vec_t&, const std::vector<LLAvatarName>&)> select_callback_t;
	// Call this to select an avatar.	
	static LLFloaterAvatarPicker* show(select_callback_t callback, 
									   BOOL allow_multiple = FALSE,
									   BOOL closeOnSelect = FALSE);

	LLFloaterAvatarPicker(const LLSD& key);
	virtual ~LLFloaterAvatarPicker();
	
	virtual	BOOL postBuild();

	void setOkBtnEnableCb(validate_callback_t cb);

	static void processAvatarPickerReply(class LLMessageSystem* msg, void**);
	void processResponse(const LLUUID& query_id, const LLSD& content);

	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg);

	void openFriendsTab();

private:
	void editKeystroke(class LLLineEditor* caller, void* user_data);

	void onBtnFind();
	void onBtnSelect();
	void onBtnRefresh();
	void onRangeAdjust();
	void onBtnClose();
	void onList();
	void onTabChanged();
	bool isSelectBtnEnabled();

	void populateNearMe();
	void populateFriend();
	BOOL visibleItemsSelected() const; // Returns true if any items in the current tab are selected.

	void find();
	void setAllowMultiple(BOOL allow_multiple);
	LLScrollListCtrl* getActiveList();

	virtual void draw();
	virtual BOOL handleKeyHere(KEY key, MASK mask);

	LLUUID				mQueryID;
	int				mNumResultsReturned;
	BOOL				mNearMeListComplete;
	BOOL				mCloseOnSelect;

	validate_signal_t mOkButtonValidateSignal;
	select_callback_t mSelectionCallback;
};

#endif
