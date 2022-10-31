/** 
* @file   lldeferredsounds.h
* @brief  Header file for lldeferredsounds
* @author Gilbert@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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
#ifndef LL_LLDEFERREDSOUNDS_H
#define LL_LLDEFERREDSOUNDS_H

#include "llsingleton.h"

struct SoundData;

class LLDeferredSounds : public LLSingleton<LLDeferredSounds>
{
    LLSINGLETON_EMPTY_CTOR(LLDeferredSounds);
    std::vector<SoundData> soundVector;
public:
    //Add sounds to be played once progress bar is hidden (such as after teleport or loading screen)
    void deferSound(SoundData& sound);

    void playdeferredSounds();
};

#endif // LL_LLDEFERREDSOUNDS_H

