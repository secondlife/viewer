/** 
 * @file llfloaterbeacons.cpp
 * @brief Front-end to LLPipeline controls for highlighting various kinds of objects.
 * @author Coco
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterbeacons.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llcheckboxctrl.h"
#include "pipeline.h"


LLFloaterBeacons::LLFloaterBeacons(const LLSD& seed)
:	LLFloater(seed)
{
	// Initialize pipeline states from saved settings.
	// OK to do at floater constructor time because beacons do not display unless the floater is open
	// therefore it is OK to not initialize the pipeline state before needed.
	// Note also that we should replace those pipeline statics with direct lookup of the saved settings
	// eliminating the need to keep these states in sync.
	LLPipeline::setRenderScriptedTouchBeacons(gSavedSettings.getBOOL("scripttouchbeacon"));
	LLPipeline::setRenderScriptedBeacons(     gSavedSettings.getBOOL("scriptsbeacon"));
	LLPipeline::setRenderPhysicalBeacons(     gSavedSettings.getBOOL("physicalbeacon"));
	LLPipeline::setRenderSoundBeacons(        gSavedSettings.getBOOL("soundsbeacon"));
	LLPipeline::setRenderParticleBeacons(     gSavedSettings.getBOOL("particlesbeacon"));
	LLPipeline::setRenderHighlights(          gSavedSettings.getBOOL("renderhighlights"));
	LLPipeline::setRenderBeacons(             gSavedSettings.getBOOL("renderbeacons"));
	mCommitCallbackRegistrar.add("Beacons.UICheck",	boost::bind(&LLFloaterBeacons::onClickUICheck, this,_1));
}

BOOL LLFloaterBeacons::postBuild()
{
	return TRUE;
}

// Callback attached to each check box control to both affect their main purpose
// and to implement the couple screwy interdependency rules that some have.

void LLFloaterBeacons::onClickUICheck(LLUICtrl *ctrl)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	std::string name = check->getName();
	if(name == "touch_only")
	{
		LLPipeline::toggleRenderScriptedTouchBeacons(NULL);
		// Don't allow both to be ON at the same time. Toggle the other one off if both now on.
		if (
			LLPipeline::getRenderScriptedTouchBeacons(NULL) &&
			LLPipeline::getRenderScriptedBeacons(NULL) )
		{
			LLPipeline::setRenderScriptedBeacons(FALSE);
			getChild<LLCheckBoxCtrl>("scripted")->setControlValue(LLSD(FALSE));
			getChild<LLCheckBoxCtrl>("scripted")->setValue(FALSE);
			getChild<LLCheckBoxCtrl>("touch_only")->setControlValue(LLSD(TRUE)); // just to be sure it's in sync with llpipeline
			getChild<LLCheckBoxCtrl>("touch_only")->setValue(TRUE);
		}
	}
	else if(name == "scripted")
	{
		LLPipeline::toggleRenderScriptedBeacons(NULL);
		// Don't allow both to be ON at the same time. Toggle the other one off if both now on.
		if (
			LLPipeline::getRenderScriptedTouchBeacons(NULL) &&
			LLPipeline::getRenderScriptedBeacons(NULL) )
		{
			LLPipeline::setRenderScriptedTouchBeacons(FALSE);
			getChild<LLCheckBoxCtrl>("touch_only")->setControlValue(LLSD(FALSE));
			getChild<LLCheckBoxCtrl>("touch_only")->setValue(FALSE);
			getChild<LLCheckBoxCtrl>("scripted")->setControlValue(LLSD(TRUE)); // just to be sure it's in sync with llpipeline
			getChild<LLCheckBoxCtrl>("scripted")->setValue(TRUE);
		}
	}
	else if(name == "physical")       LLPipeline::setRenderPhysicalBeacons(check->get());
	else if(name == "sounds")         LLPipeline::setRenderSoundBeacons(check->get());
	else if(name == "particles")      LLPipeline::setRenderParticleBeacons(check->get());
	else if(name == "highlights")
	{
		LLPipeline::toggleRenderHighlights(NULL);
		// Don't allow both to be OFF at the same time. Toggle the other one on if both now off.
		if (
			!LLPipeline::getRenderBeacons(NULL) &&
			!LLPipeline::getRenderHighlights(NULL) )
		{
			LLPipeline::setRenderBeacons(TRUE);
			getChild<LLCheckBoxCtrl>("beacons")->setControlValue(LLSD(TRUE));
			getChild<LLCheckBoxCtrl>("beacons")->setValue(TRUE);
			getChild<LLCheckBoxCtrl>("highlights")->setControlValue(LLSD(FALSE)); // just to be sure it's in sync with llpipeline
			getChild<LLCheckBoxCtrl>("highlights")->setValue(FALSE); 
		}
	}
	else if(name == "beacons")
	{
		LLPipeline::toggleRenderBeacons(NULL);
		// Don't allow both to be OFF at the same time. Toggle the other one on if both now off.
		if (
			!LLPipeline::getRenderBeacons(NULL) &&
			!LLPipeline::getRenderHighlights(NULL) )
		{
			LLPipeline::setRenderHighlights(TRUE);
			getChild<LLCheckBoxCtrl>("highlights")->setControlValue(LLSD(TRUE));
			getChild<LLCheckBoxCtrl>("highlights")->setValue(TRUE);
			getChild<LLCheckBoxCtrl>("beacons")->setControlValue(LLSD(FALSE)); // just to be sure it's in sync with llpipeline
			getChild<LLCheckBoxCtrl>("beacons")->setValue(FALSE); 
		}
	}
}
