/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
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


#ifndef LL_AUDIOENGINE_OPENAL_H
#define LL_AUDIOENGINE_OPENAL_H

#include "llaudioengine.h"
#include "lllistener_openal.h"
#include "llwindgen.h"

class LLAudioEngine_OpenAL : public LLAudioEngine
{
	public:
		LLAudioEngine_OpenAL();
		virtual ~LLAudioEngine_OpenAL();

		virtual bool init(const S32 num_channels, void *user_data);
        	virtual std::string getDriverName(bool verbose);
		virtual void allocateListener();

		virtual void shutdown();

		void setInternalGain(F32 gain);

		LLAudioBuffer* createBuffer();
		LLAudioChannel* createChannel();

		/*virtual*/ void initWind();
		/*virtual*/ void cleanupWind();
		/*virtual*/ void updateWind(LLVector3 direction, F32 camera_altitude);

	private:
		void * windDSP(void *newbuffer, int length);
	        typedef S16 WIND_SAMPLE_T;
        	LLWindGen<WIND_SAMPLE_T> *mWindGen;
        	S16 *mWindBuf;
        	U32 mWindBufFreq;
        	U32 mWindBufSamples;
        	U32 mWindBufBytes;
        	ALuint mWindSource;
	        int mNumEmptyWindALBuffers;

        	static const int MAX_NUM_WIND_BUFFERS = 80;
        	static const float WIND_BUFFER_SIZE_SEC = 0.05f; // 1/20th sec
};

class LLAudioChannelOpenAL : public LLAudioChannel
{
	public:
		LLAudioChannelOpenAL();
		virtual ~LLAudioChannelOpenAL();
	protected:
		/*virtual*/ void play();
		/*virtual*/ void playSynced(LLAudioChannel *channelp);
		/*virtual*/ void cleanup();
		/*virtual*/ bool isPlaying();

		/*virtual*/ bool updateBuffer();
		/*virtual*/ void update3DPosition();
		/*virtual*/ void updateLoop();

		ALuint mALSource;
	        ALint mLastSamplePos;
};

class LLAudioBufferOpenAL : public LLAudioBuffer{
	public:
		LLAudioBufferOpenAL();
		virtual ~LLAudioBufferOpenAL();

		bool loadWAV(const std::string& filename);
		U32 getLength();

		friend class LLAudioChannelOpenAL;
	protected:
		void cleanup();
		ALuint getBuffer() {return mALBuffer;}

		ALuint mALBuffer;
};

#endif
