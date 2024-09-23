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

#include "llappviewer.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llemojidictionary.h"
#include "llemojihelper.h"
#include "llfloaterreg.h"
#include "llkeyboard.h"
#include "llscrollcontainer.h"
#include "llscrollingpanellist.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsdserialize.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewerchat.h"

namespace {
// The following variables and constants are used for storing the floater state
// between different lifecycles of the floater and different sissions of the viewer

// Floater constants
static const S32 ALL_EMOJIS_GROUP_INDEX = -2;
// https://www.compart.com/en/unicode/U+1F50D
static const S32 ALL_EMOJIS_IMAGE_INDEX = 0x1F50D;
static const S32 USED_EMOJIS_GROUP_INDEX = -1;
// https://www.compart.com/en/unicode/U+23F2
static const S32 USED_EMOJIS_IMAGE_INDEX = 0x23F2;
// https://www.compart.com/en/unicode/U+1F6D1
static const S32 EMPTY_LIST_IMAGE_INDEX = 0x1F6D1;
// The following categories should follow the required alphabetic order
static const std::string FREQUENTLY_USED_CATEGORY = "frequently used";

// Floater state related variables
static std::list<llwchar> sRecentlyUsed;
static std::list<std::pair<llwchar, U32>> sFrequentlyUsed;

// State file related values
static std::string sStateFileName;
static const std::string sKeyRecentlyUsed("RecentlyUsed");
static const std::string sKeyFrequentlyUsed("FrequentlyUsed");
}

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

    virtual void updatePanel(bool allow_modify) override {}

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
        F32 y = (F32)(getRect().getHeight() / 2);
        LLFontGL::getFontSansSerif()->render(
            mText,                           // wstr
            0,                               // begin_offset
            x,                               // x
            y,                               // y
            LLColor4::white,                 // color
            LLFontGL::LEFT,                  // halign
            LLFontGL::VCENTER,               // valign
            LLFontGL::NORMAL,                // style
            LLFontGL::DROP_SHADOW_SOFT,      // shadow
            static_cast<S32>(mText.size())); // max_chars
    }

    virtual void updatePanel(bool allow_modify) override {}

private:
    const LLWString mText;
};

class LLEmojiGridIcon : public LLScrollingPanel
{
public:
    LLEmojiGridIcon(
        const LLPanel::Params& panel_params
        , const LLEmojiSearchResult& emoji)
        : LLScrollingPanel(panel_params)
        , mData(emoji)
        , mChar(LLWString(1, emoji.Character))
    {
    }

    virtual void draw() override
    {
        LLScrollingPanel::draw();

        F32 x = (F32)(getRect().getWidth() / 2);
        F32 y = (F32)(getRect().getHeight() / 2);
        LLFontGL::getFontEmojiLarge()->render(
            mChar,                      // wstr
            0,                          // begin_offset
            x,                          // x
            y,                          // y
            LLColor4::white,            // color
            LLFontGL::HCENTER,          // halign
            LLFontGL::VCENTER,          // valign
            LLFontGL::NORMAL,           // style
            LLFontGL::DROP_SHADOW_SOFT, // shadow
            1);                         // max_chars
    }

    virtual void updatePanel(bool allow_modify) override {}

    const LLEmojiSearchResult& getData() const { return mData; }
    const LLWString& getChar() const { return mChar; }

private:
    const LLEmojiSearchResult mData;
    const LLWString mChar;
};

class LLEmojiPreviewPanel : public LLPanel
{
public:
    LLEmojiPreviewPanel()
        : LLPanel()
    {
    }

    void setIcon(const LLEmojiGridIcon* icon)
    {
        if (icon)
        {
            setData(icon->getData().Character, icon->getData().String, icon->getData().Begin, icon->getData().End);
        }
        else
        {
            setData(0, LLStringUtil::null, 0, 0);
        }
    }

    void setData(llwchar emoji, std::string title, size_t begin, size_t end)
    {
        mWStr = LLWString(1, emoji);
        mEmoji = emoji;
        mTitle = utf8str_to_wstring(title);
        mBegin = begin;
        mEnd = end;
    }

    virtual void draw() override
    {
        LLPanel::draw();

        S32 clientHeight = getRect().getHeight();
        S32 clientWidth = getRect().getWidth();
        S32 iconWidth = clientHeight;

        F32 centerX = 0.5f * iconWidth;
        F32 centerY = 0.5f * clientHeight;
        drawIcon(centerX, centerY - 1, iconWidth);

        static LLUIColor textColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", LLColor4(0.75f, 0.75f, 0.75f, 1.0f));
        S32 max_pixels = clientWidth - iconWidth;
        drawName((F32)iconWidth, centerY, max_pixels, textColor.get());
    }

protected:
    void drawIcon(F32 x, F32 y, S32 max_pixels)
    {
        LLFontGL::getFontEmojiHuge()->render(
            mWStr,                      // wstr
            0,                          // begin_offset
            x,                          // x
            y,                          // y
            LLColor4::white,            // color
            LLFontGL::HCENTER,          // halign
            LLFontGL::VCENTER,          // valign
            LLFontGL::NORMAL,           // style
            LLFontGL::DROP_SHADOW_SOFT, // shadow
            1,                          // max_chars
            max_pixels);                // max_pixels
    }

