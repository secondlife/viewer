/** 
 * @file llsearcheditor.h
 * @brief Text editor widget that represents a search operation
 *
 * Features: 
 *		Text entry of a single line (text, delete, left and right arrow, insert, return).
 *		Callbacks either on every keystroke or just on the return key.
 *		Focus (allow multiple text entry widgets)
 *		Clipboard (cut, copy, and paste)
 *		Horizontal scrolling to allow strings longer than widget size allows 
 *		Pre-validation (limit which keys can be used)
 *		Optional line history so previous entries can be recalled by CTRL UP/DOWN
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

#ifndef LL_LLSEARCHEDITOR_H
#define LL_LLSEARCHEDITOR_H

#include "lllineeditor.h"
#include "llbutton.h"

#include <boost/function.hpp>

/*
 * @brief A line editor with a button to clear it and a callback to call on every edit event.
 */
class LLSearchEditor : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLLineEditor::Params>
	{
		Optional<boost::function<void(const std::string&, void*)> > search_callback;
		
		Optional<LLButton::Params> clear_search_button;

		Params()
		: clear_search_button("clear_search_button")
		{
			name = "search_editor";
		}
	};

protected:
	LLSearchEditor(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLSearchEditor() {}

	/*virtual*/ void	draw();

	void setText(const LLStringExplicit &new_text) { mSearchEdit->setText(new_text); }

	typedef boost::function<void (const std::string& search_string)> search_callback_t;
	void setSearchCallback(search_callback_t cb) { mSearchCallback = cb; }

	// LLUICtrl interface
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	virtual void	clear();

private:
	void onSearchEdit(LLLineEditor* caller );
	void onClearSearch(const LLSD& data);

	LLLineEditor* mSearchEdit;
	LLButton* mClearSearchButton;
	search_callback_t mSearchCallback;
};

#endif  // LL_LLSEARCHEDITOR_H
