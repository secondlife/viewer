/**
 * @file llsplitbutton.cpp
 * @brief LLSplitButton base class
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// A control that consolidates several buttons as options

#include "llviewerprecompiledheaders.h"

#include "llsplitbutton.h"

#include "llinitparam.h"
#include "llpanel.h"
#include "llfocusmgr.h"
#include "llviewerwindow.h"
#include "llrootview.h"


S32 BUTTON_PAD = 2; //pad between buttons on an items panel


static LLDefaultChildRegistry::Register<LLSplitButton> split_button("split_button");

void LLSplitButton::ArrowPositionValues::declareValues()
{
    declare("left", LEFT);
    declare("right", RIGHT);
}

LLSplitButton::ItemParams::ItemParams()
{
}

LLSplitButton::Params::Params()
:   arrow_position("arrow_position", LEFT),
    items("item"),
    arrow_button("arrow_button"),
    items_panel("items_panel")
{
}


void LLSplitButton::onFocusLost()
{
    hideButtons();
    LLUICtrl::onFocusLost();
}

void LLSplitButton::setFocus(bool b)
{
    LLUICtrl::setFocus(b);

    if (b)
    {
        if (mItemsPanel && mItemsPanel->getVisible())
        {
            mItemsPanel->setFocus(true);
        }
    }
}

void LLSplitButton::setEnabled(bool enabled)
{
    LLView::setEnabled(enabled);
    mArrowBtn->setEnabled(enabled);
}


void LLSplitButton::onArrowBtnDown()
{
    if (!mItemsPanel->getVisible())
    {
        showButtons();

        setFocus(true);

        if (mArrowBtn->hasMouseCapture() || mShownItem->hasMouseCapture())
        {
            gFocusMgr.setMouseCapture(this);
        }
    }
    else
    {
        hideButtons();
    }
}

void LLSplitButton::onHeldDownShownButton()
{
    if (!mItemsPanel->getVisible()) onArrowBtnDown();
}

void LLSplitButton::onItemSelected(LLUICtrl* ctrl)
{
    if (!ctrl) return;

    hideButtons();

    // call the callback if it exists
    if(!mSelectionCallback.empty())
    {
        mSelectionCallback(this, ctrl->getName());
    }

    gFocusMgr.setKeyboardFocus(NULL);
}

bool LLSplitButton::handleMouseUp(S32 x, S32 y, MASK mask)
{
    gFocusMgr.setMouseCapture(NULL);

    if (mShownItem->parentPointInView(x, y))
    {
        onItemSelected(mShownItem);
        return true;
    }

    for (std::list<LLButton*>::const_iterator it = mHidenItems.begin(); it != mHidenItems.end(); ++it)
    {
        LLButton* item = *it;

        S32 panel_x = 0;
        S32 panel_y = 0;
        localPointToOtherView(x, y, &panel_x, &panel_y, mItemsPanel);

        if (item->parentPointInView(panel_x, panel_y))
        {
            onItemSelected(item);
            return true;
        }
    }
    return true;
}

void LLSplitButton::showButtons()
{
    mItemsPanel->setOrigin(0, getRect().getHeight());

    // register ourselves as a "top" control
    // effectively putting us into a special draw layer
    gViewerWindow->addPopup(this);

    mItemsPanel->setFocus(true);

    //push arrow button down and show the item buttons
    mArrowBtn->setToggleState(true);
    mItemsPanel->setVisible(true);

    setUseBoundingRect(true);
}

void LLSplitButton::hideButtons()
{
    mItemsPanel->setVisible(false);
    mArrowBtn->setToggleState(false);

    setUseBoundingRect(false);
    gViewerWindow->removePopup(this);
}


// protected/private

LLSplitButton::LLSplitButton(const LLSplitButton::Params& p)
:   LLUICtrl(p),
    mArrowBtn(NULL),
    mShownItem(NULL),
    mItemsPanel(NULL),
    mArrowPosition(p.arrow_position)
{
    LLRect rc(p.rect);

    LLButton::Params arrow_params = p.arrow_button;
    S32 arrow_width = p.arrow_button.rect.width;

    //Default arrow rect values for LEFT arrow position
    S32 arrow_left = 0;
    S32 arrow_right = arrow_width;
    S32 btn_left = arrow_width;
    S32 btn_right = rc.getWidth();

    if (mArrowPosition == RIGHT)
    {
        arrow_left = rc.getWidth()- arrow_width;
        arrow_right = rc.getWidth();
        btn_left = 0;
        btn_right = arrow_left;
    }

    arrow_params.rect(LLRect(arrow_left, rc.getHeight(), arrow_right, 0));
    arrow_params.label("");
    arrow_params.mouse_down_callback.function(boost::bind(&LLSplitButton::onArrowBtnDown, this));
    mArrowBtn = LLUICtrlFactory::create<LLButton>(arrow_params);
    addChild(mArrowBtn);

    //a panel for hidden item buttons
    LLPanel::Params panel_params = p.items_panel;
    mItemsPanel= prepareItemsPanel(panel_params, static_cast<S32>(p.items.numValidElements()));
    addChild(mItemsPanel);


    LLInitParam::ParamIterator<ItemParams>::const_iterator it = p.items.begin();

    //processing shown item button
    mShownItem = prepareItemButton(*it);
    mShownItem->setHeldDownCallback(boost::bind(&LLSplitButton::onHeldDownShownButton, this));
    mShownItem->setMouseUpCallback(boost::bind(&LLSplitButton::onItemSelected, this, _1));
    mShownItem->setRect(LLRect(btn_left, rc.getHeight(), btn_right, 0));
    addChild(mShownItem);

    //processing hidden item buttons
    S32 item_top = mItemsPanel->getRect().getHeight();
    for (++it; it != p.items.end(); ++it)
    {
        LLButton* hidden_button = prepareItemButton(*it);
        hidden_button->setRect(LLRect(btn_left, item_top, btn_right, item_top - rc.getHeight()));
        hidden_button->setMouseDownCallback(boost::bind(&LLSplitButton::onItemSelected, this, _1));
        mHidenItems.push_back(hidden_button);
        mItemsPanel->addChild(hidden_button);

        //calculate next button's top
        item_top -= (rc.getHeight() + BUTTON_PAD);
    }

    mTopLostSignalConnection = setTopLostCallback(boost::bind(&LLSplitButton::hideButtons, this));
}

LLSplitButton::~LLSplitButton()
{
    // explicitly disconect to avoid hideButtons with
    // dead pointers being called on destruction
    mTopLostSignalConnection.disconnect();
}


LLButton* LLSplitButton::prepareItemButton(LLButton::Params params)
{
    params.label("");
    params.is_toggle(false);
    return LLUICtrlFactory::create<LLButton>(params);
}

LLPanel* LLSplitButton::prepareItemsPanel(LLPanel::Params params, S32 items_count)
{
    S32 num_hiden_btns = items_count - 1;
    S32 panel_height = num_hiden_btns * (getRect().getHeight() + BUTTON_PAD);
    params.visible(false);
    params.rect.width(getRect().getWidth());
    params.rect.height(panel_height);
    return LLUICtrlFactory::create<LLPanel>(params);
}

