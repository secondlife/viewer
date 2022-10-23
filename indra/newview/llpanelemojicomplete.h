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

#include "llfloater.h"
#include "lluictrl.h"

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
		Optional<S32>        max_emoji,
		                     padding;

		Optional<LLUIImage*> selected_image;

		Params();
	};

protected:
	LLPanelEmojiComplete(const LLPanelEmojiComplete::Params&);
public:
	virtual ~LLPanelEmojiComplete();

	void draw() override;
	BOOL handleHover(S32 x, S32 y, MASK mask) override;
	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent) override;
	void reshape(S32 width, S32 height, BOOL called_from_parent) override;

public:
	void setEmojiHint(const std::string& hint);
protected:
	size_t posToIndex(S32 x, S32 y) const;
	void select(size_t emoji_idx);
	void selectNext();
	void selectPrevious();
	void setFont(const LLFontGL* fontp);
	void updateConstraints();
	void updateScrollPos();

protected:
	static constexpr auto npos = std::numeric_limits<size_t>::max();

	bool            mAutoSize = false;
	const LLFontGL* mFont;
	U16             mEmojiWidth = 0;
	size_t          mMaxVisible = 0;
	S32             mPadding = 8;
	LLRect          mRenderRect;
	LLUIImagePtr	mSelectedImage;

	LLWString       mEmojis;
	U16             mVisibleEmojis = 0;
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
	void onOpen(const LLSD& key) override;
	BOOL postBuild() override;
	void reshape(S32 width, S32 height, BOOL called_from_parent) override;

protected:
	LLPanelEmojiComplete* mEmojiCtrl = nullptr;
	S32                   mEmojiCtrlHorz = 0;
};

// ============================================================================
