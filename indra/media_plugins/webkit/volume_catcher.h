/** 
 * @file volume_catcher.h
 * @brief Interface to a class with platform-specific implementations that allows control of the audio volume of all sources in the current process.
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

#ifndef VOLUME_CATCHER_H
#define VOLUME_CATCHER_H

#include "linden_common.h"

class VolumeCatcherImpl;

class VolumeCatcher
{
 public:
	VolumeCatcher();
	~VolumeCatcher();

	void setVolume(F32 volume); // 0.0 - 1.0
	
	// Set the left-right pan of audio sources
	// where -1.0 = left, 0 = center, and 1.0 = right
	void setPan(F32 pan); 

	void pump(); // call this at least a few times a second if you can - it affects how quickly we can 'catch' a new audio source and adjust its volume
	
 private:
	VolumeCatcherImpl *pimpl;
};

#endif // VOLUME_CATCHER_H
