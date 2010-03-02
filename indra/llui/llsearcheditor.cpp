/** 
 * @file llsearcheditor.cpp
 * @brief LLSearchEditor implementation
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

LLSearchEditor::LLSearchEditor(const LLSearchEditor::Params& p)
:	LLUICtrl(p),
	mSearchButton(NULL),
	mClearButton(NULL)
{
	S32 srch_btn_top = p.search_button.top_pad + p.search_button.rect.height;
	S32 srch_btn_right = p.search_button.rect.width + p.search_button.left_pad;
	LLRect srch_btn_rect(p.search_button.left_pad, srch_btn_top, srch_btn_right, p.search_button.top_pad);

	S32 clr_btn_top = p.clear_button.rect.bottom + p.clear_button.rect.height;
	S32 clr_btn_right = getRect().getWidth() - p.clear_button.pad_right;
	S32 clr_btn_left = clr_btn_right - p.clear_button.rect.width;
	LLRect clear_btn_rect(clr_btn_left, clr_btn_top, clr_btn_right, p.clear_button.rect.bottom);

	S32 text_pad_left = p.text_pad_left;
	S32 text_pad_right = p.text_pad_right;

	if (p.search_button_visible)
		text_pad_left += srch_btn_rect.getWidth();

	if (p.clear_button_visible)
		text_pad_right = getRect().getWidth() - clr_btn_left + p.clear_button.pad_left;

	// Set up line editor.
	LLLineEditor::Params line_editor_params(p);
	line_editor_params.name("filter edit box");
	line_editor_params.rect(getLocalRect());
	line_editor_params.follows.flags(FOLLOWS_ALL);
	line_editor_params.text_pad_left(text_pad_left);
	line_editor_params.text_pad_right(text_pad_right);
	line_editor_params.revert_on_esc(false);
	line_editor_params.commit_callback.function(boost::bind(&LLUICtrl::onCommit, this));
	line_editor_params.keystroke_callback(boost::bind(&LLSearchEditor::handleKeystroke, this));

	mSearchEditor = LLUICtrlFactory::create<LLLineEditor>(line_editor_params);
	mSearchEditor->setPassDelete(TRUE);
	addChild(mSearchEditor);

	if (p.search_button_visible)
	{
		// Set up search button.
		LLButton::Params srch_btn_params(p.search_button);
		srch_btn_params.name(std::string("search button"));
		srch_btn_params.rect(srch_btn_rect) ;
		srch_btn_params.follows.flags(FOLLOWS_LEFT|FOLLOWS_TOP);
		srch_btn_params.tab_stop(false);
		srch_btn_params.click_callback.function(boost::bind(&LLUICtrl::onCommit, this));
	
		mSearchButton = LLUICtrlFactory::create<LLButton>(srch_btn_params);
		mSearchEditor->addChild(mSearchButton);
	}

	if (p.clear_button_visible)
	{
		// Set up clear button.
		LLButton::Params clr_btn_params(p.clear_button);
		clr_btn_params.name(std::string("clear button"));
		clr_btn_params.rect(clear_btn_rect) ;
		clr_btn_params.follows.flags(FOLLOWS_RIGHT|FOLLOWS_TOP);
		clr_btn_params.tab_stop(false);
		clr_btn_params.click_callback.function(boost::bind(&LLSearchEditor::onClearButtonClick, this, _2));

		mClearButton = LLUICtrlFactory::create<LLButton>(clr_btn_params);
		mSearchEditor->addChild(mClearButton);
	}
}

//virtual
void LLSearchEditor::draw()
{
	if (mClearButton)
		mClearButton->setVisible(!mSearchEditor->getWText().empty());

	LLUICtrl::draw();
}

//virtual
void LLSearchEditor::setValue(const LLSD& value )
{
	mSearchEditor->setValue(value);
}

//virtual
LLSD LLSearchEditor::getValue() const
{
	return mSearchEditor->getValue();
}

//virtual
BOOL LLSearchEditor::setTextArg( const std::string& key, const LLStringExplicit& text )
{
	return mSearchEditor->setTextArg(key, text);
}

//virtual
BOOL LLSearchEditor::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	return mSearchEditor->setLabelArg(key, text);
}

//virtual
void LLSearchEditor::setLabel( const LLStringExplicit &new_label )
{
	mSearchEditor->setLabel(new_label);
}

//virtual
void LLSearchEditor::clear()
{
	if (mSearchEditor)
	{
		mSearchEditor->clear();
	}
}

//virtual
void LLSearchEditor::setFocus( BOOL b )
{
	if (mSearchEditor)
	{
		mSearchEditor->setFocus(b);
	}
}

void LLSearchEditor::onClearButtonClick(const LLSD& data)
{
	setText(LLStringUtil::null);
	mSearchEditor->onCommit(); // force keystroke callback
}

void LLSearchEditor::handleKeystroke()
{
	if (mKeystrokeCallback)
	{
		mKeystrokeCallback(this, getValue());
	}
}
