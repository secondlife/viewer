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

class LLFloaterEmojiPicker : public LLFloater
{
public:
	// The callback function will be called with an emoji char.
	typedef boost::function<void (llwchar)> select_callback_t;

	// Call this to select an emoji.
	static LLFloaterEmojiPicker* getInstance();
	static LLFloaterEmojiPicker* showInstance(select_callback_t callback);

	LLFloaterEmojiPicker(const LLSD& key);
	virtual ~LLFloaterEmojiPicker();

	virtual	BOOL postBuild();

	void show(select_callback_t callback);

private:
	void onSelect();

	virtual BOOL handleKeyHere(KEY key, MASK mask);

	class LLScrollListCtrl* mEmojis;
	select_callback_t mSelectCallback;
	std::string mEmojiName;
};

#endif