    void drawName(F32 x, F32 y, S32 max_pixels, const LLColor4& color)
    {
        F32 x0 = x;
        F32 x1 = (F32)max_pixels;
        LLFontGL* font = LLFontGL::getFontEmojiLarge();
        if (mBegin)
        {
            LLWString text = mTitle.substr(0, mBegin);
            font->render(
                text.c_str(),                          // text
                0,                             // begin_offset
                x0,                            // x
                y,                             // y
                color,                         // color
                LLFontGL::LEFT,                // halign
                LLFontGL::VCENTER,             // valign
                LLFontGL::NORMAL,              // style
                LLFontGL::DROP_SHADOW_SOFT,    // shadow
                static_cast<S32>(text.size()), // max_chars
                (S32)x1);                      // max_pixels
            F32 dx = font->getWidthF32(text.c_str());
            x0 += dx;
            x1 -= dx;
        }
        if (x1 > 0 && mEnd > mBegin)
        {
            LLWString text = mTitle.substr(mBegin, mEnd - mBegin);
            font->render(
                text,                          // text
                0,                             // begin_offset
                x0,                            // x
                y,                             // y
                LLColor4::yellow6,             // color
                LLFontGL::LEFT,                // halign
                LLFontGL::VCENTER,             // valign
                LLFontGL::NORMAL,              // style
                LLFontGL::DROP_SHADOW_SOFT,    // shadow
                static_cast<S32>(text.size()), // max_chars
                (S32)x1);                      // max_pixels
            F32 dx = font->getWidthF32(text.c_str());
            x0 += dx;
            x1 -= dx;
        }
        if (x1 > 0 && mEnd < mTitle.size())
        {
            LLWString text = mEnd ? mTitle.substr(mEnd) : mTitle;
            font->render(
                text,                          // text
                0,                             // begin_offset
                x0,                            // x
                y,                             // y
                color,                         // color
                LLFontGL::LEFT,                // halign
                LLFontGL::VCENTER,             // valign
                LLFontGL::NORMAL,              // style
                LLFontGL::DROP_SHADOW_SOFT,    // shadow
                static_cast<S32>(text.size()), // max_chars
                (S32)x1);                      // max_pixels
        }
    }

private:
    llwchar mEmoji;
    LLWString mWStr;
    LLWString mTitle;
    size_t mBegin;
    size_t mEnd;
};

LLFloaterEmojiPicker::LLFloaterEmojiPicker(const LLSD& key)
: super(key)
{
    // This floater should hover on top of our dependent (with the dependent having the focus)
    setFocusStealsFrontmost(false);
    setBackgroundVisible(false);
    setAutoFocus(false);

    loadState();
}

bool LLFloaterEmojiPicker::postBuild()
{
    mGroups = getChild<LLPanel>("Groups");
    mBadge = getChild<LLPanel>("Badge");
    mEmojiScroll = getChild<LLScrollContainer>("EmojiGridContainer");
    mEmojiGrid = getChild<LLScrollingPanelList>("EmojiGrid");
    mDummy = getChild<LLTextBox>("Dummy");

    mPreview = new LLEmojiPreviewPanel();
    mPreview->setVisible(false);
    addChild(mPreview);

    return LLFloater::postBuild();
}

void LLFloaterEmojiPicker::onOpen(const LLSD& key)
{
    mHint = key["hint"].asString();

    LLEmojiHelper::instance().setIsHideDisabled(mHint.empty());
    mFilterPattern = mHint;

    initialize();

    gFloaterView->adjustToFitScreen(this, false);
}

void LLFloaterEmojiPicker::onClose(bool app_quitting)
{
    if (!app_quitting)
    {
        LLEmojiHelper::instance().hideHelper(nullptr, true);
    }
}

void LLFloaterEmojiPicker::dirtyRect()
{
    super::dirtyRect();

    if (!mPreview)
        return;

    const S32 HPADDING = 4;
    const S32 VOFFSET = 12;
    LLRect rect(HPADDING, mDummy->getRect().mTop + 6, getRect().getWidth() - HPADDING, VOFFSET);
    if (mPreview->getRect() != rect)
    {
        mPreview->setRect(rect);
    }

    if (mEmojiScroll && mEmojiGrid)
    {
        S32 outer_width = mEmojiScroll->getRect().getWidth();
        S32 inner_width = mEmojiGrid->getRect().getWidth();
        if (outer_width != inner_width)
        {
            resizeGroupButtons();
            fillEmojis(true);
        }
    }
}

