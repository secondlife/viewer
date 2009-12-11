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
#include "llbutton.h"
#include "llcombobox.h"
#include "llfocusmgr.h"
#include "lliconctrl.h"
#include "llviewercontrol.h"
#include "llvoiceclient.h"
#include "llvoicechannel.h"

// Library includes (after viewer)
#include "lluictrlfactory.h"


static LLRegisterPanelClassWrapper<LLPanelVoiceDeviceSettings> t_panel_group_general("panel_voice_device_settings");


LLPanelVoiceDeviceSettings::LLPanelVoiceDeviceSettings()
	: LLPanel()
{
	mCtrlInputDevices = NULL;
	mCtrlOutputDevices = NULL;
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mDevicesUpdated = FALSE;

	// ask for new device enumeration
	// now do this in onOpen() instead...
	//gVoiceClient->refreshDeviceLists();
}

LLPanelVoiceDeviceSettings::~LLPanelVoiceDeviceSettings()
{
}

BOOL LLPanelVoiceDeviceSettings::postBuild()
{
	childSetCommitCallback("voice_input_device", onCommitInputDevice, this);
	childSetCommitCallback("voice_output_device", onCommitOutputDevice, this);
	
	return TRUE;
}

// virtual
void LLPanelVoiceDeviceSettings::handleVisibilityChange ( BOOL new_visibility )
{
	if (new_visibility)
	{
		initialize();	
	}
	else
	{
		cleanup();
		// when closing this window, turn of visiblity control so that 
		// next time preferences is opened we don't suspend voice
		gSavedSettings.setBOOL("ShowDeviceSettings", FALSE);
	}
}
void LLPanelVoiceDeviceSettings::draw()
{
	refresh();

	// let user know that volume indicator is not yet available
	bool is_in_tuning_mode = gVoiceClient->inTuningMode();
	childSetVisible("wait_text", !is_in_tuning_mode);

	LLPanel::draw();

	F32 voice_power = gVoiceClient->tuningGetEnergy();
	S32 discrete_power = 0;

	if (!is_in_tuning_mode)
	{
		discrete_power = 0;
	}
	else
	{
		discrete_power = llmin(4, llfloor((voice_power / LLVoiceClient::OVERDRIVEN_POWER_LEVEL) * 4.f));
	}
	
	if (is_in_tuning_mode)
	{
		for(S32 power_bar_idx = 0; power_bar_idx < 5; power_bar_idx++)
		{
			std::string view_name = llformat("%s%d", "bar", power_bar_idx);
			LLView* bar_view = getChild<LLView>(view_name);
			if (bar_view)
			{
				if (power_bar_idx < discrete_power)
				{
					LLColor4 color = (power_bar_idx >= 3) ? LLUIColorTable::instance().getColor("OverdrivenColor") : LLUIColorTable::instance().getColor("SpeakingColor");
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
		mInputDevice = s;
	}

	if(mCtrlOutputDevices)
	{
		s = mCtrlOutputDevices->getSimple();
		gSavedSettings.setString("VoiceOutputAudioDevice", s);
		mOutputDevice = s;
	}
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
	// update the live input level display
	gVoiceClient->tuningSetMicVolume();

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

void LLPanelVoiceDeviceSettings::initialize()
{
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mDevicesUpdated = FALSE;

	// ask for new device enumeration
	gVoiceClient->refreshDeviceLists();

	// put voice client in "tuning" mode
	gVoiceClient->tuningStart();
	LLVoiceChannel::suspend();
}

void LLPanelVoiceDeviceSettings::cleanup()
{
	gVoiceClient->tuningStop();
	LLVoiceChannel::resume();
}

// static
void LLPanelVoiceDeviceSettings::onCommitInputDevice(LLUICtrl* ctrl, void* user_data)
{
	if(gVoiceClient)
	{
		gVoiceClient->setCaptureDevice(ctrl->getValue().asString());
	}
}

// static
void LLPanelVoiceDeviceSettings::onCommitOutputDevice(LLUICtrl* ctrl, void* user_data)
{
	if(gVoiceClient)
	{
		gVoiceClient->setRenderDevice(ctrl->getValue().asString());
	}
}

//
// LLFloaterVoiceDeviceSettings
//

LLFloaterVoiceDeviceSettings::LLFloaterVoiceDeviceSettings(const LLSD& seed)
	: LLFloater(seed),
	  mDevicePanel(NULL)
{
	mFactoryMap["device_settings"] = LLCallbackMap(createPanelVoiceDeviceSettings, this);
	// do not automatically open singleton floaters (as result of getInstance())
//	BOOL no_open = FALSE;
//	Called from floater reg:  LLUICtrlFactory::getInstance()->buildFloater(this, "floater_device_settings.xml", no_open);	
}
BOOL LLFloaterVoiceDeviceSettings::postBuild()
{
	center();
	return TRUE;
}

// virtual
void LLFloaterVoiceDeviceSettings::onOpen(const LLSD& key)
{
	if(mDevicePanel)
	{
		mDevicePanel->initialize();
	}
}

// virtual
void LLFloaterVoiceDeviceSettings::onClose(bool app_settings)
{
	if(mDevicePanel)
	{
		mDevicePanel->apply();
		mDevicePanel->cleanup();
	}
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
