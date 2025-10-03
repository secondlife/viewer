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

class LLEmojiGridRow;
class LLEmojiGridIcon;
struct LLEmojiDescriptor;
struct LLEmojiSearchResult;

class LLFloaterEmojiPicker : public LLFloater
{
    using super = LLFloater;

public:
    // The callback function will be called with an emoji char.
    typedef boost::function<void (llwchar)> pick_callback_t;
    typedef boost::function<void ()> close_callback_t;

    LLFloaterEmojiPicker(const LLSD& key);

    virtual bool postBuild() override;
    virtual void dirtyRect() override;
    virtual void goneFromFront() override;

    void hideFloater() const;

    static std::list<llwchar>& getRecentlyUsed();
    static void onEmojiUsed(llwchar emoji);

    static void loadState();
    static void saveState();

private:
    void initialize();
    void fillGroups();
    void fillCategoryFrequentlyUsed(std::map<std::string, std::vector<LLEmojiSearchResult>>& cats);
    void fillGroupEmojis(std::map<std::string, std::vector<LLEmojiSearchResult>>& cats, U32 index);
    void createGroupButton(LLButton::Params& params, const LLRect& rect, llwchar emoji);
    void resizeGroupButtons();
    void selectEmojiGroup(U32 index);
    void fillEmojis(bool fromResize = false);
    void fillEmojisCategory(const std::vector<LLEmojiSearchResult>& emojis,
        const std::string& category, const LLPanel::Params& row_panel_params, const LLUICtrl::Params& row_list_params,
        const LLPanel::Params& icon_params, const LLRect& icon_rect, S32 max_icons, const LLColor4& bg);
    void createEmojiIcon(const LLEmojiSearchResult& emoji,
        const std::string& category, const LLPanel::Params& row_panel_params, const LLUICtrl::Params& row_list_params,
        const LLPanel::Params& icon_params, const LLRect& icon_rect, S32 max_icons, const LLColor4& bg,
        LLEmojiGridRow*& row, int& icon_index);
    void showPreview(bool show);

    void onGroupButtonClick(LLUICtrl* ctrl);
    void onGroupButtonMouseEnter(LLUICtrl* ctrl);
    void onGroupButtonMouseLeave(LLUICtrl* ctrl);
    void onEmojiMouseEnter(LLUICtrl* ctrl);
    void onEmojiMouseLeave(LLUICtrl* ctrl);
    void onEmojiMouseDown(LLUICtrl* ctrl);
    void onEmojiMouseUp(LLUICtrl* ctrl);

    void selectFocusedIcon();
    bool moveFocusedIconUp();
    bool moveFocusedIconDown();
    bool moveFocusedIconPrev();
    bool moveFocusedIconNext();

    void selectGridIcon(LLEmojiGridIcon* icon);
    void unselectGridIcon(LLEmojiGridIcon* icon);

    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    virtual bool handleKey(KEY key, MASK mask, bool called_from_parent) override;

    class LLPanel* mGroups { nullptr };
    class LLPanel* mBadge { nullptr };
    class LLScrollContainer* mEmojiScroll { nullptr };
    class LLScrollingPanelList* mEmojiGrid { nullptr };
    class LLEmojiPreviewPanel* mPreview { nullptr };
    class LLTextBox* mDummy { nullptr };

    std::vector<S32> mFilteredEmojiGroups;
    std::vector<std::map<std::string, std::vector<LLEmojiSearchResult>>> mFilteredEmojis;
    std::vector<class LLButton*> mGroupButtons;

    std::string mHint;
    std::string mFilterPattern;
    U32 mSelectedGroupIndex { 0 };
    S32 mRecentMaxIcons { 0 };
    S32 mFocusedIconRow { 0 };
    S32 mFocusedIconCol { 0 };
    LLEmojiGridIcon* mFocusedIcon { nullptr };
    LLEmojiGridIcon* mHoveredIcon { nullptr };

    U64 mRecentReturnPressedMs { 0 };
};

#endif
