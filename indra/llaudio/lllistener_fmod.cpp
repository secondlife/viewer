/** 
 * @file listener_fmod.cpp
 * @brief implementation of LISTENER class abstracting the audio
 * support as a FMOD 3D implementation (windows only)
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
#include "lllistener_fmod.h"
#include "fmod.h"

//-----------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------
LLListener_FMOD::LLListener_FMOD()
{
	init();
}

//-----------------------------------------------------------------------
LLListener_FMOD::~LLListener_FMOD()
{
}

//-----------------------------------------------------------------------
void LLListener_FMOD::init(void)
{
	// do inherited
	LLListener::init();
	mDopplerFactor = 1.0f;
	mRolloffFactor = 1.0f;
}

//-----------------------------------------------------------------------
void LLListener_FMOD::translate(LLVector3 offset)
{
	LLListener::translate(offset);

	FSOUND_3D_Listener_SetAttributes(mPosition.mV, NULL, mListenAt.mV[0],mListenAt.mV[1],mListenAt.mV[2], mListenUp.mV[0],mListenUp.mV[1],mListenUp.mV[2]);
}

//-----------------------------------------------------------------------
void LLListener_FMOD::setPosition(LLVector3 pos)
{
	LLListener::setPosition(pos);

	FSOUND_3D_Listener_SetAttributes(pos.mV, NULL, mListenAt.mV[0],mListenAt.mV[1],mListenAt.mV[2], mListenUp.mV[0],mListenUp.mV[1],mListenUp.mV[2]);
}

//-----------------------------------------------------------------------
void LLListener_FMOD::setVelocity(LLVector3 vel)
{
	LLListener::setVelocity(vel);

	FSOUND_3D_Listener_SetAttributes(NULL, vel.mV, mListenAt.mV[0],mListenAt.mV[1],mListenAt.mV[2], mListenUp.mV[0],mListenUp.mV[1],mListenUp.mV[2]);
}

//-----------------------------------------------------------------------
void LLListener_FMOD::orient(LLVector3 up, LLVector3 at)
{
	LLListener::orient(up, at);

	// Welcome to the transition between right and left
	// (coordinate systems, that is)
	// Leaving the at vector alone results in a L/R reversal
	// since DX is left-handed and we (LL, OpenGL, OpenAL) are right-handed
	at = -at;

	FSOUND_3D_Listener_SetAttributes(NULL, NULL, at.mV[0],at.mV[1],at.mV[2], up.mV[0],up.mV[1],up.mV[2]);
}

//-----------------------------------------------------------------------
void LLListener_FMOD::commitDeferredChanges()
{
	FSOUND_Update();
}


void LLListener_FMOD::setRolloffFactor(F32 factor)
{
	mRolloffFactor = factor;
	FSOUND_3D_SetRolloffFactor(factor);
}


F32 LLListener_FMOD::getRolloffFactor()
{
	return mRolloffFactor;
}


void LLListener_FMOD::setDopplerFactor(F32 factor)
{
	mDopplerFactor = factor;
	FSOUND_3D_SetDopplerFactor(factor);
}


F32 LLListener_FMOD::getDopplerFactor()
{
	return mDopplerFactor;
}


