/** 
 * @file llfloaterautoreplacesettings.h
 * @brief Auto Replace List floater
 * @copyright Copyright (c) 2011 LordGregGreg Back
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#ifndef LLFLOATERAUTOREPLACESETTINGS_H
#define LLFLOATERAUTOREPLACESETTINGS_H

#include "llfloater.h"
#include "llmediactrl.h"
#include "llscrolllistctrl.h"
#include "lllineeditor.h"

#include "llviewerinventory.h"
#include <boost/bind.hpp>
#include "llautoreplace.h"

class LLFloaterAutoReplaceSettings : public LLFloater
{
public:
	LLFloaterAutoReplaceSettings(const LLSD& key);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);

	void setData(void * data);

private:

	/** @{ @name Local Copies of Settings
	 * These are populated in the postBuild method with the values
	 * current when the floater is instantiated, and then either
	 * discarded when Cancel is pressed, or copied back to the active
	 * settings if Ok is pressed.
	 */
	bool mEnabled; ///< the global preference for AutoReplace 
	LLAutoReplaceSettings mSettings; ///< settings being modified
	/** @} */
	
	/// convenience variable - the name of the currently selected list (if any)
	std::string       mSelectedListName;
	/// the scrolling list of list names (one column, no headings, order manually controlled)
	LLScrollListCtrl* mListNames;
	/// the scroling list of keyword->replacement pairs
	LLScrollListCtrl* mReplacementsList;

	/// the keyword for the entry editing pane
	LLLineEditor*     mKeyword;
	/// saved keyword value
	std::string       mPreviousKeyword;
	/// the replacement for the entry editing pane
	LLLineEditor*     mReplacement;
	
	/// callback for when the feature enable/disable checkbox changes
	void onAutoReplaceToggled();
	/// callback for when an entry in the list of list names is selected
	void onSelectList();

	void onImportList();
	void onExportList();
	void onNewList();
	void onDeleteList();

	void onListUp();
	void onListDown();

	void onSelectEntry();
	void onAddEntry();
	void onDeleteEntry();
	void onSaveEntry();

	void onSaveChanges();
	void onCancel();

	/// updates the contents of the mListNames
	void updateListNames();
	/// updates the controls associated with mListNames (depends on whether a name is selected or not)
	void updateListNamesControls();
	/// updates the contents of the mReplacementsList
	void updateReplacementsList();
	/// enables the components that should only be active when a keyword is selected
	void enableReplacementEntry();
	/// disables the components that should only be active when a keyword is selected
	void disableReplacementEntry();

	/// called from the AddAutoReplaceList notification dialog
	bool callbackNewListName(const LLSD& notification, const LLSD& response);
	/// called from the RenameAutoReplaceList notification dialog
	bool callbackListNameConflict(const LLSD& notification, const LLSD& response);

	bool selectedListIsFirst();
	bool selectedListIsLast();

	void cleanUp();
};

#endif  // LLFLOATERAUTOREPLACESETTINGS_H
