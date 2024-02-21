/** 
* @file llfloaterhoverheight.cpp
* @brief Controller for self avatar hover height
* @author vir@lindenlab.com
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

#include "llfloaterhoverheight.h"
#include "llsliderctrl.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"

LLFloaterHoverHeight::LLFloaterHoverHeight(const LLSD& key) : LLFloater(key)
{
}

void LLFloaterHoverHeight::syncFromPreferenceSetting(void *user_data, bool update_offset)
{
	F32 value = gSavedPerAccountSettings.getF32("AvatarHoverOffsetZ");

	LLFloaterHoverHeight *self = static_cast<LLFloaterHoverHeight*>(user_data);
	LLSliderCtrl* sldrCtrl = self->getChild<LLSliderCtrl>("HoverHeightSlider");
	sldrCtrl->setValue(value,false);

	if (isAgentAvatarValid() && update_offset)
	{
		LLVector3 offset(0.0, 0.0, llclamp(value,MIN_HOVER_Z,MAX_HOVER_Z));
		LL_INFOS("Avatar") << "setting hover from preference setting " << offset[2] << LL_ENDL;
		gAgentAvatarp->setHoverOffset(offset);
		//gAgentAvatarp->sendHoverHeight();
	}
}

bool LLFloaterHoverHeight::postBuild()
{
	LLSliderCtrl* sldrCtrl = getChild<LLSliderCtrl>("HoverHeightSlider");
	sldrCtrl->setMinValue(MIN_HOVER_Z);
	sldrCtrl->setMaxValue(MAX_HOVER_Z);
	sldrCtrl->setSliderMouseUpCallback(boost::bind(&LLFloaterHoverHeight::onFinalCommit,this));
	sldrCtrl->setSliderEditorCommitCallback(boost::bind(&LLFloaterHoverHeight::onFinalCommit,this));
	childSetCommitCallback("HoverHeightSlider", &LLFloaterHoverHeight::onSliderMoved, NULL);

	// Initialize slider from pref setting.
	syncFromPreferenceSetting(this);
	// Update slider on future pref changes.
	if (gSavedPerAccountSettings.getControl("AvatarHoverOffsetZ"))
	{
		gSavedPerAccountSettings.getControl("AvatarHoverOffsetZ")->getCommitSignal()->connect(boost::bind(&syncFromPreferenceSetting, this, false));
	}
	else
	{
		LL_WARNS() << "Control not found for AvatarHoverOffsetZ" << LL_ENDL;
	}

	updateEditEnabled();

	if (!mRegionChangedSlot.connected())
	{
		mRegionChangedSlot = gAgent.addRegionChangedCallback(boost::bind(&LLFloaterHoverHeight::onRegionChanged,this));
	}
	// Set up based on initial region.
	onRegionChanged();

	return true;
}

void LLFloaterHoverHeight::onClose(bool app_quitting)
{
	if (mRegionChangedSlot.connected())
	{
		mRegionChangedSlot.disconnect();
	}
}

// static
void LLFloaterHoverHeight::onSliderMoved(LLUICtrl* ctrl, void* userData)
{
    if (isAgentAvatarValid())
    {
        LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
        F32 value = sldrCtrl->getValueF32();
        LLVector3 offset(0.0, 0.0, llclamp(value, MIN_HOVER_Z, MAX_HOVER_Z));
        LL_INFOS("Avatar") << "setting hover from slider moved" << offset[2] << LL_ENDL;
        gAgentAvatarp->setHoverOffset(offset, false);
    }
}

// Do send-to-the-server work when slider drag completes, or new
// value entered as text.
void LLFloaterHoverHeight::onFinalCommit()
{
	LLSliderCtrl* sldrCtrl = getChild<LLSliderCtrl>("HoverHeightSlider");
	F32 value = sldrCtrl->getValueF32();
	gSavedPerAccountSettings.setF32("AvatarHoverOffsetZ",value);
}

void LLFloaterHoverHeight::onRegionChanged()
{
	LLViewerRegion *region = gAgent.getRegion();
	if (region && region->simulatorFeaturesReceived())
	{
		updateEditEnabled();
	}
	else if (region)
	{
		region->setSimulatorFeaturesReceivedCallback(boost::bind(&LLFloaterHoverHeight::onSimulatorFeaturesReceived,this,_1));
	}
}

void LLFloaterHoverHeight::onSimulatorFeaturesReceived(const LLUUID &region_id)
{
	LLViewerRegion *region = gAgent.getRegion();
	if (region && (region->getRegionID()==region_id))
	{
		updateEditEnabled();
	}
}

void LLFloaterHoverHeight::updateEditEnabled()
{
	bool enabled = gAgent.getRegion() && gAgent.getRegion()->avatarHoverHeightEnabled();
	LLSliderCtrl* sldrCtrl = getChild<LLSliderCtrl>("HoverHeightSlider");
	sldrCtrl->setEnabled(enabled);
	if (enabled)
	{
		syncFromPreferenceSetting(this);
	}
}


