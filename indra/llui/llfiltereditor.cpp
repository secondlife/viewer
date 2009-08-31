/** 
 * @file llfiltereditor.cpp
 * @brief LLFilterEditor implementation
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
 
#include "llfiltereditor.h"

LLFilterEditor::LLFilterEditor(const LLFilterEditor::Params& p)
:	LLUICtrl(p)
{
	LLLineEditor::Params line_editor_p(p);
	line_editor_p.name("filter edit box");
	line_editor_p.rect(getLocalRect());
	line_editor_p.follows.flags(FOLLOWS_ALL);
	line_editor_p.text_pad_right(getRect().getHeight());
	line_editor_p.keystroke_callback(boost::bind(&LLUICtrl::onCommit, this));

	mFilterEditor = LLUICtrlFactory::create<LLLineEditor>(line_editor_p);
	addChild(mFilterEditor);

	S32 btn_width = getRect().getHeight(); // button is square, and as tall as search editor
	LLRect clear_btn_rect(getRect().getWidth() - btn_width, getRect().getHeight(), getRect().getWidth(), 0);
	LLButton::Params button_params(p.clear_filter_button);
	button_params.name(std::string("clear filter"));
	button_params.rect(clear_btn_rect) ;
	button_params.follows.flags(FOLLOWS_RIGHT|FOLLOWS_TOP);
	button_params.tab_stop(false);
	button_params.click_callback.function(boost::bind(&LLFilterEditor::onClearFilter, this, _2));

	mClearFilterButton = LLUICtrlFactory::create<LLButton>(button_params);
	mFilterEditor->addChild(mClearFilterButton);
}

//virtual
void LLFilterEditor::setValue(const LLSD& value )
{
	mFilterEditor->setValue(value);
}

//virtual
LLSD LLFilterEditor::getValue() const
{
	return mFilterEditor->getValue();
}

//virtual
BOOL LLFilterEditor::setTextArg( const std::string& key, const LLStringExplicit& text )
{
	return mFilterEditor->setTextArg(key, text);
}

//virtual
BOOL LLFilterEditor::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	return mFilterEditor->setLabelArg(key, text);
}

//virtual
void LLFilterEditor::setLabel( const LLStringExplicit &new_label )
{
	mFilterEditor->setLabel(new_label);
}

//virtual
void LLFilterEditor::clear()
{
	if (mFilterEditor)
	{
		mFilterEditor->clear();
	}
}

void LLFilterEditor::draw()
{
	mClearFilterButton->setVisible(!mFilterEditor->getWText().empty());

	LLUICtrl::draw();
}

void LLFilterEditor::onClearFilter(const LLSD& data)
{
	setText(LLStringUtil::null);
	onCommit();
}

