/** 
 * @file llflyoutbutton.cpp
 * @brief LLFlyoutButton base class
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

#include "linden_common.h"

// file includes
#include "llflyoutbutton.h"

//static LLDefaultChildRegistry::Register<LLFlyoutButton> r2("flyout_button");

const S32 FLYOUT_BUTTON_ARROW_WIDTH = 24;

LLFlyoutButton::LLFlyoutButton(const Params& p)
:   LLComboBox(p),
    mToggleState(FALSE),
    mActionButton(NULL)
{
    // Always use text box 
    // Text label button
    LLButton::Params bp(p.action_button);
    bp.name(p.label);
    bp.label(p.label);
    bp.rect.left(0).bottom(0).width(getRect().getWidth() - FLYOUT_BUTTON_ARROW_WIDTH).height(getRect().getHeight());
    bp.click_callback.function(boost::bind(&LLFlyoutButton::onActionButtonClick, this, _2));
    bp.follows.flags(FOLLOWS_ALL);

    mActionButton = LLUICtrlFactory::create<LLButton>(bp);
    addChild(mActionButton);
}

void LLFlyoutButton::onActionButtonClick(const LLSD& data)
{
    // remember last list selection?
    mList->deselect();
    onCommit();
}

void LLFlyoutButton::draw()
{
    mActionButton->setToggleState(mToggleState);
    mButton->setToggleState(mToggleState);

    //FIXME: this should be an attribute of comboboxes, whether they have a distinct label or
    // the label reflects the last selected item, for now we have to manually remove the label
    setLabel(LLStringUtil::null);
    LLComboBox::draw(); 
}

void LLFlyoutButton::setToggleState(BOOL state)
{
    mToggleState = state;
}


