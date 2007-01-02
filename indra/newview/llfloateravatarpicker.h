/** 
 * @file llfloateravatarpicker.h
 * @brief was llavatarpicker.h
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLFLOATERAVATARPICKER_H
#define LLFLOATERAVATARPICKER_H

#include "llfloater.h"

#include <vector>

class LLUICtrl;
class LLTextBox;
class LLLineEditor;
class LLButton;
class LLScrollListCtrl;
class LLMessageSystem;
class LLInventoryPanel;
class LLFolderViewItem;

class LLFloaterAvatarPicker : public LLFloater
{
public:
	// Call this to select an avatar.
	// The callback function will be called with an avatar name and UUID.
	typedef void(*callback_t)(const std::vector<std::string>&, const std::vector<LLUUID>&, void*);
	static LLFloaterAvatarPicker* show(callback_t callback, 
									   void* userdata,
									   BOOL allow_multiple = FALSE,
									   BOOL closeOnSelect = FALSE);
	virtual	BOOL postBuild();

	static void processAvatarPickerReply(LLMessageSystem* msg, void**);
	static void editKeystroke(LLLineEditor* caller, void* user_data);

protected:
	static void* createInventoryPanel(void* userdata);

	static void onBtnFind(void* userdata);
	static void onBtnAdd(void* userdata);
	static void onBtnClose(void* userdata);
	static void onList(LLUICtrl* ctrl, void* userdata);
	static void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data);

	void find();
	void setAllowMultiple(BOOL allow_multiple);

	virtual BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);

protected:
	LLScrollListCtrl*	mListNames;
	LLInventoryPanel*	mInventoryPanel;
	
	std::vector<LLUUID>				mAvatarIDs;
	std::vector<std::string>		mAvatarNames;
	BOOL				mAllowMultiple;
	LLUUID				mQueryID;
	BOOL				mResultsReturned;
	BOOL				mCloseOnSelect;

	void (*mCallback)(const std::vector<std::string>& name, const std::vector<LLUUID>& id, void* userdata);
	void* mCallbackUserdata;

protected:
	static LLFloaterAvatarPicker* sInstance;

protected:
	// do not call these directly
	LLFloaterAvatarPicker();
	virtual ~LLFloaterAvatarPicker();
};


#endif
