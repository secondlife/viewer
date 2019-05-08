/**
* @file llsearchablecontrol.h
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

#ifndef LL_SEARCHABLE_CONTROL_H
#define LL_SEARCHABLE_CONTROL_H

#include "lluicolortable.h"
#include "lluicolor.h"

namespace ll
{
	namespace ui
	{
		class SearchableControl
		{
			mutable bool mIsHighlighed;
		public:
			SearchableControl()
				: mIsHighlighed( false )
			{ }
			virtual ~SearchableControl()
			{ }

			LLColor4 getHighlightColor( ) const
			{
				static LLUIColor highlight_color = LLUIColorTable::instance().getColor("SearchableControlHighlightColor", LLColor4::red);
				return highlight_color.get();
			}

			void setHighlighted( bool aVal ) const
			{
				mIsHighlighed = aVal;
				onSetHighlight( );
			}
			bool getHighlighted( ) const
			{ return mIsHighlighed; }

			std::string getSearchText() const
			{ return _getSearchText(); } 
		protected:
			virtual std::string _getSearchText() const = 0;
			virtual void onSetHighlight( ) const
			{ }
		};
	}
}


#endif
