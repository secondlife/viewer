/** 
 * @file mac_volume_catcher.cpp
 * @brief A Mac OS X specific hack to control the volume level of all audio channels opened by a process.
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

/**************************************************************************************************************
	This code works by using CaptureComponent to capture the "Default Output" audio component
	(kAudioUnitType_Output/kAudioUnitSubType_DefaultOutput) and delegating all calls to the original component.
	It does this just to keep track of all instances of the default output component, so that it can set the
	kHALOutputParam_Volume parameter on all of them to adjust the output volume.
**************************************************************************************************************/

#include "volume_catcher.h"

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include <AudioUnit/AudioUnit.h>

struct VolumeCatcherStorage;

class VolumeCatcherImpl
{
public:

	void setVolume(F32 volume);
	void setPan(F32 pan);
	
	void setInstanceVolume(VolumeCatcherStorage *instance);
	
	std::list<VolumeCatcherStorage*> mComponentInstances;
	Component mOriginalDefaultOutput;
	Component mVolumeAdjuster;
	
	static VolumeCatcherImpl *getInstance();
private:
	// This is a singleton class -- both callers and the component implementation should use getInstance() to find the instance.
	VolumeCatcherImpl();
	static VolumeCatcherImpl *sInstance;
	
	// The singlar instance of this class is expected to last until the process exits.
	// To ensure this, we declare the destructor here but never define it, so any code which attempts to destroy the instance will not link.
	~VolumeCatcherImpl();	
	
	F32 mVolume;
	F32 mPan;
};

VolumeCatcherImpl *VolumeCatcherImpl::sInstance = NULL;;

struct VolumeCatcherStorage
{
	ComponentInstance self;
	ComponentInstance delegate;
};

static ComponentResult volume_catcher_component_entry(ComponentParameters *cp, Handle componentStorage);
static ComponentResult volume_catcher_component_open(VolumeCatcherStorage *storage, ComponentInstance self);
static ComponentResult volume_catcher_component_close(VolumeCatcherStorage *storage, ComponentInstance self);

VolumeCatcherImpl *VolumeCatcherImpl::getInstance()
{
	if(!sInstance)
	{
		sInstance = new VolumeCatcherImpl;
	}
	
	return sInstance;
}

VolumeCatcherImpl::VolumeCatcherImpl()
{
	mVolume = 1.0;	// default to full volume
	mPan = 0.0;		// and center pan
		
	ComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	// Find the original default output component
	mOriginalDefaultOutput = FindNextComponent(NULL, &desc);

	// Register our own output component with the same parameters
	mVolumeAdjuster = RegisterComponent(&desc, NewComponentRoutineUPP(volume_catcher_component_entry), 0, NULL, NULL, NULL);

	// Capture the original component, so we always get found instead.
	CaptureComponent(mOriginalDefaultOutput, mVolumeAdjuster);

}

static ComponentResult volume_catcher_component_entry(ComponentParameters *cp, Handle componentStorage)
{
	ComponentResult result = badComponentSelector;
	VolumeCatcherStorage *storage = (VolumeCatcherStorage*)componentStorage;
	
	switch(cp->what)
	{
		case kComponentOpenSelect:
//			std::cerr << "kComponentOpenSelect" << std::endl;
			result = CallComponentFunctionWithStorageProcInfo((Handle)storage, cp, (ProcPtr)volume_catcher_component_open, uppCallComponentOpenProcInfo);
		break;

		case kComponentCloseSelect:
//			std::cerr << "kComponentCloseSelect" << std::endl;
			result = CallComponentFunctionWithStorageProcInfo((Handle)storage, cp, (ProcPtr)volume_catcher_component_close, uppCallComponentCloseProcInfo);
			// CallComponentFunctionWithStorageProcInfo
		break;
		
		default:
//			std::cerr << "Delegating selector: " << cp->what << " to component instance " << storage->delegate << std::endl;
			result = DelegateComponentCall(cp, storage->delegate);
		break;
	}
	
	return result;
}

static ComponentResult volume_catcher_component_open(VolumeCatcherStorage *storage, ComponentInstance self)
{
	ComponentResult result = noErr;
	VolumeCatcherImpl *impl = VolumeCatcherImpl::getInstance();	
	
	storage = new VolumeCatcherStorage;

	storage->self = self;
	storage->delegate = NULL;

	result = OpenAComponent(impl->mOriginalDefaultOutput, &(storage->delegate));
	
	if(result != noErr)
	{
//		std::cerr << "OpenAComponent result = " << result << ", component ref = " << storage->delegate << std::endl;
		
		// If we failed to open the delagate component, our open is going to fail.  Clean things up.
		delete storage;
	}
	else
	{
		// Success -- set up this component's storage
		SetComponentInstanceStorage(self, (Handle)storage);

		// add this instance to the global list
		impl->mComponentInstances.push_back(storage);	
		
		// and set up the initial volume
		impl->setInstanceVolume(storage);
	}

	return result;
}

static ComponentResult volume_catcher_component_close(VolumeCatcherStorage *storage, ComponentInstance self)
{
	ComponentResult result = noErr;
	
	if(storage)
	{
		if(storage->delegate)
		{
			CloseComponent(storage->delegate);
			storage->delegate = NULL;
		}
		
		VolumeCatcherImpl *impl = VolumeCatcherImpl::getInstance();	
		impl->mComponentInstances.remove(storage);
		delete[] storage;
	}
		
	return result;
}

void VolumeCatcherImpl::setVolume(F32 volume)
{
	VolumeCatcherImpl *impl = VolumeCatcherImpl::getInstance();	
	impl->mVolume = volume;
	
	// Iterate through all known instances, setting the volume on each.
	for(std::list<VolumeCatcherStorage*>::iterator iter = mComponentInstances.begin(); iter != mComponentInstances.end(); ++iter)
	{
		impl->setInstanceVolume(*iter);
	}
}

void VolumeCatcherImpl::setPan(F32 pan)
{
	VolumeCatcherImpl *impl = VolumeCatcherImpl::getInstance();	
	impl->mPan = pan;
	
	// TODO: implement this.
	// This will probably require adding a "panner" audio unit to the chain somehow.
	// There's also a "3d mixer" component that we might be able to use...
}

void VolumeCatcherImpl::setInstanceVolume(VolumeCatcherStorage *instance)
{
//	std::cerr << "Setting volume on component instance: " << (instance->delegate) << " to " << mVolume << std::endl;
	
	OSStatus err = noErr;
	
	if(instance && instance->delegate)
	{
		err = AudioUnitSetParameter(
				instance->delegate, 
				kHALOutputParam_Volume, 
				kAudioUnitScope_Global,
				0, 
				mVolume, 
				0);
	}
	
	if(err)
	{
//		std::cerr << "    AudioUnitSetParameter returned " << err << std::endl;
	}
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

