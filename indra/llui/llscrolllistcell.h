/**
 * @file llscrolllistcell.h
 * @brief Scroll lists are composed of rows (items), each of which
 * contains columns (cells).
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

#ifndef LLSCROLLLISTCELL_H
#define LLSCROLLLISTCELL_H

#include "llfontgl.h"       // HAlign
#include "llfontvertexbuffer.h"       // HAlign
#include "llpointer.h"      // LLPointer<>
#include "lluistring.h"
#include "v4color.h"
#include "llui.h"
#include "llgltexture.h"

class LLCheckBoxCtrl;
class LLSD;
class LLUIImage;

/*
 * Represents a cell in a scrollable table.
 *
 * Sub-classes must return height and other properties
 * though width accessors are implemented by the base class.
 * It is therefore important for sub-class constructors to call
 * setWidth() with realistic values.
 */
class LLScrollListCell
{
public:
    struct Params : public LLInitParam::Block<Params>
    {
        Optional<std::string>       type,
                                    column;

        Optional<S32>               width;
        Optional<bool>              enabled,
                                    visible;

        Optional<void*>             userdata;
        Optional<LLSD>              value; // state of checkbox, icon id/name, date
        Optional<LLSD>              alt_value;
        Optional<std::string>       label; // description or text
        Optional<std::string>       tool_tip;

        Optional<const LLFontGL*>   font;
        Optional<LLColor4>          font_color;
        Optional<LLFontGL::HAlign>  font_halign;

        Optional<LLColor4>          color;

        Params()
        :   type("type", "text"),
            column("column"),
            width("width"),
            enabled("enabled", true),
            visible("visible", true),
            value("value"),
            alt_value("alt_value", ""),
            label("label"),
            tool_tip("tool_tip", ""),
            font("font", LLFontGL::getFontEmojiSmall()),
            font_color("font_color", LLColor4::black),
            color("color", LLColor4::white),
            font_halign("halign", LLFontGL::LEFT)
        {
            addSynonym(column, "name");
            addSynonym(font_color, "font-color");
        }
    };

    static LLScrollListCell* create(const Params&);

    LLScrollListCell(const LLScrollListCell::Params&);
    virtual ~LLScrollListCell() {};

    virtual void            draw(const LLColor4& color, const LLColor4& highlight_color) {};      // truncate to given width, if possible
    virtual S32             getWidth() const {return mWidth;}
    virtual S32             getContentWidth() const { return 0; }
    virtual S32             getHeight() const { return 0; }
    virtual const LLSD      getValue() const;
    virtual const LLSD      getAltValue() const;
    virtual void            setValue(const LLSD& value) { }
    virtual void            setAltValue(const LLSD& value) { }
    virtual const std::string &getToolTip() const { return mToolTip; }
    virtual void            setToolTip(const std::string &str) { mToolTip = str; }
    virtual bool            getVisible() const { return true; }
    virtual void            setWidth(S32 width) { mWidth = width; }
    virtual void            highlightText(S32 offset, S32 num_chars) {}
    virtual bool            isText() const { return false; }
    virtual bool            needsToolTip() const { return ! mToolTip.empty(); }
    virtual void            setColor(const LLColor4&) {}
    virtual void            onCommit() {};

    virtual bool            handleClick() { return false; }
    virtual void            setEnabled(bool enable) { }

private:
    S32 mWidth;
    std::string mToolTip;
};

class LLScrollListSpacer : public LLScrollListCell
{
public:
    LLScrollListSpacer(const LLScrollListCell::Params& p) : LLScrollListCell(p) {}
    /*virtual*/ ~LLScrollListSpacer() {};
    /*virtual*/ void            draw(const LLColor4& color, const LLColor4& highlight_color) {}
};

/*
 * Cell displaying a text label.
 */
class LLScrollListText : public LLScrollListCell
{
public:
    LLScrollListText(const LLScrollListCell::Params&);
    /*virtual*/ ~LLScrollListText();

    /*virtual*/ void    draw(const LLColor4& color, const LLColor4& highlight_color);
    /*virtual*/ S32     getContentWidth() const;
    /*virtual*/ S32     getHeight() const;
    /*virtual*/ void    setValue(const LLSD& value);
    /*virtual*/ void    setAltValue(const LLSD& value);
    /*virtual*/ const LLSD getValue() const;
    /*virtual*/ const LLSD getAltValue() const;
    /*virtual*/ bool    getVisible() const;
    /*virtual*/ void    highlightText(S32 offset, S32 num_chars);

