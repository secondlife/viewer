/** 
 * @file llsplitbutton.h
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

// A control that displays the name of the chosen item, which when clicked
// shows a scrolling box of choices.


#include "llbutton.h"
#include "llpanel.h"
#include "lluictrl.h"


#ifndef LL_LLSPLITBUTTON_H
#define LL_LLSPLITBUTTON_H

class LLSplitButton
    :   public LLUICtrl
{
public:
    typedef enum e_arrow_position
    {
        LEFT,
        RIGHT
    } EArrowPosition;

    struct ArrowPositionValues : public LLInitParam::TypeValuesHelper<EArrowPosition, ArrowPositionValues>
    {
        static void declareValues();
    };

    struct ItemParams : public LLInitParam::Block<ItemParams, LLButton::Params>
    {
        ItemParams();
    };

    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<EArrowPosition, ArrowPositionValues> arrow_position;
        Optional<LLButton::Params> arrow_button;
        Optional<LLPanel::Params> items_panel;
        Multiple<ItemParams> items;

        Params();
    };


    virtual ~LLSplitButton() {};

    //Overridden
    virtual void    onFocusLost();
    virtual void    setFocus(BOOL b);
    virtual void    setEnabled(BOOL enabled);

    //Callbacks
    void    onArrowBtnDown();
    void    onHeldDownShownButton();
    void    onItemSelected(LLUICtrl* ctrl);
    void    setSelectionCallback(commit_callback_t cb) { mSelectionCallback = cb; }

    virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);

    virtual void    showButtons();
    virtual void    hideButtons();


protected:
    friend class LLUICtrlFactory;
    LLSplitButton(const LLSplitButton::Params& p);

    LLButton* prepareItemButton(LLButton::Params params);
    LLPanel* prepareItemsPanel(LLPanel::Params params, S32 items_count);

    LLPanel* mItemsPanel;
    std::list<LLButton*> mHidenItems;
    LLButton* mArrowBtn;
    LLButton* mShownItem;
    EArrowPosition mArrowPosition;

    commit_callback_t mSelectionCallback;
};


#endif
