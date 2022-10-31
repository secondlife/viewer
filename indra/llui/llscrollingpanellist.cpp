/** 
 * @file llscrollingpanellist.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
        LL_WARNS() << "Panel index " << panel_index << " is out of range!" << LL_ENDL;
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
