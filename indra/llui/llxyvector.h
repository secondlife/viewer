/**
* @file llxyvector.h
* @author Andrey Lihatskiy
* @brief Header file for LLXYVector
*
* $LicenseInfo:firstyear=2001&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2018, Linden Research, Inc.
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

// A control that allows to set two related vector magnitudes by manipulating a single vector on a plane. 

#ifndef LL_LLXYVECTOR_H
#define LL_LLXYVECTOR_H

#include "lluictrl.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "lllineeditor.h"

class LLXYVector
    : public LLUICtrl
{
public:
    struct Params
        : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLLineEditor::Params>      x_entry;
        Optional<LLLineEditor::Params>      y_entry;
        Optional<LLPanel::Params>           touch_area;
        Optional<LLViewBorder::Params>      border;
        Optional<S32>                       edit_bar_height;
        Optional<S32>                       padding;
        Optional<S32>                       label_width;
        Optional<F32>                       min_val_x;
        Optional<F32>                       max_val_x;
        Optional<F32>                       increment_x;
        Optional<F32>                       min_val_y;
        Optional<F32>                       max_val_y;
        Optional<F32>                       increment_y;
        Optional<LLUIColor>                 arrow_color;
        Optional<LLUIColor>                 ghost_color;
        Optional<LLUIColor>                 area_color;
        Optional<LLUIColor>                 grid_color;
        Optional<BOOL>                      logarithmic;

        Params();
    };


    virtual ~LLXYVector();
    /*virtual*/ BOOL postBuild();

    virtual BOOL    handleHover(S32 x, S32 y, MASK mask);
    virtual BOOL    handleMouseUp(S32 x, S32 y, MASK mask);
    virtual BOOL    handleMouseDown(S32 x, S32 y, MASK mask);

    virtual void    draw();

    virtual void    setValue(const LLSD& value);
    void            setValue(F32 x, F32 y);
    virtual LLSD    getValue() const;

protected:
    friend class LLUICtrlFactory;
    LLXYVector(const Params&);
    void onEditChange();

protected:
    LLTextBox*          mXLabel;
    LLTextBox*          mYLabel;
    LLLineEditor*       mXEntry;
    LLLineEditor*       mYEntry;
    LLPanel*            mTouchArea;
    LLViewBorder*       mBorder;

private:
    void update();
    void setValueAndCommit(F32 x, F32 y);

    F32 mValueX;
    F32 mValueY;

    F32 mMinValueX;
    F32 mMaxValueX;
    F32 mIncrementX;
    F32 mMinValueY;
    F32 mMaxValueY;
    F32 mIncrementY;

    U32 mGhostX;
    U32 mGhostY;

    LLUIColor mArrowColor;
    LLUIColor mGhostColor;
    LLUIColor mAreaColor;
    LLUIColor mGridColor;

    BOOL mLogarithmic;
    F32 mLogScaleX;
    F32 mLogScaleY;
};

#endif

