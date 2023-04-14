/**
 * @file llfloateremojipicker.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloateremojipicker.h"

#include "llfloaterreg.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llemojidictionary.h"

#pragma optimize ("", off)

class LLEmojiScrollListItem : public LLScrollListItem
{
public:
	LLEmojiScrollListItem(const llwchar emoji, const LLScrollListItem::Params& params)
		: LLScrollListItem(params)
		, mEmoji(emoji)
	{
	}

	llwchar getEmoji() const { return mEmoji; }

	virtual void draw(const LLRect& rect,
		const LLColor4& fg_color,
		const LLColor4& hover_color, // highlight/hover selection of whole item or cell
		const LLColor4& select_color, // highlight/hover selection of whole item or cell
		const LLColor4& highlight_color, // highlights contents of cells (ex: text)
		S32 column_padding) override
	{
		LLScrollListItem::draw(rect, fg_color, hover_color, select_color, highlight_color, column_padding);

		S32 width = getColumn(0)->getWidth();
		LLFontGL::getFontEmoji()->render(LLWString(1, mEmoji), 0, rect.mLeft + width / 2, rect.getCenterY(), LLColor4::white,
			LLFontGL::HCENTER, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW_SOFT, 1, S32_MAX, nullptr, false, true);
	}

private:
	llwchar mEmoji;
};

LLFloaterEmojiPicker* LLFloaterEmojiPicker::getInstance()
{
	LLFloaterEmojiPicker* floater = LLFloaterReg::getTypedInstance<LLFloaterEmojiPicker>("emoji_picker");
	if (!floater)
		LL_WARNS() << "Cannot instantiate emoji picker" << LL_ENDL;
	return floater;
}

LLFloaterEmojiPicker* LLFloaterEmojiPicker::showInstance(select_callback_t callback)
{
	LLFloaterEmojiPicker* floater = getInstance();
	if (LLFloaterEmojiPicker* floater = getInstance())
		floater->show(callback);
	return floater;
}

void LLFloaterEmojiPicker::show(select_callback_t callback)
{
	mSelectCallback = callback;
	openFloater(mKey);
	setFocus(TRUE);
}

LLFloaterEmojiPicker::LLFloaterEmojiPicker(const LLSD& key)
: LLFloater(key)
{
}

BOOL LLFloaterEmojiPicker::postBuild()
{
	if (mEmojis = getChild<LLScrollListCtrl>("Emojis"))
	{
		mEmojis->setDoubleClickCallback(boost::bind(&LLFloaterEmojiPicker::onSelect, this));

		mEmojis->clearRows();

		const std::map<llwchar, const LLEmojiDescriptor*>& emoji2Descr = LLEmojiDictionary::instance().getEmoji2Descr();
		for (const std::pair<llwchar, const LLEmojiDescriptor*>& it : emoji2Descr)
		{
			LLScrollListItem::Params params;
			params.columns.add().column("name").value(it.second->Name);
			mEmojis->addRow(new LLEmojiScrollListItem(it.first, params), params);
		}
	}

	return TRUE;
}

LLFloaterEmojiPicker::~LLFloaterEmojiPicker()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}

void LLFloaterEmojiPicker::onSelect()
{
	if (mEmojis && mSelectCallback)
	{
		if (LLEmojiScrollListItem* item = dynamic_cast<LLEmojiScrollListItem*>(mEmojis->getFirstSelected()))
		{
			mSelectCallback(item->getEmoji());
		}
	}
}

// virtual
BOOL LLFloaterEmojiPicker::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		onSelect();
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		closeFloater();
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}
