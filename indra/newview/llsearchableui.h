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

#ifndef LL_SEARCHABLE_UI_H
#define LL_SEARCHABLE_UI_H

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

		typedef boost::shared_ptr< SearchableItem > SearchableItemPtr;
		typedef boost::shared_ptr< PanelData > PanelDataPtr;
		typedef boost::shared_ptr< TabContainerData > TabContainerDataPtr;

		typedef std::vector< TabContainerData > tTabContainerDataList;
		typedef std::vector< SearchableItemPtr > tSearchableItemList;
		typedef std::vector< PanelDataPtr > tPanelDataList;

		struct SearchableItem
		{
			LLWString mLabel;
			LLView const *mView;
			ll::ui::SearchableControl const *mCtrl;

			std::vector< boost::shared_ptr< SearchableItem >  > mChildren;

			virtual ~SearchableItem();

			void setNotHighlighted();
			virtual bool hightlightAndHide( LLWString const &aFilter );
		};

		struct PanelData
		{
			LLPanel const *mPanel;
			std::string mLabel;

			std::vector< boost::shared_ptr< SearchableItem > > mChildren;
			std::vector< boost::shared_ptr< PanelData > > mChildPanel;

			virtual ~PanelData();

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

		typedef boost::shared_ptr< SearchableItem > SearchableItemPtr;

		typedef std::vector< SearchableItemPtr > tSearchableItemList;

		struct SearchableItem
		{
			LLWString mLabel;
			LLMenuItemGL *mMenu;
			tSearchableItemList mChildren;
			ll::ui::SearchableControl const *mCtrl;
			bool mWasHiddenBySearch;

			SearchableItem();

			void setNotHighlighted( );
			bool hightlightAndHide( LLWString const &aFilter );
		};

		struct SearchData
		{
			SearchableItemPtr mRootMenu;
			LLWString mLastFilter;
		};
	}
}

#endif
