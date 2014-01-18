/** 
 * @file llcontainerview.cpp
 * @brief Container for all statistics info
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llcontainerview.h"

#include "llerror.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llui.h"
#include "llstring.h"
#include "llscrollcontainer.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLContainerView> r1("container_view");

#include "llpanel.h"
#include "llstatview.h"
static ContainerViewRegistry::Register<LLStatView> r2("stat_view");
static ContainerViewRegistry::Register<LLPanel> r3("panel", &LLPanel::fromXML);

LLContainerView::LLContainerView(const LLContainerView::Params& p)
:	LLView(p),
	mShowLabel(p.show_label),
	mLabel(p.label),
	mDisplayChildren(p.display_children)
{
	mCollapsible = TRUE;
	mScrollContainer = NULL;
}

LLContainerView::~LLContainerView()
{
	// Children all cleaned up by default view destructor.
}

BOOL LLContainerView::postBuild()
{
	setDisplayChildren(mDisplayChildren);
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
	return TRUE;
}

bool LLContainerView::addChild(LLView* child, S32 tab_group)
{
	bool res = LLView::addChild(child, tab_group);
	if (res)
	{
		sendChildToBack(child);
	}
	return res;
}

BOOL LLContainerView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if (mDisplayChildren)
	{
		handled = (LLView::childrenHandleMouseDown(x, y, mask) != NULL);
	}
	if (!handled)
	{
		if( mCollapsible && mShowLabel && (y >= getRect().getHeight() - 10) )
		{
			setDisplayChildren(!mDisplayChildren);
			reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
			handled = TRUE;
		}
	}
	return handled;
}

BOOL LLContainerView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if (mDisplayChildren)
	{
		handled = (LLView::childrenHandleMouseUp(x, y, mask) != NULL);
	}
	return handled;
}


void LLContainerView::draw()
{
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.f, 0.f, 0.f, 0.25f));
	}
		
	// Draw the label
	if (mShowLabel)
	{
		LLFontGL::getFontMonospace()->renderUTF8(
			mLabel, 0, 2, getRect().getHeight() - 2, LLColor4(1,1,1,1), LLFontGL::LEFT, LLFontGL::TOP);
	}

	LLView::draw();
}


void LLContainerView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect scroller_rect;
	scroller_rect.setOriginAndSize(0, 0, width, height);

	if (mScrollContainer)
	{
		scroller_rect = mScrollContainer->getContentWindowRect();
	}
	else
	{
		// if we're uncontained - make height as small as possible
		scroller_rect.mTop = 0;
	}

	arrange(scroller_rect.getWidth(), scroller_rect.getHeight(), called_from_parent);

	// sometimes, after layout, our container will change size (scrollbars popping in and out)
	// if so, attempt another layout
	if (mScrollContainer)
	{
		LLRect new_container_rect = mScrollContainer->getContentWindowRect();

		if ((new_container_rect.getWidth() != scroller_rect.getWidth()) ||
			(new_container_rect.getHeight() != scroller_rect.getHeight()))  // the container size has changed, attempt to arrange again
		{
			arrange(new_container_rect.getWidth(), new_container_rect.getHeight(), called_from_parent);
		}
	}
}

void LLContainerView::arrange(S32 width, S32 height, BOOL called_from_parent)
{
	// Determine the sizes and locations of all contained views
	S32 total_height = 0;
	S32 top, left, right, bottom;
	//LLView *childp;

	// These will be used for the children
	left = 4;
	top = getRect().getHeight() - 4;
	right = width - 2;
	bottom = top;
	
	// Leave some space for the top label/grab handle
	if (mShowLabel)
	{
		total_height += 20;
	}

	if (mDisplayChildren)
	{
		// Determine total height
		U32 child_height = 0;
		for (child_list_const_iter_t child_iter = getChildList()->begin();
			 child_iter != getChildList()->end(); ++child_iter)
		{
			LLView *childp = *child_iter;
			if (!childp->getVisible())
			{
				llwarns << "Incorrect visibility!" << llendl;
			}
			LLRect child_rect = childp->getRequiredRect();
			child_height += child_rect.getHeight();
			child_height += 2;
		}
		total_height += child_height;
	}

	if (total_height < height)
		total_height = height;
	
	LLRect my_rect = getRect();
	if (followsTop())
	{
		my_rect.mBottom = my_rect.mTop - total_height;
	}
	else
	{
		my_rect.mTop = my_rect.mBottom + total_height;
	}

	my_rect.mRight = my_rect.mLeft + width;
	setRect(my_rect);

	top = total_height;
	if (mShowLabel)
	{
		top -= 20;
	}
	
	bottom = top;

	if (mDisplayChildren)
	{
		// Iterate through all children, and put in container from top down.
		for (child_list_const_iter_t child_iter = getChildList()->begin();
			 child_iter != getChildList()->end(); ++child_iter)
		{
			LLView *childp = *child_iter;
			LLRect child_rect = childp->getRequiredRect();
			bottom -= child_rect.getHeight();
			LLRect r(left, bottom + child_rect.getHeight(), right, bottom);
			childp->setRect(r);
			childp->reshape(right - left, top - bottom);
			top = bottom - 2;
			bottom = top;
		}
	}
	
	if (!called_from_parent)
	{
		if (getParent())
		{
			getParent()->reshape(getParent()->getRect().getWidth(), getParent()->getRect().getHeight(), FALSE);
		}
	}

}

LLRect LLContainerView::getRequiredRect()
{
	LLRect req_rect;
	//LLView *childp;
	U32 total_height = 0;
	
	// Determine the sizes and locations of all contained views

	// Leave some space for the top label/grab handle

	if (mShowLabel)
	{
		total_height = 20;
	}
		

	if (mDisplayChildren)
	{
		// Determine total height
		U32 child_height = 0;
		for (child_list_const_iter_t child_iter = getChildList()->begin();
			 child_iter != getChildList()->end(); ++child_iter)
		{
			LLView *childp = *child_iter;
			LLRect child_rect = childp->getRequiredRect();
			child_height += child_rect.getHeight();
			child_height += 2;
		}

		total_height += child_height;
	}
	req_rect.mTop = total_height;
	return req_rect;
}

void LLContainerView::setLabel(const std::string& label)
{
	mLabel = label;
}

void LLContainerView::setDisplayChildren(const BOOL displayChildren)
{
	mDisplayChildren = displayChildren;
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *childp = *child_iter;
		childp->setVisible(mDisplayChildren);
	}
}