void LLFloaterEmojiPicker::initialize()
{
    S32 groupIndex = mSelectedGroupIndex && mSelectedGroupIndex <= mFilteredEmojiGroups.size() ?
        mFilteredEmojiGroups[mSelectedGroupIndex - 1] : ALL_EMOJIS_GROUP_INDEX;

    fillGroups();

    if (mFilteredEmojis.empty())
    {
        if (!mHint.empty())
        {
            hideFloater();
            return;
        }

        mGroups->setVisible(false);
        mFocusedIconRow = -1;
        mFocusedIconCol = -1;
        mFocusedIcon = nullptr;
        mHoveredIcon = nullptr;
        mEmojiScroll->goToTop();
        mEmojiGrid->clearPanels();

        if (mFilterPattern.empty())
        {
            showPreview(false);
        }
        else
        {
            std::size_t begin, end;
            LLStringUtil::format_map_t args;
            args["[FILTER]"] = mFilterPattern.substr(1);
            std::string title(getString("text_no_emoji_for_filter", args));
            LLEmojiDictionary::searchInShortCode(begin, end, title, mFilterPattern);
            mPreview->setData(EMPTY_LIST_IMAGE_INDEX, title, begin, end);
            showPreview(true);
        }
        return;
    }

    mGroups->setVisible(true);
    mPreview->setIcon(nullptr);
    showPreview(true);

    mSelectedGroupIndex = groupIndex == ALL_EMOJIS_GROUP_INDEX ? 0 :
        static_cast<U32>((1 + std::distance(mFilteredEmojiGroups.begin(),
            std::find(mFilteredEmojiGroups.begin(), mFilteredEmojiGroups.end(), groupIndex))) %
        (1 + mFilteredEmojiGroups.size()));

    mGroupButtons[mSelectedGroupIndex]->setToggleState(true);
    mGroupButtons[mSelectedGroupIndex]->setUseFontColor(true);

    fillEmojis();
}

void LLFloaterEmojiPicker::fillGroups()
{
    // Do not use deleteAllChildren() because mBadge shouldn't be removed
    for (LLButton* button : mGroupButtons)
    {
        mGroups->removeChild(button);
        button->die();
    }
    mFilteredEmojiGroups.clear();
    mFilteredEmojis.clear();
    mGroupButtons.clear();

    LLButton::Params params;
    params.font = LLFontGL::getFontEmojiLarge();
    params.name = "all_categories";

    LLRect rect;
    rect.mTop = mGroups->getRect().getHeight();
    rect.mBottom = mBadge->getRect().getHeight();

    // Create button for "All categories"
    params.name = "all_categories";
    createGroupButton(params, rect, ALL_EMOJIS_IMAGE_INDEX);

    // Create group and button for "Frequently used"
    if (!sFrequentlyUsed.empty())
    {
        std::map<std::string, std::vector<LLEmojiSearchResult>> cats;
        fillCategoryFrequentlyUsed(cats);

        if (!cats.empty())
        {
            mFilteredEmojiGroups.push_back(USED_EMOJIS_GROUP_INDEX);
            mFilteredEmojis.emplace_back(cats);
            params.name = "used_categories";
            createGroupButton(params, rect, USED_EMOJIS_IMAGE_INDEX);
        }
    }

    const std::vector<LLEmojiGroup>& groups = LLEmojiDictionary::instance().getGroups();

    // List all categories in the dictionary
    for (U32 i = 0; i < groups.size(); ++i)
    {
        std::map<std::string, std::vector<LLEmojiSearchResult>> cats;

        fillGroupEmojis(cats, i);

        if (!cats.empty())
        {
            mFilteredEmojiGroups.push_back(i);
            mFilteredEmojis.emplace_back(cats);
            params.name = "group_" + std::to_string(i);
            createGroupButton(params, rect, groups[i].Character);
        }
    }

    resizeGroupButtons();
}

void LLFloaterEmojiPicker::fillCategoryFrequentlyUsed(std::map<std::string, std::vector<LLEmojiSearchResult>>& cats)
{
    if (sFrequentlyUsed.empty())
        return;

    std::vector<LLEmojiSearchResult> emojis;

    // In case of empty mFilterPattern we'd use sFrequentlyUsed directly
    if (!mFilterPattern.empty())
    {
        // List all emojis in "Frequently used"
        const LLEmojiDictionary::emoji2descr_map_t& emoji2descr = LLEmojiDictionary::instance().getEmoji2Descr();
        std::size_t begin, end;
        for (const auto& emoji : sFrequentlyUsed)
        {
            auto e2d = emoji2descr.find(emoji.first);
            if (e2d != emoji2descr.end() && !e2d->second->ShortCodes.empty())
            {
                for (const std::string& shortcode : e2d->second->ShortCodes)
                {
                if (LLEmojiDictionary::searchInShortCode(begin, end, shortcode, mFilterPattern))
                {
                    emojis.emplace_back(emoji.first, shortcode, begin, end);
                }
            }
        }
        }
        if (emojis.empty())
            return;
    }

    cats.emplace(std::make_pair(FREQUENTLY_USED_CATEGORY, emojis));
}

void LLFloaterEmojiPicker::fillGroupEmojis(std::map<std::string, std::vector<LLEmojiSearchResult>>& cats, U32 index)
{
    const std::vector<LLEmojiGroup>& groups = LLEmojiDictionary::instance().getGroups();
    const LLEmojiDictionary::cat2descrs_map_t& category2Descr = LLEmojiDictionary::instance().getCategory2Descrs();

    for (const std::string& category : groups[index].Categories)
    {
        const LLEmojiDictionary::cat2descrs_map_t::const_iterator& c2d = category2Descr.find(category);
        if (c2d == category2Descr.end())
            continue;

        std::vector<LLEmojiSearchResult> emojis;

        // In case of empty mFilterPattern we'd use category2Descr directly
        if (!mFilterPattern.empty())
        {
            // List all emojis in category
            std::size_t begin, end;
            for (const LLEmojiDescriptor* descr : c2d->second)
            {
                if (!descr->ShortCodes.empty())
                {
                    for (const std::string& shortcode : descr->ShortCodes)
                    {
                    if (LLEmojiDictionary::searchInShortCode(begin, end, shortcode, mFilterPattern))
                    {
                        emojis.emplace_back(descr->Character, shortcode, begin, end);
                    }
                }
            }
            }
            if (emojis.empty())
                continue;
        }

        cats.emplace(std::make_pair(category, emojis));
    }
}

