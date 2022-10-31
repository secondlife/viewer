/** 
 * @file listener.h
 * @brief Description of LISTENER base class abstracting the audio support.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LISTENER_H
#define LL_LISTENER_H

#include "v3math.h"

class LLListener 
{
 private:
 protected:
    LLVector3 mPosition;
    LLVector3 mVelocity;
    LLVector3 mListenAt;
    LLVector3 mListenUp;

 public:

 private:
 protected:
 public:  
    LLListener();
    virtual ~LLListener();
    virtual void init();  

    virtual void set(LLVector3 pos, LLVector3 vel, LLVector3 up, LLVector3 at);

    virtual void setPosition(LLVector3 pos);
    virtual void setVelocity(LLVector3 vel);

    virtual void orient(LLVector3 up, LLVector3 at);
    virtual void translate(LLVector3 offset);

    virtual void setDopplerFactor(F32 factor);
    virtual void setRolloffFactor(F32 factor);

    virtual LLVector3 getPosition();
    virtual LLVector3 getAt();
    virtual LLVector3 getUp();

    virtual F32 getDopplerFactor();
    virtual F32 getRolloffFactor();

    virtual void commitDeferredChanges();
};

#endif

