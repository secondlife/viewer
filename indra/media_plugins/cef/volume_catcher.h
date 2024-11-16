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

    void setVolume(F32 volume);
    void setPan(F32 pan);

    void pump();

#if LL_LINUX
    void onEnablePipeWireVolumeCatcher(bool enable);
#endif

private:
#if LL_LINUX || LL_WINDOWS
    VolumeCatcherImpl *pimpl;
#endif
};

#endif // VOLUME_CATCHER_H
