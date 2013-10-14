/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
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

		/*virtual*/ bool initWind();
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
    	static const float WIND_BUFFER_SIZE_SEC; // 1/20th sec
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
