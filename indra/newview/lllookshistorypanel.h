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

#ifndef LL_LLPANELLOOKSHISTORY_H
#define LL_LLPANELLOOKSHISTORY_H

#include "lluictrlfactory.h"
#include "llscrolllistctrl.h"

#include "llpanelappearancetab.h"
#include "lllookshistory.h"

class LLLooksHistoryPanel : public LLPanelAppearanceTab
{
public:
    LLLooksHistoryPanel();
    virtual ~LLLooksHistoryPanel();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onSearchEdit(const std::string& string);
    /*virtual*/ void onShowOnMap();
    /*virtual*/ void onLooks();
    ///*virtual*/ void onCopySLURL();

    void showLooksHistory();
    void handleItemSelect(const LLSD& data);

    static void onDoubleClickItem(void* user_data);

private:
    enum LOOKS_HISTORY_COLUMN_ORDER
    {
        LIST_ICON,
        LIST_ITEM_TITLE,
        LIST_INDEX
    };

    LLLooksHistory*     mLooksHistory;
    LLScrollListCtrl*       mHistoryItems;
    std::string             mFilterSubString;
};

#endif //LL_LLPANELLOOKSHISTORY_H
