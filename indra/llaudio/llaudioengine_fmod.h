/** 
 * @file audioengine_fmod.h
 * @brief Definition of LLAudioEngine class abstracting the audio
 * support as a FMOD 3D implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_AUDIOENGINE_FMOD_H
#define LL_AUDIOENGINE_FMOD_H

#include "llaudioengine.h"
#include "lllistener_fmod.h"
#include "llwindgen.h"

#include "fmod.h"

class LLAudioStreamManagerFMOD;

class LLAudioEngine_FMOD : public LLAudioEngine 
{
public:
	LLAudioEngine_FMOD();
	virtual ~LLAudioEngine_FMOD();

	// initialization/startup/shutdown
	virtual bool init(const S32 num_channels, void *user_data);
       	virtual std::string getDriverName(bool verbose);
	virtual void allocateListener();

	virtual void shutdown();

	/*virtual*/ bool initWind();
	/*virtual*/ void cleanupWind();

	/*virtual*/void updateWind(LLVector3 direction, F32 camera_height_above_water);

#if LL_DARWIN
	typedef S32 MIXBUFFERFORMAT;
#else
	typedef S16 MIXBUFFERFORMAT;
#endif

protected:
	/*virtual*/ LLAudioBuffer *createBuffer(); // Get a free buffer, or flush an existing one if you have to.
	/*virtual*/ LLAudioChannel *createChannel(); // Create a new audio channel.

	/*virtual*/ void setInternalGain(F32 gain);
protected:
	static signed char F_CALLBACKAPI callbackMetaData(char* name, char* value, void* userdata);

	//F32 mMinDistance[MAX_BUFFERS];
	//F32 mMaxDistance[MAX_BUFFERS];

	bool mInited;

	// On Windows, userdata is the HWND of the application window.
	void* mUserData;

	LLWindGen<MIXBUFFERFORMAT> *mWindGen;
	FSOUND_DSPUNIT *mWindDSP;
};


class LLAudioChannelFMOD : public LLAudioChannel
{
public:
	LLAudioChannelFMOD();
	virtual ~LLAudioChannelFMOD();

protected:
	/*virtual*/ void play();
	/*virtual*/ void playSynced(LLAudioChannel *channelp);
	/*virtual*/ void cleanup();
	/*virtual*/ bool isPlaying();

	/*virtual*/ bool updateBuffer();
	/*virtual*/ void update3DPosition();
	/*virtual*/ void updateLoop();

protected:
	int mChannelID;
	S32 mLastSamplePos;
};


class LLAudioBufferFMOD : public LLAudioBuffer
{
public:
	LLAudioBufferFMOD();
	virtual ~LLAudioBufferFMOD();

	/*virtual*/ bool loadWAV(const std::string& filename);
	/*virtual*/ U32 getLength();
	friend class LLAudioChannelFMOD;

	void set3DMode(bool use3d);
protected:
	FSOUND_SAMPLE *getSample()	{ return mSamplep; }
protected:
	FSOUND_SAMPLE *mSamplep;
};


#endif // LL_AUDIOENGINE_FMOD_H
