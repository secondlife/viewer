/** 
 * @file listener_fmodex.cpp
 * @brief Implementation of LISTENER class abstracting the audio
 * support as a FMODEX implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "linden_common.h"
#include "llaudioengine.h"
#include "lllistener_fmodex.h"
#include "fmod.hpp"

//-----------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------
LLListener_FMODEX::LLListener_FMODEX(FMOD::System *system)
{
	mSystem = system;
	init();
}

//-----------------------------------------------------------------------
LLListener_FMODEX::~LLListener_FMODEX()
{
}

//-----------------------------------------------------------------------
void LLListener_FMODEX::init(void)
{
	// do inherited
	LLListener::init();
	mDopplerFactor = 1.0f;
	mRolloffFactor = 1.0f;
}

//-----------------------------------------------------------------------
void LLListener_FMODEX::translate(LLVector3 offset)
{
	LLListener::translate(offset);

	mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV, NULL, (FMOD_VECTOR*)mListenAt.mV, (FMOD_VECTOR*)mListenUp.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODEX::setPosition(LLVector3 pos)
{
	LLListener::setPosition(pos);

	mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV, NULL, (FMOD_VECTOR*)mListenAt.mV, (FMOD_VECTOR*)mListenUp.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODEX::setVelocity(LLVector3 vel)
{
	LLListener::setVelocity(vel);

	mSystem->set3DListenerAttributes(0, NULL, (FMOD_VECTOR*)mVelocity.mV, (FMOD_VECTOR*)mListenAt.mV, (FMOD_VECTOR*)mListenUp.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODEX::orient(LLVector3 up, LLVector3 at)
{
	LLListener::orient(up, at);

	// Welcome to the transition between right and left
	// (coordinate systems, that is)
	// Leaving the at vector alone results in a L/R reversal
	// since DX is left-handed and we (LL, OpenGL, OpenAL) are right-handed
	at = -at;

	mSystem->set3DListenerAttributes(0, NULL, NULL, (FMOD_VECTOR*)at.mV, (FMOD_VECTOR*)up.mV);
}

//-----------------------------------------------------------------------
void LLListener_FMODEX::commitDeferredChanges()
{
	mSystem->update();
}


void LLListener_FMODEX::setRolloffFactor(F32 factor)
{
	//An internal FMODEx optimization skips 3D updates if there have not been changes to the 3D sound environment.
	//Sadly, a change in rolloff is not accounted for, thus we must touch the listener properties as well.
	//In short: Changing the position ticks a dirtyflag inside fmodex, which makes it not skip 3D processing next update call.
	if(mRolloffFactor != factor)
	{
		LLVector3 pos = mVelocity - LLVector3(0.f,0.f,.1f);
		mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)pos.mV, NULL, NULL, NULL);
		mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mVelocity.mV, NULL, NULL, NULL);
	}
	mRolloffFactor = factor;
	mSystem->set3DSettings(mDopplerFactor, 1.f, mRolloffFactor);
}


F32 LLListener_FMODEX::getRolloffFactor()
{
	return mRolloffFactor;
}


void LLListener_FMODEX::setDopplerFactor(F32 factor)
{
	mDopplerFactor = factor;
	mSystem->set3DSettings(mDopplerFactor, 1.f, mRolloffFactor);
}


F32 LLListener_FMODEX::getDopplerFactor()
{
	return mDopplerFactor;
}


