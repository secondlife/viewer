/** 
 * @file listener_fmodstudio.cpp
 * @brief Implementation of LISTENER class abstracting the audio
 * support as a FMODSTUDIO implementation
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#include "linden_common.h"
#include "llaudioengine.h"
#include "lllistener_fmodstudio.h"
#include "fmodstudio/fmod.hpp"

//-----------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------
LLListener_FMODSTUDIO::LLListener_FMODSTUDIO(FMOD::System *system)
{
    mSystem = system;
    init();
}

//-----------------------------------------------------------------------
LLListener_FMODSTUDIO::~LLListener_FMODSTUDIO()
{
}

//-----------------------------------------------------------------------
void LLListener_FMODSTUDIO::init(void)
{
    // do inherited
    LLListener::init();
    mDopplerFactor = 1.0f;
    mRolloffFactor = 1.0f;
}

//-----------------------------------------------------------------------
void LLListener_FMODSTUDIO::translate(LLVector3 offset)
{
    LLListener::translate(offset);

    mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV, NULL, (FMOD_VECTOR*)mListenAt.mV, (FMOD_VECTOR*)mListenUp.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODSTUDIO::setPosition(LLVector3 pos)
{
    LLListener::setPosition(pos);

    mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV, NULL, (FMOD_VECTOR*)mListenAt.mV, (FMOD_VECTOR*)mListenUp.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODSTUDIO::setVelocity(LLVector3 vel)
{
    LLListener::setVelocity(vel);

    mSystem->set3DListenerAttributes(0, NULL, (FMOD_VECTOR*)mVelocity.mV, (FMOD_VECTOR*)mListenAt.mV, (FMOD_VECTOR*)mListenUp.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODSTUDIO::orient(LLVector3 up, LLVector3 at)
{
    LLListener::orient(up, at);

    // at = -at; by default Fmod studio is 'left-handed' but we are providing
    // flag FMOD_INIT_3D_RIGHTHANDED so no correction are needed

    mSystem->set3DListenerAttributes(0, NULL, NULL, (FMOD_VECTOR*)at.mV, (FMOD_VECTOR*)up.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODSTUDIO::commitDeferredChanges()
{
    if (!mSystem)
    {
        return;
    }

    mSystem->update();
}


void LLListener_FMODSTUDIO::setRolloffFactor(F32 factor)
{
    //An internal FMOD optimization skips 3D updates if there have not been changes to the 3D sound environment.
    // (this was true for FMODex, looks to be still true for FMOD STUDIO, but needs a recheck)
    //Sadly, a change in rolloff is not accounted for, thus we must touch the listener properties as well.
    //In short: Changing the position ticks a dirtyflag inside fmod, which makes it not skip 3D processing next update call.
    if (mRolloffFactor != factor)
    {
        LLVector3 pos = mPosition - LLVector3(0.f, 0.f, .1f);
        mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)pos.mV, NULL, NULL, NULL);
        mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV, NULL, NULL, NULL);
    }
    mRolloffFactor = factor;
    mSystem->set3DSettings(mDopplerFactor, 1.f, mRolloffFactor);
}


F32 LLListener_FMODSTUDIO::getRolloffFactor()
{
    return mRolloffFactor;
}


void LLListener_FMODSTUDIO::setDopplerFactor(F32 factor)
{
    mDopplerFactor = factor;
    mSystem->set3DSettings(mDopplerFactor, 1.f, mRolloffFactor);
}


F32 LLListener_FMODSTUDIO::getDopplerFactor()
{
    return mDopplerFactor;
}
