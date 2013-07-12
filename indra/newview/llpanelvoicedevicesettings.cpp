/** 
 * @file llpanelvoicedevicesettings.cpp
 * @author Richard Nelson
 * @brief Voice communication set-up 
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llpanelvoicedevicesettings.h"

// Viewer includes
#include "llcombobox.h"
#include "llsliderctrl.h"
#include "llviewercontrol.h"
#include "llvoiceclient.h"
#include "llvoicechannel.h"

// Library includes (after viewer)
#include "lluictrlfactory.h"


static LLRegisterPanelClassWrapper<LLPanelVoiceDeviceSettings> t_panel_group_general("panel_voice_device_settings");
static const std::string DEFAULT_DEVICE("Default");


LLPanelVoiceDeviceSettings::LLPanelVoiceDeviceSettings()
	: LLPanel()
{
	mCtrlInputDevices = NULL;
	mCtrlOutputDevices = NULL;
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mDevicesUpdated = FALSE;
	mUseTuningMode = true;

	// grab "live" mic volume level
	mMicVolume = gSavedSettings.getF32("AudioLevelMic");

}

LLPanelVoiceDeviceSettings::~LLPanelVoiceDeviceSettings()
{
}

BOOL LLPanelVoiceDeviceSettings::postBuild()
{
	LLSlider* volume_slider = getChild<LLSlider>("mic_volume_slider");
	// set mic volume tuning slider based on last mic volume setting
	volume_slider->setValue(mMicVolume);

	mCtrlInputDevices = getChild<LLComboBox>("voice_input_device");
	mCtrlOutputDevices = getChild<LLComboBox>("voice_output_device");

	mCtrlInputDevices->setCommitCallback(
		boost::bind(&LLPanelVoiceDeviceSettings::onCommitInputDevice, this));
	mCtrlOutputDevices->setCommitCallback(
		boost::bind(&LLPanelVoiceDeviceSettings::onCommitOutputDevice, this));

	mLocalizedDeviceNames[DEFAULT_DEVICE]				= getString("default_text");
	mLocalizedDeviceNames["No Device"]					= getString("name_no_device");
	mLocalizedDeviceNames["Default System Device"]		= getString("name_default_system_device");
	
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
	bool is_in_tuning_mode = LLVoiceClient::getInstance()->inTuningMode();
	getChildView("wait_text")->setVisible( !is_in_tuning_mode && mUseTuningMode);

	LLPanel::draw();

	if (is_in_tuning_mode)
	{
		const S32 num_bars = 5;
		F32 voice_power = LLVoiceClient::getInstance()->tuningGetEnergy() / LLVoiceClient::OVERDRIVEN_POWER_LEVEL;
		S32 discrete_power = llmin(num_bars, llfloor(voice_power * (F32)num_bars + 0.1f));

		for(S32 power_bar_idx = 0; power_bar_idx < num_bars; power_bar_idx++)
		{
			std::string view_name = llformat("%s%d", "bar", power_bar_idx);
			LLView* bar_view = getChild<LLView>(view_name);
			if (bar_view)
			{
				gl_rect_2d(bar_view->getRect(), LLColor4::grey, TRUE);

				LLColor4 color;
				if (power_bar_idx < discrete_power)
				{
					color = (power_bar_idx >= 3) ? LLUIColorTable::instance().getColor("OverdrivenColor") : LLUIColorTable::instance().getColor("SpeakingColor");
				}
				else
				{
					color = LLUIColorTable::instance().getColor("PanelFocusBackgroundColor");
				}

				LLRect color_rect = bar_view->getRect();
				color_rect.stretch(-1);
				gl_rect_2d(color_rect, color, TRUE);
			}
		}
	}
}

void LLPanelVoiceDeviceSettings::apply()
{
	std::string s;
	if(mCtrlInputDevices)
	{
		s = mCtrlInputDevices->getValue().asString();
		gSavedSettings.setString("VoiceInputAudioDevice", s);
		mInputDevice = s;
	}

	if(mCtrlOutputDevices)
	{
		s = mCtrlOutputDevices->getValue().asString();
		gSavedSettings.setString("VoiceOutputAudioDevice", s);
		mOutputDevice = s;
	}

	// assume we are being destroyed by closing our embedding window
	LLSlider* volume_slider = getChild<LLSlider>("mic_volume_slider");
	if(volume_slider)
	{
		F32 slider_value = (F32)volume_slider->getValue().asReal();
		gSavedSettings.setF32("AudioLevelMic", slider_value);
		mMicVolume = slider_value;
	}
}

void LLPanelVoiceDeviceSettings::cancel()
{
	gSavedSettings.setString("VoiceInputAudioDevice", mInputDevice);
	gSavedSettings.setString("VoiceOutputAudioDevice", mOutputDevice);

	if(mCtrlInputDevices)
		mCtrlInputDevices->setValue(mInputDevice);

	if(mCtrlOutputDevices)
		mCtrlOutputDevices->setValue(mOutputDevice);

	gSavedSettings.setF32("AudioLevelMic", mMicVolume);
	LLSlider* volume_slider = getChild<LLSlider>("mic_volume_slider");
	if(volume_slider)
	{
		volume_slider->setValue(mMicVolume);
	}
}

void LLPanelVoiceDeviceSettings::refresh()
{
	//grab current volume
	LLSlider* volume_slider = getChild<LLSlider>("mic_volume_slider");
	// set mic volume tuning slider based on last mic volume setting
	F32 current_volume = (F32)volume_slider->getValue().asReal();
	LLVoiceClient::getInstance()->tuningSetMicVolume(current_volume);

	// Fill in popup menus
	bool device_settings_available = LLVoiceClient::getInstance()->deviceSettingsAvailable();

	if (mCtrlInputDevices)
	{
		mCtrlInputDevices->setEnabled(device_settings_available);
	}

	if (mCtrlOutputDevices)
	{
		mCtrlOutputDevices->setEnabled(device_settings_available);
	}

	getChild<LLSlider>("mic_volume_slider")->setEnabled(device_settings_available);

	if(!device_settings_available)
	{
		// The combo boxes are disabled, since we can't get the device settings from the daemon just now.
		// Put the currently set default (ONLY) in the box, and select it.
		if(mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add(getLocalizedDeviceName(mInputDevice), mInputDevice, ADD_BOTTOM);
			mCtrlInputDevices->setValue(mInputDevice);
		}
		if(mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add(getLocalizedDeviceName(mOutputDevice), mOutputDevice, ADD_BOTTOM);
			mCtrlOutputDevices->setValue(mOutputDevice);
		}
		mDevicesUpdated = FALSE;
	}
	else if (!mDevicesUpdated)
	{
		LLVoiceDeviceList::const_iterator iter;
		
		if(mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add(getLocalizedDeviceName(DEFAULT_DEVICE), DEFAULT_DEVICE, ADD_BOTTOM);

			for(iter=LLVoiceClient::getInstance()->getCaptureDevices().begin(); 
				iter != LLVoiceClient::getInstance()->getCaptureDevices().end();
				iter++)
			{
				mCtrlInputDevices->add(getLocalizedDeviceName(*iter), *iter, ADD_BOTTOM);
			}

			// Fix invalid input audio device preference.
			if (!mCtrlInputDevices->setSelectedByValue(mInputDevice, TRUE))
			{
				mCtrlInputDevices->setValue(DEFAULT_DEVICE);
				gSavedSettings.setString("VoiceInputAudioDevice", DEFAULT_DEVICE);
				mInputDevice = DEFAULT_DEVICE;
			}
		}
		
		if(mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add(getLocalizedDeviceName(DEFAULT_DEVICE), DEFAULT_DEVICE, ADD_BOTTOM);

			for(iter= LLVoiceClient::getInstance()->getRenderDevices().begin(); 
				iter !=  LLVoiceClient::getInstance()->getRenderDevices().end(); iter++)
			{
				mCtrlOutputDevices->add(getLocalizedDeviceName(*iter), *iter, ADD_BOTTOM);
			}

			// Fix invalid output audio device preference.
			if (!mCtrlOutputDevices->setSelectedByValue(mOutputDevice, TRUE))
			{
				mCtrlOutputDevices->setValue(DEFAULT_DEVICE);
				gSavedSettings.setString("VoiceOutputAudioDevice", DEFAULT_DEVICE);
				mOutputDevice = DEFAULT_DEVICE;
			}
		}
		mDevicesUpdated = TRUE;
	}	
}

void LLPanelVoiceDeviceSettings::initialize()
{
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mMicVolume = gSavedSettings.getF32("AudioLevelMic");
	mDevicesUpdated = FALSE;

	// ask for new device enumeration
	LLVoiceClient::getInstance()->refreshDeviceLists();

	// put voice client in "tuning" mode
	if (mUseTuningMode)
	{
		LLVoiceClient::getInstance()->tuningStart();
		LLVoiceChannel::suspend();
	}
}

void LLPanelVoiceDeviceSettings::cleanup()
{
	if (mUseTuningMode)
	{
		LLVoiceClient::getInstance()->tuningStop();
		LLVoiceChannel::resume();
	}
}

// returns English name if no translation found
std::string LLPanelVoiceDeviceSettings::getLocalizedDeviceName(const std::string& en_dev_name)
{
	std::map<std::string, std::string>::const_iterator it = mLocalizedDeviceNames.find(en_dev_name);
	return it != mLocalizedDeviceNames.end() ? it->second : en_dev_name;
}

void LLPanelVoiceDeviceSettings::onCommitInputDevice()
{
	if(LLVoiceClient::getInstance())
	{
		LLVoiceClient::getInstance()->setCaptureDevice(
			mCtrlInputDevices->getValue().asString());
	}
}

void LLPanelVoiceDeviceSettings::onCommitOutputDevice()
{
	if(LLVoiceClient::getInstance())
	{
		LLVoiceClient::getInstance()->setRenderDevice(
			mCtrlInputDevices->getValue().asString());
	}
}
