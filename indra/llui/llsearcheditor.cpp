/** 
 * @file lllineeditor.cpp
 * @brief LLLineEditor base class
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

// Text editor widget to let users enter a single line.

#include "linden_common.h"
 
#include "llsearcheditor.h"

//static LLDefaultChildRegistry::Register<LLSearchEditor> r2("search_editor");

LLSearchEditor::LLSearchEditor(const LLSearchEditor::Params& p)
:	LLUICtrl(p)
{
	LLLineEditor::Params line_editor_p(p);
	line_editor_p.name("search edit box");
	line_editor_p.rect(getLocalRect());
	line_editor_p.follows.flags(FOLLOWS_ALL);
	line_editor_p.text_pad_right(getRect().getHeight());
	line_editor_p.keystroke_callback(boost::bind(&LLSearchEditor::onSearchEdit, this, _1));

	mSearchEdit = LLUICtrlFactory::create<LLLineEditor>(line_editor_p);
	addChild(mSearchEdit);

	S32 btn_width = getRect().getHeight(); // button is square, and as tall as search editor
	LLRect clear_btn_rect(getRect().getWidth() - btn_width, getRect().getHeight(), getRect().getWidth(), 0);
	LLButton::Params button_params(p.clear_search_button);
	button_params.name(std::string("clear search"));
	button_params.rect(clear_btn_rect) ;
	button_params.follows.flags(FOLLOWS_RIGHT|FOLLOWS_TOP);
	button_params.tab_stop(false);
	button_params.click_callback.function(boost::bind(&LLSearchEditor::onClearSearch, this, _2));

	mClearSearchButton = LLUICtrlFactory::create<LLButton>(button_params);
	mSearchEdit->addChild(mClearSearchButton);
}

//virtual
void LLSearchEditor::setValue(const LLSD& value )
{
	mSearchEdit->setValue(value);
}

//virtual
LLSD LLSearchEditor::getValue() const
{
	return mSearchEdit->getValue();
}

//virtual
BOOL LLSearchEditor::setTextArg( const std::string& key, const LLStringExplicit& text )
{
	return mSearchEdit->setTextArg(key, text);
}

//virtual
BOOL LLSearchEditor::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	return mSearchEdit->setLabelArg(key, text);
}

//virtual
void LLSearchEditor::clear()
{
	if (mSearchEdit)
	{
		mSearchEdit->clear();
	}
}

void LLSearchEditor::draw()
{
	mClearSearchButton->setVisible(!mSearchEdit->getWText().empty());

	LLUICtrl::draw();
}


void LLSearchEditor::onSearchEdit(LLLineEditor* caller )
{
	if (mSearchCallback)
	{
		mSearchCallback(caller->getText());
	}
}

void LLSearchEditor::onClearSearch(const LLSD& data)
{
	setText(LLStringUtil::null);
	if (mSearchCallback)
	{
		mSearchCallback(LLStringUtil::null);
	}
}

