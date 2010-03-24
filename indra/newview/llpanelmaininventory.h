/** 
 * @file llpanelmaininventory.h
 * @brief llpanelmaininventory.h
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELMAININVENTORY_H
#define LL_LLPANELMAININVENTORY_H

#include "llpanel.h"
#include "llinventoryobserver.h"

#include "llfolderview.h"

class LLFolderViewItem;
class LLInventoryPanel;
class LLSaveFolderState;
class LLFilterEditor;
class LLTabContainer;
class LLFloaterInventoryFinder;
class LLMenuGL;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLPanelMainInventory
//
// This is a panel used to view and control an agent's inventory,
// including all the fixin's (e.g. AllItems/RecentItems tabs, filter floaters).
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLPanelMainInventory : public LLPanel, LLInventoryObserver
{
public:
	friend class LLFloaterInventoryFinder;

	LLPanelMainInventory();
	~LLPanelMainInventory();

	BOOL postBuild();

	virtual BOOL handleKeyHere(KEY key, MASK mask);

	// Inherited functionality
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									   EDragAndDropType cargo_type,
									   void* cargo_data,
									   EAcceptance* accept,
									   std::string& tooltip_msg);
	/*virtual*/ void changed(U32);
	/*virtual*/ void draw();

	LLInventoryPanel* getPanel() { return mActivePanel; }
	LLInventoryPanel* getActivePanel() { return mActivePanel; }
	const LLInventoryPanel* getActivePanel() const { return mActivePanel; }

	const std::string& getFilterText() const { return mFilterText; }
	
	void setSelectCallback(const LLFolderView::signal_t::slot_type& cb);

protected:
	//
	// Misc functions
	//
	void setFilterTextFromFilter();
	void startSearch();
	
	void toggleFindOptions();
	void onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, BOOL user_action);

	static BOOL filtersVisible(void* user_data);
	void onClearSearch();
	static void onFoldersByName(void *user_data);
	static BOOL checkFoldersByName(void *user_data);
	void onFilterEdit(const std::string& search_string );
	static BOOL incrementalFind(LLFolderViewItem* first_item, const char *find_text, BOOL backward);
	void onFilterSelected();

	const std::string getFilterSubString();
	void setFilterSubString(const std::string& string);
	
	// menu callbacks
	void doToSelected(const LLSD& userdata);
	void closeAllFolders();
	void newWindow();
	void doCreate(const LLSD& userdata);
	void resetFilters();
	void setSortBy(const LLSD& userdata);
	void saveTexture(const LLSD& userdata);
	bool isSaveTextureEnabled(const LLSD& userdata);
	void updateItemcountText();

private:
	LLFloaterInventoryFinder* getFinder();

	LLFilterEditor*				mFilterEditor;
	LLTabContainer*				mFilterTabs;
	LLHandle<LLFloater>			mFinderHandle;
	LLInventoryPanel*			mActivePanel;
	LLSaveFolderState*			mSavedFolderState;
	std::string					mFilterText;
	std::string					mFilterSubString;


	//////////////////////////////////////////////////////////////////////////////////
	// List Commands                                                                //
protected:
	void initListCommandsHandlers();
	void updateListCommands();
	void onGearButtonClick();
	void onAddButtonClick();
	void showActionMenu(LLMenuGL* menu, std::string spawning_view_name);
	void onTrashButtonClick();
	void onClipboardAction(const LLSD& userdata);
	BOOL isActionEnabled(const LLSD& command_name);
	void onCustomAction(const LLSD& command_name);
	bool handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept);
	/**
	 * Set upload cost in "Upload" sub menu.
	 */
	void setUploadCostIfNeeded();
private:
	LLPanel*					mListCommands;
	LLMenuGL*					mMenuGearDefault;
	LLMenuGL*					mMenuAdd;

	bool						mNeedUploadCost;
	// List Commands                                                              //
	////////////////////////////////////////////////////////////////////////////////
};

#endif // LL_LLPANELMAININVENTORY_H



