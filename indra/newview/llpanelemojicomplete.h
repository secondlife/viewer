/**
* @file llpanelemojicomplete.h
* @brief Header file for LLPanelEmojiComplete
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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

#pragma once

#include "llemojidictionary.h"
#include "llfloater.h"
#include "lluictrl.h"

class LLScrollbar;

// ============================================================================
// LLPanelEmojiComplete
//

class LLPanelEmojiComplete : public LLUICtrl
{
    friend class LLUICtrlFactory;
public:
    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<bool>       autosize;
        Optional<bool>       noscroll;
        Optional<bool>       vertical;
        Optional<S32>        max_visible,
                             padding;

        Optional<LLUIImage*> selected_image;

        Params();
    };

protected:
    LLPanelEmojiComplete(const LLPanelEmojiComplete::Params&);

public:
    virtual ~LLPanelEmojiComplete();

    void draw() override;
    bool handleHover(S32 x, S32 y, MASK mask) override;
    bool handleKey(KEY key, MASK mask, bool called_from_parent) override;
    bool handleMouseDown(S32 x, S32 y, MASK mask) override;
    bool handleMouseUp(S32 x, S32 y, MASK mask) override;
    bool handleScrollWheel(S32 x, S32 y, S32 clicks) override;
    void onCommit() override;
    void reshape(S32 width, S32 height, bool called_from_parent) override;

public:
    size_t getEmojiCount() const { return mEmojis.size(); }
    void setEmojis(const LLWString& emojis);
    void setEmojiHint(const std::string& hint);
    bool isAutoSize() const { return mAutoSize; }
    U32 getMaxShortCodeWidth() const;

protected:
    void onEmojisChanged();
    void onScrollbarChange(S32 index);
    size_t posToIndex(S32 x, S32 y) const;
    void select(size_t emoji_idx);
    void selectNext();
    void selectPrevious();
    void updateConstraints();
    void updateScrollPos();

protected:
    const bool      mAutoSize;
    const bool      mNoScroll;
    const bool      mVertical;
    const size_t    mMaxVisible;
    const S32       mPadding;
    const LLUIImagePtr mSelectedImage;
    const LLFontGL* mIconFont;
    const LLFontGL* mTextFont;

    std::vector<LLEmojiSearchResult> mEmojis;
    LLScrollbar*    mScrollbar;
    LLRect          mRenderRect;
    U16             mEmojiWidth = 0;
    U16             mEmojiHeight = 0;
    size_t          mTotalEmojis = 0;
    size_t          mVisibleEmojis = 0;
    size_t          mFirstVisible = 0;
    size_t          mScrollPos = 0;
    size_t          mCurSelected = 0;
    LLVector2       mLastHover;
};

// ============================================================================
// LLFloaterEmojiComplete
//

class LLFloaterEmojiComplete : public LLFloater
{
public:
    LLFloaterEmojiComplete(const LLSD& sdKey);

public:
    bool handleKey(KEY key, MASK mask, bool called_from_parent) override;
    void onOpen(const LLSD& key) override;
    bool postBuild() override;
    void reshape(S32 width, S32 height, bool called_from_parent) override;

protected:
    LLPanelEmojiComplete* mEmojiCtrl = nullptr;
    S32 mEmojiCtrlHorz = 0;
    S32 mEmojiCtrlVert = 0;
};

// ============================================================================
