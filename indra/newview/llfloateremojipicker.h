/**
 * @file llfloateremojipicker.h
 * @brief Header file for llfloateremojipicker
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLFLOATEREMOJIPICKER_H
#define LLFLOATEREMOJIPICKER_H

#include "llfloater.h"

struct LLEmojiDescriptor;

class LLFloaterEmojiPicker : public LLFloater
{
	using super = LLFloater;

public:
	// The callback function will be called with an emoji char.
	typedef boost::function<void (llwchar)> pick_callback_t;
	typedef boost::function<void ()> close_callback_t;

	// Call this to select an emoji.
	static LLFloaterEmojiPicker* getInstance();
	static LLFloaterEmojiPicker* showInstance(pick_callback_t pick_callback = nullptr, close_callback_t close_callback = nullptr);

	LLFloaterEmojiPicker(const LLSD& key);
	virtual ~LLFloaterEmojiPicker();

	virtual	BOOL postBuild() override;
	virtual void dirtyRect() override;

	void show(pick_callback_t pick_callback = nullptr, close_callback_t close_callback = nullptr);

	virtual void closeFloater(bool app_quitting = false) override;

private:
	void fillEmojis();
	void fillEmojiList();
	void fillEmojiGrid();

	bool matchesCategory(const LLEmojiDescriptor* descr);
	bool matchesPattern(const LLEmojiDescriptor* descr);

	void onCategoryCommit();
	void onSearchKeystroke();
	void onPreviewEmojiClick();
	void onGridMouseEnter();
	void onGridMouseLeave();
	void onEmojiMouseEnter(LLUICtrl* ctrl);
	void onEmojiMouseLeave(LLUICtrl* ctrl);
	void onEmojiMouseClick(LLUICtrl* ctrl, MASK mask);
	void onEmojiSelect();
	void onEmojiEmpty();
	void onEmojiPick();

	void selectGridIcon(LLUICtrl* ctrl);
	void unselectGridIcon(LLUICtrl* ctrl);

	virtual BOOL handleKeyHere(KEY key, MASK mask) override;

	class LLComboBox* mCategory { nullptr };
	class LLLineEditor* mSearch { nullptr };
	class LLScrollListCtrl* mEmojiList { nullptr };
	class LLScrollContainer* mEmojiScroll { nullptr };
	class LLScrollingPanelList* mEmojiGrid { nullptr };
	class LLButton* mPreviewEmoji { nullptr };
	class LLTextBox* mDescription { nullptr };

	pick_callback_t mEmojiPickCallback;
	close_callback_t mFloaterCloseCallback;

	S32 mRecentGridWidth { 0 };
	LLUICtrl* mHoveredIcon { nullptr };

	static std::string mSelectedCategory;
	static std::string mSearchPattern;
	static int mSelectedEmojiIndex;
	static bool mUseGrid;
};

#endif
