/** 
 * @file llpanelblockedlist.h
 * @brief Container for blocked Residents & Objects list
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELBLOCKEDLIST_H
#define LL_LLPANELBLOCKEDLIST_H

#include "llpanel.h"
#include "llmutelist.h"
#include "llfloater.h"
// #include <vector>

// class LLButton;
// class LLLineEditor;
// class LLMessageSystem;
// class LLUUID;
 class LLScrollListCtrl;

class LLPanelBlockedList
	:	public LLPanel, public LLMuteListObserver
{
public:
	LLPanelBlockedList();
	~LLPanelBlockedList();

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

	// LLMuteListObserver callback interface implementation.
	/* virtual */ void onChange() {	refreshBlockedList();}
	
private:
	void refreshBlockedList();
	void updateButtons();

	// UI callbacks
	void onBackBtnClick();
	void onRemoveBtnClick();
	void onPickBtnClick();
	void onBlockByNameClick();

	void callbackBlockPicked(const std::vector<std::string>& names, const std::vector<LLUUID>& ids);
	static void callbackBlockByName(const std::string& text);

private:
	LLScrollListCtrl* mBlockedList;
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
