/** 
 * @file llchicletbar.cpp
 * @brief LLChicletBar class implementation
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h" // must be first include
#include "llchicletbar.h"

#include "llchiclet.h"
#include "lllayoutstack.h"
#include "llpaneltopinfobar.h"
#include "llsyswellwindow.h"
#include "llfloaternotificationstabbed.h"

namespace
{
	const std::string& PANEL_CHICLET_NAME	= "chiclet_list_panel";
}

LLChicletBar::LLChicletBar()
:	mChicletPanel(NULL),
	mToolbarStack(NULL)
{
	buildFromFile("panel_chiclet_bar.xml");
}

BOOL LLChicletBar::postBuild()
{
	mToolbarStack = getChild<LLLayoutStack>("toolbar_stack");
	mChicletPanel = getChild<LLChicletPanel>("chiclet_list");

	showWellButton("notification_well", !LLFloaterNotificationsTabbed::getInstance()->isWindowEmpty());

	LLPanelTopInfoBar::instance().setResizeCallback(boost::bind(&LLChicletBar::fitWithTopInfoBar, this));
	LLPanelTopInfoBar::instance().setVisibleCallback(boost::bind(&LLChicletBar::fitWithTopInfoBar, this));

	return TRUE;
}

void LLChicletBar::showWellButton(const std::string& well_name, bool visible)
{
	LLView * panel = findChild<LLView>(well_name + "_panel");
	if (!panel)	return;

	panel->setVisible(visible);
}

void LLChicletBar::log(LLView* panel, const std::string& descr)
{
	if (NULL == panel) return;
	LLView* layout = panel->getParent();
	LL_DEBUGS("Chiclet Bar Rects") << descr << ": "
		<< "panel: " << panel->getName()
		<< ", rect: " << panel->getRect()
		<< " layout: " << layout->getName()
		<< ", rect: " << layout->getRect()
		<< LL_ENDL;
}

void LLChicletBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	static S32 debug_calling_number = 0;
	LL_DEBUGS() << "**************************************** " << ++debug_calling_number << LL_ENDL;

	S32 current_width = getRect().getWidth();
	S32 delta_width = width - current_width;
	LL_DEBUGS() << "Reshaping: "
		<< ", width: " << width
		<< ", cur width: " << current_width
		<< ", delta_width: " << delta_width
		<< ", called_from_parent: " << called_from_parent
		<< LL_ENDL;

	if (mChicletPanel)			log(mChicletPanel, "before");

	// Difference between chiclet bar width required to fit its children and the actual width. (see EXT-991)
	// Positive value means that chiclet bar is not wide enough.
	// Negative value means that there is free space.
	static S32 extra_shrink_width = 0;
	bool should_be_reshaped = true;

	if (mChicletPanel && mToolbarStack)
	{
		// Firstly, update layout stack to ensure we deal with correct panel sizes.
		{
			// Force the updating of layout to reset panels collapse factor.
			mToolbarStack->updateLayout();
		}

		// chiclet bar is narrowed
		if (delta_width < 0)
		{
			if (extra_shrink_width > 0) // not enough space
			{
				extra_shrink_width += llabs(delta_width);
				should_be_reshaped = false;
			}
			else
			{
				extra_shrink_width = processWidthDecreased(delta_width);

				// increase new width to extra_shrink_width value to not reshape less than chiclet bar minimum
				width += extra_shrink_width;
			}
		}
		// chiclet bar is widened
		else
		{
			if (extra_shrink_width > delta_width)
			{
				// Still not enough space.
				// Only subtract the delta from the required delta and don't reshape.
				extra_shrink_width -= delta_width;
				should_be_reshaped = false;
			}
			else if (extra_shrink_width > 0)
			{
				// If we have some extra shrink width let's reduce delta_width & width
				delta_width -= extra_shrink_width;
				width -= extra_shrink_width;
				extra_shrink_width = 0;
			}
		}
	}

	if (should_be_reshaped)
	{
		LL_DEBUGS() << "Reshape all children with width: " << width << LL_ENDL;
		LLPanel::reshape(width, height, called_from_parent);
	}

	if (mChicletPanel)			log(mChicletPanel, "after");
}

S32 LLChicletBar::processWidthDecreased(S32 delta_width)
{
	bool still_should_be_processed = true;

	const S32 chiclet_panel_shrink_headroom = getChicletPanelShrinkHeadroom();

	// Decreasing width of chiclet panel.
	if (chiclet_panel_shrink_headroom > 0)
	{
		// we have some space to decrease chiclet panel
		S32 shrink_by = llmin(-delta_width, chiclet_panel_shrink_headroom);

		LL_DEBUGS() << "delta_width: " << delta_width
			<< ", panel_delta_min: " << chiclet_panel_shrink_headroom
			<< ", shrink_by: " << shrink_by
			<< LL_ENDL;

		// is chiclet panel wide enough to process resizing?
		delta_width += chiclet_panel_shrink_headroom;

		still_should_be_processed = delta_width < 0;

		LL_DEBUGS() << "Shrinking chiclet panel by " << shrink_by << " px" << LL_ENDL;
		mChicletPanel->getParent()->reshape(mChicletPanel->getParent()->getRect().getWidth() - shrink_by, mChicletPanel->getParent()->getRect().getHeight());
		log(mChicletPanel, "after processing panel decreasing via chiclet panel");

		LL_DEBUGS() << "RS_CHICLET_PANEL"
			<< ", delta_width: " << delta_width
			<< LL_ENDL;
	}

	S32 extra_shrink_width = 0;

	if (still_should_be_processed)
	{
		extra_shrink_width = -delta_width;
		LL_WARNS() << "There is no enough width to reshape all children: "
			<< extra_shrink_width << LL_ENDL;
	}

	return extra_shrink_width;
}

S32 LLChicletBar::getChicletPanelShrinkHeadroom() const
{
	static const S32 min_width = mChicletPanel->getMinWidth();
	const S32 cur_width = mChicletPanel->getParent()->getRect().getWidth();

	S32 shrink_headroom = cur_width - min_width;
	llassert(shrink_headroom >= 0); // the panel cannot get narrower than the minimum
	return shrink_headroom;
}

void LLChicletBar::fitWithTopInfoBar()
{
	LLPanelTopInfoBar& top_info_bar = LLPanelTopInfoBar::instance();

	LLRect rect = getRect();
	S32 width = rect.getWidth();

	if (top_info_bar.getVisible())
	{
		S32 delta = top_info_bar.calcScreenRect().mRight - calcScreenRect().mLeft;
		if (delta < 0 && rect.mLeft < llabs(delta))
			delta = -rect.mLeft;
		rect.setLeftTopAndSize(rect.mLeft + delta, rect.mTop, rect.getWidth(), rect.getHeight());
		width = rect.getWidth() - delta;
	}
	else
	{
		LLView* parent = getParent();
		if (parent)
		{
			LLRect parent_rect = parent->getRect();
			rect.setLeftTopAndSize(0, rect.mTop, rect.getWidth(), rect.getHeight());
			width = parent_rect.getWidth();
		}
	}

	setRect(rect);
	LLPanel::reshape(width, rect.getHeight(), false);
}