    /*virtual*/ void    setColor(const LLColor4&);
    /*virtual*/ bool    isText() const;
    /*virtual*/ const std::string & getToolTip() const;
    /*virtual*/ bool    needsToolTip() const;

    S32             getTextWidth() const { return mTextWidth;}
    void            setTextWidth(S32 value);
    virtual void    setWidth(S32 width);

    void            setText(const LLStringExplicit& text);
    void            setFontStyle(const U8 font_style);
    void            setAlignment(LLFontGL::HAlign align);

protected:

    LLUIString      mText;
    LLUIString      mAltText;
    S32             mTextWidth;
    const LLFontGL* mFont;
    LLFontVertexBuffer mFontBuffer;
    LLColor4        mColor;
    LLColor4        mHighlightColor;
    U8              mUseColor;
    LLFontGL::HAlign mFontAlignment;
    bool            mVisible;
    S32             mHighlightCount;
    S32             mHighlightOffset;

    LLPointer<LLUIImage> mRoundedRectImage;

    static U32 sCount;
};

/*
 * Cell displaying an image. AT the moment, this is specifically UI image
 */
class LLScrollListIcon : public LLScrollListCell
{
public:
    LLScrollListIcon(const LLScrollListCell::Params& p);
    /*virtual*/ ~LLScrollListIcon();
    /*virtual*/ void    draw(const LLColor4& color, const LLColor4& highlight_color);
    /*virtual*/ S32     getWidth() const;
    /*virtual*/ S32     getHeight() const;
    /*virtual*/ const LLSD      getValue() const;
    /*virtual*/ void    setColor(const LLColor4&);
    /*virtual*/ void    setValue(const LLSD& value);

private:
    LLPointer<LLUIImage>    mIcon;
    LLColor4                mColor;
    LLFontGL::HAlign        mAlignment;
};


class LLScrollListBar : public LLScrollListCell
{
public:
    LLScrollListBar(const LLScrollListCell::Params& p);
    /*virtual*/ ~LLScrollListBar();
    /*virtual*/ void    draw(const LLColor4& color, const LLColor4& highlight_color);
    /*virtual*/ S32     getWidth() const;
    /*virtual*/ S32     getHeight() const;
    /*virtual*/ const LLSD      getValue() const;
    /*virtual*/ void    setColor(const LLColor4&);
    /*virtual*/ void    setValue(const LLSD& value);

private:
    LLColor4                    mColor;
    F32                         mRatio;
    S32                         mBottom;
    S32                         mRightPad;
    S32                         mLeftPad;
};
/*
 * An interactive cell containing a check box.
 */
class LLScrollListCheck : public LLScrollListCell
{
public:
    LLScrollListCheck( const LLScrollListCell::Params&);
    /*virtual*/ ~LLScrollListCheck();
    /*virtual*/ void    draw(const LLColor4& color, const LLColor4& highlight_color);
    /*virtual*/ S32     getHeight() const           { return 0; }
    /*virtual*/ const LLSD  getValue() const;
    /*virtual*/ void    setValue(const LLSD& value);
    /*virtual*/ void    onCommit();

    /*virtual*/ bool    handleClick();
    /*virtual*/ void    setEnabled(bool enable);

    LLCheckBoxCtrl* getCheckBox()               { return mCheckBox; }

private:
    LLCheckBoxCtrl* mCheckBox;
};

class LLScrollListDate : public LLScrollListText
{
public:
    LLScrollListDate( const LLScrollListCell::Params& p );
    virtual void    setValue(const LLSD& value);
    virtual const LLSD getValue() const;

private:
    LLDate      mDate;
};

/*
* Cell displaying icon and text.
*/

class LLScrollListIconText : public LLScrollListText
{
public:
    LLScrollListIconText(const LLScrollListCell::Params& p);
    /*virtual*/ ~LLScrollListIconText();
    /*virtual*/ void    draw(const LLColor4& color, const LLColor4& highlight_color);
    /*virtual*/ const LLSD      getValue() const;
    /*virtual*/ void    setValue(const LLSD& value);

    /*virtual*/ void    setWidth(S32 width);

private:
    LLPointer<LLUIImage>    mIcon;
    S32                     mPad;
};

#endif
