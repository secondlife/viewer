/** 
 * @file llfloatervoicedevicesettings.cpp
 * @author Richard Nelson
 * @brief Voice communication set-up 
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llfloatervoicedevicesettings.h"

// Viewer includes
#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfocusmgr.h"
#include "lliconctrl.h"
#include "llprefsvoice.h"
#include "llsliderctrl.h"
#include "llviewercontrol.h"
#include "llvoiceclient.h"
#include "llimpanel.h"

// Library includes (after viewer)
#include "lluictrlfactory.h"


LLPanelVoiceDeviceSettings::LLPanelVoiceDeviceSettings()
{
	mCtrlInputDevices = NULL;
	mCtrlOutputDevices = NULL;
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mDevicesUpdated = FALSE;

	// grab "live" mic volume level
	mMicVolume = gSavedSettings.getF32("AudioLevelMic");

	// ask for new device enumeration
	// now do this in onOpen() instead...
	//gVoiceClient->refreshDeviceLists();
}

LLPanelVoiceDeviceSettings::~LLPanelVoiceDeviceSettings()
{
}

BOOL LLPanelVoiceDeviceSettings::postBuild()
{
	LLSlider* volume_slider = getChild<LLSlider>("mic_volume_slider");
	// set mic volume tuning slider based on last mic volume setting
	volume_slider->setValue(mMicVolume);

	childSetCommitCallback("voice_input_device", onCommitInputDevice, this);
	childSetCommitCallback("voice_output_device", onCommitOutputDevice, this);
	
	return TRUE;
}

void LLPanelVoiceDeviceSettings::draw()
{
	// let user know that volume indicator is not yet available
	childSetVisible("wait_text", !gVoiceClient->inTuningMode());

	LLPanel::draw();

	F32 voice_power = gVoiceClient->tuningGetEnergy();
	S32 discrete_power = 0;

	if (!gVoiceClient->inTuningMode())
	{
		discrete_power = 0;
	}
	else
	{
		discrete_power = llmin(4, llfloor((voice_power / LLVoiceClient::OVERDRIVEN_POWER_LEVEL) * 4.f));
	}
	
	if (gVoiceClient->inTuningMode())
	{
		for(S32 power_bar_idx = 0; power_bar_idx < 5; power_bar_idx++)
		{
			std::string view_name = llformat("%s%d", "bar", power_bar_idx);
			LLView* bar_view = getChild<LLView>(view_name);
			if (bar_view)
			{
				if (power_bar_idx < discrete_power)
				{
					LLColor4 color = (power_bar_idx >= 3) ? gSavedSettings.getColor4("OverdrivenColor") : gSavedSettings.getColor4("SpeakingColor");
					gl_rect_2d(bar_view->getRect(), color, TRUE);
				}
				gl_rect_2d(bar_view->getRect(), LLColor4::grey, FALSE);
			}
		}
	}
}

void LLPanelVoiceDeviceSettings::apply()
{
	std::string s;
	if(mCtrlInputDevices)
	{
		s = mCtrlInputDevices->getSimple();
		gSavedSettings.setString("VoiceInputAudioDevice", s);
	}

	if(mCtrlOutputDevices)
	{
		s = mCtrlOutputDevices->getSimple();
		gSavedSettings.setString("VoiceOutputAudioDevice", s);
	}

	// assume we are being destroyed by closing our embedding window
	gSavedSettings.setF32("AudioLevelMic", mMicVolume);
}

void LLPanelVoiceDeviceSettings::cancel()
{
	gSavedSettings.setString("VoiceInputAudioDevice", mInputDevice);
	gSavedSettings.setString("VoiceOutputAudioDevice", mOutputDevice);

	if(mCtrlInputDevices)
		mCtrlInputDevices->setSimple(mInputDevice);

	if(mCtrlOutputDevices)
		mCtrlOutputDevices->setSimple(mOutputDevice);
}

void LLPanelVoiceDeviceSettings::refresh()
{
	//grab current volume
	LLSlider* volume_slider = getChild<LLSlider>("mic_volume_slider");
	// set mic volume tuning slider based on last mic volume setting
	mMicVolume = (F32)volume_slider->getValue().asReal();
	gVoiceClient->tuningSetMicVolume(mMicVolume);

	// Fill in popup menus
	mCtrlInputDevices = getChild<LLComboBox>("voice_input_device");
	mCtrlOutputDevices = getChild<LLComboBox>("voice_output_device");

	if(!gVoiceClient->deviceSettingsAvailable())
	{
		// The combo boxes are disabled, since we can't get the device settings from the daemon just now.
		// Put the currently set default (ONLY) in the box, and select it.
		if(mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add( mInputDevice, ADD_BOTTOM );
			mCtrlInputDevices->setSimple(mInputDevice);
		}
		if(mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add( mOutputDevice, ADD_BOTTOM );
			mCtrlOutputDevices->setSimple(mOutputDevice);
		}
	}
	else if (!mDevicesUpdated)
	{
		LLVoiceClient::deviceList *devices;
		
		LLVoiceClient::deviceList::iterator iter;
		
		if(mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add( getString("default_text"), ADD_BOTTOM );

			devices = gVoiceClient->getCaptureDevices();
			for(iter=devices->begin(); iter != devices->end(); iter++)
			{
				mCtrlInputDevices->add( *iter, ADD_BOTTOM );
			}

			if(!mCtrlInputDevices->setSimple(mInputDevice))
			{
				mCtrlInputDevices->setSimple(getString("default_text"));
			}
		}
		
		if(mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add( getString("default_text"), ADD_BOTTOM );

			devices = gVoiceClient->getRenderDevices();
			for(iter=devices->begin(); iter != devices->end(); iter++)
			{
				mCtrlOutputDevices->add( *iter, ADD_BOTTOM );
			}

			if(!mCtrlOutputDevices->setSimple(mOutputDevice))
			{
				mCtrlOutputDevices->setSimple(getString("default_text"));
			}
		}
		mDevicesUpdated = TRUE;
	}	
}

void LLPanelVoiceDeviceSettings::onOpen()
{
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mMicVolume = gSavedSettings.getF32("AudioLevelMic");
	mDevicesUpdated = FALSE;

	// ask for new device enumeration
	gVoiceClient->refreshDeviceLists();

	// put voice client in "tuning" mode
	gVoiceClient->tuningStart();
	LLVoiceChannel::suspend();
}

void LLPanelVoiceDeviceSettings::onClose(bool app_quitting)
{
	gVoiceClient->tuningStop();
	LLVoiceChannel::resume();
}

// static
void LLPanelVoiceDeviceSettings::onCommitInputDevice(LLUICtrl* ctrl, void* user_data)
{
	gSavedSettings.setString("VoiceInputAudioDevice", ctrl->getValue().asString());
}

// static
void LLPanelVoiceDeviceSettings::onCommitOutputDevice(LLUICtrl* ctrl, void* user_data)
{
	gSavedSettings.setString("VoiceOutputAudioDevice", ctrl->getValue().asString());
}

//
// LLFloaterVoiceDeviceSettings
//

LLFloaterVoiceDeviceSettings::LLFloaterVoiceDeviceSettings(const LLSD& seed)
	: LLFloater(std::string("floater_device_settings")),
	  mDevicePanel(NULL)
{
	mFactoryMap["device_settings"] = LLCallbackMap(createPanelVoiceDeviceSettings, this);
	// do not automatically open singleton floaters (as result of getInstance())
	BOOL no_open = FALSE;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_device_settings.xml", &mFactoryMap, no_open);
	center();
}

void LLFloaterVoiceDeviceSettings::onOpen()
{
	if(mDevicePanel)
	{
		mDevicePanel->onOpen();
	}

	LLFloater::onOpen();
}

void LLFloaterVoiceDeviceSettings::onClose(bool app_quitting)
{
	if(mDevicePanel)
	{
		mDevicePanel->onClose(app_quitting);
	}

	setVisible(FALSE);
}

void LLFloaterVoiceDeviceSettings::apply()
{
	if (mDevicePanel)
	{
		mDevicePanel->apply();
	}
}

void LLFloaterVoiceDeviceSettings::cancel()
{
	if (mDevicePanel)
	{
		mDevicePanel->cancel();
	}
}

void LLFloaterVoiceDeviceSettings::draw()
{
	if (mDevicePanel)
	{
		mDevicePanel->refresh();
	}
	LLFloater::draw();
}

// static
void* LLFloaterVoiceDeviceSettings::createPanelVoiceDeviceSettings(void* user_data)
{
	LLFloaterVoiceDeviceSettings* floaterp = (LLFloaterVoiceDeviceSettings*)user_data;
	floaterp->mDevicePanel = new LLPanelVoiceDeviceSettings();
	return floaterp->mDevicePanel;
}
