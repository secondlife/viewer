/** 
 * @file streamingaudio_fmod.h
 * @author Tofu Linden
 * @brief Definition of LLStreamingAudio_FMOD implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_STREAMINGAUDIO_FMOD_H
#define LL_STREAMINGAUDIO_FMOD_H

#include "stdtypes.h" // from llcommon

#include "llstreamingaudio.h"

class LLAudioStreamManagerFMOD;

class LLStreamingAudio_FMOD : public LLStreamingAudioInterface
{
 public:
	LLStreamingAudio_FMOD();
	/*virtual*/ ~LLStreamingAudio_FMOD();

	/*virtual*/ void start(const std::string& url);
	/*virtual*/ void stop();
	/*virtual*/ void pause(int pause);
	/*virtual*/ void update();
	/*virtual*/ int isPlaying();
	/*virtual*/ void setGain(F32 vol);
	/*virtual*/ F32 getGain();
	/*virtual*/ std::string getURL();

private:
	LLAudioStreamManagerFMOD *mCurrentInternetStreamp;
	int mFMODInternetStreamChannel;
	std::list<LLAudioStreamManagerFMOD *> mDeadStreams;

	std::string mURL;
	F32 mGain;
};


#endif // LL_STREAMINGAUDIO_FMOD_H