void LLFloaterEmojiPicker::createGroupButton(LLButton::Params& params, const LLRect& rect, llwchar emoji)
{
    LLButton* button = LLUICtrlFactory::create<LLButton>(params);
    button->setClickedCallback([this](LLUICtrl* ctrl, const LLSD&) { onGroupButtonClick(ctrl); });
    button->setMouseEnterCallback([this](LLUICtrl* ctrl, const LLSD&) { onGroupButtonMouseEnter(ctrl); });
    button->setMouseLeaveCallback([this](LLUICtrl* ctrl, const LLSD&) { onGroupButtonMouseLeave(ctrl); });

    button->setRect(rect);
    button->setTabStop(false);
    button->setLabel(LLUIString(LLWString(1, emoji)));
    button->setUseFontColor(false);

    mGroupButtons.push_back(button);
    mGroups->addChild(button);
}

void LLFloaterEmojiPicker::resizeGroupButtons()
{
    U32 groupCount = (U32)mGroupButtons.size();
    if (!groupCount)
        return;

    S32 totalWidth = mGroups->getRect().getWidth();
    S32 badgeWidth = totalWidth / groupCount;
    S32 leftOffset = (totalWidth - badgeWidth * groupCount) / 2;

    for (U32 i = 0; i < groupCount; ++i)
    {
        LLRect rect = mGroupButtons[i]->getRect();
        rect.mLeft = leftOffset + badgeWidth * i;
        rect.mRight = rect.mLeft + badgeWidth;
        mGroupButtons[i]->setRect(rect);
    }

    LLRect rect = mBadge->getRect();
    rect.mLeft = leftOffset + badgeWidth * mSelectedGroupIndex;
    rect.mRight = rect.mLeft + badgeWidth;
    mBadge->setRect(rect);
}

void LLFloaterEmojiPicker::selectEmojiGroup(U32 index)
{
    if (index == mSelectedGroupIndex || index >= mGroupButtons.size())
        return;

    if (mSelectedGroupIndex < mGroupButtons.size())
    {
        mGroupButtons[mSelectedGroupIndex]->setUseFontColor(false);
        mGroupButtons[mSelectedGroupIndex]->setToggleState(false);
    }

    mSelectedGroupIndex = index;
    mGroupButtons[mSelectedGroupIndex]->setToggleState(true);
    mGroupButtons[mSelectedGroupIndex]->setUseFontColor(true);

    LLButton* button = mGroupButtons[mSelectedGroupIndex];
    LLRect rect = mBadge->getRect();
    rect.mLeft = button->getRect().mLeft;
    rect.mRight = button->getRect().mRight;
    mBadge->setRect(rect);

    fillEmojis();
}

void LLFloaterEmojiPicker::fillEmojis(bool fromResize)
{
    S32 scrollbar_size = mEmojiScroll->getSize();
    if (scrollbar_size < 0)
    {
        static LLUICachedControl<S32> scrollbar_size_control("UIScrollbarSize", 0);
        scrollbar_size = scrollbar_size_control;
    }

    const S32 scroll_width = mEmojiScroll->getRect().getWidth();
    const S32 client_width = scroll_width - scrollbar_size - mEmojiScroll->getBorderWidth() * 2;
    const S32 grid_padding = mEmojiGrid->getPadding();
    const S32 icon_spacing = mEmojiGrid->getSpacing();
    const S32 row_width = client_width - grid_padding * 2;
    const S32 icon_size = 28; // icon width and height
    const S32 max_icons = llmax(1, (row_width + icon_spacing) / (icon_size + icon_spacing));

    // Optimization: don't rearrange for different widths with the same maxIcons
    if (fromResize && (max_icons == mRecentMaxIcons))
        return;

    mRecentMaxIcons = max_icons;

    mFocusedIconRow = 0;
    mFocusedIconCol = 0;
    mFocusedIcon = nullptr;
    mHoveredIcon = nullptr;
    mEmojiScroll->goToTop();
    mEmojiGrid->clearPanels();
    mPreview->setIcon(nullptr);

    if (mEmojiGrid->getRect().getWidth() != client_width)
    {
        LLRect rect = mEmojiGrid->getRect();
        rect.mRight = rect.mLeft + client_width;
        mEmojiGrid->setRect(rect);
    }

    LLPanel::Params row_panel_params;
    row_panel_params.rect = LLRect(0, icon_size, row_width, 0);

    LLScrollingPanelList::Params row_list_params;
    row_list_params.rect = row_panel_params.rect;
    row_list_params.is_horizontal = true;
    row_list_params.padding = 0;
    row_list_params.spacing = icon_spacing;

    LLPanel::Params icon_params;
    LLRect icon_rect(0, icon_size, icon_size, 0);

    static LLUIColor bg_color = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", LLColor4(0.75f, 0.75f, 0.75f, 1.0f));

    if (!mSelectedGroupIndex)
    {
        // List all groups
        for (const auto& group : mFilteredEmojis)
        {
            // List all categories in the group
            for (const auto& category : group)
            {
                // List all emojis in the category
                fillEmojisCategory(category.second, category.first, row_panel_params,
                    row_list_params, icon_params, icon_rect, max_icons, bg_color);
            }
        }
    }
    else
    {
        // List all categories in the selected group
        const auto& group = mFilteredEmojis[mSelectedGroupIndex - 1];
        for (const auto& category : group)
        {
            // List all emojis in the category
            fillEmojisCategory(category.second, category.first, row_panel_params,
                row_list_params, icon_params, icon_rect, max_icons, bg_color);
        }
    }

    if (mEmojiGrid->getPanelList().empty())
    {
        showPreview(false);
        mFocusedIconRow = -1;
        mFocusedIconCol = -1;
        if (!mHint.empty())
        {
            hideFloater();
        }
    }
    else
    {
        showPreview(true);
        mFocusedIconRow = 0;
        mFocusedIconCol = 0;
        moveFocusedIconNext();
    }
}

