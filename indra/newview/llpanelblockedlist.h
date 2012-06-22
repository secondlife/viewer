/** 
 * @file llpanelblockedlist.h
 * @brief Container for blocked Residents & Objects list
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLPANELBLOCKEDLIST_H
#define LL_LLPANELBLOCKEDLIST_H

#include "llpanel.h"
#include "llmutelist.h"
#include "llfloater.h"

class LLAvatarName;
class LLBlockList;

class LLPanelBlockedList : public LLPanel
{
public:
	LLPanelBlockedList();
	~LLPanelBlockedList(){};

	virtual BOOL postBuild();
	virtual void draw();
	virtual void onOpen(const LLSD& key);
	
	void selectBlocked(const LLUUID& id);

	/**
	 *	Shows current Panel in side tray and select passed blocked item.
	 * 
	 *	@param idToSelect - LLUUID of blocked Resident or Object to be selected. 
	 *			If it is LLUUID::null, nothing will be selected.
	 */
	static void showPanelAndSelect(const LLUUID& idToSelect);
	
private:

	typedef enum e_sort_oder{
		E_SORT_BY_NAME = 0,
		E_SORT_BY_TYPE = 1,
	} ESortOrder;

	void updateButtons();

	// UI callbacks
	void unblockItem();
	void blockResidentByName();
	void blockObjectByName();
	void onFilterEdit(const std::string& search_string);

	// List commnads
	void onCustomAction(const LLSD& userdata);
	BOOL isActionChecked(const LLSD& userdata);

	void callbackBlockPicked(const uuid_vec_t& ids, const std::vector<LLAvatarName> names);
	static void callbackBlockByName(const std::string& text);

private:
	LLBlockList* mBlockedList;
};

//-----------------------------------------------------------------------------
// LLFloaterGetBlockedObjectName()
//-----------------------------------------------------------------------------
// Class for handling mute object by name floater.
class LLFloaterGetBlockedObjectName : public LLFloater
{
	friend class LLFloaterReg;
public:
	typedef boost::function<void (const std::string&)> get_object_name_callback_t;

	virtual BOOL postBuild();

	virtual BOOL handleKeyHere(KEY key, MASK mask);

	static LLFloaterGetBlockedObjectName* show(get_object_name_callback_t callback);

private:
	LLFloaterGetBlockedObjectName(const LLSD& key);
	virtual ~LLFloaterGetBlockedObjectName();

	// UI Callbacks
	void applyBlocking();
	void cancelBlocking();

	get_object_name_callback_t mGetObjectNameCallback;
};


#endif // LL_LLPANELBLOCKEDLIST_H
