/** 
 * @file llmultislider.h
 * @brief A simple multislider
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_MULTI_SLIDER_H
#define LL_MULTI_SLIDER_H

#include "llf32uictrl.h"
#include "v4color.h"

class LLUICtrlFactory;

class LLMultiSlider : public LLF32UICtrl
{
public:
    struct SliderParams : public LLInitParam::Block<SliderParams>
    {
        Optional<std::string>   name;
        Mandatory<F32>          value;
        SliderParams();
    };

    struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
    {
        Optional<S32>   max_sliders;

        Optional<bool>  allow_overlap,
                        loop_overlap,
                        draw_track,
                        use_triangle;

        Optional<F32>   overlap_threshold;

        Optional<LLUIColor> track_color,
                            thumb_disabled_color,
                            thumb_highlight_color,
                            thumb_outline_color,
                            thumb_center_color,
                            thumb_center_selected_color,
                            triangle_color;

        Optional<std::string>   orientation,
                                thumb_image;

        Optional<CommitCallbackParam>   mouse_down_callback,
                                        mouse_up_callback;
        Optional<S32>       thumb_width;

        Multiple<SliderParams>  sliders;
        Params();
    };

protected:
    LLMultiSlider(const Params&);
    friend class LLUICtrlFactory;
public:
    virtual ~LLMultiSlider();

    // Multi-slider rounds values to nearest increments (bias towards rounding down)
    F32                 getNearestIncrement(F32 value) const;

    void                setSliderValue(const std::string& name, F32 value, BOOL from_event = FALSE);
    F32                 getSliderValue(const std::string& name) const;
    F32                 getSliderValueFromPos(S32 xpos, S32 ypos) const;
    LLRect              getSliderThumbRect(const std::string& name) const;

    void                setSliderThumbImage(const std::string &name);
    void                clearSliderThumbImage();


    const std::string&  getCurSlider() const                    { return mCurSlider; }
    F32                 getCurSliderValue() const               { return getSliderValue(mCurSlider); }
    void                setCurSlider(const std::string& name);
    void                resetCurSlider();
    void                setCurSliderValue(F32 val, BOOL from_event = false) { setSliderValue(mCurSlider, val, from_event); }

    /*virtual*/ void    setValue(const LLSD& value) override;
    /*virtual*/ LLSD    getValue() const override { return mValue; }

    boost::signals2::connection setMouseDownCallback( const commit_signal_t::slot_type& cb );
    boost::signals2::connection setMouseUpCallback( const commit_signal_t::slot_type& cb );

    bool                findUnusedValue(F32& initVal);
    const std::string&  addSlider();
    const std::string&  addSlider(F32 val);
    bool                addSlider(F32 val, const std::string& name);
    void                deleteSlider(const std::string& name);
    void                deleteCurSlider()           { deleteSlider(mCurSlider); }
    /*virtual*/ void    clear() override;

    /*virtual*/ BOOL    handleHover(S32 x, S32 y, MASK mask) override;
    /*virtual*/ BOOL    handleMouseUp(S32 x, S32 y, MASK mask) override;
    /*virtual*/ BOOL    handleMouseDown(S32 x, S32 y, MASK mask) override;
    /*virtual*/ BOOL    handleKeyHere(KEY key, MASK mask) override;
    /*virtual*/ void    onMouseLeave(S32 x, S32 y, MASK mask) override;
    /*virtual*/ void    draw() override;

    S32             getMaxNumSliders() { return mMaxNumSliders; }
    S32             getCurNumSliders() { return mValue.size(); }
    F32             getOverlapThreshold() { return mOverlapThreshold; }
    bool            canAddSliders() { return mValue.size() < mMaxNumSliders; }


protected:
    LLSD            mValue;
    std::string     mCurSlider;
    std::string     mHoverSlider;
    static S32      mNameCounter;

    S32             mMaxNumSliders;
    BOOL            mAllowOverlap;
    BOOL            mLoopOverlap;
    F32             mOverlapThreshold;
    BOOL            mDrawTrack;
    BOOL            mUseTriangle;           /// hacked in toggle to use a triangle

    S32             mMouseOffset;
    LLRect          mDragStartThumbRect;
    S32             mThumbWidth;

    std::map<std::string, LLRect>   
                    mThumbRects;
    LLUIColor       mTrackColor;
    LLUIColor       mThumbOutlineColor;
    LLUIColor       mThumbHighlightColor;
    LLUIColor       mThumbCenterColor;
    LLUIColor       mThumbCenterSelectedColor;
    LLUIColor       mDisabledThumbColor;
    LLUIColor       mTriangleColor;
    LLUIImagePtr    mThumbImagep; //blimps on the slider, for now no 'disabled' support
    LLUIImagePtr    mRoundedSquareImgp; //blimps on the slider, for now no 'disabled' support

    const EOrientation  mOrientation;

    commit_signal_t*    mMouseDownSignal;
    commit_signal_t*    mMouseUpSignal;
};

#endif  // LL_MULTI_SLIDER_H
