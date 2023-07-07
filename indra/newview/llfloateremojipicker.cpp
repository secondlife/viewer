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
#include "llfloaterreg.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llscrollcontainer.h"
#include "llscrollingpanellist.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsdserialize.h"
#include "lltextbox.h" 
#include "llviewerchat.h" 

namespace {
// The following variables and constants are used for storing the floater state
// between different lifecycles of the floater and different sissions of the viewer

// Floater state related variables
static U32 sSelectedGroupIndex = 0;
static std::string sSearchPattern;
static std::list<llwchar> sRecentlyUsed;
static std::list<std::pair<llwchar, U32>> sFrequentlyUsed;

// State file related values
static std::string sStateFileName;
static const std::string sKeySelectedGroupIndex("SelectedGroupIndex");
static const std::string sKeySearchPattern("SearchPattern");
static const std::string sKeyRecentlyUsed("RecentlyUsed");
static const std::string sKeyFrequentlyUsed("FrequentlyUsed");
}

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
    LLEmojiGridIcon(
        const LLPanel::Params& panel_params
        , const LLEmojiDescriptor* descr
        , std::string category)
        : LLScrollingPanel(panel_params)
        , mEmoji(descr->Character)
        , mText(LLWString(1, mEmoji))
        , mDescr(descr->getShortCodes())
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
    loadState();
}

BOOL LLFloaterEmojiPicker::postBuild()
{
    // Should be initialized first
    mPreviewEmoji = getChild<LLButton>("PreviewEmoji");
    mPreviewEmoji->setClickedCallback([this](LLUICtrl*, const LLSD&) { onPreviewEmojiClick(); });

    mDescription = getChild<LLTextBox>("Description");
    mDescription->setVisible(FALSE);

    mGroups = getChild<LLPanel>("Groups");
    mBadge = getChild<LLPanel>("Badge");

    mSearch = getChild<LLLineEditor>("Search");
    mSearch->setKeystrokeCallback([this](LLLineEditor*, void*) { onSearchKeystroke(); }, NULL);
    mSearch->setFont(LLViewerChat::getChatFont());
    mSearch->setText(sSearchPattern);

    mEmojiScroll = getChild<LLScrollContainer>("EmojiGridContainer");
    mEmojiScroll->setMouseEnterCallback([this](LLUICtrl*, const LLSD&) { onGridMouseEnter(); });
    mEmojiScroll->setMouseLeaveCallback([this](LLUICtrl*, const LLSD&) { onGridMouseLeave(); });

    mEmojiGrid = getChild<LLScrollingPanelList>("EmojiGrid");

    fillGroups();
    fillEmojis();

    return TRUE;
}

void LLFloaterEmojiPicker::dirtyRect()
{
    super::dirtyRect();

    if (mEmojiScroll && mEmojiScroll->getRect().getWidth() != mRecentGridWidth)
    {
        moveGroups();
        fillEmojis(true);
    }
}

LLFloaterEmojiPicker::~LLFloaterEmojiPicker()
{
    gFocusMgr.releaseFocusIfNeeded( this );
}

void LLFloaterEmojiPicker::fillGroups()
{
    LLButton::Params params;
    params.font = LLFontGL::getFontEmoji();

    LLRect rect;
    rect.mTop = mGroups->getRect().getHeight();
    rect.mBottom = mBadge->getRect().getHeight();

    const std::vector<LLEmojiGroup>& groups = LLEmojiDictionary::instance().getGroups();
    for (const LLEmojiGroup& group : groups)
    {
        LLButton* button = LLUICtrlFactory::create<LLButton>(params);
        button->setClickedCallback([this](LLUICtrl* ctrl, const LLSD&) { onGroupButtonClick(ctrl); });
        button->setMouseEnterCallback([this](LLUICtrl* ctrl, const LLSD&) { onGroupButtonMouseEnter(ctrl); });
        button->setMouseLeaveCallback([this](LLUICtrl* ctrl, const LLSD&) { onGroupButtonMouseLeave(ctrl); });

        button->setRect(rect);

        LLUIString text;
        text.insert(0, LLWString(1, group.Character));
        button->setLabel(text);

        if (mGroupButtons.size() == sSelectedGroupIndex)
        {
            button->setToggleState(TRUE);
            button->setUseFontColor(TRUE);
        }

        mGroupButtons.push_back(button);
        mGroups->addChild(button);
    }

    moveGroups();
}

void LLFloaterEmojiPicker::moveGroups()
{
    const std::vector<LLEmojiGroup>& groups = LLEmojiDictionary::instance().getGroups();
    if (groups.empty())
        return;

    int badgeWidth = mGroups->getRect().getWidth() / groups.size();
    if (badgeWidth == mRecentBadgeWidth)
        return;

    mRecentBadgeWidth = badgeWidth;

    for (int i = 0; i < mGroupButtons.size(); ++i)
    {
        LLRect rect = mGroupButtons[i]->getRect();
        rect.mLeft = badgeWidth * i;
        rect.mRight = rect.mLeft + badgeWidth;
        mGroupButtons[i]->setRect(rect);
    }

    LLRect rect = mBadge->getRect();
    rect.mLeft = badgeWidth * sSelectedGroupIndex;
    rect.mRight = rect.mLeft + badgeWidth;
    mBadge->setRect(rect);
}

