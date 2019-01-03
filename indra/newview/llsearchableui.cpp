/**
* @file llsearchableui.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llsearchableui.h"

#include "llview.h"
#include "lltabcontainer.h"
#include "llmenugl.h"

ll::prefs::SearchableItem::~SearchableItem()
{}

void ll::prefs::SearchableItem::setNotHighlighted()
{
	mCtrl->setHighlighted( false );
}

bool ll::prefs::SearchableItem::hightlightAndHide( LLWString const &aFilter )
{
	if( mCtrl->getHighlighted() )
		return true;

	LLView const *pView = dynamic_cast< LLView const* >( mCtrl );
	if( pView && !pView->getVisible() )
		return false;

	if( aFilter.empty() )
	{
		mCtrl->setHighlighted( false );
		return true;
	}

	if( mLabel.find( aFilter ) != LLWString::npos )
	{
		mCtrl->setHighlighted( true );
		return true;
	}

	return false;
}

ll::prefs::PanelData::~PanelData()
{}

bool ll::prefs::PanelData::hightlightAndHide( LLWString const &aFilter )
{
	for( tSearchableItemList::iterator itr = mChildren.begin(); itr  != mChildren.end(); ++itr )
		(*itr)->setNotHighlighted( );

	bool bVisible(false);
	for( tSearchableItemList::iterator itr = mChildren.begin(); itr  != mChildren.end(); ++itr )
		bVisible |= (*itr)->hightlightAndHide( aFilter );

	for( tPanelDataList::iterator itr = mChildPanel.begin(); itr  != mChildPanel.end(); ++itr )
		bVisible |= (*itr)->hightlightAndHide( aFilter );

	return bVisible;
}

bool ll::prefs::TabContainerData::hightlightAndHide( LLWString const &aFilter )
{
	for( tSearchableItemList::iterator itr = mChildren.begin(); itr  != mChildren.end(); ++itr )
		(*itr)->setNotHighlighted( );

	bool bVisible(false);
	for( tSearchableItemList::iterator itr = mChildren.begin(); itr  != mChildren.end(); ++itr )
		bVisible |= (*itr)->hightlightAndHide( aFilter );

	for( tPanelDataList::iterator itr = mChildPanel.begin(); itr  != mChildPanel.end(); ++itr )
	{
		bool bPanelVisible = (*itr)->hightlightAndHide( aFilter );
		if( (*itr)->mPanel )
			mTabContainer->setTabVisibility( (*itr)->mPanel, bPanelVisible );
		bVisible |= bPanelVisible;
	}

	return bVisible;
}

ll::statusbar::SearchableItem::SearchableItem()
	: mMenu(0)
	, mCtrl(0)
	, mWasHiddenBySearch( false )
{ }

void ll::statusbar::SearchableItem::setNotHighlighted( )
{
	for( tSearchableItemList::iterator itr = mChildren.begin(); itr  != mChildren.end(); ++itr )
		(*itr)->setNotHighlighted( );

	if( mCtrl )
	{
		mCtrl->setHighlighted( false );

		if( mWasHiddenBySearch )
			mMenu->setVisible( TRUE );
	}
}

bool ll::statusbar::SearchableItem::hightlightAndHide( LLWString const &aFilter )
{
	if( mMenu && !mMenu->getVisible() && !mWasHiddenBySearch )
		return false;

	setNotHighlighted( );

	bool bVisible(false);
	for( tSearchableItemList::iterator itr = mChildren.begin(); itr  != mChildren.end(); ++itr )
		bVisible |= (*itr)->hightlightAndHide( aFilter );

	if( aFilter.empty() )
	{
		if( mCtrl )
			mCtrl->setHighlighted( false );
		return true;
	}

	if( mLabel.find( aFilter ) != LLWString::npos )
	{
		if( mCtrl )
			mCtrl->setHighlighted( true );
		return true;
	}

	if( mCtrl && !bVisible )
	{
		mWasHiddenBySearch = true;
		mMenu->setVisible(FALSE);
	}
	return bVisible;
}
