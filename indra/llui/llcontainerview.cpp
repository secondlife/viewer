/** 
 * @file llcontainerview.cpp
 * @brief Container for all statistics info
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llcontainerview.h"

#include "llerror.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llui.h"
#include "llstring.h"
#include "llscrollcontainer.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLContainerView> r("container_view");

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
	S32 desired_width = width;
	S32 desired_height = height;

	if (mScrollContainer)
	{
		BOOL dum_bool;
		mScrollContainer->calcVisibleSize(&desired_width, &desired_height, &dum_bool, &dum_bool);
	}
	else
	{
		// if we're uncontained - make height as small as possible
		desired_height = 0;
	}

	arrange(desired_width, desired_height, called_from_parent);

	// sometimes, after layout, our container will change size (scrollbars popping in and out)
	// if so, attempt another layout
	if (mScrollContainer)
	{
		S32 new_container_width;
		S32 new_container_height;
		BOOL dum_bool;
		mScrollContainer->calcVisibleSize(&new_container_width, &new_container_height, &dum_bool, &dum_bool);

		if ((new_container_width != desired_width) ||
			(new_container_height != desired_height))  // the container size has changed, attempt to arrange again
		{
			arrange(new_container_width, new_container_height, called_from_parent);
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
	
	if (followsTop())
	{
		// HACK: casting away const. Should use setRect or some helper function instead.
		const_cast<LLRect&>(getRect()).mBottom = getRect().mTop - total_height;
	}
	else
	{
		// HACK: casting away const. Should use setRect or some helper function instead.
		const_cast<LLRect&>(getRect()).mTop = getRect().mBottom + total_height;
	}
	// HACK: casting away const. Should use setRect or some helper function instead.
		const_cast<LLRect&>(getRect()).mRight = getRect().mLeft + width;

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
