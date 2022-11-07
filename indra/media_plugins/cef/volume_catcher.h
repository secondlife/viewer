/** 
 * @file volume_catcher.h
 * @brief Interface to a class with platform-specific implementations that allows control of the audio volume of all sources in the current process.
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
