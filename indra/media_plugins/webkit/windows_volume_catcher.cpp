/** 
 * @file windows_volume_catcher.cpp
 * @brief A Windows implementation of volume level control of all audio channels opened by a process.
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
 * @endcond
 */

#include "volume_catcher.h"
#include <windows.h>
#include "llsingleton.h"
class VolumeCatcherImpl : public LLSingleton<VolumeCatcherImpl>
{
friend LLSingleton<VolumeCatcherImpl>;
public:

	void setVolume(F32 volume);
	void setPan(F32 pan);
	
private:
	// This is a singleton class -- both callers and the component implementation should use getInstance() to find the instance.
	VolumeCatcherImpl();
	~VolumeCatcherImpl();

	typedef void (WINAPI *set_volume_func_t)(F32);
	typedef void (WINAPI *set_mute_func_t)(bool);

	set_volume_func_t mSetVolumeFunc;
	set_mute_func_t mSetMuteFunc;

	// tests if running on Vista, 7, 8 + once in CTOR
	bool isWindowsVistaOrHigher();

	F32 	mVolume;
	F32 	mPan;
	bool mSystemIsVistaOrHigher;
};

bool VolumeCatcherImpl::isWindowsVistaOrHigher()
{
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	return osvi.dwMajorVersion >= 6;
}

VolumeCatcherImpl::VolumeCatcherImpl()
:	mVolume(1.0f),			// default volume is max
	mPan(0.f)				// default pan is centered
{
	mSystemIsVistaOrHigher = isWindowsVistaOrHigher();

	if ( mSystemIsVistaOrHigher )
	{
		HMODULE handle = ::LoadLibrary(L"winmm.dll");
		if(handle)
		{
			mSetVolumeFunc = (set_volume_func_t)::GetProcAddress(handle, "setPluginVolume");
			mSetMuteFunc = (set_mute_func_t)::GetProcAddress(handle, "setPluginMute");
		}
	}
}

VolumeCatcherImpl::~VolumeCatcherImpl()
{
}

void VolumeCatcherImpl::setVolume(F32 volume)
{
	mVolume = volume;

	if ( mSystemIsVistaOrHigher )
	{
		// set both left/right to same volume
		// TODO: use pan value to set independently
		DWORD left_channel  = (DWORD)(mVolume * 65535.0f);
		DWORD right_channel =  (DWORD)(mVolume * 65535.0f);
		DWORD hw_volume = left_channel << 16 | right_channel;
		::waveOutSetVolume(NULL, hw_volume);
	}
	else
	{
		if (mSetMuteFunc)
		{
			mSetMuteFunc(volume == 0.f);
		}
		if (mSetVolumeFunc)
		{
			mSetVolumeFunc(mVolume);
		}
	}
}

void VolumeCatcherImpl::setPan(F32 pan)
{	// remember pan for calculating individual channel levels later
	mPan = pan;
}

/////////////////////////////////////////////////////

VolumeCatcher::VolumeCatcher()
{
	pimpl = VolumeCatcherImpl::getInstance();
}

VolumeCatcher::~VolumeCatcher()
{
	// Let the instance persist until exit.
}

void VolumeCatcher::setVolume(F32 volume)
{
	pimpl->setVolume(volume);
}

void VolumeCatcher::setPan(F32 pan)
{
	pimpl->setPan(pan);
}

void VolumeCatcher::pump()
{
	// No periodic tasks are necessary for this implementation.
}


