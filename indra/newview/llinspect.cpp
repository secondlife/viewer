/** 
 * @file llinspect.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"

#include "llinspect.h"

#include "lltooltip.h"
#include "llcontrol.h"	// LLCachedControl
#include "llui.h"		// LLUI::sSettingsGroups
#include "llviewermenu.h"

LLInspect::LLInspect(const LLSD& key)
:	LLFloater(key),
	mCloseTimer(),
	mOpenTimer()
{
}

LLInspect::~LLInspect()
{
}

// virtual
void LLInspect::draw()
{
	static LLCachedControl<F32> FADE_TIME(*LLUI::sSettingGroups["config"], "InspectorFadeTime", 1.f);
	static LLCachedControl<F32> STAY_TIME(*LLUI::sSettingGroups["config"], "InspectorShowTime", 1.f);
	if (mOpenTimer.getStarted())
	{
		LLFloater::draw();
		if (mOpenTimer.getElapsedTimeF32() > STAY_TIME)
		{
			mOpenTimer.stop();
			mCloseTimer.start();
		}

	}
	else if (mCloseTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mCloseTimer.getElapsedTimeF32(), 0.f, FADE_TIME, 1.f, 0.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
		if (mCloseTimer.getElapsedTimeF32() > FADE_TIME)
		{
			closeFloater(false);
		}
	}
	else
	{
		LLFloater::draw();
	}
}

// virtual
void LLInspect::onOpen(const LLSD& data)
{
	LLFloater::onOpen(data);
	
	mCloseTimer.stop();
	mOpenTimer.start();
}

// virtual
void LLInspect::onFocusLost()
{
	LLFloater::onFocusLost();
	
	// Start closing when we lose focus
	mCloseTimer.start();
	mOpenTimer.stop();
}

// virtual
BOOL LLInspect::handleHover(S32 x, S32 y, MASK mask)
{
	mOpenTimer.pause();
	return LLView::handleHover(x, y, mask);
}

BOOL LLInspect::handleToolTip(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;


	//delegate handling of tooltip to the hovered child
	LLView* child_handler = childFromPoint(x,y);
	if (child_handler && !child_handler->getToolTip().empty())// show tooltip if a view has non-empty tooltip message
	{
		//build LLInspector params to get correct tooltip setting, etc. background image
		LLInspector::Params params;
		params.fillFrom(LLUICtrlFactory::instance().getDefaultParams<LLInspector>());
		params.message = child_handler->getToolTip();
		//set up delay if there is no visible tooltip at this moment
		params.delay_time =  LLToolTipMgr::instance().toolTipVisible() ? 0.f : LLUI::sSettingGroups["config"]->getF32( "ToolTipDelay" );
		LLToolTipMgr::instance().show(params);
		handled = TRUE;
	}
	return handled;
}
// virtual
void LLInspect::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mOpenTimer.unpause();
}

bool LLInspect::childHasVisiblePopupMenu()
{
	// Child text-box may spawn a pop-up menu, if mouse is over the menu, Inspector 
	// will hide(which is not expected).
	// This is an attempt to find out if child control has spawned a menu.

	LLView* child_menu = gMenuHolder->getVisibleMenu();
	if(child_menu)
	{
		LLRect floater_rc = calcScreenRect();
		LLRect menu_screen_rc = child_menu->calcScreenRect();
		S32 mx, my;
		LLUI::getMousePositionScreen(&mx, &my);

		// This works wrong if we spawn a menu near Inspector and menu overlaps Inspector.
		if(floater_rc.overlaps(menu_screen_rc) && menu_screen_rc.pointInRect(mx, my))
		{
			return true;
		}
	}
	return false;
}
