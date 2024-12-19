/**
 * @file llpanelteleporthistory.h
 * @brief Teleport history represented by a scrolling list
 * class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELTELEPORTHISTORY_H
#define LL_LLPANELTELEPORTHISTORY_H

#include "lluictrlfactory.h"

#include "llpanelplacestab.h"
#include "llteleporthistory.h"
#include "llmenugl.h"

class LLTeleportHistoryStorage;
class LLAccordionCtrl;
class LLAccordionCtrlTab;
class LLFlatListView;
class LLMenuButton;

class LLTeleportHistoryPanel : public LLPanelPlacesTab
{
public:
    LLTeleportHistoryPanel();
    virtual ~LLTeleportHistoryPanel();

    bool postBuild() override;
    void draw() override;

    void onSearchEdit(const std::string& string) override;
    void onShowOnMap() override;
    void onShowProfile() override;
    void onTeleport() override;
    ///*virtual*/ void onCopySLURL();
    void onRemoveSelected() override;
    void updateVerbs() override;
    bool isSingleItemSelected() override;

    LLToggleableMenu* getSelectionMenu() override;
    LLToggleableMenu* getSortingMenu() override;
    LLToggleableMenu* getCreateMenu() override;

    bool handleDragAndDropToTrash(bool drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept) override { return false; }

private:

    void onDoubleClickItem();
    void onReturnKeyPressed();
    void onAccordionTabRightClick(LLView *view, S32 x, S32 y, MASK mask);
    void onAccordionTabOpen(LLAccordionCtrlTab *tab);
    void onAccordionTabClose(LLAccordionCtrlTab *tab);
    void onExpandAllFolders();
    void onCollapseAllFolders();
    void onClearTeleportHistory();
    bool onClearTeleportHistoryDialog(const LLSD& notification, const LLSD& response);

    void refresh() override;
    void getNextTab(const LLDate& item_date, S32& curr_tab, LLDate& tab_date);
    void onTeleportHistoryChange(S32 removed_index);
    void replaceItem(S32 removed_index);
    void showTeleportHistory();
    void handleItemSelect(LLFlatListView* );
    LLFlatListView* getFlatListViewFromTab(LLAccordionCtrlTab *);
    static void gotSLURLCallback(const std::string& slurl);
    void onGearMenuAction(const LLSD& userdata);
    bool isActionEnabled(const LLSD& userdata) const;

    void setAccordionCollapsedByUser(LLUICtrl* acc_tab, bool collapsed);
    bool isAccordionCollapsedByUser(LLUICtrl* acc_tab);
    void onAccordionExpand(LLUICtrl* ctrl, const LLSD& param);

    static void confirmTeleport(S32 hist_idx);
    static void createLandmark(S32 hist_idx);
    static bool onTeleportConfirmation(const LLSD& notification, const LLSD& response, S32 hist_idx);

    LLTeleportHistoryStorage*   mTeleportHistory;
    LLAccordionCtrl*        mHistoryAccordion;

    LLFlatListView*         mLastSelectedFlatlList;
    S32             mLastSelectedItemIndex;
    bool                mDirty;
    S32             mCurrentItem;

    typedef std::vector<LLAccordionCtrlTab*> item_containers_t;
    item_containers_t mItemContainers;

    LLContextMenu*          mAccordionTabMenu;

    LLToggleableMenu*           mGearItemMenu;
    LLToggleableMenu*           mSortingMenu;

    boost::signals2::connection mTeleportHistoryChangedConnection;
};



#endif //LL_LLPANELTELEPORTHISTORY_H
