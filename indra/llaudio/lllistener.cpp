/** 
 * @file listener.cpp
 * @brief Implementation of LISTENER class abstracting the audio support
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "lllistener.h"

#define DEFAULT_AT  0.0f,0.0f,-1.0f
#define DEFAULT_UP  0.0f,1.0f,0.0f

//-----------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------
LLListener::LLListener()
{
	init();
}

//-----------------------------------------------------------------------
LLListener::~LLListener()
{
}

//-----------------------------------------------------------------------
void LLListener::init(void)
{
	mPosition.zeroVec();
	mListenAt.setVec(DEFAULT_AT);
	mListenUp.setVec(DEFAULT_UP);
	mVelocity.zeroVec();
}

//-----------------------------------------------------------------------
void LLListener::translate(LLVector3 offset)
{
	mPosition += offset;
}

//-----------------------------------------------------------------------
void LLListener::setPosition(LLVector3 pos)
{
	mPosition = pos;
}

//-----------------------------------------------------------------------
LLVector3 LLListener::getPosition(void)
{
	return(mPosition);
}

//-----------------------------------------------------------------------
LLVector3 LLListener::getAt(void)
{
	return(mListenAt);
}

//-----------------------------------------------------------------------
LLVector3 LLListener::getUp(void)
{
	return(mListenUp);
}

//-----------------------------------------------------------------------
void LLListener::setVelocity(LLVector3 vel)
{
	mVelocity = vel;
}

//-----------------------------------------------------------------------
void LLListener::orient(LLVector3 up, LLVector3 at)
{
	mListenUp = up;
	mListenAt = at;
}

//-----------------------------------------------------------------------
void LLListener::set(LLVector3 pos, LLVector3 vel, LLVector3 up, LLVector3 at)
{
	mPosition = pos;
	mVelocity = vel;

	setPosition(pos);
	setVelocity(vel);
	orient(up,at);
}

//-----------------------------------------------------------------------
void LLListener::setDopplerFactor(F32 factor)
{
}

//-----------------------------------------------------------------------
F32 LLListener::getDopplerFactor()
{
	return (1.f);
}

//-----------------------------------------------------------------------
void LLListener::setRolloffFactor(F32 factor)
{
}

//-----------------------------------------------------------------------
F32 LLListener::getRolloffFactor()
{
	return (1.f);
}

//-----------------------------------------------------------------------
void LLListener::commitDeferredChanges()
{
}

