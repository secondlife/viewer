/** 
 * @file linux_volume_catcher.cpp
 * @brief A Linux-specific, PulseAudio-specific hack to detect and volume-adjust new audio sources
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

#include "linden_common.h"

#include <glib.h>

#include <pulse/introspect.h>
#include <pulse/context.h>
#include <pulse/subscribe.h>
#include <pulse/glib-mainloop.h> // There's no special reason why we want the *glib* PA mainloop, but the generic polling implementation seems broken.

class LinuxVolumeCatcherImpl
{
public:
	//void setVol(F32 volume); // 0.0-1.0
	//void pump(void);
	//LinuxVolumeCatcherImpl *impl;

private:
	F32 mDesiredVolume;
	pa_glib_mainloop *mMainloop;
	pa_context *mLastContext;
	bool connected;
	std::set<U32> mSinkInputIndices;
	std::map<U32,U32> mSinkInputNumChannels;
}


