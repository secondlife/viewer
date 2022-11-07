/** 
 * @file llhudeffect.cpp
 * @brief LLHUDEffect class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llhudeffect.h"

#include "message.h"
#include "llgl.h"
#include "llagent.h"
#include "llrendersphere.h"

#include "llviewerobjectlist.h"
#include "lldrawable.h"

LLHUDEffect::LLHUDEffect(const U8 type)
:   LLHUDObject(type),
    mID(),
    mDuration(1.f),
    mColor()
{
    mNeedsSendToSim = FALSE;
    mOriginatedHere = FALSE;
    mDead = FALSE;
}

LLHUDEffect::~LLHUDEffect()
{
}


void LLHUDEffect::packData(LLMessageSystem *mesgsys)
{
    mesgsys->addUUIDFast(_PREHASH_ID, mID);
    mesgsys->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    mesgsys->addU8Fast(_PREHASH_Type, mType);
    mesgsys->addF32Fast(_PREHASH_Duration, mDuration);
    mesgsys->addBinaryData(_PREHASH_Color, mColor.mV, 4);
}

void LLHUDEffect::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
    mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_ID, mID, blocknum);
    mesgsys->getU8Fast(_PREHASH_Effect, _PREHASH_Type, mType, blocknum);
    mesgsys->getF32Fast(_PREHASH_Effect, _PREHASH_Duration, mDuration, blocknum);
    mesgsys->getBinaryDataFast(_PREHASH_Effect,_PREHASH_Color, mColor.mV, 4, blocknum);
}

void LLHUDEffect::render()
{
    LL_ERRS() << "Never call this!" << LL_ENDL;
}

void LLHUDEffect::setID(const LLUUID &id)
{
    mID = id;
}

const LLUUID &LLHUDEffect::getID() const
{
    return mID;
}

void LLHUDEffect::setColor(const LLColor4U &color)
{
    mColor = color;
}

void LLHUDEffect::setDuration(const F32 duration)
{
    mDuration = duration;
}

void LLHUDEffect::setNeedsSendToSim(const BOOL send_to_sim)
{
    mNeedsSendToSim = send_to_sim;
}

BOOL LLHUDEffect::getNeedsSendToSim() const
{
    return mNeedsSendToSim;
}


void LLHUDEffect::setOriginatedHere(const BOOL orig_here)
{
    mOriginatedHere = orig_here;
}

BOOL LLHUDEffect::getOriginatedHere() const
{
    return mOriginatedHere;
}

//static
void LLHUDEffect::getIDType(LLMessageSystem *mesgsys, S32 blocknum, LLUUID &id, U8 &type)
{
    mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_ID, id, blocknum);
    mesgsys->getU8Fast(_PREHASH_Effect, _PREHASH_Type, type, blocknum);
}

BOOL LLHUDEffect::isDead() const
{
    return mDead;
}

void LLHUDEffect::update()
{
    // do nothing
}
