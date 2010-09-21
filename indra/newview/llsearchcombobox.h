/** 
 * @file llsearchcombobox.h
 * @brief LLSearchComboBox class definition
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
