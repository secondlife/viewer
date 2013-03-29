/** 
 * @file streamingaudio.h
 * @author Tofu Linden
 * @brief Definition of LLStreamingAudioInterface base class abstracting the streaming audio interface
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

#ifndef LL_STREAMINGAUDIO_H
#define LL_STREAMINGAUDIO_H

#include "stdtypes.h" // from llcommon

// Entirely abstract.  Based exactly on the historic API.
class LLStreamingAudioInterface
{
 public:
	virtual ~LLStreamingAudioInterface() {}

	virtual void start(const std::string& url) = 0;
	virtual void stop() = 0;
	virtual void pause(int pause) = 0;
	virtual void update() = 0;
	virtual int isPlaying() = 0;
	// use a value from 0.0 to 1.0, inclusive
	virtual void setGain(F32 vol) = 0;
	virtual F32 getGain() = 0;
	virtual std::string getURL() = 0;
};

#endif // LL_STREAMINGAUDIO_H
