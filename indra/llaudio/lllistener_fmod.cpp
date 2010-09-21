/** 
 * @file listener_fmod.cpp
 * @brief implementation of LISTENER class abstracting the audio
 * support as a FMOD 3D implementation (windows only)
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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


