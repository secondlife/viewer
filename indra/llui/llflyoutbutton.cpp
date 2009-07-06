/** 
 * @file llflyoutbutton.cpp
 * @brief LLFlyoutButton base class
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

#include "linden_common.h"

// file includes
#include "llflyoutbutton.h"

//static LLDefaultWidgetRegistry::Register<LLFlyoutButton> r2("flyout_button");

const S32 FLYOUT_BUTTON_ARROW_WIDTH = 24;

LLFlyoutButton::LLFlyoutButton(const Params& p)
:	LLComboBox(p),
	mToggleState(FALSE),
	mActionButton(NULL)
{
	// Always use text box 
	// Text label button
	LLButton::Params bp(p.action_button);
	bp.name(p.label);
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

void LLFlyoutButton::setEnabled(BOOL enabled)
{
	mActionButton->setEnabled(enabled);
	LLComboBox::setEnabled(enabled);
}


void LLFlyoutButton::setToggleState(BOOL state)
{
	mToggleState = state;
}