void LLFloaterEmojiPicker::fillEmojisCategory(const std::vector<LLEmojiSearchResult>& emojis,
    const std::string& category, const LLPanel::Params& row_panel_params, const LLUICtrl::Params& row_list_params,
    const LLPanel::Params& icon_params, const LLRect& icon_rect, S32 max_icons, const LLColor4& bg)
{
    // Place the category title
    std::string title =
        category == FREQUENTLY_USED_CATEGORY ? getString("title_for_frequently_used") :
        isupper(category.front()) ? category : LLStringUtil::capitalize(category);
    LLEmojiGridDivider* div = new LLEmojiGridDivider(row_panel_params, title);
    mEmojiGrid->addPanel(div, true);

    int icon_index = 0;
    LLEmojiGridRow* row = nullptr;

    if (mFilterPattern.empty())
    {
        const LLEmojiDictionary::emoji2descr_map_t& emoji2descr = LLEmojiDictionary::instance().getEmoji2Descr();
        LLEmojiSearchResult emoji { 0, "", 0, 0 };
        if (category == FREQUENTLY_USED_CATEGORY)
        {
            for (const auto& code : sFrequentlyUsed)
            {
                const LLEmojiDictionary::emoji2descr_map_t::const_iterator& e2d = emoji2descr.find(code.first);
                if (e2d != emoji2descr.end() && !e2d->second->ShortCodes.empty())
                {
                    emoji.Character = code.first;
                    emoji.String = e2d->second->ShortCodes.front();
                    createEmojiIcon(emoji, category, row_panel_params, row_list_params, icon_params,
                        icon_rect, max_icons, bg, row, icon_index);
                }
            }
        }
        else
        {
            const LLEmojiDictionary::cat2descrs_map_t& category2Descr = LLEmojiDictionary::instance().getCategory2Descrs();
            const LLEmojiDictionary::cat2descrs_map_t::const_iterator& c2d = category2Descr.find(category);
            if (c2d != category2Descr.end())
            {
                for (const LLEmojiDescriptor* descr : c2d->second)
                {
                    emoji.Character = descr->Character;
                    emoji.String = descr->ShortCodes.front();
                    createEmojiIcon(emoji, category, row_panel_params, row_list_params, icon_params,
                        icon_rect, max_icons, bg, row, icon_index);
                }
            }
        }
    }
    else
    {
        for (const LLEmojiSearchResult& emoji : emojis)
        {
            createEmojiIcon(emoji, category, row_panel_params, row_list_params, icon_params,
                icon_rect, max_icons, bg, row, icon_index);
        }
    }
}

void LLFloaterEmojiPicker::createEmojiIcon(const LLEmojiSearchResult& emoji,
    const std::string& category, const LLPanel::Params& row_panel_params, const LLUICtrl::Params& row_list_params,
    const LLPanel::Params& icon_params, const LLRect& icon_rect, S32 max_icons, const LLColor4& bg,
    LLEmojiGridRow*& row, int& icon_index)
{
    // Place a new row each (max_icons) icons
    if (!(icon_index % max_icons))
    {
        row = new LLEmojiGridRow(row_panel_params, *(const LLScrollingPanelList::Params*)&row_list_params);
        mEmojiGrid->addPanel(row, true);
    }

    // Place a new icon to the current row
    LLEmojiGridIcon* icon = new LLEmojiGridIcon(icon_params, emoji);
    icon->setMouseEnterCallback([this](LLUICtrl* ctrl, const LLSD&) { onEmojiMouseEnter(ctrl); });
    icon->setMouseLeaveCallback([this](LLUICtrl* ctrl, const LLSD&) { onEmojiMouseLeave(ctrl); });
    icon->setMouseDownCallback([this](LLUICtrl* ctrl, S32, S32, MASK) { onEmojiMouseDown(ctrl); });
    icon->setMouseUpCallback([this](LLUICtrl* ctrl, S32, S32, MASK) { onEmojiMouseUp(ctrl); });
    icon->setBackgroundColor(bg);
    icon->setBackgroundOpaque(1);
    icon->setRect(icon_rect);
    row->mList->addPanel(icon, true);

    icon_index++;
}

