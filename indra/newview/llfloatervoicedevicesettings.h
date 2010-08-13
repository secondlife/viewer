/** 
 * @file llpanelvoicedevicesettings.h
 * @author Richard Nelson
 * @brief Voice communication set-up wizard
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

#ifndef LL_LLFLOATERVOICEDEVICESETTINGS_H
#define LL_LLFLOATERVOICEDEVICESETTINGS_H

#include "llfloater.h"

class LLPanelVoiceDeviceSettings : public LLPanel
{
public:
	LLPanelVoiceDeviceSettings();
	~LLPanelVoiceDeviceSettings();

	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();
	void apply();
	void cancel();
	void refresh();
	void initialize();
	void cleanup();

	/*virtual*/ void handleVisibilityChange ( BOOL new_visibility );
	
protected:
	static void onCommitInputDevice(LLUICtrl* ctrl, void* user_data);
	static void onCommitOutputDevice(LLUICtrl* ctrl, void* user_data);

	F32 mMicVolume;
	std::string mInputDevice;
	std::string mOutputDevice;
	class LLComboBox		*mCtrlInputDevices;
	class LLComboBox		*mCtrlOutputDevices;
	BOOL mDevicesUpdated;
};

class LLFloaterVoiceDeviceSettings : public LLFloater
{
	friend class LLFloaterReg;

public:

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_settings);
	/*virtual*/ void draw();
	void apply();
	void cancel();
private:
	LLFloaterVoiceDeviceSettings(const LLSD& seed);
	
protected:
	static void* createPanelVoiceDeviceSettings(void* user_data);
		
protected:
	LLPanelVoiceDeviceSettings* mDevicePanel;
};

#endif // LL_LLFLOATERVOICEDEVICESETTINGS_H