void LLFloaterEmojiPicker::fillEmojis(bool fromResize)
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
    if (fromResize && (maxIcons == mRecentMaxIcons))
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

    auto listCategory = [&](std::string category, const std::vector<const LLEmojiDescriptor*>& emojis, int maxRows = 0)
    {
        int rowCount = 0;
        int iconIndex = 0;
        bool showDivider = true;
        bool mixedFolder = maxRows;
        LLEmojiGridRow* row = nullptr;
        if (!mixedFolder)
        {
            LLStringUtil::capitalize(category);
        }
        for (const LLEmojiDescriptor* descr : emojis)
        {
            if (sSearchPattern.empty() || matchesPattern(descr))
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
                    if (maxRows && ++rowCount > maxRows)
                        break;
                    row = new LLEmojiGridRow(row_panel_params, row_list_params);
                    mEmojiGrid->addPanel(row, true);
                }

                // Place a new icon to the current row
                LLEmojiGridIcon* icon = new LLEmojiGridIcon(icon_params, descr, mixedFolder ? LLStringUtil::capitalize(descr->Category) : category);
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

    const std::vector<LLEmojiGroup>& groups = LLEmojiDictionary::instance().getGroups();
    const LLEmojiDictionary::emoji2descr_map_t& emoji2descr = LLEmojiDictionary::instance().getEmoji2Descr();
    const LLEmojiDictionary::cat2descrs_map_t& category2Descr = LLEmojiDictionary::instance().getCategory2Descrs();
    if (!sSelectedGroupIndex)
    {
        std::vector<const LLEmojiDescriptor*> recentlyUsed;
        for (llwchar emoji : sRecentlyUsed)
        {
            auto it = emoji2descr.find(emoji);
            if (it != emoji2descr.end())
            {
                recentlyUsed.push_back(it->second);
            }
        }
        listCategory(getString("title_for_recently_used"), recentlyUsed, 1);

        std::vector<const LLEmojiDescriptor*> frequentlyUsed;
        for (auto& emoji : sFrequentlyUsed)
        {
            auto it = emoji2descr.find(emoji.first);
            if (it != emoji2descr.end())
            {
                frequentlyUsed.push_back(it->second);
            }
        }
        listCategory(getString("title_for_frequently_used"), frequentlyUsed, 1);

        // List all groups
        for (const LLEmojiGroup& group : groups)
        {
            // List all categories in group
            for (const std::string& category : group.Categories)
            {
                // List all emojis in category
                const LLEmojiDictionary::cat2descrs_map_t::const_iterator& item = category2Descr.find(category);
                if (item != category2Descr.end())
                {
                    listCategory(category, item->second);
                }
            }
        }
    }
    else
    {
        // List all categories in the selected group
        for (const std::string& category : groups[sSelectedGroupIndex].Categories)
        {
            // List all emojis in category
            const LLEmojiDictionary::cat2descrs_map_t::const_iterator& item = category2Descr.find(category);
            if (item != category2Descr.end())
            {
                listCategory(category, item->second);
            }
        }
    }
}

bool LLFloaterEmojiPicker::matchesPattern(const LLEmojiDescriptor* descr)
{
    for (const std::string& shortCode : descr->ShortCodes)
        if (shortCode.find(sSearchPattern) != std::string::npos)
            return true;
    return false;
}

void LLFloaterEmojiPicker::onGroupButtonClick(LLUICtrl* ctrl)
{
    if (LLButton* button = dynamic_cast<LLButton*>(ctrl))
    {
        if (button == mGroupButtons[sSelectedGroupIndex] || button->getToggleState())
            return;

        auto it = std::find(mGroupButtons.begin(), mGroupButtons.end(), button);
        if (it == mGroupButtons.end())
            return;

        mGroupButtons[sSelectedGroupIndex]->setUseFontColor(FALSE);
        mGroupButtons[sSelectedGroupIndex]->setToggleState(FALSE);
        sSelectedGroupIndex = it - mGroupButtons.begin();
        mGroupButtons[sSelectedGroupIndex]->setToggleState(TRUE);
        mGroupButtons[sSelectedGroupIndex]->setUseFontColor(TRUE);

        LLRect rect = mBadge->getRect();
        rect.mLeft = button->getRect().mLeft;
        rect.mRight = button->getRect().mRight;
        mBadge->setRect(rect);

        mSearch->setFocus(TRUE);

        fillEmojis();
    }
}

void LLFloaterEmojiPicker::onSearchKeystroke()
{
    sSearchPattern = mSearch->getText();
    fillEmojis();
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

void LLFloaterEmojiPicker::onGroupButtonMouseEnter(LLUICtrl* ctrl)
{
    if (LLButton* button = dynamic_cast<LLButton*>(ctrl))
    {
        button->setUseFontColor(TRUE);
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
            onEmojiUsed(icon->getEmoji());
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
    saveState();
    LLFloater::closeFloater(app_quitting);
    if (mFloaterCloseCallback)
    {
        mFloaterCloseCallback();
    }
}

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
        sFrequentlyUsed.push_back(std::make_pair(emoji, 1));
}

void LLFloaterEmojiPicker::loadState()
{
    if (!sStateFileName.empty())
        return; // Already loaded

    sStateFileName = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "emoji_floater_state.xml");

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

    sSelectedGroupIndex = state[sKeySelectedGroupIndex].asInteger();

    sSearchPattern = state[sKeySearchPattern].asString();

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
        U32 delta = sFrequentlyUsed.back().second;
        for (auto& it : sFrequentlyUsed)
        {
            it.second = std::max((U32)0, it.second - delta);
        }
    }
}

void LLFloaterEmojiPicker::saveState()
{
    if (sStateFileName.empty())
        return; // Not loaded

    if (LLAppViewer::instance()->isSecondInstance())
        return; // Not allowed

    LLSD state = LLSD::emptyMap();

    if (sSelectedGroupIndex)
    {
        state[sKeySelectedGroupIndex] = (int)sSelectedGroupIndex;
    }

    if (!sSearchPattern.empty())
    {
        state[sKeySearchPattern] = sSearchPattern;
    }

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
