/** 
 * @file llsearcheditor.cpp
 * @brief LLSearchEditor implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// Text editor widget to let users enter a single line.

#include "linden_common.h"
 
#include "llsearcheditor.h"
#include "llkeyboard.h"

LLSearchEditor::LLSearchEditor(const LLSearchEditor::Params& p)
:   LLUICtrl(p),
    mSearchButton(NULL),
    mClearButton(NULL),
    mEditorImage(p.background_image),
    mEditorImageFocused(p.background_image_focused),
    mEditorSearchImage(p.background_image_highlight),
    mHighlightTextField(p.highlight_text_field)
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
    line_editor_params.background_image(p.background_image);
    line_editor_params.background_image_focused(p.background_image_focused);
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

    if (mHighlightTextField)
    {   
        if (!mSearchEditor->getWText().empty())
        {
            mSearchEditor->setBgImage(mEditorSearchImage);
            mSearchEditor->setBgImageFocused(mEditorSearchImage);
        }
        else
        {
            mSearchEditor->setBgImage(mEditorImage);
            mSearchEditor->setBgImageFocused(mEditorImageFocused);
        }
    }

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

    KEY key = gKeyboard->currentKey();
    if (key == KEY_LEFT ||
        key == KEY_RIGHT)
    {
            return;
    }

    if (mTextChangedCallback)
    {
        mTextChangedCallback(this, getValue());
    }
}
