/**
* @file llsearchableui.h
*
* $LicenseInfo:firstyear=2019&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2019, Linden Research, Inc.
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

#pragma once

class LLMenuItemGL;
class LLView;
class LLPanel;
class LLTabContainer;

#include "llsearchablecontrol.h"

namespace ll
{
    namespace prefs
    {
        struct SearchableItem;
        struct PanelData;
        struct TabContainerData;

        using SearchableItemPtr = std::shared_ptr< SearchableItem >;
        using PanelDataPtr = std::shared_ptr< PanelData >;
        using TabContainerDataPtr = std::shared_ptr< TabContainerData >;

        using tTabContainerDataList = std::vector< TabContainerData >;
        using tSearchableItemList = std::vector< SearchableItemPtr >;
        using tPanelDataList = std::vector< PanelDataPtr >;

        struct SearchableItem
        {
            LLWString mLabel;
            LLView const *mView;
            ll::ui::SearchableControl const *mCtrl;

            std::vector< std::shared_ptr< SearchableItem >  > mChildren;

            virtual ~SearchableItem();

            void setNotHighlighted();
            virtual bool hightlightAndHide( LLWString const &aFilter );
        };

        struct PanelData
        {
            LLPanel const *mPanel;
            std::string mLabel;

            std::vector< std::shared_ptr< SearchableItem > > mChildren;
            std::vector< std::shared_ptr< PanelData > > mChildPanel;

            virtual ~PanelData();

            void setNotHighlighted();
            virtual bool hightlightAndHide( LLWString const &aFilter );
        };

        struct TabContainerData: public PanelData
        {
            LLTabContainer *mTabContainer;
            virtual bool hightlightAndHide( LLWString const &aFilter );
        };

        struct SearchData
        {
            TabContainerDataPtr mRootTab;
            LLWString mLastFilter;
        };
    }
    namespace statusbar
    {
        struct SearchableItem;

        using SearchableItemPtr = std::shared_ptr< SearchableItem >;

        using tSearchableItemList = std::vector< SearchableItemPtr >;

        struct SearchableItem
        {
            LLWString mLabel;
            LLMenuItemGL *mMenu;
            tSearchableItemList mChildren;
            ll::ui::SearchableControl const *mCtrl;
            bool mWasHiddenBySearch;

            SearchableItem();

            void setNotHighlighted( );
            bool hightlightAndHide( LLWString const &aFilter, bool hide = true );
        };

        struct SearchData
        {
            SearchableItemPtr mRootMenu;
            LLWString mLastFilter;
        };
    }
}

