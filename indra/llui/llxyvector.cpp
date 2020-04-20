/**
* @file llxyvector.cpp
* @author Andrey Lihatskiy
* @brief Implementation for LLXYVector
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

#include "linden_common.h"

#include "llxyvector.h"

#include "llstring.h"
#include "llrect.h"

#include "lluictrlfactory.h"
#include "llrender.h"

#include "llmath.h"

// Globals
static LLDefaultChildRegistry::Register<LLXYVector> register_xy_vector("xy_vector");


const F32 CENTER_CIRCLE_RADIUS = 2;
const S32 ARROW_ANGLE = 30;
const S32 ARROW_LENGTH_LONG = 10;
const S32 ARROW_LENGTH_SHORT = 6;

LLXYVector::Params::Params()
  : x_entry("x_entry"),
    y_entry("y_entry"),
    touch_area("touch_area"),
    border("border"),
    edit_bar_height("edit_bar_height", 18),
    padding("padding", 4),
    min_val_x("min_val_x", -1.0f),
    max_val_x("max_val_x", 1.0f),
    increment_x("increment_x", 0.05f),
    min_val_y("min_val_y", -1.0f),
    max_val_y("max_val_y", 1.0f),
    increment_y("increment_y", 0.05f),
    label_width("label_width", 16),
    arrow_color("arrow_color", LLColor4::white),
    ghost_color("ghost_color"),
    area_color("area_color", LLColor4::grey4),
    grid_color("grid_color", LLColor4::grey % 0.25f),
    logarithmic("logarithmic", FALSE)
{
}

LLXYVector::LLXYVector(const LLXYVector::Params& p)
  : LLUICtrl(p),
    mArrowColor(p.arrow_color()),
    mAreaColor(p.area_color),
    mGridColor(p.grid_color),
    mMinValueX(p.min_val_x),
    mMaxValueX(p.max_val_x),
    mIncrementX(p.increment_x),
    mMinValueY(p.min_val_y),
    mMaxValueY(p.max_val_y),
    mIncrementY(p.increment_y),
    mLogarithmic(p.logarithmic),
    mValueX(0),
    mValueY(0)
{
    mGhostColor = p.ghost_color.isProvided() ? p.ghost_color() % 0.3f : p.arrow_color() % 0.3f;

    LLRect border_rect = getLocalRect();
    LLViewBorder::Params params = p.border;
    params.rect(border_rect);
    mBorder = LLUICtrlFactory::create<LLViewBorder>(params);
    addChild(mBorder);

    LLTextBox::Params x_label_params;
    x_label_params.initial_value(p.x_entry.label());
    x_label_params.rect = LLRect(p.padding,
                                 border_rect.mTop - p.padding,
                                 p.label_width,
                                 border_rect.getHeight() - p.edit_bar_height);
    mXLabel = LLUICtrlFactory::create<LLTextBox>(x_label_params);
    addChild(mXLabel);
    LLLineEditor::Params x_params = p.x_entry;
    x_params.rect = LLRect(p.padding + p.label_width,
                           border_rect.mTop - p.padding,
                           border_rect.getCenterX(),
                           border_rect.getHeight() - p.edit_bar_height);
    x_params.commit_callback.function(boost::bind(&LLXYVector::onEditChange, this));
    mXEntry = LLUICtrlFactory::create<LLLineEditor>(x_params);
    mXEntry->setPrevalidateInput(LLTextValidate::validateFloat);
    addChild(mXEntry);

    LLTextBox::Params y_label_params;
    y_label_params.initial_value(p.y_entry.label());
    y_label_params.rect = LLRect(border_rect.getCenterX() + p.padding,
                                 border_rect.mTop - p.padding,
                                 border_rect.getCenterX() + p.label_width,
                                 border_rect.getHeight() - p.edit_bar_height);
    mYLabel = LLUICtrlFactory::create<LLTextBox>(y_label_params);
    addChild(mYLabel);
    LLLineEditor::Params y_params = p.y_entry;
    y_params.rect = LLRect(border_rect.getCenterX() + p.padding + p.label_width,
                           border_rect.getHeight() - p.padding,
                           border_rect.getWidth() - p.padding,
                           border_rect.getHeight() - p.edit_bar_height);
    y_params.commit_callback.function(boost::bind(&LLXYVector::onEditChange, this));
    mYEntry = LLUICtrlFactory::create<LLLineEditor>(y_params);
    mYEntry->setPrevalidateInput(LLTextValidate::validateFloat);
    addChild(mYEntry);

    LLPanel::Params touch_area = p.touch_area;
    touch_area.rect = LLRect(p.padding,
                             border_rect.mTop - p.edit_bar_height - p.padding,
                             border_rect.getWidth() - p.padding,
                             p.padding);
    mTouchArea = LLUICtrlFactory::create<LLPanel>(touch_area);
    addChild(mTouchArea);
}

LLXYVector::~LLXYVector()
{
}

BOOL LLXYVector::postBuild()
{
    mLogScaleX = (2 * log(mMaxValueX)) / mTouchArea->getRect().getWidth();
    mLogScaleY = (2 * log(mMaxValueY)) / mTouchArea->getRect().getHeight();

    return TRUE;
}

void drawArrow(S32 tailX, S32 tailY, S32 tipX, S32 tipY, LLColor4 color)
{
    gl_line_2d(tailX, tailY, tipX, tipY, color);

    S32 dx = tipX - tailX;
    S32 dy = tipY - tailY;

    S32 arrowLength = (abs(dx) < ARROW_LENGTH_LONG && abs(dy) < ARROW_LENGTH_LONG) ? ARROW_LENGTH_SHORT : ARROW_LENGTH_LONG;
   
    F32 theta = std::atan2(dy, dx);

    F32 rad = ARROW_ANGLE * std::atan(1) * 4 / 180;
    F32 x = tipX - arrowLength * cos(theta + rad);
    F32 y = tipY - arrowLength * sin(theta + rad);
    F32 rad2 = -1 * ARROW_ANGLE * std::atan(1) * 4 / 180;
    F32 x2 = tipX - arrowLength * cos(theta + rad2);
    F32 y2 = tipY - arrowLength * sin(theta + rad2);
    gl_triangle_2d(tipX, tipY, x, y, x2, y2, color, true);
}

void LLXYVector::draw()
{
    S32 centerX = mTouchArea->getRect().getCenterX();
    S32 centerY = mTouchArea->getRect().getCenterY();
    S32 pointX;
    S32 pointY;

    if (mLogarithmic)
    {
        pointX = (log(llabs(mValueX) + 1)) / mLogScaleX;
        pointX *= (mValueX < 0) ? -1 : 1;
        pointX += centerX;

        pointY = (log(llabs(mValueY) + 1)) / mLogScaleY;
        pointY *= (mValueY < 0) ? -1 : 1;
        pointY += centerY;
    }
    else // linear
    {
        pointX = centerX + (mValueX * mTouchArea->getRect().getWidth() / (2 * mMaxValueX));
        pointY = centerY + (mValueY * mTouchArea->getRect().getHeight() / (2 * mMaxValueY));
    }

    // fill
    gl_rect_2d(mTouchArea->getRect(), mAreaColor, true);

    // draw grid
    gl_line_2d(centerX, mTouchArea->getRect().mTop, centerX, mTouchArea->getRect().mBottom, mGridColor);
    gl_line_2d(mTouchArea->getRect().mLeft, centerY, mTouchArea->getRect().mRight, centerY, mGridColor);

    // draw ghost
    if (hasMouseCapture())
    {
        drawArrow(centerX, centerY, mGhostX, mGhostY, mGhostColor);
    }
    else
    {
        mGhostX = pointX;
        mGhostY = pointY;
    }

    if (abs(mValueX) >= mIncrementX || abs(mValueY) >= mIncrementY)
    {
        // draw the vector arrow
        drawArrow(centerX, centerY, pointX, pointY, mArrowColor);
    }
    else
    {
        // skip the arrow, set color for center circle
        gGL.color4fv(mArrowColor.get().mV);
    }

    // draw center circle
    gl_circle_2d(centerX, centerY, CENTER_CIRCLE_RADIUS, 12, true);

    LLView::draw();
}

void LLXYVector::onEditChange()
{
    if (getEnabled())
    {
        setValueAndCommit(mXEntry->getValue().asReal(), mYEntry->getValue().asReal());
    }
}

void LLXYVector::setValue(const LLSD& value)
{
    if (value.isArray())
    {
        setValue(value[0].asReal(), value[1].asReal());
    }
}

void LLXYVector::setValue(F32 x, F32 y)
{
    mValueX = ll_round(llclamp(x, mMinValueX, mMaxValueX), mIncrementX);
    mValueY = ll_round(llclamp(y, mMinValueY, mMaxValueY), mIncrementY);

    update();
}

void LLXYVector::setValueAndCommit(F32 x, F32 y)
{
    if (mValueX != x || mValueY != y)
    {
        setValue(x, y);
        onCommit();
    }
}

LLSD LLXYVector::getValue() const
{
    LLSD value;
    value.append(mValueX);
    value.append(mValueY);
    return value;
}

void LLXYVector::update()
{
    mXEntry->setValue(mValueX);
    mYEntry->setValue(mValueY);
}

BOOL LLXYVector::handleHover(S32 x, S32 y, MASK mask)
{
    if (hasMouseCapture())
    {
        if (mLogarithmic)
        {
            F32 valueX = llfastpow(F_E, mLogScaleX*(llabs(x - mTouchArea->getRect().getCenterX()))) - 1;
            valueX *= (x < mTouchArea->getRect().getCenterX()) ? -1 : 1;

            F32 valueY = llfastpow(F_E, mLogScaleY*(llabs(y - mTouchArea->getRect().getCenterY()))) - 1;
            valueY *= (y < mTouchArea->getRect().getCenterY()) ? -1 : 1;

            setValueAndCommit(valueX, valueY);
        }
        else //linear
        {
            F32 valueX = 2 * mMaxValueX * F32(x - mTouchArea->getRect().getCenterX()) / mTouchArea->getRect().getWidth();
            F32 valueY = 2 * mMaxValueY * F32(y - mTouchArea->getRect().getCenterY()) / mTouchArea->getRect().getHeight();

            setValueAndCommit(valueX, valueY);
        }
    }

    return TRUE;
}

BOOL LLXYVector::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (hasMouseCapture())
    {
        gFocusMgr.setMouseCapture(NULL);
        make_ui_sound("UISndClickRelease");
    }

    if (mTouchArea->getRect().pointInRect(x, y))
    {
        return TRUE;
    }
    else
    {
        return LLUICtrl::handleMouseUp(x, y, mask);
    }
}

BOOL LLXYVector::handleMouseDown(S32 x, S32 y, MASK mask)
{

    if (mTouchArea->getRect().pointInRect(x, y))
    {
        gFocusMgr.setMouseCapture(this);
        make_ui_sound("UISndClick");

        return TRUE;
    }
    else
    {
        return LLUICtrl::handleMouseDown(x, y, mask);
    }
}

