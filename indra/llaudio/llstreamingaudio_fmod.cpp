/** 
 * @file streamingaudio_fmod.cpp
 * @brief LLStreamingAudio_FMOD implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llmath.h"

#include "fmod.h"
#include "fmod_errors.h"

#include "llstreamingaudio_fmod.h"


class LLAudioStreamManagerFMOD
{
public:
	LLAudioStreamManagerFMOD(const std::string& url);
	int	startStream();
	bool stopStream(); // Returns true if the stream was successfully stopped.
	bool ready();

	const std::string& getURL() 	{ return mInternetStreamURL; }

	int getOpenState();
protected:
	FSOUND_STREAM* mInternetStream;
	bool mReady;

	std::string mInternetStreamURL;
};



//---------------------------------------------------------------------------
// Internet Streaming
//---------------------------------------------------------------------------
LLStreamingAudio_FMOD::LLStreamingAudio_FMOD() :
	mCurrentInternetStreamp(NULL),
	mFMODInternetStreamChannel(-1),
	mGain(1.0f)
{
	// Number of milliseconds of audio to buffer for the audio card.
	// Must be larger than the usual Second Life frame stutter time.
	FSOUND_Stream_SetBufferSize(200);

	// Here's where we set the size of the network buffer and some buffering 
	// parameters.  In this case we want a network buffer of 16k, we want it 
	// to prebuffer 40% of that when we first connect, and we want it 
	// to rebuffer 80% of that whenever we encounter a buffer underrun.

	// Leave the net buffer properties at the default.
	//FSOUND_Stream_Net_SetBufferProperties(20000, 40, 80);
}


LLStreamingAudio_FMOD::~LLStreamingAudio_FMOD()
{
	// nothing interesting/safe to do.
}


void LLStreamingAudio_FMOD::start(const std::string& url)
{
	//if (!mInited)
	//{
	//	llwarns << "startInternetStream before audio initialized" << llendl;
	//	return;
	//}

	// "stop" stream but don't clear url, etc. in case url == mInternetStreamURL
	stop();

	if (!url.empty())
	{
		llinfos << "Starting internet stream: " << url << llendl;
		mCurrentInternetStreamp = new LLAudioStreamManagerFMOD(url);
		mURL = url;
	}
	else
	{
		llinfos << "Set internet stream to null" << llendl;
		mURL.clear();
	}
}


void LLStreamingAudio_FMOD::update()
{
	// Kill dead internet streams, if possible
	std::list<LLAudioStreamManagerFMOD *>::iterator iter;
	for (iter = mDeadStreams.begin(); iter != mDeadStreams.end();)
	{
		LLAudioStreamManagerFMOD *streamp = *iter;
		if (streamp->stopStream())
		{
			llinfos << "Closed dead stream" << llendl;
			delete streamp;
			mDeadStreams.erase(iter++);
		}
		else
		{
			iter++;
		}
	}

	// Don't do anything if there are no streams playing
	if (!mCurrentInternetStreamp)
	{
		return;
	}

	int open_state = mCurrentInternetStreamp->getOpenState();

	if (!open_state)
	{
		// Stream is live

		// start the stream if it's ready
		if (mFMODInternetStreamChannel < 0)
		{
			mFMODInternetStreamChannel = mCurrentInternetStreamp->startStream();

			if (mFMODInternetStreamChannel != -1)
			{
				// Reset volume to previously set volume
				setGain(getGain());
				FSOUND_SetPaused(mFMODInternetStreamChannel, false);
			}
		}
	}
		
	switch(open_state)
	{
	default:
	case 0:
		// success
		break;
	case -1:
		// stream handle is invalid
		llwarns << "InternetStream - invalid handle" << llendl;
		stop();
		return;
	case -2:
		// opening
		break;
	case -3:
		// failed to open, file not found, perhaps
		llwarns << "InternetStream - failed to open" << llendl;
		stop();
		return;
	case -4:
		// connecting
		break;
	case -5:
		// buffering
		break;
	}

}

void LLStreamingAudio_FMOD::stop()
{
	if (mFMODInternetStreamChannel != -1)
	{
		FSOUND_SetPaused(mFMODInternetStreamChannel, true);
		FSOUND_SetPriority(mFMODInternetStreamChannel, 0);
		mFMODInternetStreamChannel = -1;
	}

	if (mCurrentInternetStreamp)
	{
		llinfos << "Stopping internet stream: " << mCurrentInternetStreamp->getURL() << llendl;
		if (mCurrentInternetStreamp->stopStream())
		{
			delete mCurrentInternetStreamp;
		}
		else
		{
			llwarns << "Pushing stream to dead list: " << mCurrentInternetStreamp->getURL() << llendl;
			mDeadStreams.push_back(mCurrentInternetStreamp);
		}
		mCurrentInternetStreamp = NULL;
		//mURL.clear();
	}
}

void LLStreamingAudio_FMOD::pause(int pauseopt)
{
	if (pauseopt < 0)
	{
		pauseopt = mCurrentInternetStreamp ? 1 : 0;
	}

	if (pauseopt)
	{
		if (mCurrentInternetStreamp)
		{
			stop();
		}
	}
	else
	{
		start(getURL());
	}
}


// A stream is "playing" if it has been requested to start.  That
// doesn't necessarily mean audio is coming out of the speakers.
int LLStreamingAudio_FMOD::isPlaying()
{
	if (mCurrentInternetStreamp)
	{
		return 1; // Active and playing
	}
	else if (!mURL.empty())
	{
		return 2; // "Paused"
	}
	else
	{
		return 0;
	}
}


F32 LLStreamingAudio_FMOD::getGain()
{
	return mGain;
}


std::string LLStreamingAudio_FMOD::getURL()
{
	return mURL;
}


void LLStreamingAudio_FMOD::setGain(F32 vol)
{
	mGain = vol;

	if (mFMODInternetStreamChannel != -1)
	{
		vol = llclamp(vol, 0.f, 1.f);
		int vol_int = llround(vol * 255.f);
		FSOUND_SetVolumeAbsolute(mFMODInternetStreamChannel, vol_int);
	}
}


///////////////////////////////////////////////////////
// manager of possibly-multiple internet audio streams

LLAudioStreamManagerFMOD::LLAudioStreamManagerFMOD(const std::string& url) :
	mInternetStream(NULL),
	mReady(false)
{
	mInternetStreamURL = url;
	mInternetStream = FSOUND_Stream_Open(url.c_str(), FSOUND_NORMAL | FSOUND_NONBLOCKING, 0, 0);
	if (!mInternetStream)
	{
		llwarns << "Couldn't open fmod stream, error "
			<< FMOD_ErrorString(FSOUND_GetError())
			<< llendl;
		mReady = false;
		return;
	}

	mReady = true;
}

int LLAudioStreamManagerFMOD::startStream()
{
	// We need a live and opened stream before we try and play it.
	if (!mInternetStream || getOpenState())
	{
		llwarns << "No internet stream to start playing!" << llendl;
		return -1;
	}

	// Make sure the stream is set to 2D mode.
	FSOUND_Stream_SetMode(mInternetStream, FSOUND_2D);

	return FSOUND_Stream_PlayEx(FSOUND_FREE, mInternetStream, NULL, true);
}

bool LLAudioStreamManagerFMOD::stopStream()
{
	if (mInternetStream)
	{
		int read_percent = 0;
		int status = 0;
		int bitrate = 0;
		unsigned int flags = 0x0;
		FSOUND_Stream_Net_GetStatus(mInternetStream, &status, &read_percent, &bitrate, &flags);

		bool close = true;
		switch (status)
		{
		case FSOUND_STREAM_NET_CONNECTING:
			close = false;
			break;
		case FSOUND_STREAM_NET_NOTCONNECTED:
		case FSOUND_STREAM_NET_BUFFERING:
		case FSOUND_STREAM_NET_READY:
		case FSOUND_STREAM_NET_ERROR:
		default:
			close = true;
		}

		if (close)
		{
			FSOUND_Stream_Close(mInternetStream);
			mInternetStream = NULL;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

int LLAudioStreamManagerFMOD::getOpenState()
{
	int open_state = FSOUND_Stream_GetOpenState(mInternetStream);
	return open_state;
}
