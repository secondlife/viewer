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
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llscrollcontainer.h"
#include "llscrollingpanellist.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lltextbox.h" 
#include "llviewerchat.h" 

std::string LLFloaterEmojiPicker::mSelectedCategory;
std::string LLFloaterEmojiPicker::mSearchPattern;

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

		LLWString wstr(1, mEmoji);
		S32 width = getColumn(0)->getWidth();
		F32 x = rect.mLeft + width / 2;
		F32 y = rect.getCenterY();
		LLFontGL::getFontEmoji()->render(
			wstr,                       // wstr
			0,                          // begin_offset
			x,                          // x
			y,                          // y
			LLColor4::white,            // color
			LLFontGL::HCENTER,          // halign
			LLFontGL::VCENTER,          // valign
			LLFontGL::NORMAL,           // style
			LLFontGL::DROP_SHADOW_SOFT, // shadow
			1,                          // max_chars
			S32_MAX,                    // max_pixels
			nullptr,                    // right_x
			false,                      // use_ellipses
			true);                      // use_color
	}

private:
	llwchar mEmoji;
};

class LLEmojiGridRow : public LLScrollingPanel
{
public:
	LLEmojiGridRow(const LLPanel::Params& panel_params,
		const LLScrollingPanelList::Params& list_params)
		: LLScrollingPanel(panel_params)
		, mList(new LLScrollingPanelList(list_params))
	{
		addChild(mList);
	}

	virtual void updatePanel(BOOL allow_modify) override {}

public:
	LLScrollingPanelList* mList;
};

class LLEmojiGridDivider : public LLScrollingPanel
{
public:
	LLEmojiGridDivider(const LLPanel::Params& panel_params, std::string text)
		: LLScrollingPanel(panel_params)
		, mText(utf8string_to_wstring(text))
	{
	}

	virtual void draw() override
	{
		LLScrollingPanel::draw();

		F32 x = 4; // padding-left
		F32 y = getRect().getHeight() / 2;
		LLFontGL::getFontSansSerifBold()->render(
			mText,                      // wstr
			0,                          // begin_offset
			x,                          // x
			y,                          // y
			LLColor4::white,            // color
			LLFontGL::LEFT,             // halign
			LLFontGL::VCENTER,          // valign
			LLFontGL::NORMAL,           // style
			LLFontGL::DROP_SHADOW_SOFT, // shadow
			mText.size(),               // max_chars
			S32_MAX,                    // max_pixels
			nullptr,                    // right_x
			false,                      // use_ellipses
			true);                      // use_color
	}

	virtual void updatePanel(BOOL allow_modify) override {}

private:
	const LLWString mText;
};

class LLEmojiGridIcon : public LLScrollingPanel
{
public:
	LLEmojiGridIcon(const LLPanel::Params& panel_params, const LLEmojiDescriptor* descr, std::string category)
		: LLScrollingPanel(panel_params)
		, mEmoji(descr->Character)
		, mText(LLWString(1, mEmoji))
		, mDescr(descr->Name)
		, mCategory(category)
	{
	}

	virtual void draw() override
	{
		LLScrollingPanel::draw();

		F32 x = getRect().getWidth() / 2;
		F32 y = getRect().getHeight() / 2;
		LLFontGL::getFontEmoji()->render(
			mText,                      // wstr
			0,                          // begin_offset
			x,                          // x
			y,                          // y
			LLColor4::white,            // color
			LLFontGL::HCENTER,          // halign
			LLFontGL::VCENTER,          // valign
			LLFontGL::NORMAL,           // style
			LLFontGL::DROP_SHADOW_SOFT, // shadow
			1,                          // max_chars
			S32_MAX,                    // max_pixels
			nullptr,                    // right_x
			false,                      // use_ellipses
			true);                      // use_color
	}

	virtual void updatePanel(BOOL allow_modify) override {}

