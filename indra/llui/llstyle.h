/**
 * @file llstyle.h
 * @brief Text style class
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

#ifndef LL_LLSTYLE_H
#define LL_LLSTYLE_H

#include "v4color.h"
#include "llui.h"
#include "llinitparam.h"
#include "lluiimage.h"

class LLFontGL;
class LLStyle;

typedef LLPointer<LLStyle> LLStyleSP;
typedef LLPointer<const LLStyle> LLStyleConstSP;

class LLStyle : public LLRefCount
{
public:
    struct Params : public LLInitParam::Block<Params>
    {
        Optional<bool>                  visible;
        Optional<LLFontGL::ShadowType>  drop_shadow;
        Optional<LLUIColor>             color,
                                        readonly_color,
                                        selected_color;
        Optional<F32>                   alpha;
        Optional<const LLFontGL*>       font;
        Optional<LLUIImage*>            image;
        Optional<std::string>           link_href;
        Optional<bool>                  is_link;
        Params();
    };
    LLStyle(const Params& p = Params());
    LLStyleSP clone() const { return makeCopy(); }
    LLStyleConstSP cloneConst() const { return makeCopy(); }

public:
    const LLUIColor& getColor() const { return mColor; }
    void setColor(const LLUIColor &color) { mColor = color; }

    const LLUIColor& getReadOnlyColor() const { return mReadOnlyColor; }
    void setReadOnlyColor(const LLUIColor& color) { mReadOnlyColor = color; }

    const LLUIColor& getSelectedColor() const { return mSelectedColor; }
    void setSelectedColor(const LLUIColor& color) { mSelectedColor = color; }

    F32 getAlpha() const { return mAlpha; }
    void setAlpha(F32 alpha) { mAlpha = alpha; }

    bool isVisible() const;
    void setVisible(bool is_visible);

    LLFontGL::ShadowType getShadowType() const { return mDropShadow; }

    void setFont(const LLFontGL* font);
    const LLFontGL* getFont() const;

    const std::string& getLinkHREF() const { return mLink; }
    void setLinkHREF(const std::string& href);
    bool isLink() const;

    LLPointer<LLUIImage> getImage() const;
    void setImage(const LLUUID& src);
    void setImage(const std::string& name);

    bool isImage() const { return mImagep.notNull(); }

    bool operator==(const LLStyle &rhs) const
    {
        return
            mVisible == rhs.mVisible
            && mColor == rhs.mColor
            && mReadOnlyColor == rhs.mReadOnlyColor
            && mSelectedColor == rhs.mSelectedColor
            && mFont == rhs.mFont
            && mLink == rhs.mLink
            && mImagep == rhs.mImagep
            && mDropShadow == rhs.mDropShadow
            && mAlpha == rhs.mAlpha;
    }

    bool operator!=(const LLStyle& rhs) const { return !(*this == rhs); }

public:
    LLFontGL::ShadowType        mDropShadow;

protected:
    ~LLStyle() = default;
    LLStyle* makeCopy() const;

private:
    std::string         mFontName;
    std::string         mLink;
    LLUIColor           mColor;
    LLUIColor           mReadOnlyColor;
    LLUIColor           mSelectedColor;
    const LLFontGL*     mFont;
    LLPointer<LLUIImage> mImagep;
    F32                 mAlpha;
    bool                mVisible;
    bool                mIsLink;
};

#endif  // LL_LLSTYLE_H