void LLFloaterEmojiPicker::showPreview(bool show)
{
    mDummy->setVisible(!show);
    mPreview->setVisible(show);
}

void LLFloaterEmojiPicker::onGroupButtonClick(LLUICtrl* ctrl)
{
    if (LLButton* button = dynamic_cast<LLButton*>(ctrl))
    {
        if (button == mGroupButtons[mSelectedGroupIndex] || button->getToggleState())
            return;

        auto it = std::find(mGroupButtons.begin(), mGroupButtons.end(), button);
        if (it == mGroupButtons.end())
            return;

        selectEmojiGroup((U32)(it - mGroupButtons.begin()));
    }
}

void LLFloaterEmojiPicker::onGroupButtonMouseEnter(LLUICtrl* ctrl)
{
    if (LLButton* button = dynamic_cast<LLButton*>(ctrl))
    {
        button->setUseFontColor(true);
    }
}

void LLFloaterEmojiPicker::onGroupButtonMouseLeave(LLUICtrl* ctrl)
{
    if (LLButton* button = dynamic_cast<LLButton*>(ctrl))
    {
        button->setUseFontColor(button->getToggleState());
    }
}

void LLFloaterEmojiPicker::onEmojiMouseEnter(LLUICtrl* ctrl)
{
    if (LLEmojiGridIcon* icon = dynamic_cast<LLEmojiGridIcon*>(ctrl))
    {
        if (mFocusedIcon && mFocusedIcon != icon && mFocusedIcon->isBackgroundVisible())
        {
            unselectGridIcon(mFocusedIcon);
        }

        if (mHoveredIcon && mHoveredIcon != icon)
        {
            unselectGridIcon(mHoveredIcon);
        }

        selectGridIcon(icon);

        mHoveredIcon = icon;
    }
}

void LLFloaterEmojiPicker::onEmojiMouseLeave(LLUICtrl* ctrl)
{
    if (LLEmojiGridIcon* icon = dynamic_cast<LLEmojiGridIcon*>(ctrl))
    {
        if (icon == mHoveredIcon)
        {
            if (icon != mFocusedIcon)
            {
                unselectGridIcon(icon);
            }
            mHoveredIcon = nullptr;
        }

        if (!mHoveredIcon && mFocusedIcon && !mFocusedIcon->isBackgroundVisible())
        {
            selectGridIcon(mFocusedIcon);
        }
    }
}

void LLFloaterEmojiPicker::onEmojiMouseDown(LLUICtrl* ctrl)
{
    if (getSoundFlags() & MOUSE_DOWN)
    {
        make_ui_sound("UISndClick");
    }
}

void LLFloaterEmojiPicker::onEmojiMouseUp(LLUICtrl* ctrl)
{
    if (getSoundFlags() & MOUSE_UP)
    {
        make_ui_sound("UISndClickRelease");
    }

    if (LLEmojiGridIcon* icon = dynamic_cast<LLEmojiGridIcon*>(ctrl))
    {
        LLSD value(wstring_to_utf8str(icon->getChar()));
        setValue(value);

        onCommit();

        if (!mHint.empty() || !(gKeyboard->currentMask(true) & MASK_SHIFT))
        {
            hideFloater();
        }
    }
}

void LLFloaterEmojiPicker::selectFocusedIcon()
{
    if (mFocusedIcon && mFocusedIcon != mHoveredIcon)
    {
        unselectGridIcon(mFocusedIcon);
    }

    // Both mFocusedIconRow and mFocusedIconCol should be already verified
    LLEmojiGridRow* row = dynamic_cast<LLEmojiGridRow*>(mEmojiGrid->getPanelList()[mFocusedIconRow]);
    mFocusedIcon = row ? dynamic_cast<LLEmojiGridIcon*>(row->mList->getPanelList()[mFocusedIconCol]) : nullptr;

    if (mFocusedIcon && !mHoveredIcon)
    {
        selectGridIcon(mFocusedIcon);
    }
}

bool LLFloaterEmojiPicker::moveFocusedIconUp()
{
    for (S32 i = mFocusedIconRow - 1; i >= 0; --i)
    {
        LLScrollingPanel* panel = mEmojiGrid->getPanelList()[i];
        LLEmojiGridRow* row = dynamic_cast<LLEmojiGridRow*>(panel);
        if (row && row->mList->getPanelList().size() > mFocusedIconCol)
        {
            mEmojiScroll->scrollToShowRect(row->getBoundingRect());
            mFocusedIconRow = i;
            selectFocusedIcon();
            return true;
        }
    }

    return false;
}

bool LLFloaterEmojiPicker::moveFocusedIconDown()
{
    auto rowCount = mEmojiGrid->getPanelList().size();
    for (size_t i = mFocusedIconRow + 1; i < rowCount; ++i)
    {
        LLScrollingPanel* panel = mEmojiGrid->getPanelList()[i];
        LLEmojiGridRow* row = dynamic_cast<LLEmojiGridRow*>(panel);
        if (row && row->mList->getPanelList().size() > mFocusedIconCol)
        {
            mEmojiScroll->scrollToShowRect(row->getBoundingRect());
            mFocusedIconRow = static_cast<S32>(i);
            selectFocusedIcon();
            return true;
        }
    }

    return false;
}

