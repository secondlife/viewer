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
        sbparams.doc_size((S32)mTotalEmojis);
        sbparams.doc_pos(0);
        sbparams.page_size((S32)mVisibleEmojis);
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

    if (!mTotalEmojis)
        return;

    const size_t firstVisibleIdx = mScrollPos;
    const size_t lastVisibleIdx = llmin(mScrollPos + mVisibleEmojis, mTotalEmojis);

    if (mCurSelected >= firstVisibleIdx && mCurSelected < lastVisibleIdx)
    {
        S32 x, y, width, height;
        if (mVertical)
        {
            x = mRenderRect.mLeft;
            y = mRenderRect.mTop - static_cast<S32>(mCurSelected - firstVisibleIdx + 1) * mEmojiHeight;
            width = mRenderRect.getWidth();
            height = mEmojiHeight;
        }
        else
        {
            x = mRenderRect.mLeft + static_cast<S32>(mCurSelected - firstVisibleIdx) * mEmojiWidth;
            y = mRenderRect.mBottom;
            width = mEmojiWidth;
            height = mRenderRect.getHeight();
        }
        mSelectedImage->draw(x, y, width, height);
    }

    F32 iconCenterX = mRenderRect.mLeft + (F32)mEmojiWidth / 2;
    F32 iconCenterY = mRenderRect.mTop - (F32)mEmojiHeight / 2;
    F32 textLeft = mVertical ? (F32)(mRenderRect.mLeft + mEmojiWidth + mPadding) : 0.f;
    F32 textWidth = mVertical ? (F32)(getRect().getWidth() - textLeft - mPadding) : 0.f;

    for (size_t curIdx = firstVisibleIdx; curIdx < lastVisibleIdx; curIdx++)
    {
        LLWString text(1, mEmojis[curIdx].Character);
        mIconFont->render(text, 0, iconCenterX, iconCenterY,
            LLColor4::white, LLFontGL::HCENTER, LLFontGL::VCENTER, LLFontGL::NORMAL,
            LLFontGL::DROP_SHADOW_SOFT, 1);
        if (mVertical)
        {
            const std::string& shortCode = mEmojis[curIdx].String;
            F32 x0 = textLeft;
            F32 x1 = textWidth;
            if (mEmojis[curIdx].Begin)
            {
                std::string text = shortCode.substr(0, mEmojis[curIdx].Begin);
                mTextFont->renderUTF8(text, 0, x0, iconCenterY, LLColor4::white,
                    LLFontGL::LEFT, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
                    static_cast<S32>(text.size()), (S32)x1);
                x0 += mTextFont->getWidthF32(text);
                x1 = textLeft + textWidth - x0;
            }
            if (x1 > 0 && mEmojis[curIdx].End > mEmojis[curIdx].Begin)
            {
                std::string text = shortCode.substr(mEmojis[curIdx].Begin, mEmojis[curIdx].End - mEmojis[curIdx].Begin);
                mTextFont->renderUTF8(text, 0, x0, iconCenterY, LLColor4::yellow6,
                    LLFontGL::LEFT, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
                    static_cast<S32>(text.size()), (S32)x1);
                x0 += mTextFont->getWidthF32(text);
                x1 = textLeft + textWidth - x0;
            }
            if (x1 > 0 && mEmojis[curIdx].End < shortCode.size())
            {
                std::string text = shortCode.substr(mEmojis[curIdx].End);
                mTextFont->renderUTF8(text, 0, x0, iconCenterY, LLColor4::white,
                    LLFontGL::LEFT, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
                    static_cast<S32>(text.size()), (S32)x1);
            }
            iconCenterY -= mEmojiHeight;
        }
        else
        {
            iconCenterX += mEmojiWidth;
        }
    }
}

bool LLPanelEmojiComplete::handleHover(S32 x, S32 y, MASK mask)
{
    if (mScrollbar && mScrollbar->getVisible() && childrenHandleHover(x, y, mask))
        return true;

    LLVector2 curHover((F32)x, (F32)y);
    if ((mLastHover - curHover).lengthSquared() > MIN_MOUSE_MOVE_DELTA)
    {
        size_t index = posToIndex(x, y);
        if (index < mTotalEmojis)
            mCurSelected = index;
        mLastHover = curHover;
    }

    return true;
}

bool LLPanelEmojiComplete::handleKey(KEY key, MASK mask, bool called_from_parent)
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
        return true;
    }

    return LLUICtrl::handleKey(key, mask, called_from_parent);
}

bool LLPanelEmojiComplete::handleMouseDown(S32 x, S32 y, MASK mask)
{
    if (mScrollbar && mScrollbar->getVisible() && childrenHandleMouseDown(x, y, mask))
        return true;

    mCurSelected = posToIndex(x, y);
    mLastHover = LLVector2((F32)x, (F32)y);

    return true;
}

bool LLPanelEmojiComplete::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (mScrollbar && mScrollbar->getVisible() && childrenHandleMouseUp(x, y, mask))
        return true;

    mCurSelected = posToIndex(x, y);
    onCommit();

    return true;
}

