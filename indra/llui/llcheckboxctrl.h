/**
 * @file llcheckboxctrl.h
 * @brief LLCheckBoxCtrl base class
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

#ifndef LL_LLCHECKBOXCTRL_H
#define LL_LLCHECKBOXCTRL_H

#include "lluictrl.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "v4color.h"

//
// Constants
//

constexpr bool RADIO_STYLE = true;
constexpr bool CHECK_STYLE = false;

//
// Classes
//
class LLFontGL;
class LLViewBorder;

class LLCheckBoxCtrl
: public LLUICtrl
, public ll::ui::SearchableControl
{
public:

    enum EWordWrap
    {
        WRAP_NONE,
        WRAP_UP,
        WRAP_DOWN
    };

    struct WordWrap : public LLInitParam::TypeValuesHelper<EWordWrap, WordWrap>
    {
        static void declareValues();
    };

    struct Params
    :   public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<bool>          initial_value;  // override LLUICtrl initial_value

        Optional<LLTextBox::Params> label_text;
        Optional<LLButton::Params> check_button;

        Optional<EWordWrap, WordWrap>   word_wrap;

        Ignored                 radio_style;

        Params();
    };

    virtual ~LLCheckBoxCtrl();

protected:
    LLCheckBoxCtrl(const Params&);
    friend class LLUICtrlFactory;

public:
    // LLView interface

    virtual void        setEnabled( bool b );

    virtual void        reshape(S32 width, S32 height, bool called_from_parent = true);

    // LLUICtrl interface
    virtual void        setValue(const LLSD& value );
    virtual LLSD        getValue() const;
            bool        get() const { return (bool)getValue().asBoolean(); }
            void        set(bool value) { setValue(value); }

    virtual void        setTentative(bool b);
    virtual bool        getTentative() const;

    virtual bool        setLabelArg( const std::string& key, const LLStringExplicit& text );

    virtual void        clear();
    virtual void        onCommit();

    // LLCheckBoxCtrl interface
    virtual bool        toggle() { return mButton->toggleState(); }      // returns new state

    void                setBtnFocus() { mButton->setFocus(true); }

    void                setEnabledColor( const LLUIColor&color ) { mTextEnabledColor = color; }
    void                setDisabledColor( const LLUIColor&color ) { mTextDisabledColor = color; }

    void                setLabel( const LLStringExplicit& label );
    std::string         getLabel() const;

    void                setFont( const LLFontGL* font ) { mFont = font; }
    const LLFontGL*     getFont() const { return mFont; }

    virtual void        setControlName(const std::string& control_name, LLView* context);

    virtual bool        isDirty()   const;      // Returns true if the user has modified this control.
    virtual void        resetDirty();           // Clear dirty state

protected:
    virtual std::string _getSearchText() const
    {
        return getLabel() + getToolTip();
    }

    virtual void onSetHighlight() const // When highlight, really do highlight the label
    {
        if( mLabel )
            mLabel->ll::ui::SearchableControl::setHighlighted( ll::ui::SearchableControl::getHighlighted() );
    }

protected:
    // note: value is stored in toggle state of button
    LLButton*       mButton;
    LLTextBox*      mLabel;
    const LLFontGL* mFont;

    LLUIColor       mTextEnabledColor;
    LLUIColor       mTextDisabledColor;

    EWordWrap       mWordWrap; // off, shifts text up, shifts text down
};

// Build time optimization, generate once in .cpp file
#ifndef LLCHECKBOXCTRL_CPP
extern template class LLCheckBoxCtrl* LLView::getChild<class LLCheckBoxCtrl>(
    std::string_view name, bool recurse) const;
#endif

#endif  // LL_LLCHECKBOXCTRL_H
