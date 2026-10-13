/**
 * @file volume_catcher.cpp
 * @brief Linux volume catcher which will pick an implementation to use
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

#include "volume_catcher_linux.h"

VolumeCatcher::VolumeCatcher()
{
}

void VolumeCatcher::onEnablePipeWireVolumeCatcher(bool enable)
{
    if (pimpl != nullptr)
        return;

    if (enable)
    {
        LL_DEBUGS() << "volume catcher using pipewire" << LL_ENDL;
        pimpl = new VolumeCatcherPipeWire();
    }
    else
    {
        LL_DEBUGS() << "volume catcher using pulseaudio" << LL_ENDL;
        pimpl = new VolumeCatcherPulseAudio();
    }
}

VolumeCatcher::~VolumeCatcher()
{
    if (pimpl != nullptr)
    {
        delete pimpl;
        pimpl = nullptr;
    }
}

void VolumeCatcher::setVolume(F32 volume)
{
    if (pimpl != nullptr) {
        pimpl->setVolume(volume);
    }
}

void VolumeCatcher::setPan(F32 pan)
{
    if (pimpl != nullptr)
        pimpl->setPan(pan);
}

void VolumeCatcher::pump()
{
    if (pimpl != nullptr)
        pimpl->pump();
}