bool LLPanelEmojiComplete::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    if (mNoScroll)
        return false;

    if (mScrollbar && mScrollbar->getVisible() && mScrollbar->handleScrollWheel(x, y, clicks))
    {
        mCurSelected = posToIndex(x, y);
        return true;
    }

    if (mTotalEmojis > mVisibleEmojis)
    {
        // In case of wheel up (clicks < 0) we shouldn't subtract more than value of mScrollPos
        // Example: if mScrollPos = 0, clicks = -1 then (mScrollPos + clicks) becomes SIZE_MAX
        // As a result of llclamp<size_t>() mScrollPos becomes (mTotalEmojis - mVisibleEmojis)
        S32 newScrollPos = llmax(0, (S32)mScrollPos + clicks);
        mScrollPos = llclamp<size_t>((size_t)newScrollPos, 0, mTotalEmojis - mVisibleEmojis);
        mCurSelected = posToIndex(x, y);
        return true;
    }

    return false;
}

void LLPanelEmojiComplete::onCommit()
{
    if (mCurSelected < mTotalEmojis)
    {
        setValue(ll_convert_to<std::string>(mEmojis[mCurSelected].Character));
        LLUICtrl::onCommit();
    }
}

void LLPanelEmojiComplete::reshape(S32 width, S32 height, bool called_from_parent)
{
    LLUICtrl::reshape(width, height, called_from_parent);
    if (mAutoSize)
    {
        updateConstraints();
    }
    else
    {
        onEmojisChanged();
    }
}

void LLPanelEmojiComplete::setEmojis(const LLWString& emojis)
{
    mEmojis.clear();

    auto& emoji2descr = LLEmojiDictionary::instance().getEmoji2Descr();
    for (const llwchar& emoji : emojis)
    {
        std::string shortCode;
        if (mVertical)
        {
            auto it = emoji2descr.find(emoji);
            if (it != emoji2descr.end() && !it->second->ShortCodes.empty())
            {
                shortCode = it->second->ShortCodes.front();
            }
        }
        mEmojis.emplace_back(emoji, shortCode, 0, 0);
    }

    mTotalEmojis = mEmojis.size();
    mCurSelected = 0;

    onEmojisChanged();
}

void LLPanelEmojiComplete::setEmojiHint(const std::string& hint)
{
    llwchar curEmoji = mCurSelected < mTotalEmojis ? mEmojis[mCurSelected].Character : 0;

    LLEmojiDictionary::instance().findByShortCode(mEmojis, hint);
    mTotalEmojis = mEmojis.size();

    mCurSelected = 0;
    for (size_t i = 1; i < mTotalEmojis; ++i)
    {
        if (mEmojis[i].Character == curEmoji)
        {
            mCurSelected = i;
            break;
        }
    }

    onEmojisChanged();
}

U32 LLPanelEmojiComplete::getMaxShortCodeWidth() const
{
    U32 max_width = 0;
    for (const LLEmojiSearchResult& result : mEmojis)
    {
        U32 width = mTextFont->getWidth(result.String);
        if (width > max_width)
        {
            max_width = width;
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
            height = static_cast<S32>(mVisibleEmojis) * mEmojiHeight;
        }
        else
        {
            width = static_cast<S32>(mVisibleEmojis) * mEmojiWidth;
            height = getRect().getHeight();
        }
        LLUICtrl::reshape(width, height, false);
    }
    else
    {
        mVisibleEmojis = mVertical ?
            mEmojiHeight ? getRect().getHeight() / mEmojiHeight : 0 :
            mEmojiWidth ? getRect().getWidth() / mEmojiWidth : 0;
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

    mEmojiWidth = (U16)(mIconFont->getWidthF32(LLWString(1, 0x1F431).c_str()) + mPadding * 2);
    if (mVertical)
    {
        mEmojiHeight = mIconFont->getLineHeight() + mPadding * 2;
        if (!mNoScroll && mVisibleEmojis < mTotalEmojis)
        {
            mRenderRect.mRight -= mScrollbar->getThickness();
            mScrollbar->setDocSize(static_cast<S32>(mTotalEmojis));
            mScrollbar->setPageSize(static_cast<S32>(mVisibleEmojis));
            mScrollbar->setOrigin(mRenderRect.mRight, 0);
            mScrollbar->reshape(mScrollbar->getThickness(), mRenderRect.mTop, true);
            mScrollbar->setVisible(true);
        }
        else
        {
            mScrollbar->setVisible(false);
        }
    }
    else
    {
        mEmojiHeight = mRenderRect.getHeight();
        mRenderRect.stretch((mRenderRect.getWidth() - static_cast<S32>(mVisibleEmojis) * mEmojiWidth) / -2, 0);
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
        mScrollPos = (size_t)(mCurSelected - ((float)mCurSelected / (mTotalEmojis - 2) * (mVisibleEmojis - 2)));
    }

    if (mScrollbar && mScrollbar->getVisible())
    {
        mScrollbar->setDocPos(static_cast<S32>(mScrollPos));
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

bool LLFloaterEmojiComplete::handleKey(KEY key, MASK mask, bool called_from_parent)
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
        return true;

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

    gFloaterView->adjustToFitScreen(this, false);
}

bool LLFloaterEmojiComplete::postBuild()
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

void LLFloaterEmojiComplete::reshape(S32 width, S32 height, bool called_from_parent)
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
