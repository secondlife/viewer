/** 
 * @file windows_volume_catcher.cpp
 * @brief A Windows implementation of volume level control of all audio channels opened by a process.
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

	typedef void (*set_volume_func_t)(F32);
	typedef void (*set_mute_func_t)(bool);

	set_volume_func_t mSetVolumeFunc;
	set_mute_func_t mSetMuteFunc;

	F32 	mVolume;
	F32 	mPan;
};

VolumeCatcherImpl::VolumeCatcherImpl()
:	mVolume(1.0f),	// default volume is max
	mPan(0.f)		// default pan is centered
{
	HMODULE handle = ::LoadLibrary(L"winmm.dll");
	if(handle)
	{
		mSetVolumeFunc = (set_volume_func_t)::GetProcAddress(handle, "setPluginVolume");
		mSetMuteFunc = (set_mute_func_t)::GetProcAddress(handle, "setPluginMute");
	}
}

VolumeCatcherImpl::~VolumeCatcherImpl()
{
}


void VolumeCatcherImpl::setVolume(F32 volume)
{
	//F32 left_volume = volume * min(1.f, 1.f - mPan);
	//F32 right_volume = volume * max(0.f, 1.f + mPan);
	
	mVolume = volume;
	if (mSetMuteFunc)
	{
		mSetMuteFunc(volume == 0.f);
	}
	if (mSetVolumeFunc)
	{
		mSetVolumeFunc(mVolume);
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

