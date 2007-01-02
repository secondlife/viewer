/** 
 * @file llscrollingpanellist.cpp
 * @brief 
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llstl.h"

#include "llscrollingpanellist.h"

/////////////////////////////////////////////////////////////////////
// LLScrollingPanelList

// This could probably be integrated with LLScrollContainer -SJB

void LLScrollingPanelList::clearPanels()
{
	deleteAllChildren();
	mPanelList.clear();
	reshape( 1, 1, FALSE );
}

void LLScrollingPanelList::addPanel( LLScrollingPanel* panel )
{
	addChildAtEnd( panel );
	mPanelList.push_front( panel );
	
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
	reshape( max_width, total_height, FALSE );

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

void LLScrollingPanelList::setValue(const LLSD& value)
{

}

EWidgetType LLScrollingPanelList::getWidgetType() const
{
	return WIDGET_TYPE_SCROLLING_PANEL_LIST;
}

LLString LLScrollingPanelList::getWidgetTag() const
{
	return LL_SCROLLING_PANEL_LIST_TAG;
}

void LLScrollingPanelList::draw()
{
	if( getVisible() )
	{
		updatePanelVisiblilty();
	}
	LLUICtrl::draw();
}


// static
LLView* LLScrollingPanelList::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
    LLString name("scrolling_panel_list");
    node->getAttributeString("name", name);

    LLRect rect;
    createRect(node, rect, parent, LLRect());

    LLScrollingPanelList* scrolling_panel_list = new LLScrollingPanelList(name, rect);
    scrolling_panel_list->initFromXML(node, parent);
    return scrolling_panel_list;
}

// virtual
LLXMLNodePtr LLScrollingPanelList::getXML(bool save_children) const
{
    LLXMLNodePtr node = LLUICtrl::getXML();
    return node;
}
