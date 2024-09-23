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

LLScrollingPanelList::Params::Params()
    : is_horizontal("is_horizontal")
    , padding("padding")
    , spacing("spacing")
{
}

LLScrollingPanelList::LLScrollingPanelList(const Params& p)
    : LLUICtrl(p)
    , mIsHorizontal(p.is_horizontal)
    , mPadding(p.padding.isProvided() ? p.padding : DEFAULT_PADDING)
    , mSpacing(p.spacing.isProvided() ? p.spacing : DEFAULT_SPACING)
{
}

void LLScrollingPanelList::clearPanels()
{
    deleteAllChildren();
    mPanelList.clear();
    rearrange();
}

S32 LLScrollingPanelList::addPanel(LLScrollingPanel* panel, bool back)
{
    if (back)
    {
        addChild(panel);
        mPanelList.push_back(panel);
    }
    else
    {
        addChildInBack(panel);
        mPanelList.push_front(panel);
    }

    rearrange();

    return mIsHorizontal ? getRect().getWidth() : getRect().getHeight();
}

void LLScrollingPanelList::removePanel(LLScrollingPanel* panel)
{
    U32 index = 0;
    LLScrollingPanelList::panel_list_t::const_iterator iter;

    if (!mPanelList.empty())
    {
        LLScrollingPanelList::panel_list_t::const_iterator iter =
            std::find(mPanelList.begin(), mPanelList.end(), panel);
        if (iter != mPanelList.end())
        {
            removeChild(panel);
            mPanelList.erase(iter);
            rearrange();
        }
    }
}

void LLScrollingPanelList::removePanel(U32 panel_index)
{
    if (panel_index >= mPanelList.size())
    {
        LL_WARNS() << "Panel index " << panel_index << " is out of range!" << LL_ENDL;
        return;
    }

    LLScrollingPanelList::panel_list_t::const_iterator iter = mPanelList.begin() + panel_index;
    removeChild(*iter);
    mPanelList.erase(iter);
    rearrange();
}

void LLScrollingPanelList::updatePanels(bool allow_modify)
{
    for (LLScrollingPanel* childp : mPanelList)
    {
        childp->updatePanel(allow_modify);
    }
}

void LLScrollingPanelList::rearrange()
{
    // Resize this view
    S32 new_width, new_height;
    if (!mPanelList.empty())
    {
        new_width = new_height = mPadding * 2;
        for (LLScrollingPanel* childp : mPanelList)
        {
            const LLRect& rect = childp->getRect();
            if (mIsHorizontal)
            {
                new_width += rect.getWidth() + mSpacing;
                new_height = llmax(new_height, rect.getHeight());
            }
            else
            {
                new_height += rect.getHeight() + mSpacing;
                new_width = llmax(new_width, rect.getWidth());
            }
        }

        if (mIsHorizontal)
        {
            new_width -= mSpacing;
        }
        else
        {
            new_height -= mSpacing;
        }
    }
    else
    {
        new_width = new_height = 1;
    }

    LLRect rc = getRect();
    if (mIsHorizontal || !followsRight())
    {
        rc.mRight = rc.mLeft + new_width;
    }
    if (!mIsHorizontal || !followsBottom())
    {
        rc.mBottom = rc.mTop - new_height;
    }

    if (rc.mRight != getRect().mRight || rc.mBottom != getRect().mBottom)
    {
        setRect(rc);
        notifySizeChanged();
    }

    // Reposition each of the child views
    S32 pos = mIsHorizontal ? mPadding : rc.getHeight() - mPadding;
    for (LLScrollingPanel* childp : mPanelList)
    {
        const LLRect& rect = childp->getRect();
        if (mIsHorizontal)
        {
            childp->translate(pos - rect.mLeft, rc.getHeight() - mPadding - rect.mTop);
            pos += rect.getWidth() + mSpacing;
        }
        else
        {
            childp->translate(mPadding - rect.mLeft, pos - rect.mTop);
            pos -= rect.getHeight() + mSpacing;
        }
    }
}

void LLScrollingPanelList::updatePanelVisiblilty()
{
    // Determine visibility of children.

    LLRect parent_screen_rect;
    getParent()->localPointToScreen(
        mPadding, mPadding,
        &parent_screen_rect.mLeft, &parent_screen_rect.mBottom );
    getParent()->localPointToScreen(
        getParent()->getRect().getWidth() - mPadding,
        getParent()->getRect().getHeight() - mPadding,
        &parent_screen_rect.mRight, &parent_screen_rect.mTop );

    for (LLScrollingPanel* childp : mPanelList)
    {
        if (childp->isDead())
            continue;

        const LLRect& local_rect = childp->getRect();
        LLRect screen_rect;
        childp->localPointToScreen(
            0, 0,
            &screen_rect.mLeft, &screen_rect.mBottom );
        childp->localPointToScreen(
            local_rect.getWidth(), local_rect.getHeight(),
            &screen_rect.mRight, &screen_rect.mTop );

        bool intersects =
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

void LLScrollingPanelList::notifySizeChanged()
{
    LLSD info;
    info["action"] = "size_changes";
    info["height"] = getRect().getHeight();
    info["width"] = getRect().getWidth();
    notifyParent(info);
}

// EOF
