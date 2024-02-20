/** 
 * @file llfloatersounddevices.cpp
 * @author Leyla Farazha
 * @brief Sound Preferences used for minimal skin
 *
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#include "llfloatersounddevices.h"

#include "lldraghandle.h"

#include "llpanelvoicedevicesettings.h"

// Library includes
#include "indra_constants.h"

// protected
LLFloaterSoundDevices::LLFloaterSoundDevices(const LLSD& key)
:	LLTransientDockableFloater(NULL, false, key)
{
	LLTransientFloaterMgr::getInstance()->addControlView(this);

	// force docked state since this floater doesn't save it between recreations
	setDocked(true);
}

LLFloaterSoundDevices::~LLFloaterSoundDevices()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

// virtual
bool LLFloaterSoundDevices::postBuild()
{
	LLTransientDockableFloater::postBuild();

	updateTransparency(TT_ACTIVE); // force using active floater transparency (STORM-730)

	LLPanelVoiceDeviceSettings* panel = findChild<LLPanelVoiceDeviceSettings>("device_settings_panel");
	if (panel)
	{
		panel->setUseTuningMode(false);
		getChild<LLUICtrl>("voice_input_device")->setCommitCallback(boost::bind(&LLPanelVoiceDeviceSettings::apply, panel));
		getChild<LLUICtrl>("voice_output_device")->setCommitCallback(boost::bind(&LLPanelVoiceDeviceSettings::apply, panel));
		getChild<LLUICtrl>("mic_volume_slider")->setCommitCallback(boost::bind(&LLPanelVoiceDeviceSettings::apply, panel));
	}
	return true;
}

//virtual
void LLFloaterSoundDevices::setDocked(bool docked, bool pop_on_undock/* = true*/)
{
	LLTransientDockableFloater::setDocked(docked, pop_on_undock);
}

// virtual
void LLFloaterSoundDevices::setFocus(bool b)
{
	LLTransientDockableFloater::setFocus(b);

	// Force using active floater transparency
	// We have to override setFocus() for because selecting an item of the
	// combobox causes the floater to lose focus and thus become transparent.
	updateTransparency(TT_ACTIVE);
}
