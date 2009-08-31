/** 
 * @file audioengine_fmod.h
 * @brief Definition of LLAudioEngine class abstracting the audio
 * support as a FMOD 3D implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

	/*virtual*/ void initWind();
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
