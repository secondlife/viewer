/** 
 * @file llsearchcombobox.h
 * @brief LLSearchComboBox class definition
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

#ifndef LL_LLSEARCHCOMBOBOX_H
#define LL_LLSEARCHCOMBOBOX_H

#include "llcombobox.h"
#include "llsearchhistory.h"

/**
 * Search control with text box for search queries and a drop down list
 * with recent queries. Supports text auto-complete and filtering of drop down list
 * according to typed text.
 */
class LLSearchComboBox : public LLComboBox
{
public:

	struct Params :	public LLInitParam::Block<Params, LLComboBox::Params>
	{
		Optional<LLButton::Params> search_button;
		Optional<bool> dropdown_button_visible;

		Params();
	};

	/**
	 * Removes an entry from combo box, case insensitive
	 */
	BOOL remove(const std::string& name);

	/**
	 * Clears search history
	 */
	void clearHistory();

	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);

	~LLSearchComboBox();

protected:

	LLSearchComboBox(const Params&p);
	friend class LLUICtrlFactory;

	/**
	 * Handles typing in text box
	 */
	void onTextEntry(LLLineEditor* line_editor);

	/**
	 * Hides drop down list and focuses text box
	 */
	void hideList();

	/**
	 * Rebuilds search history, case insensitive
	 * If filter is an empty string - whole history will be added to combo box
	 * if filter is valid string - only matching entries will be added
	 */
	virtual void rebuildSearchHistory(const std::string& filter);

	/**
	 * Callback for prearrange event
	 */
	void onSearchPrearrange(const LLSD& data);

	/**
	 * Callback for text box or combo box commit
	 */
	void onSelectionCommit();

	/**
	 * Sets focus to text box
	 */
	void focusTextEntry();

	LLButton* mSearchButton;
};

#endif //LL_LLSEARCHCOMBOBOX_H