bool LLFloaterEmojiPicker::moveFocusedIconPrev()
{
    if (mHoveredIcon)
        return false;

    if (mFocusedIconCol > 0)
    {
        mFocusedIconCol--;
        selectFocusedIcon();
        return true;
    }

    for (S32 i = mFocusedIconRow - 1; i >= 0; --i)
    {
        LLScrollingPanel* panel = mEmojiGrid->getPanelList()[i];
        LLEmojiGridRow* row = dynamic_cast<LLEmojiGridRow*>(panel);
        if (row && row->mList->getPanelList().size())
        {
            mEmojiScroll->scrollToShowRect(row->getBoundingRect());
            mFocusedIconCol = static_cast<S32>(row->mList->getPanelList().size()) - 1;
            mFocusedIconRow = i;
            selectFocusedIcon();
            return true;
        }
    }

    return false;
}

bool LLFloaterEmojiPicker::moveFocusedIconNext()
{
    if (mHoveredIcon)
        return false;

    LLScrollingPanel* panel = mEmojiGrid->getPanelList()[mFocusedIconRow];
    LLEmojiGridRow* row = dynamic_cast<LLEmojiGridRow*>(panel);
    S32 colCount = row ? static_cast<S32>(row->mList->getPanelList().size()) : 0;
    if (mFocusedIconCol < colCount - 1)
    {
        mFocusedIconCol++;
        selectFocusedIcon();
        return true;
    }

    auto rowCount = mEmojiGrid->getPanelList().size();
    for (size_t i = mFocusedIconRow + 1; i < rowCount; ++i)
    {
        LLScrollingPanel* panel = mEmojiGrid->getPanelList()[i];
        LLEmojiGridRow* row = dynamic_cast<LLEmojiGridRow*>(panel);
        if (row && row->mList->getPanelList().size())
        {
            mEmojiScroll->scrollToShowRect(row->getBoundingRect());
            mFocusedIconCol = 0;
            mFocusedIconRow = static_cast<S32>(i);
            selectFocusedIcon();
            return true;
        }
    }

    return false;
}

void LLFloaterEmojiPicker::selectGridIcon(LLEmojiGridIcon* icon)
{
    icon->setBackgroundVisible(true);
    mPreview->setIcon(icon);
}

void LLFloaterEmojiPicker::unselectGridIcon(LLEmojiGridIcon* icon)
{
    icon->setBackgroundVisible(false);
    mPreview->setIcon(nullptr);
}

// virtual
bool LLFloaterEmojiPicker::handleKey(KEY key, MASK mask, bool called_from_parent)
{
    if (mask == MASK_NONE)
    {
        switch (key)
        {
        case KEY_UP:
            moveFocusedIconUp();
            return true;
        case KEY_DOWN:
            moveFocusedIconDown();
            return true;
        case KEY_LEFT:
            moveFocusedIconPrev();
            return true;
        case KEY_RIGHT:
            moveFocusedIconNext();
            return true;
        case KEY_ESCAPE:
            hideFloater();
            return true;
        }
    }

    if (mask == MASK_ALT)
    {
        switch (key)
        {
        case KEY_LEFT:
            selectEmojiGroup(static_cast<U32>((mSelectedGroupIndex + mFilteredEmojis.size()) % mGroupButtons.size()));
            return true;
        case KEY_RIGHT:
            selectEmojiGroup(static_cast<U32>((mSelectedGroupIndex + 1) % mGroupButtons.size()));
            return true;
        }
    }

    if (key == KEY_RETURN)
    {
        U64 time = totalTime();
        // <Shift+Return> comes twice for unknown reason
        if (mFocusedIcon && (time - mRecentReturnPressedMs > 100000)) // Min interval 0.1 sec.
        {
            onEmojiMouseDown(mFocusedIcon);
            onEmojiMouseUp(mFocusedIcon);
        }
        mRecentReturnPressedMs = time;
        return true;
    }

    if (mHint.empty())
    {
        if (key >= 0x20 && key < 0x80)
        {
            if (!mEmojiGrid->getPanelList().empty())
            {
                if (mFilterPattern.empty())
                {
                    mFilterPattern = ":";
                }
                mFilterPattern += (char)key;
                initialize();
            }
            return true;
        }
        else if (key == KEY_BACKSPACE)
        {
            if (!mFilterPattern.empty())
            {
                mFilterPattern.pop_back();
                if (mFilterPattern == ":")
                {
                    mFilterPattern.clear();
                }
                initialize();
            }
            return true;
        }
    }

    return super::handleKey(key, mask, called_from_parent);
}

// virtual
void LLFloaterEmojiPicker::goneFromFront()
{
    hideFloater();
}

void LLFloaterEmojiPicker::hideFloater() const
{
    LLEmojiHelper::instance().hideHelper(nullptr, true);
}

// static
std::list<llwchar>& LLFloaterEmojiPicker::getRecentlyUsed()
{
    loadState();
    return sRecentlyUsed;
}

