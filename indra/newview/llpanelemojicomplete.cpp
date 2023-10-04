/**
* @file llpanelemojicomplete.h
* @brief Header file for LLPanelEmojiComplete
*
* $LicenseInfo:firstyear=2012&license=lgpl$
* Second Life Viewer Source Code
* Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llemojidictionary.h"
#include "llemojihelper.h"
#include "llpanelemojicomplete.h"
#include "llscrollbar.h"
#include "lluictrlfactory.h"

constexpr U32 MIN_MOUSE_MOVE_DELTA = 4;
constexpr U32 MIN_SHORT_CODE_WIDTH = 100;
constexpr U32 DEF_PADDING = 8;

// ============================================================================
// LLPanelEmojiComplete
//

static LLDefaultChildRegistry::Register<LLPanelEmojiComplete> r("emoji_complete");

LLPanelEmojiComplete::Params::Params()
    : autosize("autosize")
    , noscroll("noscroll")
    , vertical("vertical")
    , max_visible("max_visible")
    , padding("padding", DEF_PADDING)
    , selected_image("selected_image")
{
}

LLPanelEmojiComplete::LLPanelEmojiComplete(const LLPanelEmojiComplete::Params& p)
    : LLUICtrl(p)
    , mAutoSize(p.autosize)
    , mNoScroll(p.noscroll)
    , mVertical(p.vertical)
    , mMaxVisible(p.max_visible)
    , mPadding(p.padding)
    , mSelectedImage(p.selected_image)
    , mIconFont(LLFontGL::getFontEmojiHuge())
    , mTextFont(LLFontGL::getFontSansSerifBig())
    , mScrollbar(nullptr)
{
    if (mVertical)
    {
        LLScrollbar::Params sbparams;
        sbparams.orientation(LLScrollbar::VERTICAL);
        sbparams.change_callback([this](S32 index, LLScrollbar*) { onScrollbarChange(index); });
        mScrollbar = LLUICtrlFactory::create<LLScrollbar>(sbparams);
        addChild(mScrollbar);
    }
}

LLPanelEmojiComplete::~LLPanelEmojiComplete()
{
}

void LLPanelEmojiComplete::draw()
{
    LLUICtrl::draw();

    if (mEmojis.empty())
        return;

    const size_t firstVisibleIdx = mScrollPos;
    const size_t lastVisibleIdx = llmin(mScrollPos + mVisibleEmojis, mTotalEmojis);

    if (mCurSelected >= firstVisibleIdx && mCurSelected < lastVisibleIdx)
    {
        S32 x, y, width, height;
        if (mVertical)
        {
            x = mRenderRect.mLeft;
            y = mRenderRect.mTop - (mCurSelected - firstVisibleIdx + 1) * mEmojiHeight;
            width = mRenderRect.getWidth();
            height = mEmojiHeight;
        }
        else
        {
            x = mRenderRect.mLeft + (mCurSelected - firstVisibleIdx) * mEmojiWidth;
            y = mRenderRect.mBottom;
            width = mEmojiWidth;
            height = mRenderRect.getHeight();
        }
        mSelectedImage->draw(x, y, width, height);
    }

    S32 iconCenterX = mRenderRect.mLeft + mEmojiWidth / 2;
    S32 iconCenterY = mRenderRect.mTop - mEmojiHeight / 2;
    S32 textLeft = mVertical ? mRenderRect.mLeft + mEmojiWidth + mPadding : 0;
    S32 textWidth = mVertical ? getRect().getWidth() - textLeft - mPadding : 0;

    for (U32 curIdx = firstVisibleIdx; curIdx < lastVisibleIdx; curIdx++)
    {
        mIconFont->render(mEmojis, curIdx, iconCenterX, iconCenterY,
            LLColor4::white, LLFontGL::HCENTER, LLFontGL::VCENTER, LLFontGL::NORMAL,
            LLFontGL::DROP_SHADOW_SOFT, 1, S32_MAX, nullptr, false, true);
        if (mVertical)
        {
            llwchar emoji = mEmojis[curIdx];
            auto& emoji2descr = LLEmojiDictionary::instance().getEmoji2Descr();
            auto it = emoji2descr.find(emoji);
            if (it != emoji2descr.end())
            {
                const std::string& shortCode = it->second->ShortCodes.front();
                mTextFont->renderUTF8(shortCode, 0, textLeft, iconCenterY, LLColor4::white,
                    LLFontGL::LEFT, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
                    shortCode.size(), textWidth, NULL, FALSE, FALSE);
            }
            iconCenterY -= mEmojiHeight;
        }
        else
        {
            iconCenterX += mEmojiWidth;
        }
    }
}

BOOL LLPanelEmojiComplete::handleHover(S32 x, S32 y, MASK mask)
{
    if (mScrollbar && mScrollbar->getVisible() && childrenHandleHover(x, y, mask))
        return TRUE;

    LLVector2 curHover(x, y);
    if ((mLastHover - curHover).lengthSquared() > MIN_MOUSE_MOVE_DELTA)
    {
        size_t index = posToIndex(x, y);
        if (index < mTotalEmojis)
            mCurSelected = index;
        mLastHover = curHover;
    }

    return TRUE;
}

BOOL LLPanelEmojiComplete::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
    bool handled = false;
    if (mTotalEmojis && MASK_NONE == mask)
    {
        switch (key)
        {
        case KEY_HOME:
            select(0);
            handled = true;
            break;

        case KEY_END:
            select(mTotalEmojis - 1);
            handled = true;
            break;

        case KEY_PAGE_DOWN:
            select(mCurSelected + mVisibleEmojis - 1);
            handled = true;
            break;

        case KEY_PAGE_UP:
            select(mCurSelected - llmin(mCurSelected, mVisibleEmojis + 1));
            handled = true;
            break;

        case KEY_LEFT:
        case KEY_UP:
            selectPrevious();
            handled = true;
            break;

        case KEY_RIGHT:
        case KEY_DOWN:
            selectNext();
            handled = true;
            break;

        case KEY_RETURN:
            onCommit();
            handled = true;
            break;
        }
    }

    if (handled)
    {
        return TRUE;
    }

    return LLUICtrl::handleKey(key, mask, called_from_parent);
}

BOOL LLPanelEmojiComplete::handleMouseDown(S32 x, S32 y, MASK mask)
{
    if (mScrollbar && mScrollbar->getVisible() && childrenHandleMouseDown(x, y, mask))
        return TRUE;

    mCurSelected = posToIndex(x, y);
    mLastHover = LLVector2(x, y);

    return TRUE;
}

BOOL LLPanelEmojiComplete::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (mScrollbar && mScrollbar->getVisible() && childrenHandleMouseUp(x, y, mask))
        return TRUE;

    mCurSelected = posToIndex(x, y);
    onCommit();

    return TRUE;
}

BOOL LLPanelEmojiComplete::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    if (mNoScroll)
        return FALSE;

    if (mScrollbar && mScrollbar->getVisible() && mScrollbar->handleScrollWheel(x, y, clicks))
    {
        mCurSelected = posToIndex(x, y);
        return TRUE;
    }

    if (mTotalEmojis > mVisibleEmojis)
    {
        mScrollPos = llclamp<size_t>(mScrollPos + clicks, 0, mTotalEmojis - mVisibleEmojis);
        mCurSelected = posToIndex(x, y);
        return TRUE;
    }

    return FALSE;
}

void LLPanelEmojiComplete::onCommit()
{
    if (mCurSelected < mTotalEmojis)
    {
        LLWString wstr;
        wstr.push_back(mEmojis.at(mCurSelected));
        setValue(wstring_to_utf8str(wstr));
        LLUICtrl::onCommit();
    }
}

void LLPanelEmojiComplete::reshape(S32 width, S32 height, BOOL called_from_parent)
{
    LLUICtrl::reshape(width, height, called_from_parent);
    updateConstraints();
}

void LLPanelEmojiComplete::setEmojis(const LLWString& emojis)
{
    mEmojis = emojis;
    mTotalEmojis = mEmojis.size();
    mCurSelected = 0;

    onEmojisChanged();
}

void LLPanelEmojiComplete::setEmojiHint(const std::string& hint)
{
    llwchar curEmoji = mCurSelected < mTotalEmojis ? mEmojis.at(mCurSelected) : 0;

    mEmojis = LLEmojiDictionary::instance().findMatchingEmojis(hint);
    mTotalEmojis = mEmojis.size();
    size_t curEmojiIdx = curEmoji ? mEmojis.find(curEmoji) : std::string::npos;
    mCurSelected = std::string::npos != curEmojiIdx ? curEmojiIdx : 0;

    onEmojisChanged();
}

U32 LLPanelEmojiComplete::getMaxShortCodeWidth() const
{
    U32 max_width = 0;
    auto& emoji2descr = LLEmojiDictionary::instance().getEmoji2Descr();
    for (llwchar emoji : mEmojis)
    {
        auto it = emoji2descr.find(emoji);
        if (it != emoji2descr.end())
        {
            const std::string& shortCode = it->second->ShortCodes.front();
            S32 width = mTextFont->getWidth(shortCode);
            if (width > max_width)
            {
                max_width = width;
            }
        }
    }
    return max_width;
}

void LLPanelEmojiComplete::onEmojisChanged()
{
    if (mAutoSize)
    {
        S32 width, height;
        mVisibleEmojis = llmin(mTotalEmojis, mMaxVisible);
        if (mVertical)
        {
            U32 maxShortCodeWidth = getMaxShortCodeWidth();
            U32 shortCodeWidth = llmax(maxShortCodeWidth, MIN_SHORT_CODE_WIDTH);
            width = mEmojiWidth + shortCodeWidth + mPadding * 2;
            if (!mNoScroll && mVisibleEmojis < mTotalEmojis)
            {
                width += mScrollbar->getThickness();
            }
            height = mVisibleEmojis * mEmojiHeight;
        }
        else
        {
            width = mVisibleEmojis * mEmojiWidth;
            height = getRect().getHeight();
        }
        LLUICtrl::reshape(width, height, false);
    }

    updateConstraints();
}

void LLPanelEmojiComplete::onScrollbarChange(S32 index)
{
    mScrollPos = llclamp<size_t>(index, 0, mTotalEmojis - mVisibleEmojis);
}

size_t LLPanelEmojiComplete::posToIndex(S32 x, S32 y) const
{
    if (mRenderRect.pointInRect(x, y))
    {
        U32 pos = mVertical ? (U32)(mRenderRect.mTop - y) / mEmojiHeight : x / mEmojiWidth;
        return llmin(mScrollPos + pos, mTotalEmojis - 1);
    }
    return std::string::npos;
}

void LLPanelEmojiComplete::select(size_t emoji_idx)
{
    mCurSelected = llclamp<size_t>(emoji_idx, 0, mTotalEmojis - 1);

    updateScrollPos();
}

void LLPanelEmojiComplete::selectNext()
{
    if (!mTotalEmojis)
        return;

    mCurSelected = (mCurSelected < mTotalEmojis - 1) ? mCurSelected + 1 : 0;

    updateScrollPos();
}

void LLPanelEmojiComplete::selectPrevious()
{
    if (!mTotalEmojis)
        return;

    mCurSelected = (mCurSelected && mCurSelected < mTotalEmojis) ? mCurSelected - 1 : mTotalEmojis - 1;

    updateScrollPos();
}

void LLPanelEmojiComplete::updateConstraints()
{
    mRenderRect = getLocalRect();

    mEmojiWidth = mIconFont->getWidthF32(u8"\U0001F431") + mPadding * 2;
    if (mVertical)
    {
        mEmojiHeight = mIconFont->getLineHeight() + mPadding * 2;
        mRenderRect.mBottom = mRenderRect.mTop - mVisibleEmojis * mEmojiHeight;
        if (!mNoScroll && mVisibleEmojis < mTotalEmojis)
        {
            mRenderRect.mRight -= mScrollbar->getThickness();
            mScrollbar->setDocSize(mTotalEmojis);
            mScrollbar->setPageSize(mVisibleEmojis);
            mScrollbar->setOrigin(mRenderRect.mRight, 0);
            mScrollbar->reshape(mScrollbar->getThickness(), mRenderRect.mTop, TRUE);
            mScrollbar->setVisible(TRUE);
        }
        else
        {
            mScrollbar->setVisible(FALSE);
        }
    }
    else
    {
        mEmojiHeight = mRenderRect.getHeight();
        mRenderRect.stretch((mRenderRect.getWidth() - mVisibleEmojis * mEmojiWidth) / -2, 0);
    }

    updateScrollPos();
}

void LLPanelEmojiComplete::updateScrollPos()
{
    if (mNoScroll || 0 == mTotalEmojis || mTotalEmojis < mVisibleEmojis || 0 == mCurSelected)
    {
        mScrollPos = 0;
        if (mCurSelected >= mVisibleEmojis)
        {
            mCurSelected = mVisibleEmojis ? mVisibleEmojis - 1 : 0;
        }
    }
    else if (mTotalEmojis - 1 == mCurSelected)
    {
        mScrollPos = mTotalEmojis - mVisibleEmojis;
    }
    else
    {
        mScrollPos = mCurSelected - ((float)mCurSelected / (mTotalEmojis - 2) * (mVisibleEmojis - 2));
    }

    if (mScrollbar && mScrollbar->getVisible())
    {
        mScrollbar->setDocPos(mScrollPos);
    }
}

// ============================================================================
// LLFloaterEmojiComplete
//

LLFloaterEmojiComplete::LLFloaterEmojiComplete(const LLSD& sdKey)
    : LLFloater(sdKey)
{
    // This floater should hover on top of our dependent (with the dependent having the focus)
    setFocusStealsFrontmost(false);
    setAutoFocus(false);
    setBackgroundVisible(false);
    setIsChrome(true);
}

BOOL LLFloaterEmojiComplete::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
    bool handled = false;
    if (MASK_NONE == mask)
    {
        switch (key)
        {
            case KEY_ESCAPE:
                LLEmojiHelper::instance().hideHelper();
                handled = true;
                break;
        }
    }

    if (handled)
        return TRUE;

    return LLFloater::handleKey(key, mask, called_from_parent);
}

void LLFloaterEmojiComplete::onOpen(const LLSD& key)
{
    mEmojiCtrl->setEmojiHint(key["hint"].asString());
    if (0 == mEmojiCtrl->getEmojiCount())
    {
        LLEmojiHelper::instance().hideHelper();
        return;
    }

    if (mEmojiCtrl->isAutoSize())
    {
        LLRect outer_rect = getRect();
        const LLRect& inner_rect = mEmojiCtrl->getRect();
        outer_rect.mTop = outer_rect.mBottom + inner_rect.mBottom * 2 + inner_rect.getHeight();
        outer_rect.mRight = outer_rect.mLeft + inner_rect.mLeft * 2 + inner_rect.getWidth();
        setRect(outer_rect);
    }

    gFloaterView->adjustToFitScreen(this, FALSE);
}

BOOL LLFloaterEmojiComplete::postBuild()
{
    mEmojiCtrl = findChild<LLPanelEmojiComplete>("emoji_complete_ctrl");
    mEmojiCtrl->setCommitCallback(
        [this](LLUICtrl* ctrl, const LLSD& param)
        {
            setValue(param);
            onCommit();
        });

    mEmojiCtrlHorz = getRect().getWidth() - mEmojiCtrl->getRect().getWidth();
    mEmojiCtrlVert = getRect().getHeight() - mEmojiCtrl->getRect().getHeight();

    return LLFloater::postBuild();
}

void LLFloaterEmojiComplete::reshape(S32 width, S32 height, BOOL called_from_parent)
{
    if (called_from_parent)
    {
        LLFloater::reshape(width, height, called_from_parent);
    }
    else
    {
        LLRect outer(getRect()), inner(mEmojiCtrl->getRect());
        outer.mRight = outer.mLeft + inner.getWidth() + mEmojiCtrlHorz;
        outer.mTop = outer.mBottom + inner.getHeight() + mEmojiCtrlVert;
        setRect(outer);
    }
}

// ============================================================================
