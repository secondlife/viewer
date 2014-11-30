/** 
 * @file llpanelpresetspulldown.cpp
 * @brief A panel showing a quick way to pick presets
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#include "llpanelpresetspulldown.h"

#include "llviewercontrol.h"
#include "llstatusbar.h"

#include "llbutton.h"
#include "lltabcontainer.h"
#include "llfloaterreg.h"
#include "llfloaterpreference.h"
#include "llsliderctrl.h"

/* static */ const F32 LLPanelPresetsPulldown::sAutoCloseFadeStartTimeSec = 4.0f;
/* static */ const F32 LLPanelPresetsPulldown::sAutoCloseTotalTimeSec = 5.0f;

///----------------------------------------------------------------------------
/// Class LLPanelPresetsPulldown
///----------------------------------------------------------------------------

// Default constructor
LLPanelPresetsPulldown::LLPanelPresetsPulldown()
{
	mHoverTimer.stop();

	mCommitCallbackRegistrar.add("Presets.GoMoveViewPrefs", boost::bind(&LLPanelPresetsPulldown::onMoveViewButtonClick, this, _2));
	mCommitCallbackRegistrar.add("Presets.GoGraphicsPrefs", boost::bind(&LLPanelPresetsPulldown::onGraphicsButtonClick, this, _2));
	buildFromFile( "panel_presets_pulldown.xml");
}

BOOL LLPanelPresetsPulldown::postBuild()
{
	return LLPanel::postBuild();
}

/*virtual*/
void LLPanelPresetsPulldown::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x,y,mask);
}

/*virtual*/
void LLPanelPresetsPulldown::onTopLost()
{
	setVisible(FALSE);
}

/*virtual*/
void LLPanelPresetsPulldown::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x,y,mask);
}

/*virtual*/ 
void LLPanelPresetsPulldown::onVisibilityChange ( BOOL new_visibility )
{
	if (new_visibility)	
	{
		mHoverTimer.start(); // timer will be stopped when mouse hovers over panel
	}
	else
	{
		mHoverTimer.stop();

	}
}

void LLPanelPresetsPulldown::onMoveViewButtonClick(const LLSD& user_data)
{
	// close the minicontrol, we're bringing up the big one
	setVisible(FALSE);

	// bring up the prefs floater
	LLFloater* prefsfloater = LLFloaterReg::showInstance("preferences");
	if (prefsfloater)
	{
		// grab the 'move' panel from the preferences floater and
		// bring it the front!
		LLTabContainer* tabcontainer = prefsfloater->getChild<LLTabContainer>("pref core");
		LLPanel* movepanel = prefsfloater->getChild<LLPanel>("move");
		if (tabcontainer && movepanel)
		{
			tabcontainer->selectTabPanel(movepanel);
		}
	}
}

void LLPanelPresetsPulldown::onGraphicsButtonClick(const LLSD& user_data)
{
	// close the minicontrol, we're bringing up the big one
	setVisible(FALSE);

	// bring up the prefs floater
	LLFloater* prefsfloater = LLFloaterReg::showInstance("preferences");
	if (prefsfloater)
	{
		// grab the 'graphics' panel from the preferences floater and
		// bring it the front!
		LLTabContainer* tabcontainer = prefsfloater->getChild<LLTabContainer>("pref core");
		LLPanel* graphicspanel = prefsfloater->getChild<LLPanel>("display");
		if (tabcontainer && graphicspanel)
		{
			tabcontainer->selectTabPanel(graphicspanel);
		}
	}
}

//virtual
void LLPanelPresetsPulldown::draw()
{
	F32 alpha = mHoverTimer.getStarted() 
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), sAutoCloseFadeStartTimeSec, sAutoCloseTotalTimeSec, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);

	LLPanel::draw();

	if (alpha == 0.f)
	{
		setVisible(FALSE);
	}
}
