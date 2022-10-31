/** 
 * @file llmultisliderctrl.h
 * @brief LLMultiSliderCtrl base class
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

#ifndef LL_MULTI_SLIDERCTRL_H
#define LL_MULTI_SLIDERCTRL_H

#include "llf32uictrl.h"
#include "v4color.h"
#include "llmultislider.h"
#include "lltextbox.h"
#include "llrect.h"


//
// Classes
//
class LLFontGL;
class LLLineEditor;
class LLSlider;


class LLMultiSliderCtrl : public LLF32UICtrl
{
public:
    struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
    {
        Optional<S32>           label_width,
                                text_width;
        Optional<bool>          show_text,
                                can_edit_text;
        Optional<S32>           decimal_digits,
                                thumb_width;
        Optional<S32>           max_sliders;    
        Optional<bool>          allow_overlap,
                                loop_overlap,
                                draw_track,
                                use_triangle;

        Optional<std::string>   orientation,
                                thumb_image;

        Optional<F32>           overlap_threshold;

        Optional<LLUIColor>     text_color,
                                text_disabled_color,
                                thumb_highlight_color;

        Optional<CommitCallbackParam>   mouse_down_callback,
                                        mouse_up_callback;

        Multiple<LLMultiSlider::SliderParams>   sliders;

        Params();
    };

protected:
    LLMultiSliderCtrl(const Params&);
    friend class LLUICtrlFactory;
public:
    virtual ~LLMultiSliderCtrl();

    F32             getSliderValue(const std::string& name) const   { return mMultiSlider->getSliderValue(name); }
    void            setSliderValue(const std::string& name, F32 v, BOOL from_event = FALSE);

    virtual void    setValue(const LLSD& value );
    virtual LLSD    getValue() const        { return mMultiSlider->getValue(); }
    virtual BOOL    setLabelArg( const std::string& key, const LLStringExplicit& text );

    const std::string& getCurSlider() const                 { return mMultiSlider->getCurSlider(); }
    F32             getCurSliderValue() const               { return mCurValue; }
    void            setCurSlider(const std::string& name);      
    void            resetCurSlider();
    void            setCurSliderValue(F32 val, BOOL from_event = false) { setSliderValue(mMultiSlider->getCurSlider(), val, from_event); }

    virtual void    setMinValue(const LLSD& min_value)  { setMinValue((F32)min_value.asReal()); }
    virtual void    setMaxValue(const LLSD& max_value)  { setMaxValue((F32)max_value.asReal());  }

    BOOL            isMouseHeldDown();

    virtual void    setEnabled( BOOL b );
    virtual void    clear();
    virtual void    setPrecision(S32 precision);
    void            setMinValue(F32 min_value) {mMultiSlider->setMinValue(min_value);}
    void            setMaxValue(F32 max_value) {mMultiSlider->setMaxValue(max_value);}
    void            setIncrement(F32 increment) {mMultiSlider->setIncrement(increment);}

    F32             getNearestIncrement(F32 value) const { return mMultiSlider->getNearestIncrement(value); }
    F32             getSliderValueFromPos(S32 x, S32 y) const { return mMultiSlider->getSliderValueFromPos(x, y); }
    LLRect          getSliderThumbRect(const std::string &name) const { return mMultiSlider->getSliderThumbRect(name); }

    void            setSliderThumbImage(const std::string &name) { mMultiSlider->setSliderThumbImage(name); }
    void            clearSliderThumbImage() { mMultiSlider->clearSliderThumbImage(); }

    /// for adding and deleting sliders
    const std::string&  addSlider();
    const std::string&  addSlider(F32 val);
    bool                addSlider(F32 val, const std::string& name);
    void            deleteSlider(const std::string& name);
    void            deleteCurSlider()           { deleteSlider(mMultiSlider->getCurSlider()); }

    F32             getMinValue() const { return mMultiSlider->getMinValue(); }
    F32             getMaxValue() const { return mMultiSlider->getMaxValue(); }

    S32             getMaxNumSliders() { return mMultiSlider->getMaxNumSliders(); }
    S32             getCurNumSliders() { return mMultiSlider->getCurNumSliders(); }
    F32             getOverlapThreshold() { return mMultiSlider->getOverlapThreshold(); }
    bool            canAddSliders() { return mMultiSlider->canAddSliders(); }

    void            setLabel(const std::string& label)              { if (mLabelBox) mLabelBox->setText(label); }
    void            setLabelColor(const LLColor4& c)            { mTextEnabledColor = c; }
    void            setDisabledLabelColor(const LLColor4& c)    { mTextDisabledColor = c; }

    boost::signals2::connection setSliderMouseDownCallback( const commit_signal_t::slot_type& cb );
    boost::signals2::connection setSliderMouseUpCallback( const commit_signal_t::slot_type& cb );

    virtual void    onTabInto();

    virtual void    setTentative(BOOL b);           // marks value as tentative
    virtual void    onCommit();                     // mark not tentative, then commit

    virtual void        setControlName(const std::string& control_name, LLView* context);
    
    static void     onSliderCommit(LLUICtrl* caller, const LLSD& userdata);
    
    static void     onEditorCommit(LLUICtrl* ctrl, const LLSD& userdata);
    static void     onEditorGainFocus(LLFocusableElement* caller, void *userdata);
    static void     onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

private:
    void            updateText();
    void            reportInvalidData();

private:
    const LLFontGL* mFont;
    BOOL            mShowText;
    BOOL            mCanEditText;

    S32             mPrecision;
    LLTextBox*      mLabelBox;
    S32             mLabelWidth;

    F32             mCurValue;
    LLMultiSlider*  mMultiSlider;
    LLLineEditor*   mEditor;
    LLTextBox*      mTextBox;

    LLUIColor   mTextEnabledColor;
    LLUIColor   mTextDisabledColor;
};

#endif  // LL_MULTI_SLIDERCTRL_H
