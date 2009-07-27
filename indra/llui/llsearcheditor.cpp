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
:	LLUICtrl(p)
{
	const S32 fudge = 2;
	S32 btn_height = getRect().getHeight() - (fudge * 2);

	LLLineEditor::Params line_editor_params(p);
	line_editor_params.name("filter edit box");
	line_editor_params.rect(getLocalRect());
	line_editor_params.follows.flags(FOLLOWS_ALL);
	line_editor_params.text_pad_left(btn_height + fudge);
	line_editor_params.commit_callback.function(boost::bind(&LLUICtrl::onCommit, this));

	mSearchEditor = LLUICtrlFactory::create<LLLineEditor>(line_editor_params);
	addChild(mSearchEditor);

	LLRect search_btn_rect(fudge, fudge + btn_height, fudge + btn_height, fudge);
	LLButton::Params button_params(p.search_button);
	button_params.name(std::string("clear filter"));
	button_params.rect(search_btn_rect) ;
	button_params.follows.flags(FOLLOWS_RIGHT|FOLLOWS_TOP);
	button_params.tab_stop(false);
	button_params.click_callback.function(boost::bind(&LLUICtrl::onCommit, this));

	mSearchButton = LLUICtrlFactory::create<LLButton>(button_params);
	mSearchEditor->addChild(mSearchButton);
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
void LLSearchEditor::clear()
{
	if (mSearchEditor)
	{
		mSearchEditor->clear();
	}
}
