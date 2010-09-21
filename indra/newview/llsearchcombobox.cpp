/**
 * @file llsearchcombobox.cpp
 * @brief Search Combobox implementation
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

#include "llviewerprecompiledheaders.h"
#include "llsearchcombobox.h"

#include "llkeyboard.h"
#include "lltrans.h"  // for LLTrans::getString()
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLSearchComboBox> r1("search_combo_box");

class LLSearchHistoryBuilder
{
public:
	LLSearchHistoryBuilder(LLSearchComboBox* combo_box, const std::string& filter);

	virtual void buildSearchHistory();

	virtual ~LLSearchHistoryBuilder(){}

protected:

	virtual bool filterSearchHistory();

	LLSearchComboBox* mComboBox;
	std::string mFilter;
	LLSearchHistory::search_history_list_t mFilteredSearchHistory;
};

LLSearchComboBox::Params::Params()
: search_button("search_button")
, dropdown_button_visible("dropdown_button_visible", false)
{
}

LLSearchComboBox::LLSearchComboBox(const Params&p)
: LLComboBox(p)
{
	S32 btn_top = p.search_button.top_pad + p.search_button.rect.height;
	S32 btn_right = p.search_button.rect.width + p.search_button.left_pad;
	LLRect search_btn_rect(p.search_button.left_pad, btn_top, btn_right, p.search_button.top_pad);

	LLButton::Params button_params(p.search_button);
	button_params.name(std::string("search_btn"));
	button_params.rect(search_btn_rect) ;
	button_params.follows.flags(FOLLOWS_LEFT|FOLLOWS_TOP);
	button_params.tab_stop(false);
	button_params.click_callback.function(boost::bind(&LLSearchComboBox::onSelectionCommit, this));
	mSearchButton = LLUICtrlFactory::create<LLButton>(button_params);
	mTextEntry->addChild(mSearchButton);
	mTextEntry->setPassDelete(TRUE);

	setButtonVisible(p.dropdown_button_visible);
	mTextEntry->setCommitCallback(boost::bind(&LLComboBox::onTextCommit, this, _2));
	mTextEntry->setKeystrokeCallback(boost::bind(&LLComboBox::onTextEntry, this, _1), NULL);
	setCommitCallback(boost::bind(&LLSearchComboBox::onSelectionCommit, this));
	setPrearrangeCallback(boost::bind(&LLSearchComboBox::onSearchPrearrange, this, _2));
	mSearchButton->setCommitCallback(boost::bind(&LLSearchComboBox::onTextCommit, this, _2));
}

void LLSearchComboBox::rebuildSearchHistory(const std::string& filter)
{
	LLSearchHistoryBuilder builder(this, filter);
	builder.buildSearchHistory();
}

void LLSearchComboBox::onSearchPrearrange(const LLSD& data)
{
	std::string filter = data.asString();
	rebuildSearchHistory(filter);

	mList->mouseOverHighlightNthItem(-1); // Clear highlight on the last selected item.
}

void LLSearchComboBox::onTextEntry(LLLineEditor* line_editor)
{
	KEY key = gKeyboard->currentKey();

	if (line_editor->getText().empty())
	{
		prearrangeList(); // resets filter
		hideList();
	}
	// Typing? (moving cursor should not affect showing the list)
	else if (key != KEY_LEFT && key != KEY_RIGHT && key != KEY_HOME && key != KEY_END)
	{
		prearrangeList(line_editor->getText());
		if (mList->getItemCount() != 0)
		{
			showList();
			focusTextEntry();
		}
		else
		{
			// Hide the list if it's empty.
			hideList();
		}
	}
	LLComboBox::onTextEntry(line_editor);
}

void LLSearchComboBox::focusTextEntry()
{
	// We can't use "mTextEntry->setFocus(TRUE)" instead because
	// if the "select_on_focus" parameter is true it places the cursor
	// at the beginning (after selecting text), thus screwing up updateSelection().
	if (mTextEntry)
	{
		gFocusMgr.setKeyboardFocus(mTextEntry);
	}
}

void LLSearchComboBox::hideList()
{
	LLComboBox::hideList();
	if (mTextEntry && hasFocus())
		focusTextEntry();
}

LLSearchComboBox::~LLSearchComboBox()
{
}

void LLSearchComboBox::onSelectionCommit()
{
	std::string search_query = getSimple();
	LLStringUtil::trim(search_query);

	// Order of add() and mTextEntry->setText does matter because add() will select first item 
	// in drop down list and its label will be copied to text box rewriting mTextEntry->setText() call
	if(!search_query.empty())
	{
		remove(search_query);
		add(search_query, ADD_TOP);
	}

	mTextEntry->setText(search_query);
	setControlValue(search_query);
}

BOOL LLSearchComboBox::remove(const std::string& name)
{
	BOOL found = mList->selectItemByLabel(name, FALSE);

	if (found)
	{
		LLScrollListItem* item = mList->getFirstSelected();
		if (item)
		{
			LLComboBox::remove(mList->getItemIndex(item));
		}
	}

	return found;
}

void LLSearchComboBox::clearHistory()
{
	removeall();
	setTextEntry(LLStringUtil::null);
}

BOOL LLSearchComboBox::handleKeyHere(KEY key,MASK mask )
{
	if(mTextEntry->hasFocus() && MASK_NONE == mask && KEY_DOWN == key)
	{
		S32 first = 0;
		S32 size = 0;

		// get entered text (without auto-complete part)
		mTextEntry->getSelectionRange(&first, &size);
		std::string search_query = mTextEntry->getText();
		search_query.erase(first, size);

		onSearchPrearrange(search_query);
	}
	return LLComboBox::handleKeyHere(key, mask);
}

LLSearchHistoryBuilder::LLSearchHistoryBuilder(LLSearchComboBox* combo_box, const std::string& filter)
: mComboBox(combo_box)
, mFilter(filter)
{
}

bool LLSearchHistoryBuilder::filterSearchHistory()
{
	// *TODO: an STL algorithm would look nicer
	mFilteredSearchHistory.clear();

	std::string filter_copy = mFilter;
	LLStringUtil::toLower(filter_copy);

	LLSearchHistory::search_history_list_t history = 
		LLSearchHistory::getInstance()->getSearchHistoryList();

	LLSearchHistory::search_history_list_t::const_iterator it = history.begin();
	for ( ; it != history.end(); ++it)
	{
		std::string search_query = (*it).search_query;
		LLStringUtil::toLower(search_query);

		if (search_query.find(filter_copy) != std::string::npos)
			mFilteredSearchHistory.push_back(*it);
	}

	return mFilteredSearchHistory.size();
}

void LLSearchHistoryBuilder::buildSearchHistory()
{
	mFilteredSearchHistory.clear();

	LLSearchHistory::search_history_list_t filtered_items;
	LLSearchHistory::search_history_list_t* itemsp = NULL;
	LLSearchHistory* sh = LLSearchHistory::getInstance();

	if (mFilter.empty())
	{
		itemsp = &sh->getSearchHistoryList();
	}
	else
	{
		filterSearchHistory();
		itemsp = &mFilteredSearchHistory;
		itemsp->sort();
	}

	mComboBox->removeall();

	LLSearchHistory::search_history_list_t::const_iterator it = itemsp->begin();
	for ( ; it != itemsp->end(); it++)
	{
		LLSearchHistory::LLSearchHistoryItem item = *it;
		mComboBox->add(item.search_query);
	}
}
