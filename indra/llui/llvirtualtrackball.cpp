/**
* @file LLVirtualTrackball.cpp
* @author Andrey Lihatskiy
* @brief Implementation for LLVirtualTrackball
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

// A control for positioning the sun and the moon in the celestial sphere.

#include "linden_common.h"
#include "llvirtualtrackball.h"
#include "llstring.h"
#include "llrect.h"
#include "lluictrlfactory.h"
#include "llrender.h"

// Globals
static LLDefaultChildRegistry::Register<LLVirtualTrackball> register_virtual_trackball("sun_moon_trackball");

const LLVector3 VectorZero(1.0f, 0.0f, 0.0f);

LLVirtualTrackball::Params::Params()
  : border("border"),
    image_moon_back("image_moon_back"),
    image_moon_front("image_moon_front"),
    image_sphere("image_sphere"),
    image_sun_back("image_sun_back"),
    image_sun_front("image_sun_front"),
    btn_rotate_top("button_rotate_top"),
    btn_rotate_bottom("button_rotate_bottom"),
    btn_rotate_left("button_rotate_left"),
    btn_rotate_right("button_rotate_right"),
    thumb_mode("thumb_mode"),
    lbl_N("labelN"),
    lbl_S("labelS"),
    lbl_W("labelW"),
    lbl_E("labelE"),
    increment_angle_mouse("increment_angle_mouse", 0.5f),
    increment_angle_btn("increment_angle_btn", 3.0f)
{
}

LLVirtualTrackball::LLVirtualTrackball(const LLVirtualTrackball::Params& p)
  : LLUICtrl(p),
    mImgMoonBack(p.image_moon_back),
    mImgMoonFront(p.image_moon_front),
    mImgSunBack(p.image_sun_back),
    mImgSunFront(p.image_sun_front),
    mImgSphere(p.image_sphere),
    mThumbMode(p.thumb_mode() == "moon" ? ThumbMode::MOON : ThumbMode::SUN),
    mIncrementMouse(DEG_TO_RAD * p.increment_angle_mouse()),
    mIncrementBtn(DEG_TO_RAD * p.increment_angle_btn())
{
    LLRect border_rect = getLocalRect();
    S32 centerX = border_rect.getCenterX();
    S32 centerY = border_rect.getCenterY();
    U32 btn_size = 32; // width & height
    U32 axis_offset_lt = 16; // offset from the axis for left/top sides
    U32 axis_offset_rb = btn_size - axis_offset_lt; //  and for right/bottom

    LLViewBorder::Params border = p.border;
    border.rect(border_rect);
    mBorder = LLUICtrlFactory::create<LLViewBorder>(border);
    addChild(mBorder);

    
    LLButton::Params btn_rt = p.btn_rotate_top;
    btn_rt.rect(LLRect(centerX - axis_offset_lt, border_rect.mTop, centerX + axis_offset_rb, border_rect.mTop - btn_size));
    btn_rt.click_callback.function(boost::bind(&LLVirtualTrackball::onRotateTopClick, this));
    btn_rt.mouse_held_callback.function(boost::bind(&LLVirtualTrackball::onRotateTopClick, this));
    btn_rt.mouseenter_callback.function(boost::bind(&LLVirtualTrackball::onRotateTopMouseEnter, this));
    mBtnRotateTop = LLUICtrlFactory::create<LLButton>(btn_rt);
    addChild(mBtnRotateTop);

    LLTextBox::Params lbl_N = p.lbl_N;
    LLRect rect_N = btn_rt.rect;
    //rect_N.translate(btn_rt.rect().getWidth(), 0);
    lbl_N.rect = rect_N;
    lbl_N.initial_value(lbl_N.label());
    mLabelN = LLUICtrlFactory::create<LLTextBox>(lbl_N);
    addChild(mLabelN);


    LLButton::Params btn_rr = p.btn_rotate_right;
    btn_rr.rect(LLRect(border_rect.mRight - btn_size, centerY + axis_offset_lt, border_rect.mRight, centerY - axis_offset_rb));
    btn_rr.click_callback.function(boost::bind(&LLVirtualTrackball::onRotateRightClick, this));
    btn_rr.mouse_held_callback.function(boost::bind(&LLVirtualTrackball::onRotateRightClick, this));
    btn_rr.mouseenter_callback.function(boost::bind(&LLVirtualTrackball::onRotateRightMouseEnter, this));
    mBtnRotateRight = LLUICtrlFactory::create<LLButton>(btn_rr);
    addChild(mBtnRotateRight);

    LLTextBox::Params lbl_E = p.lbl_E;
    LLRect rect_E = btn_rr.rect;
    //rect_E.translate(0, -1 * btn_rr.rect().getHeight());
    lbl_E.rect = rect_E;
    lbl_E.initial_value(lbl_E.label());
    mLabelE = LLUICtrlFactory::create<LLTextBox>(lbl_E);
    addChild(mLabelE);


    LLButton::Params btn_rb = p.btn_rotate_bottom;
    btn_rb.rect(LLRect(centerX - axis_offset_lt, border_rect.mBottom + btn_size, centerX + axis_offset_rb, border_rect.mBottom));
    btn_rb.click_callback.function(boost::bind(&LLVirtualTrackball::onRotateBottomClick, this));
    btn_rb.mouse_held_callback.function(boost::bind(&LLVirtualTrackball::onRotateBottomClick, this));
    btn_rb.mouseenter_callback.function(boost::bind(&LLVirtualTrackball::onRotateBottomMouseEnter, this));
    mBtnRotateBottom = LLUICtrlFactory::create<LLButton>(btn_rb);
    addChild(mBtnRotateBottom);

    LLTextBox::Params lbl_S = p.lbl_S;
    LLRect rect_S = btn_rb.rect;
    //rect_S.translate(btn_rb.rect().getWidth(), 0);
    lbl_S.rect = rect_S;
    lbl_S.initial_value(lbl_S.label());
    mLabelS = LLUICtrlFactory::create<LLTextBox>(lbl_S);
    addChild(mLabelS);


    LLButton::Params btn_rl = p.btn_rotate_left;
    btn_rl.rect(LLRect(border_rect.mLeft, centerY + axis_offset_lt, border_rect.mLeft + btn_size, centerY - axis_offset_rb));
    btn_rl.click_callback.function(boost::bind(&LLVirtualTrackball::onRotateLeftClick, this));
    btn_rl.mouse_held_callback.function(boost::bind(&LLVirtualTrackball::onRotateLeftClick, this));
    btn_rl.mouseenter_callback.function(boost::bind(&LLVirtualTrackball::onRotateLeftMouseEnter, this));
    mBtnRotateLeft = LLUICtrlFactory::create<LLButton>(btn_rl);
    addChild(mBtnRotateLeft);

    LLTextBox::Params lbl_W = p.lbl_W;
    LLRect rect_W = btn_rl.rect;
    //rect_W.translate(0, -1* btn_rl.rect().getHeight());
    lbl_W.rect = rect_W;
    lbl_W.initial_value(lbl_W.label());
    mLabelW = LLUICtrlFactory::create<LLTextBox>(lbl_W);
    addChild(mLabelW);


    LLPanel::Params touch_area;
    touch_area.rect = LLRect(centerX - mImgSphere->getWidth() / 2,
                             centerY + mImgSphere->getHeight() / 2,
                             centerX + mImgSphere->getWidth() / 2,
                             centerY - mImgSphere->getHeight() / 2);
    mTouchArea = LLUICtrlFactory::create<LLPanel>(touch_area);
    addChild(mTouchArea);
}

LLVirtualTrackball::~LLVirtualTrackball()
{
}

BOOL LLVirtualTrackball::postBuild()
{
    return TRUE;
}


void LLVirtualTrackball::drawThumb(S32 x, S32 y, ThumbMode mode, bool upperHemi)
{
    LLUIImage* thumb;
    if (mode == ThumbMode::SUN)
    {
        if (upperHemi)
        {
            thumb = mImgSunFront;
        }
        else
        {
            thumb = mImgSunBack;
        }
    }
    else
    {
        if (upperHemi)
        {
            thumb = mImgMoonFront;
        }
        else
        {
            thumb = mImgMoonBack;
        }
    }
    thumb->draw(LLRect(x - thumb->getWidth() / 2,
                       y + thumb->getHeight() / 2,
                       x + thumb->getWidth() / 2,
                       y - thumb->getHeight() / 2));
}

bool LLVirtualTrackball::pointInTouchCircle(S32 x, S32 y) const
{
    S32 centerX = mTouchArea->getRect().getCenterX();
    S32 centerY = mTouchArea->getRect().getCenterY();

    bool in_circle = pow(x - centerX, 2) + pow(y - centerY, 2) <= pow(mTouchArea->getRect().getWidth() / 2, 2);
    return in_circle;
}

void LLVirtualTrackball::draw()
{
    LLVector3 draw_point = VectorZero * mValue;

    S32 halfwidth = mTouchArea->getRect().getWidth() / 2;
    S32 halfheight = mTouchArea->getRect().getHeight() / 2;
    draw_point.mV[VX] = (draw_point.mV[VX] + 1.0) * halfwidth + mTouchArea->getRect().mLeft;
    draw_point.mV[VY] = (draw_point.mV[VY] + 1.0) * halfheight + mTouchArea->getRect().mBottom;
    bool upper_hemisphere = (draw_point.mV[VZ] >= 0.f);

    mImgSphere->draw(mTouchArea->getRect(), upper_hemisphere ? UI_VERTEX_COLOR : UI_VERTEX_COLOR % 0.5f);
    drawThumb(draw_point.mV[VX], draw_point.mV[VY], mThumbMode, upper_hemisphere);


    if (LLView::sDebugRects)
    {
        gGL.color4fv(LLColor4::red.mV);
        gl_circle_2d(mTouchArea->getRect().getCenterX(), mTouchArea->getRect().getCenterY(), mImgSphere->getWidth() / 2, 60, false);
        gl_circle_2d(draw_point.mV[VX], draw_point.mV[VY], mImgSunFront->getWidth() / 2, 12, false);
    }

    // hide the direction labels when disabled
    BOOL enabled = isInEnabledChain();
    mLabelN->setVisible(enabled);
    mLabelE->setVisible(enabled);
    mLabelS->setVisible(enabled);
    mLabelW->setVisible(enabled);

    LLView::draw();
}

void LLVirtualTrackball::onRotateTopClick()
{
    if (getEnabled())
    {
        LLQuaternion delta;
        delta.setAngleAxis(mIncrementBtn, 1, 0, 0); 
        mValue *= delta;
        setValueAndCommit(mValue);

        make_ui_sound("UISndClick");
    }
}

void LLVirtualTrackball::onRotateBottomClick()
{
    if (getEnabled())
    {
        LLQuaternion delta;
        delta.setAngleAxis(mIncrementBtn, -1, 0, 0);
        mValue *= delta;
        setValueAndCommit(mValue);

        make_ui_sound("UISndClick");
    }
}

void LLVirtualTrackball::onRotateLeftClick()
{
    if (getEnabled())
    {
        LLQuaternion delta;
        delta.setAngleAxis(mIncrementBtn, 0, 1, 0);
        mValue *= delta;
        setValueAndCommit(mValue);

        make_ui_sound("UISndClick");
    }
}

void LLVirtualTrackball::onRotateRightClick()
{
    if (getEnabled())
    {
        LLQuaternion delta;
        delta.setAngleAxis(mIncrementBtn, 0, -1, 0);
        mValue *= delta;
        setValueAndCommit(mValue);

        make_ui_sound("UISndClick");
    }
}

void LLVirtualTrackball::onRotateTopMouseEnter()
{
    mBtnRotateTop->setHighlight(true);
}

void LLVirtualTrackball::onRotateBottomMouseEnter()
{
    mBtnRotateBottom->setHighlight(true);
}

void LLVirtualTrackball::onRotateLeftMouseEnter()
{
    mBtnRotateLeft->setHighlight(true);
}

void LLVirtualTrackball::onRotateRightMouseEnter()
{
    mBtnRotateRight->setHighlight(true);
}

void LLVirtualTrackball::setValue(const LLSD& value)
{
	if (value.isArray() && value.size() == 4)
    {
        mValue.setValue(value);
    }
}

void LLVirtualTrackball::setRotation(const LLQuaternion &value)
{
    mValue = value;
}

void LLVirtualTrackball::setValue(F32 x, F32 y, F32 z, F32 w)
{
    mValue.set(x, y, z, w);
}

void LLVirtualTrackball::setValueAndCommit(const LLQuaternion &value)
{
	mValue = value;
    onCommit();
}

LLSD LLVirtualTrackball::getValue() const
{
    return mValue.getValue();
}

LLQuaternion LLVirtualTrackball::getRotation() const
{
	return mValue;
}

BOOL LLVirtualTrackball::handleHover(S32 x, S32 y, MASK mask)
{
    if (hasMouseCapture())
    {
        if (mDragMode == DRAG_SCROLL)
        { // trackball (move to roll) mode
            LLQuaternion delta;

            F32 rotX = x - mPrevX;
            F32 rotY = y - mPrevY;

            if (abs(rotX) > 1)
            {
                F32 direction = (rotX < 0) ? -1 : 1;
                delta.setAngleAxis(mIncrementMouse * abs(rotX), 0, direction, 0);  // changing X - rotate around Y axis
                mValue *= delta;
            }

            if (abs(rotY) > 1)
            {
                F32 direction = (rotY < 0) ? 1 : -1; // reverse for Y (value increases from bottom to top)
                delta.setAngleAxis(mIncrementMouse * abs(rotY), direction, 0, 0);  // changing Y - rotate around X axis
                mValue *= delta;
            }
        }
        else
        { // set on click mode
            if (!pointInTouchCircle(x, y))
            {
                return TRUE; // don't drag outside the circle
            }

            F32 radius = mTouchArea->getRect().getWidth() / 2;
            F32 xx = x - mTouchArea->getRect().getCenterX();
            F32 yy = y - mTouchArea->getRect().getCenterY();
            F32 dist = sqrt(pow(xx, 2) + pow(yy, 2));

            F32 azimuth = llclamp(acosf(xx / dist), 0.0f, F_PI);
            F32 altitude = llclamp(acosf(dist / radius), 0.0f, F_PI_BY_TWO);

            if (yy < 0)
            {
                azimuth = F_TWO_PI - azimuth;
            }

            LLVector3 draw_point = VectorZero * mValue;
            if (draw_point.mV[VZ] >= 0.f)
            {
                if (is_approx_zero(altitude)) // don't change the hemisphere
                {
                    altitude = F_APPROXIMATELY_ZERO;
                }
                altitude *= -1;
            }

            mValue.setAngleAxis(altitude, 0, 1, 0);
            LLQuaternion az_quat;
            az_quat.setAngleAxis(azimuth, 0, 0, 1);
            mValue *= az_quat;
        }

        mPrevX = x;
        mPrevY = y;
        onCommit();
    }
    return TRUE;
}

BOOL LLVirtualTrackball::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (hasMouseCapture())
    {
        mPrevX = 0;
        mPrevY = 0;
        gFocusMgr.setMouseCapture(NULL);
        make_ui_sound("UISndClickRelease");
    }
    return LLView::handleMouseUp(x, y, mask);
}

BOOL LLVirtualTrackball::handleMouseDown(S32 x, S32 y, MASK mask)
{
    if (pointInTouchCircle(x, y))
    {
        mPrevX = x;
        mPrevY = y;
        gFocusMgr.setMouseCapture(this);
        mDragMode = (mask == MASK_CONTROL) ? DRAG_SCROLL : DRAG_SET;
        make_ui_sound("UISndClick");
    }
    return LLView::handleMouseDown(x, y, mask);
}

BOOL LLVirtualTrackball::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    if (pointInTouchCircle(x, y))
    {

        //make_ui_sound("UISndClick");
    }
    return LLView::handleRightMouseDown(x, y, mask);
}

BOOL LLVirtualTrackball::handleKeyHere(KEY key, MASK mask)
{
    BOOL handled = FALSE;
    switch (key)
    {
    case KEY_DOWN:
        onRotateTopClick();
        handled = TRUE;
        break;
    case KEY_LEFT:
        onRotateRightClick();
        handled = TRUE;
        break;
    case KEY_UP:
        onRotateBottomClick();
        handled = TRUE;
        break;
    case KEY_RIGHT:
        onRotateLeftClick();
        handled = TRUE;
        break;
    default:
        break;
    }
    return handled;
}