	llwchar getEmoji() const { return mEmoji; }
	LLWString getText() const { return mText; }
	std::string getDescr() const { return mDescr; }
	std::string getCategory() const { return mCategory; }

private:
	const llwchar mEmoji;
	const LLWString mText;
	const std::string mDescr;
	const std::string mCategory;
};

LLFloaterEmojiPicker* LLFloaterEmojiPicker::getInstance()
{
	LLFloaterEmojiPicker* floater = LLFloaterReg::getTypedInstance<LLFloaterEmojiPicker>("emoji_picker");
	if (!floater)
		LL_ERRS() << "Cannot instantiate emoji picker" << LL_ENDL;
	return floater;
}

LLFloaterEmojiPicker* LLFloaterEmojiPicker::showInstance(pick_callback_t pick_callback, close_callback_t close_callback)
{
	LLFloaterEmojiPicker* floater = getInstance();
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
	mPreviewEmoji->setClickedCallback([this](LLUICtrl*, const LLSD&) { onPreviewEmojiClick(); });

	mDescription = getChild<LLTextBox>("Description");
	mDescription->setVisible(FALSE);

	mCategory = getChild<LLComboBox>("Category");
	mCategory->setCommitCallback([this](LLUICtrl*, const LLSD&) { onCategoryCommit(); });
	const LLEmojiDictionary::cat2descrs_map_t& cat2Descrs = LLEmojiDictionary::instance().getCategory2Descrs();
	mCategory->clearRows();
	for (const LLEmojiDictionary::cat2descrs_item_t& item : cat2Descrs)
	{
		std::string value = item.first;
		std::string name = value;
		LLStringUtil::capitalize(name);
		mCategory->add(name, value);
	}
	mCategory->setSelectedByValue(mSelectedCategory, true);

	mSearch = getChild<LLLineEditor>("Search");
	mSearch->setKeystrokeCallback([this](LLLineEditor*, void*) { onSearchKeystroke(); }, NULL);
	mSearch->setFont(LLViewerChat::getChatFont());
	mSearch->setText(mSearchPattern);

	mEmojiScroll = getChild<LLScrollContainer>("EmojiGridContainer");
	mEmojiScroll->setMouseEnterCallback([this](LLUICtrl*, const LLSD&) { onGridMouseEnter(); });
	mEmojiScroll->setMouseLeaveCallback([this](LLUICtrl*, const LLSD&) { onGridMouseLeave(); });

	mEmojiGrid = getChild<LLScrollingPanelList>("EmojiGrid");

	fillEmojiGrid();

	return TRUE;
}

void LLFloaterEmojiPicker::dirtyRect()
{
	super::dirtyRect();

	if (mEmojiScroll && mEmojiScroll->getRect().getWidth() != mRecentGridWidth)
	{
		fillEmojiGrid();
	}
}

LLFloaterEmojiPicker::~LLFloaterEmojiPicker()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}

