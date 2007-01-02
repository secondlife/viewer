/** 
 * @file llhudeffect.cpp
 * @brief LLHUDEffect class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llhudeffect.h"

#include "message.h"
#include "llgl.h"
#include "llagent.h"
#include "llsphere.h"
#include "llimagegl.h"

#include "llviewerobjectlist.h"
#include "lldrawable.h"

LLHUDEffect::LLHUDEffect(const U8 type)
:	LLHUDObject(type),
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
	llerrs << "Never call this!" << llendl;
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
