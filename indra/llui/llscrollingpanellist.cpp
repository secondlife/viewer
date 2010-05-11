/** 
 * @file llscrollingpanellist.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llstl.h"

#include "llscrollingpanellist.h"

static LLDefaultChildRegistry::Register<LLScrollingPanelList> r("scrolling_panel_list");


/////////////////////////////////////////////////////////////////////
// LLScrollingPanelList

// This could probably be integrated with LLScrollContainer -SJB

void LLScrollingPanelList::clearPanels()
{
	deleteAllChildren();
	mPanelList.clear();

	LLRect rc = getRect();
	rc.setLeftTopAndSize(rc.mLeft, rc.mTop, 1, 1);
	setRect(rc);

	notifySizeChanged(rc.getHeight());
}

S32 LLScrollingPanelList::addPanel( LLScrollingPanel* panel )
{
	addChildInBack( panel );
	mPanelList.push_front( panel );

	// Resize this view
	S32 total_height = 0;
	S32 max_width = 0;
	S32 cur_gap = 0;
	for (std::deque<LLScrollingPanel*>::iterator iter = mPanelList.begin();
		 iter != mPanelList.end(); ++iter)
	{
		LLScrollingPanel *childp = *iter;
		total_height += childp->getRect().getHeight() + cur_gap;
		max_width = llmax( max_width, childp->getRect().getWidth() );
		cur_gap = GAP_BETWEEN_PANELS;
	}
 	LLRect rc = getRect();
 	rc.setLeftTopAndSize(rc.mLeft, rc.mTop, max_width, total_height);
 	setRect(rc);

	notifySizeChanged(rc.getHeight());

	// Reposition each of the child views
	S32 cur_y = total_height;
	for (std::deque<LLScrollingPanel*>::iterator iter = mPanelList.begin();
		 iter != mPanelList.end(); ++iter)
	{
		LLScrollingPanel *childp = *iter;
		cur_y -= childp->getRect().getHeight();
		childp->translate( -childp->getRect().mLeft, cur_y - childp->getRect().mBottom);
		cur_y -= GAP_BETWEEN_PANELS;
	}

	return total_height;
}

void LLScrollingPanelList::removePanel(LLScrollingPanel* panel) 
{
	U32 index = 0;
	LLScrollingPanelList::panel_list_t::const_iterator iter;

	if (!mPanelList.empty()) 
	{
		for (iter = mPanelList.begin(); iter != mPanelList.end(); ++iter, ++index) 
		{
			if (*iter == panel) 
			{
				break;
			}
		}
		if(iter != mPanelList.end())
		{
			removePanel(index);
		}
	}
}

void LLScrollingPanelList::removePanel( U32 panel_index )
{
	if ( mPanelList.empty() || panel_index >= mPanelList.size() )
	{
		llwarns << "Panel index " << panel_index << " is out of range!" << llendl;
		return;
	}
	else
	{
		removeChild( mPanelList.at(panel_index) );
		mPanelList.erase( mPanelList.begin() + panel_index );
	}

	const S32 GAP_BETWEEN_PANELS = 6;

	// Resize this view
	S32 total_height = 0;
	S32 max_width = 0;
	S32 cur_gap = 0;
	for (std::deque<LLScrollingPanel*>::iterator iter = mPanelList.begin();
		 iter != mPanelList.end(); ++iter)
	{
		LLScrollingPanel *childp = *iter;
		total_height += childp->getRect().getHeight() + cur_gap;
		max_width = llmax( max_width, childp->getRect().getWidth() );
		cur_gap = GAP_BETWEEN_PANELS;
	}
	LLRect rc = getRect();
	rc.setLeftTopAndSize(rc.mLeft, rc.mTop, max_width, total_height);
	setRect(rc);

	notifySizeChanged(rc.getHeight());

	// Reposition each of the child views
	S32 cur_y = total_height;
	for (std::deque<LLScrollingPanel*>::iterator iter = mPanelList.begin();
		 iter != mPanelList.end(); ++iter)
	{
		LLScrollingPanel *childp = *iter;
		cur_y -= childp->getRect().getHeight();
		childp->translate( -childp->getRect().mLeft, cur_y - childp->getRect().mBottom);
		cur_y -= GAP_BETWEEN_PANELS;
	}
}

void LLScrollingPanelList::updatePanels(BOOL allow_modify)
{
    for (std::deque<LLScrollingPanel*>::iterator iter = mPanelList.begin();
		 iter != mPanelList.end(); ++iter)
    {
		LLScrollingPanel *childp = *iter;
		childp->updatePanel(allow_modify);
    }
}

void LLScrollingPanelList::updatePanelVisiblilty()
{
	// Determine visibility of children.
	S32 BORDER_WIDTH = 2;  // HACK

	LLRect parent_local_rect = getParent()->getRect();
	parent_local_rect.stretch( -BORDER_WIDTH );
	
	LLRect parent_screen_rect;
	getParent()->localPointToScreen( 
		BORDER_WIDTH, 0, 
		&parent_screen_rect.mLeft, &parent_screen_rect.mBottom );
	getParent()->localPointToScreen( 
		parent_local_rect.getWidth() - BORDER_WIDTH, parent_local_rect.getHeight() - BORDER_WIDTH,
		&parent_screen_rect.mRight, &parent_screen_rect.mTop );

	for (std::deque<LLScrollingPanel*>::iterator iter = mPanelList.begin();
		 iter != mPanelList.end(); ++iter)
	{
		LLScrollingPanel *childp = *iter;
		const LLRect& local_rect = childp->getRect();
		LLRect screen_rect;
		childp->localPointToScreen( 
			0, 0, 
			&screen_rect.mLeft, &screen_rect.mBottom );
		childp->localPointToScreen(
			local_rect.getWidth(), local_rect.getHeight(),
			&screen_rect.mRight, &screen_rect.mTop );

		BOOL intersects = 
			( (screen_rect.mRight > parent_screen_rect.mLeft) && (screen_rect.mLeft < parent_screen_rect.mRight) ) && 
			( (screen_rect.mTop > parent_screen_rect.mBottom) && (screen_rect.mBottom < parent_screen_rect.mTop) );

		childp->setVisible( intersects );
	}
}


void LLScrollingPanelList::draw()
{
	updatePanelVisiblilty();

	LLUICtrl::draw();
}

void LLScrollingPanelList::notifySizeChanged(S32 height)
{
	LLSD info;
	info["action"] = "size_changes";
	info["height"] = height;
	notifyParent(info);
}

// EOF