void LLFloaterEmojiPicker::fillEmojiGrid()
{
	mRecentGridWidth = mEmojiScroll->getRect().getWidth();

	S32 scrollbarSize = mEmojiScroll->getSize();
	if (scrollbarSize < 0)
	{
		static LLUICachedControl<S32> scrollbar_size_control("UIScrollbarSize", 0);
		scrollbarSize = scrollbar_size_control;
	}

	const S32 clientWidth = mRecentGridWidth - scrollbarSize - mEmojiScroll->getBorderWidth() * 2;
	const S32 gridPadding = mEmojiGrid->getPadding();
	const S32 iconSpacing = mEmojiGrid->getSpacing();
	const S32 rowWidth = clientWidth - gridPadding * 2;
	const S32 iconSize = 28; // icon width and height
	const S32 maxIcons = llmax(1, (rowWidth + iconSpacing) / (iconSize + iconSpacing));

	// Optimization: don't rearrange for different widths with the same maxIcons
	if (maxIcons == mRecentMaxIcons)
		return;
	mRecentMaxIcons = maxIcons;

	mHoveredIcon = nullptr;
	mEmojiGrid->clearPanels();
	mPreviewEmoji->setLabel(LLUIString());

	if (mEmojiGrid->getRect().getWidth() != clientWidth)
	{
		LLRect rect = mEmojiGrid->getRect();
		rect.mRight = rect.mLeft + clientWidth;
		mEmojiGrid->setRect(rect);
	}

	LLPanel::Params row_panel_params;
	row_panel_params.rect = LLRect(0, iconSize, rowWidth, 0);

	LLScrollingPanelList::Params row_list_params;
	row_list_params.rect = row_panel_params.rect;
	row_list_params.is_horizontal = TRUE;
	row_list_params.padding = 0;
	row_list_params.spacing = iconSpacing;

	LLPanel::Params icon_params;
	LLRect icon_rect(0, iconSize, iconSize, 0);

	static const LLColor4 bgcolors[] =
	{
		LLColor4(0.8f, 0.6f, 0.8f, 1.0f),
		LLColor4(0.8f, 0.8f, 0.4f, 1.0f),
		LLColor4(0.6f, 0.6f, 0.8f, 1.0f),
		LLColor4(0.4f, 0.7f, 0.4f, 1.0f),
		LLColor4(0.5f, 0.7f, 0.9f, 1.0f),
		LLColor4(0.7f, 0.8f, 0.2f, 1.0f)
	};

	static constexpr U32 bgcolorCount = sizeof(bgcolors) / sizeof(*bgcolors);

	auto listCategory = [&](std::string category, const std::vector<const LLEmojiDescriptor*>& emojis, bool showDivider)
	{
		int iconIndex = 0;
		LLEmojiGridRow* row = nullptr;
		LLStringUtil::capitalize(category);
		for (const LLEmojiDescriptor* descr : emojis)
		{
			if (mSearchPattern.empty() || matchesPattern(descr))
			{
				// Place a category title if needed
				if (showDivider)
				{
					LLEmojiGridDivider* div = new LLEmojiGridDivider(row_panel_params, category);
					mEmojiGrid->addPanel(div, true);
					showDivider = false;
				}

				// Place a new row each (maxIcons) icons
				if (!(iconIndex % maxIcons))
				{
					row = new LLEmojiGridRow(row_panel_params, row_list_params);
					mEmojiGrid->addPanel(row, true);
				}

				// Place a new icon to the current row
				LLEmojiGridIcon* icon = new LLEmojiGridIcon(icon_params, descr, category);
				icon->setMouseEnterCallback([this](LLUICtrl* ctrl, const LLSD&) { onEmojiMouseEnter(ctrl); });
				icon->setMouseLeaveCallback([this](LLUICtrl* ctrl, const LLSD&) { onEmojiMouseLeave(ctrl); });
				icon->setMouseUpCallback([this](LLUICtrl* ctrl, S32, S32, MASK mask) { onEmojiMouseClick(ctrl, mask); });
				icon->setBackgroundColor(bgcolors[iconIndex % bgcolorCount]);
				icon->setBackgroundOpaque(1);
				icon->setRect(icon_rect);
				row->mList->addPanel(icon, true);

				iconIndex++;
			}
		}
	};

	const LLEmojiDictionary::cat2descrs_map_t& category2Descr = LLEmojiDictionary::instance().getCategory2Descrs();
	if (mSelectedCategory.empty())
	{
		// List all categories with titles
		for (const LLEmojiDictionary::cat2descrs_item_t& item : category2Descr)
		{
			listCategory(item.first, item.second, TRUE);
		}
	}
	else
	{
		// List one category without title
		const LLEmojiDictionary::cat2descrs_map_t::const_iterator& item = category2Descr.find(mSelectedCategory);
		if (item != category2Descr.end())
		{
			listCategory(mSelectedCategory, item->second, FALSE);
		}
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
	for (const std::string& shortCode : descr->ShortCodes)
		if (shortCode.find(mSearchPattern) != std::string::npos)
			return true;
	for (const std::string& category : descr->Categories)
		if (category.find(mSearchPattern) != std::string::npos)
			return true;
	return false;
}

void LLFloaterEmojiPicker::onCategoryCommit()
{
	mSelectedCategory = mCategory->getSelectedValue().asString();
	mRecentMaxIcons = 0;
	fillEmojiGrid();
}

void LLFloaterEmojiPicker::onSearchKeystroke()
{
	mSearchPattern = mSearch->getText();
	mRecentMaxIcons = 0;
	fillEmojiGrid();
}

void LLFloaterEmojiPicker::onPreviewEmojiClick()
{
	if (mEmojiPickCallback)
	{
		if (LLEmojiGridIcon* icon = dynamic_cast<LLEmojiGridIcon*>(mHoveredIcon))
		{
			mEmojiPickCallback(icon->getEmoji());
		}
	}
}

void LLFloaterEmojiPicker::onGridMouseEnter()
{
	mSearch->setVisible(FALSE);
	mDescription->setText(LLStringExplicit(""), LLStyle::Params());
	mDescription->setVisible(TRUE);
}

void LLFloaterEmojiPicker::onGridMouseLeave()
{
	mDescription->setVisible(FALSE);
	mDescription->setText(LLStringExplicit(""), LLStyle::Params());
	mSearch->setVisible(TRUE);
	mSearch->setFocus(TRUE);
}

void LLFloaterEmojiPicker::onEmojiMouseEnter(LLUICtrl* ctrl)
{
	if (ctrl)
	{
		if (mHoveredIcon && mHoveredIcon != ctrl)
		{
			unselectGridIcon(mHoveredIcon);
		}

		selectGridIcon(ctrl);

		mHoveredIcon = ctrl;
	}
}

void LLFloaterEmojiPicker::onEmojiMouseLeave(LLUICtrl* ctrl)
{
	if (ctrl)
	{
		if (ctrl == mHoveredIcon)
		{
			unselectGridIcon(ctrl);
		}
	}
}

void LLFloaterEmojiPicker::onEmojiMouseClick(LLUICtrl* ctrl, MASK mask)
{
	if (mEmojiPickCallback)
	{
		if (LLEmojiGridIcon* icon = dynamic_cast<LLEmojiGridIcon*>(ctrl))
		{
			mPreviewEmoji->handleAnyMouseClick(0, 0, 0, EMouseClickType::CLICK_LEFT, TRUE);
			mPreviewEmoji->handleAnyMouseClick(0, 0, 0, EMouseClickType::CLICK_LEFT, FALSE);
			if (!(mask & 4))
			{
				closeFloater();
			}
		}
	}
}

void LLFloaterEmojiPicker::selectGridIcon(LLUICtrl* ctrl)
{
	if (LLEmojiGridIcon* icon = dynamic_cast<LLEmojiGridIcon*>(ctrl))
	{
		icon->setBackgroundVisible(TRUE);

		LLUIString text;
		text.insert(0, icon->getText());
		mPreviewEmoji->setLabel(text);

		std::string descr = icon->getDescr() + "\n" + icon->getCategory();
		mDescription->setText(LLStringExplicit(descr), LLStyle::Params());
	}
}

void LLFloaterEmojiPicker::unselectGridIcon(LLUICtrl* ctrl)
{
	if (LLEmojiGridIcon* icon = dynamic_cast<LLEmojiGridIcon*>(ctrl))
	{
		icon->setBackgroundVisible(FALSE);
		mPreviewEmoji->setLabel(LLUIString());
		mDescription->setText(LLStringExplicit(""), LLStyle::Params());
	}
}

// virtual
BOOL LLFloaterEmojiPicker::handleKeyHere(KEY key, MASK mask)
{
	if (mask == MASK_NONE)
	{
		switch (key)
		{
		case KEY_ESCAPE:
			closeFloater();
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
	{
		mFloaterCloseCallback();
	}
}
