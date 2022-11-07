/**
* @file virtualtrackball.h
* @author Andrey Lihatskiy
* @brief Header file for LLVirtualTrackball
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

#ifndef LL_LLVIRTUALTRACKBALL_H
#define LL_LLVIRTUALTRACKBALL_H

#include "lluictrl.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "llbutton.h"

class LLVirtualTrackball
    : public LLUICtrl
{
public:
    enum ThumbMode
    {
        SUN,
        MOON
    };
    enum DragMode
    {
        DRAG_SET,
        DRAG_SCROLL
    };

    struct Params
        : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLViewBorder::Params>      border;
        Optional<LLUIImage*>                image_moon_back,
            image_moon_front,
            image_sphere,
            image_sun_back,
            image_sun_front;

        Optional<std::string>               thumb_mode;
        Optional<F32>                       increment_angle_mouse,
            increment_angle_btn;

        Optional<LLTextBox::Params>         lbl_N,
            lbl_S,
            lbl_W,
            lbl_E;

        Optional<LLButton::Params>          btn_rotate_top,
            btn_rotate_bottom,
            btn_rotate_left,
            btn_rotate_right;

        Params();
    };


    virtual ~LLVirtualTrackball();
    /*virtual*/ BOOL postBuild();

    virtual BOOL    handleHover(S32 x, S32 y, MASK mask);
    virtual BOOL    handleMouseUp(S32 x, S32 y, MASK mask);
    virtual BOOL    handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL    handleRightMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL    handleKeyHere(KEY key, MASK mask);

    virtual void    draw();

    virtual void    setValue(const LLSD& value);
    void            setValue(F32 x, F32 y, F32 z, F32 w);
    virtual LLSD    getValue() const;

    void            setRotation(const LLQuaternion &value);
    LLQuaternion    getRotation() const;

    static void             getAzimuthAndElevation(const LLQuaternion &quat, F32 &azimuth, F32 &elevation);
    static void             getAzimuthAndElevationDeg(const LLQuaternion &quat, F32 &azimuth, F32 &elevation);

protected:
    friend class LLUICtrlFactory;
    LLVirtualTrackball(const Params&);
    void onEditChange();

protected:
    LLTextBox*          mNLabel;
    LLTextBox*          mELabel;
    LLTextBox*          mSLabel;
    LLTextBox*          mWLabel;

    LLButton*           mBtnRotateTop;
    LLButton*           mBtnRotateBottom;
    LLButton*           mBtnRotateLeft;
    LLButton*           mBtnRotateRight;

    LLTextBox*          mLabelN;
    LLTextBox*          mLabelS;
    LLTextBox*          mLabelW;
    LLTextBox*          mLabelE;

    LLPanel*            mTouchArea;
    LLViewBorder*       mBorder;

private:
    void setValueAndCommit(const LLQuaternion &value);
    void drawThumb(S32 x, S32 y, ThumbMode mode, bool upperHemi = true);
    bool pointInTouchCircle(S32 x, S32 y) const;

    void onRotateTopClick();
    void onRotateBottomClick();
    void onRotateLeftClick();
    void onRotateRightClick();

    void onRotateTopMouseEnter();
    void onRotateBottomMouseEnter();
    void onRotateLeftMouseEnter();
    void onRotateRightMouseEnter();

    S32 mPrevX;
    S32 mPrevY;

    LLUIImage*     mImgMoonBack;
    LLUIImage*     mImgMoonFront;
    LLUIImage*     mImgSunBack;
    LLUIImage*     mImgSunFront;
    LLUIImage*     mImgBtnRotTop;
    LLUIImage*     mImgBtnRotLeft;
    LLUIImage*     mImgBtnRotRight;
    LLUIImage*     mImgBtnRotBottom;
    LLUIImage*     mImgSphere;

    LLQuaternion   mValue;
    ThumbMode      mThumbMode;
    DragMode       mDragMode;

    F32            mIncrementMouse;
    F32            mIncrementBtn;
};

#endif

