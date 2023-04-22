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

#include "llcombobox.h"
#include "llemojidictionary.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lltextbox.h" 
#include "llviewerchat.h" 

std::string LLFloaterEmojiPicker::mSelectedCategory;
std::string LLFloaterEmojiPicker::mSearchPattern;
int LLFloaterEmojiPicker::mSelectedEmojiIndex;

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

LLFloaterEmojiPicker* LLFloaterEmojiPicker::showInstance(pick_callback_t pick_callback, close_callback_t close_callback)
{
	LLFloaterEmojiPicker* floater = getInstance();
	if (floater)
		floater->show(pick_callback, close_callback);
	return floater;
}

void LLFloaterEmojiPicker::show(pick_callback_t pick_callback, close_callback_t close_callback)
{
	mEmojiPickCallback = pick_callback;
	mFloaterCloseCallback = close_callback;
	openFloater(mKey);
	setFocus(TRUE);
}

LLFloaterEmojiPicker::LLFloaterEmojiPicker(const LLSD& key)
: LLFloater(key)
{
}

BOOL LLFloaterEmojiPicker::postBuild()
{
	// Should be initialized first
	mPreviewEmoji = getChild<LLButton>("PreviewEmoji");
	mPreviewEmoji->setClickedCallback(boost::bind(&LLFloaterEmojiPicker::onPreviewEmojiClick, this));

	mCategory = getChild<LLComboBox>("Category");
	mCategory->setCommitCallback(boost::bind(&LLFloaterEmojiPicker::onCategoryCommit, this));
	const auto& cat2Descrs = LLEmojiDictionary::instance().getCategory2Descrs();
	mCategory->clearRows();
	for (const auto& item : cat2Descrs)
	{
		std::string value = item.first;
		std::string name = value;
		LLStringUtil::capitalize(name);
		mCategory->add(name, value);
	}
	mCategory->setSelectedByValue(mSelectedCategory, true);

	mSearch = getChild<LLLineEditor>("Search");
	mSearch->setKeystrokeCallback(boost::bind(&LLFloaterEmojiPicker::onSearchKeystroke, this, _1, _2), NULL);
	mSearch->setFont(LLViewerChat::getChatFont());
	mSearch->setText(mSearchPattern);

	mEmojis = getChild<LLScrollListCtrl>("Emojis");
	mEmojis->setCommitCallback(boost::bind(&LLFloaterEmojiPicker::onEmojiSelect, this));
	mEmojis->setDoubleClickCallback(boost::bind(&LLFloaterEmojiPicker::onEmojiPick, this));
	fillEmojis();

	return TRUE;
}

LLFloaterEmojiPicker::~LLFloaterEmojiPicker()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}

void LLFloaterEmojiPicker::fillEmojis()
{
	mEmojis->clearRows();

	const auto& emoji2Descr = LLEmojiDictionary::instance().getEmoji2Descr();
	for (const std::pair<const llwchar, const LLEmojiDescriptor*>& it : emoji2Descr)
	{
		const LLEmojiDescriptor* descr = it.second;

		if (!mSelectedCategory.empty() && !matchesCategory(descr))
			continue;

		if (!mSearchPattern.empty() && !matchesPattern(descr))
			continue;

		LLScrollListItem::Params params;
		params.columns.add().column("name").value(descr->Name);
		mEmojis->addRow(new LLEmojiScrollListItem(it.first, params), params);
	}

	if (mEmojis->getItemCount())
	{
		if (mSelectedEmojiIndex > 0 && mSelectedEmojiIndex < mEmojis->getItemCount())
			mEmojis->selectNthItem(mSelectedEmojiIndex);
		else
			mEmojis->selectFirstItem();

		mEmojis->scrollToShowSelected();
	}
	else
	{
		onEmojiEmpty();
	}
}

bool LLFloaterEmojiPicker::matchesCategory(const LLEmojiDescriptor* descr)
{
	return std::find(descr->Categories.begin(), descr->Categories.end(), mSelectedCategory) != descr->Categories.end();
}

bool LLFloaterEmojiPicker::matchesPattern(const LLEmojiDescriptor* descr)
{
	if (descr->Name.find(mSearchPattern) != std::string::npos)
		return true;
	for (auto shortCode : descr->ShortCodes)
		if (shortCode.find(mSearchPattern) != std::string::npos)
			return true;
	for (auto category : descr->Categories)
		if (category.find(mSearchPattern) != std::string::npos)
			return true;
	return false;
}

void LLFloaterEmojiPicker::onCategoryCommit()
{
	mSelectedCategory = mCategory->getSelectedValue().asString();
	mSelectedEmojiIndex = 0;
	fillEmojis();
}

void LLFloaterEmojiPicker::onSearchKeystroke(LLLineEditor* caller, void* user_data)
{
	mSearchPattern = mSearch->getText();
	mSelectedEmojiIndex = 0;
	fillEmojis();
}

void LLFloaterEmojiPicker::onPreviewEmojiClick()
{
	if (mEmojis && mEmojiPickCallback)
	{
		if (LLEmojiScrollListItem* item = dynamic_cast<LLEmojiScrollListItem*>(mEmojis->getFirstSelected()))
		{
			mEmojiPickCallback(item->getEmoji());
		}
	}
}

void LLFloaterEmojiPicker::onEmojiSelect()
{
	const LLEmojiScrollListItem* item = dynamic_cast<LLEmojiScrollListItem*>(mEmojis->getFirstSelected());
	if (item)
	{
		mSelectedEmojiIndex = mEmojis->getFirstSelectedIndex();
		LLUIString text;
		text.insert(0, LLWString(1, item->getEmoji()));
		if (mPreviewEmoji)
			mPreviewEmoji->setLabel(text);
		return;
	}

	onEmojiEmpty();
}

void LLFloaterEmojiPicker::onEmojiEmpty()
{
	mSelectedEmojiIndex = 0;
	if (mPreviewEmoji)
		mPreviewEmoji->setLabel(LLUIString());
}

void LLFloaterEmojiPicker::onEmojiPick()
{
	if (mEmojis && mEmojiPickCallback)
	{
		if (LLEmojiScrollListItem* item = dynamic_cast<LLEmojiScrollListItem*>(mEmojis->getFirstSelected()))
		{
			mEmojiPickCallback(item->getEmoji());
			closeFloater();
		}
	}
}

// virtual
BOOL LLFloaterEmojiPicker::handleKeyHere(KEY key, MASK mask)
{
	if (mask == MASK_NONE)
	{
		switch (key)
		{
		case KEY_RETURN:
			if (mCategory->hasFocus())
				break;
			onEmojiPick();
			return TRUE;
		case KEY_ESCAPE:
			closeFloater();
			return TRUE;
		case KEY_UP:
			mEmojis->selectPrevItem();
			mEmojis->scrollToShowSelected();
			return TRUE;
		case KEY_DOWN:
			mEmojis->selectNextItem();
			mEmojis->scrollToShowSelected();
			return TRUE;
		}
	}

	return LLFloater::handleKeyHere(key, mask);
}

// virtual
void LLFloaterEmojiPicker::closeFloater(bool app_quitting)
{
	LLFloater::closeFloater(app_quitting);
	if (mFloaterCloseCallback)
		mFloaterCloseCallback();
}
