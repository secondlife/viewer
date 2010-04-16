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

class Mixer
{
public:
	static Mixer* create(U32 index);
	~Mixer();

	void setMute(bool mute);
	void setVolume(F32 volume_left, F32 volume_right);

private:
	Mixer(HMIXER handle, U32 mute_control_id, U32 volume_control_id, U32 min_volume, U32 max_volume);

	HMIXER	mHandle;
	U32		mMuteControlID;
	U32		mVolumeControlID;
	U32		mMinVolume;
	U32		mMaxVolume;
};

Mixer* Mixer::create(U32 index)
{
	HMIXER mixer_handle;
	MMRESULT result = mixerOpen( &mixer_handle,
							index,	
							0,	// HWND to call when state changes - not used
							0,	// user data for callback - not used
							MIXER_OBJECTF_MIXER );

	if (result == MMSYSERR_NOERROR)
	{
		MIXERLINE mixer_line;
		mixer_line.cbStruct = sizeof( MIXERLINE );
		// try speakers first
		mixer_line.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

		MMRESULT result = mixerGetLineInfo( reinterpret_cast< HMIXEROBJ >( mixer_handle ),
								&mixer_line,
								MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE );
		if (result != MMSYSERR_NOERROR)
		{
			// failed - try headphones next
			mixer_line.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_HEADPHONES;
			result = mixerGetLineInfo( reinterpret_cast< HMIXEROBJ >( mixer_handle ),
									&mixer_line,
									MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE );
		}

		if (result == MMSYSERR_NOERROR)
		{
			// get control id
			MIXERCONTROL mixer_control;
			MIXERLINECONTROLS mixer_line_controls;
			mixer_line_controls.cbStruct = sizeof( MIXERLINECONTROLS );
			mixer_line_controls.dwLineID = mixer_line.dwLineID;
			mixer_line_controls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
			mixer_line_controls.cControls = 1;
			mixer_line_controls.cbmxctrl = sizeof( MIXERCONTROL );
			mixer_line_controls.pamxctrl = &mixer_control;

			result = mixerGetLineControls( reinterpret_cast< HMIXEROBJ >( mixer_handle ),
				&mixer_line_controls,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE );
			if (result == MMSYSERR_NOERROR )
			{
				// We have a mute mixer.  Remember the mute control id
				U32 mute_control_id = mixer_control.dwControlID;

				// now query for volume controls
				mixer_line_controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
				result = mixerGetLineControls( reinterpret_cast< HMIXEROBJ >( mixer_handle ),
					&mixer_line_controls,
					MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE );

				if (result == MMSYSERR_NOERROR)
				{
					// we have both mute and volume controls for this mixer, so remember it
					return new Mixer(mixer_handle, 
								mute_control_id, 
								mixer_control.dwControlID, 
								mixer_control.Bounds.dwMinimum, 
								mixer_control.Bounds.dwMaximum);
				}
			}
		}
	}
	return NULL;
}

Mixer::Mixer(HMIXER handle, U32 mute_control_id, U32 volume_control_id, U32 min_volume, U32 max_volume)
:	mHandle(handle),
	mMuteControlID(mute_control_id),
	mVolumeControlID(volume_control_id),
	mMinVolume(min_volume),
	mMaxVolume(max_volume)
{}

Mixer::~Mixer()
{
}

void Mixer::setMute(bool mute)
{
	MIXERCONTROLDETAILS_BOOLEAN mixer_control_details_bool = { mute };
	MIXERCONTROLDETAILS mixer_control_details;
	mixer_control_details.cbStruct = sizeof( MIXERCONTROLDETAILS );
	mixer_control_details.dwControlID = mMuteControlID;
	mixer_control_details.cChannels = 1;
	mixer_control_details.cMultipleItems = 0;
	mixer_control_details.cbDetails = sizeof( MIXERCONTROLDETAILS_BOOLEAN );
	mixer_control_details.paDetails = &mixer_control_details_bool;

	mixerSetControlDetails( reinterpret_cast< HMIXEROBJ >( mHandle ),
								 &mixer_control_details,
								 MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE );
}

void Mixer::setVolume(F32 volume_left, F32 volume_right)
{
	U32 volume_left_mixer = (U32)
							((F32)mMinVolume 
								+ (volume_left * ((F32)mMaxVolume - (F32)mMinVolume)));
	U32 volume_right_mixer = (U32)
							((F32)mMinVolume 
								+ (volume_right * ((F32)mMaxVolume - (F32)mMinVolume)));
	MIXERCONTROLDETAILS_UNSIGNED mixer_control_details_unsigned[ 2 ] = { volume_left_mixer, volume_right_mixer };
	MIXERCONTROLDETAILS mixer_control_details;
	mixer_control_details.cbStruct = sizeof( MIXERCONTROLDETAILS );
	mixer_control_details.dwControlID = mVolumeControlID;
	mixer_control_details.cChannels = 2;
	mixer_control_details.cMultipleItems = 0;
	mixer_control_details.cbDetails = sizeof( MIXERCONTROLDETAILS_UNSIGNED );
	mixer_control_details.paDetails = &mixer_control_details_unsigned;

	mixerSetControlDetails( reinterpret_cast< HMIXEROBJ >( mHandle ),
								 &mixer_control_details,
								 MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE );
}


class VolumeCatcherImpl
{
public:

	void setVolume(F32 volume);
	void setPan(F32 pan);
	
	static VolumeCatcherImpl *getInstance();
private:
	// This is a singleton class -- both callers and the component implementation should use getInstance() to find the instance.
	VolumeCatcherImpl();
	~VolumeCatcherImpl();

	static VolumeCatcherImpl *sInstance;
	
	F32 	mVolume;
	F32 	mPan;
	typedef std::vector<Mixer*> mixer_vector_t;
	mixer_vector_t	mMixers;
};

VolumeCatcherImpl *VolumeCatcherImpl::sInstance = NULL;

VolumeCatcherImpl *VolumeCatcherImpl::getInstance()
{
	if(!sInstance)
	{
		sInstance = new VolumeCatcherImpl;
	}
	
	return sInstance;
}

VolumeCatcherImpl::VolumeCatcherImpl()
:	mVolume(1.0f),
	mPan(0.f) // 0 is centered
{
	U32 num_mixers = mixerGetNumDevs();
	for (U32 mixer_index = 0; mixer_index < num_mixers; ++mixer_index)
	{
		Mixer* mixerp = Mixer::create(mixer_index);
		if (mixerp)
		{
			mMixers.push_back(mixerp);
		}
	}
}

VolumeCatcherImpl::~VolumeCatcherImpl()
{
	for(mixer_vector_t::iterator it = mMixers.begin(), end_it = mMixers.end();
		it != end_it;
		++it)
	{
		delete *it;
		*it = NULL;
	}
}


void VolumeCatcherImpl::setVolume(F32 volume)
{
	F32 left_volume = volume * min(1.f, 1.f - mPan);
	F32 right_volume = volume * max(0.f, 1.f + mPan);
	
	for(mixer_vector_t::iterator it = mMixers.begin(), end_it = mMixers.end();
		it != end_it;
		++it)
	{
		(*it)->setVolume(left_volume, right_volume);
		
		if (volume == 0.f && mVolume != 0.f)
		{
			(*it)->setMute(true);
		}
		else if (mVolume == 0.f && volume != 0.f)
		{
			(*it)->setMute(false);
		}

	}
	mVolume = volume;
}

void VolumeCatcherImpl::setPan(F32 pan)
{
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

