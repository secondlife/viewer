/** 
 * @file streamingaudio.h
 * @author Tofu Linden
 * @brief Definition of LLStreamingAudioInterface base class abstracting the streaming audio interface
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