// static
void LLFloaterEmojiPicker::onEmojiUsed(llwchar emoji)
{
    // Update sRecentlyUsed
    auto itr = std::find(sRecentlyUsed.begin(), sRecentlyUsed.end(), emoji);
    if (itr == sRecentlyUsed.end())
    {
        sRecentlyUsed.push_front(emoji);
    }
    else if (itr != sRecentlyUsed.begin())
    {
        sRecentlyUsed.erase(itr);
        sRecentlyUsed.push_front(emoji);
    }

    // Increment and reorder sFrequentlyUsed
    auto itf = sFrequentlyUsed.begin();
    while (itf != sFrequentlyUsed.end())
    {
        if (itf->first == emoji)
        {
            itf->second++;
            while (itf != sFrequentlyUsed.begin())
            {
                auto prior = itf;
                prior--;
                if (prior->second > itf->second)
                    break;
                prior->swap(*itf);
                itf = prior;
            }
            break;
        }
        itf++;
    }
    // Append new if not found
    if (itf == sFrequentlyUsed.end())
    {
        // Insert before others with count == 1
        while (itf != sFrequentlyUsed.begin())
        {
            auto prior = itf;
            prior--;
            if (prior->second > 1)
                break;
            itf = prior;
        }
        sFrequentlyUsed.insert(itf, std::make_pair(emoji, 1));
    }
}

// static
void LLFloaterEmojiPicker::loadState()
{
    if (!sStateFileName.empty())
        return; // Already loaded

    sStateFileName = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "emoji_floater_state.xml");

    llifstream file;
    file.open(sStateFileName.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "Emoji floater state file is missing or inaccessible: " << sStateFileName << LL_ENDL;
        return;
    }

    LLSD state;
    LLSDSerialize::fromXML(state, file);
    if (state.isUndefined())
    {
        LL_WARNS() << "Emoji floater state file is missing or ill-formed: " << sStateFileName << LL_ENDL;
        return;
    }

    // Load and parse sRecentlyUsed
    std::string recentlyUsed = state[sKeyRecentlyUsed];
    std::vector<std::string> rtokens = LLStringUtil::getTokens(recentlyUsed, ",");
    int maxCountR = 20;
    for (const std::string& token : rtokens)
    {
        llwchar emoji = (llwchar)atoi(token.c_str());
        if (std::find(sRecentlyUsed.begin(), sRecentlyUsed.end(), emoji) == sRecentlyUsed.end())
        {
            sRecentlyUsed.push_back(emoji);
            if (!--maxCountR)
                break;
        }
    }

    // Load and parse sFrequentlyUsed
    std::string frequentlyUsed = state[sKeyFrequentlyUsed];
    std::vector<std::string> ftokens = LLStringUtil::getTokens(frequentlyUsed, ",");
    int maxCountF = 20;
    for (const std::string& token : ftokens)
    {
        std::vector<std::string> pair = LLStringUtil::getTokens(token, ":");
        if (pair.size() == 2)
        {
            llwchar emoji = (llwchar)atoi(pair[0].c_str());
            if (emoji)
            {
                U32 count = atoi(pair[1].c_str());
                auto it = std::find_if(sFrequentlyUsed.begin(), sFrequentlyUsed.end(),
                    [emoji](std::pair<llwchar, U32>& it) { return it.first == emoji; });
                if (it != sFrequentlyUsed.end())
                {
                    it->second += count;
                }
                else
                {
                    sFrequentlyUsed.push_back(std::make_pair(emoji, count));
                    if (!--maxCountF)
                        break;
                }
            }
        }
    }

    // Normalize by minimum
    if (!sFrequentlyUsed.empty())
    {
        U32 delta = sFrequentlyUsed.back().second - 1;
        for (auto& it : sFrequentlyUsed)
        {
            it.second = std::max((U32)0, it.second - delta);
        }
    }
}

// static
void LLFloaterEmojiPicker::saveState()
{
    if (sStateFileName.empty())
        return; // Not loaded

    if (LLAppViewer::instance()->isSecondInstance())
        return; // Not allowed

    LLSD state = LLSD::emptyMap();

    if (!sRecentlyUsed.empty())
    {
        U32 maxCount = 20;
        std::string recentlyUsed;
        for (llwchar emoji : sRecentlyUsed)
        {
            if (!recentlyUsed.empty())
                recentlyUsed += ",";
            char buffer[32];
            sprintf(buffer, "%u", (U32)emoji);
            recentlyUsed += buffer;
            if (!--maxCount)
                break;
        }
        state[sKeyRecentlyUsed] = recentlyUsed;
    }

    if (!sFrequentlyUsed.empty())
    {
        U32 maxCount = 20;
        std::string frequentlyUsed;
        for (auto& it : sFrequentlyUsed)
        {
            if (!frequentlyUsed.empty())
                frequentlyUsed += ",";
            char buffer[32];
            sprintf(buffer, "%u:%u", (U32)it.first, (U32)it.second);
            frequentlyUsed += buffer;
            if (!--maxCount)
                break;
        }
        state[sKeyFrequentlyUsed] = frequentlyUsed;
    }

    llofstream stream(sStateFileName.c_str());
    LLSDSerialize::toPrettyXML(state, stream);
}
