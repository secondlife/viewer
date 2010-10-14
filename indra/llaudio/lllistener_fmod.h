/** 
 * @file listener_fmod.h
 * @brief Description of LISTENER class abstracting the audio support
 * as an FMOD 3D implementation (windows and Linux)
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

#ifndef LL_LISTENER_FMOD_H
#define LL_LISTENER_FMOD_H

#include "lllistener.h"

class LLListener_FMOD : public LLListener
{
 public:  
	LLListener_FMOD();
	virtual ~LLListener_FMOD();
	virtual void init();  

	virtual void translate(LLVector3 offset);
	virtual void setPosition(LLVector3 pos);
	virtual void setVelocity(LLVector3 vel);
	virtual void orient(LLVector3 up, LLVector3 at);
	virtual void commitDeferredChanges();

	virtual void setDopplerFactor(F32 factor);
	virtual F32 getDopplerFactor();
	virtual void setRolloffFactor(F32 factor);
	virtual F32 getRolloffFactor();

 protected:
	 F32 mDopplerFactor;
	 F32 mRolloffFactor;
};

#endif


