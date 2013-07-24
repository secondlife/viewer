/** 
 * @file llvieweraudio.h
 * @brief Audio functions that used to be in viewer.cpp
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

#ifndef LL_VIEWERAUDIO_H
#define LL_VIEWERAUDIO_H

#include "llframetimer.h"
#include "llsingleton.h"

// comment out to turn off wind
#define kAUDIO_ENABLE_WIND 
//#define kAUDIO_ENABLE_WATER 1	// comment out to turn off water
#define kAUDIO_NUM_BUFFERS 30
#define kAUDIO_NUM_SOURCES 30 

void init_audio();
void audio_update_volume(bool force_update = true);
void audio_update_listener();
void audio_update_wind(bool force_update = true);

class LLViewerAudio : public LLSingleton<LLViewerAudio>
{
public:

	enum EFadeState
	{
		FADE_IDLE,
		FADE_IN,
		FADE_OUT,
	};

	LLViewerAudio();
	virtual ~LLViewerAudio();
	
	void startInternetStreamWithAutoFade(std::string streamURI);
	void stopInternetStreamWithAutoFade();
	
	bool onIdleUpdate();

	EFadeState getFadeState() { return mFadeState; }
	bool isDone() { return mDone; };
	F32 getFadeVolume();
	bool getForcedTeleportFade() { return mForcedTeleportFade; };
	void setForcedTeleportFade(bool fade) { mForcedTeleportFade = fade;} ;
	void setNextStreamURI(std::string stream) { mNextStreamURI = stream; } ;
	void setWasPlaying(bool playing) { mWasPlaying = playing;} ;

private:

	bool mDone;
	F32 mFadeTime;
	std::string mNextStreamURI;
	EFadeState mFadeState;
	LLFrameTimer stream_fade_timer;
	bool mIdleListnerActive;
	bool mForcedTeleportFade;
	bool mWasPlaying;
	boost::signals2::connection	mTeleportFailedConnection;
	boost::signals2::connection	mTeleportFinishedConnection;
	boost::signals2::connection mTeleportStartedConnection;

	void registerIdleListener();
	void deregisterIdleListener() { mIdleListnerActive = false; };
	void startFading();
	void onTeleportFailed();
	void onTeleportFinished(const LLVector3d& pos, const bool& local);
	void onTeleportStarted();
};

#endif //LL_VIEWER_H
