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

#ifndef LL_LLPANELVOICEDEVICESETTINGS_H
#define LL_LLPANELVOICEDEVICESETTINGS_H

#include "llpanel.h"

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
	void onCommitInputDevice();
	void onCommitOutputDevice();

	F32 mMicVolume;
	std::string mInputDevice;
	std::string mOutputDevice;
	class LLComboBox		*mCtrlInputDevices;
	class LLComboBox		*mCtrlOutputDevices;
	BOOL mDevicesUpdated;
};

#endif // LL_LLPANELVOICEDEVICESETTINGS_H
