/**
 * @file llthumbnailctrl.h
 * @brief LLThumbnailCtrl base class
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023 Linden Research, Inc.
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

#ifndef LL_LLTHUMBNAILCTRL_H
#define LL_LLTHUMBNAILCTRL_H

#include "llui.h"
#include "lluictrl.h"
#include "llviewborder.h" // for params

class LLUICtrlFactory;
class LLUUID;
class LLViewerFetchedTexture;

//
// Classes
//

//
class LLThumbnailCtrl
: public LLUICtrl
{
public:
    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLViewBorder::Params> border;
        Optional<LLUIColor>        border_color;
        Optional<std::string>      image_name;
        Optional<LLUIImage*>       fallback_image;
        Optional<bool>             border_visible;
        Optional<bool>             interactable;
        Optional<bool>             show_loading;

        Params();
    };
protected:
    LLThumbnailCtrl(const Params&);
    friend class LLUICtrlFactory;

public:
    virtual ~LLThumbnailCtrl();

    virtual void draw() override;
    void setVisible(bool visible) override;

    virtual void setValue(const LLSD& value ) override;
    void setInitImmediately(bool val) { mInitImmediately = val; }
    void clearTexture();

    virtual bool handleHover(S32 x, S32 y, MASK mask) override;

protected:
    void initImage();
    void unloadImage();

private:
    bool mBorderVisible;
    bool mInteractable;
    bool mShowLoadingPlaceholder;
    bool mInited;
    bool mInitImmediately;
    std::string mLoadingPlaceholderString;
    LLUUID mImageAssetID;
    LLViewBorder* mBorder;
    LLUIColor mBorderColor;

    LLPointer<LLViewerFetchedTexture> mTexturep;
    LLPointer<LLUIImage> mImagep;
    LLPointer<LLUIImage> mFallbackImagep;
};

#endif
