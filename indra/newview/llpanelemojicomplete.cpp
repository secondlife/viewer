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
#include "lluictrlfactory.h"

constexpr U32 MIN_MOUSE_MOVE_DELTA = 4;
constexpr U32 MIN_SHORT_CODE_WIDTH = 100;

// ============================================================================
// LLPanelEmojiComplete
//

static LLDefaultChildRegistry::Register<LLPanelEmojiComplete> r("emoji_complete");

LLPanelEmojiComplete::Params::Params()
	: autosize("autosize")
	, noscroll("noscroll")
	, vertical("vertical")
	, max_emoji("max_emoji")
	, padding("padding")
	, selected_image("selected_image")
{
}

LLPanelEmojiComplete::LLPanelEmojiComplete(const LLPanelEmojiComplete::Params& p)
	: LLUICtrl(p)
	, mAutoSize(p.autosize)
	, mNoScroll(p.noscroll)
	, mVertical(p.vertical)
	, mMaxVisible(p.max_emoji)
	, mPadding(p.padding)
	, mSelectedImage(p.selected_image)
	, mIconFont(LLFontGL::getFontEmojiHuge())
	, mTextFont(LLFontGL::getFontSansSerifBig())
{
}

LLPanelEmojiComplete::~LLPanelEmojiComplete()
{
}

void LLPanelEmojiComplete::draw()
{
    if (mEmojis.empty())
        return;

    const size_t firstVisibleIdx = mScrollPos;
    const size_t lastVisibleIdx = llmin(mScrollPos + mVisibleEmojis, mEmojis.size()) - 1;

    if (mCurSelected >= firstVisibleIdx && mCurSelected <= lastVisibleIdx)
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

    for (U32 curIdx = firstVisibleIdx; curIdx <= lastVisibleIdx; curIdx++)
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
	LLVector2 curHover(x, y);
	if ((mLastHover - curHover).lengthSquared() > MIN_MOUSE_MOVE_DELTA)
	{
		mCurSelected = posToIndex(x, y);
		mLastHover = curHover;
	}

	return TRUE;
}

BOOL LLPanelEmojiComplete::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	bool handled = false;
	if (MASK_NONE == mask)
	{
		switch (key)
		{
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
				if (!mEmojis.empty())
				{
					onCommit();
					handled = true;
				}
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
	mCurSelected = posToIndex(x, y);
	mLastHover = LLVector2(x, y);

	return TRUE;
}

BOOL LLPanelEmojiComplete::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mCurSelected = posToIndex(x, y);
	onCommit();

	return TRUE;
}

void LLPanelEmojiComplete::onCommit()
{
	if (mCurSelected < mEmojis.size())
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
	mCurSelected = 0;

	onEmojisChanged();
}

void LLPanelEmojiComplete::setEmojiHint(const std::string& hint)
{
	llwchar curEmoji = (mCurSelected < mEmojis.size()) ? mEmojis.at(mCurSelected) : 0;

	mEmojis = LLEmojiDictionary::instance().findMatchingEmojis(hint);
	size_t curEmojiIdx = curEmoji ? mEmojis.find(curEmoji) : std::string::npos;
	mCurSelected = (std::string::npos != curEmojiIdx) ? curEmojiIdx : 0;

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
		mVisibleEmojis = std::min(mEmojis.size(), mMaxVisible);
        if (mVertical)
        {
            U32 maxShortCodeWidth = getMaxShortCodeWidth();
            U32 shortCodeWidth = std::max(maxShortCodeWidth, MIN_SHORT_CODE_WIDTH);
            S32 width = mEmojiWidth + shortCodeWidth + mPadding * 2;
            reshape(width, mVisibleEmojis * mEmojiHeight, false);
        }
        else
        {
            S32 height = getRect().getHeight();
            reshape(mVisibleEmojis * mEmojiWidth, height, false);
        }
	}
	else
	{
		updateConstraints();
	}

	mScrollPos = llmin(mScrollPos, mEmojis.size());
}

size_t LLPanelEmojiComplete::posToIndex(S32 x, S32 y) const
{
	if (mRenderRect.pointInRect(x, y))
	{
		U32 pos = mVertical ? (U32)(mRenderRect.mTop - y) / mEmojiHeight : x / mEmojiWidth;
		return mScrollPos + llmin((size_t)pos, mEmojis.size() - 1);
	}
	return npos;
}

void LLPanelEmojiComplete::select(size_t emoji_idx)
{
	mCurSelected = llclamp<size_t>(emoji_idx, 0, mEmojis.size());
	updateScrollPos();
}

void LLPanelEmojiComplete::selectNext()
{
	select(mCurSelected + 1 < mEmojis.size() ? mCurSelected + 1 : 0);
}

void LLPanelEmojiComplete::selectPrevious()
{
	select(mCurSelected - 1 >= 0 ? mCurSelected - 1 : mEmojis.size() - 1);
}

void LLPanelEmojiComplete::updateConstraints()
{
    mRenderRect = getLocalRect();
    S32 ctrlWidth = mRenderRect.getWidth();
    S32 ctrlHeight = mRenderRect.getHeight();

    mEmojiHeight = mIconFont->getLineHeight() + mPadding * 2;
    mEmojiWidth = mIconFont->getWidthF32(u8"\U0001F431") + mPadding * 2;
    if (mVertical)
    {
        mVisibleEmojis = ctrlHeight / mEmojiHeight;
        mRenderRect.mBottom = mRenderRect.mTop - mVisibleEmojis * mEmojiHeight;
    }
    else
    {
        mVisibleEmojis = ctrlWidth / mEmojiWidth;
        S32 padding = (ctrlWidth - mVisibleEmojis * mEmojiWidth) / 2;
        mRenderRect.mLeft += padding;
        mRenderRect.mRight -= padding;
        if (mEmojiHeight > ctrlHeight)
        {
            mEmojiHeight = ctrlHeight;
        }
    }

    updateScrollPos();
}

void LLPanelEmojiComplete::updateScrollPos()
{
	const size_t cntEmoji = mEmojis.size();
	if (mNoScroll || 0 == cntEmoji || cntEmoji < mVisibleEmojis || 0 == mCurSelected)
	{
		mScrollPos = 0;
		if (mCurSelected >= mVisibleEmojis)
		{
			mCurSelected = mVisibleEmojis ? mVisibleEmojis - 1 : 0;
		}
	}
	else if (cntEmoji - 1 == mCurSelected)
	{
		mScrollPos = mCurSelected - mVisibleEmojis + 1;
	}
	else
	{
		mScrollPos = mCurSelected - ((float)mCurSelected / (cntEmoji - 2) * (mVisibleEmojis - 2));
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
		std::bind([&](const LLSD& sdValue)
		{
			setValue(sdValue);
			onCommit();
		}, std::placeholders::_2));
	mEmojiCtrlHorz = getRect().getWidth() - mEmojiCtrl->getRect().getWidth();

	return LLFloater::postBuild();
}

void LLFloaterEmojiComplete::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (!called_from_parent)
	{
		LLRect rctFloater = getRect(), rctCtrl = mEmojiCtrl->getRect();
		rctFloater.mRight = rctFloater.mLeft + rctCtrl.getWidth() + mEmojiCtrlHorz;
		setRect(rctFloater);

		return;
	}

	LLFloater::reshape(width, height, called_from_parent);
}

// ============================================================================
