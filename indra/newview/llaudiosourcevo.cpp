/** 
 * @file llaudiosourcevo.cpp
 * @author Douglas Soo, James Cook
 * @brief Audio sources attached to viewer objects
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llaudiosourcevo.h"

#include "llagent.h"
#include "llmutelist.h"
#include "llviewerparcelmgr.h"

LLAudioSourceVO::LLAudioSourceVO(const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain, LLViewerObject *objectp)
:	LLAudioSource(sound_id, owner_id, gain), 
	mObjectp(objectp), 
	mActualGain(gain)
{
	setAmbient(FALSE);
	updateGain();
	update();
}

LLAudioSourceVO::~LLAudioSourceVO()
{
	if (mObjectp)
	{
		mObjectp->clearAttachedSound();
	}
	mObjectp = NULL;
}

void LLAudioSourceVO::setGain(const F32 gain)
{
	mActualGain = llclamp(gain, 0.f, 1.f);
	updateGain();
}

void LLAudioSourceVO::updateGain()
{
	if (!mObjectp)
	{
		return;
	}

	BOOL mute = FALSE;
	if (gParcelMgr)
	{
		LLVector3d pos_global;

		if (mObjectp->isAttachment())
		{
			LLViewerObject* parent = mObjectp;
			while (parent 
				   && !parent->isAvatar())
			{
				parent = (LLViewerObject*)parent->getParent();
			}
			if (parent)
				pos_global = parent->getPositionGlobal();
		}
		
		else
			pos_global = mObjectp->getPositionGlobal();
		
		if (!gParcelMgr->canHearSound(pos_global))
		{
			mute = TRUE;
		}
	}

	if (!mute && gMuteListp)
	{
		if (gMuteListp->isMuted(mObjectp->getID()))
		{
			mute = TRUE;
		}
		else if (gMuteListp->isMuted(mOwnerID, LLMute::flagObjectSounds))
		{
			mute = TRUE;
		}
		else if (mObjectp->isAttachment())
		{
			LLViewerObject* parent = mObjectp;
			while (parent 
				   && !parent->isAvatar())
			{
				parent = (LLViewerObject*)parent->getParent();
			}
			if (parent 
				&& gMuteListp->isMuted(parent->getID()))
			{
				mute = TRUE;
			}
		}
	}

	if (!mute)
	{
		mGain = mActualGain;
	}
	else
	{
		mGain = 0.f;
	}
}


void LLAudioSourceVO::update()
{
	if (!mObjectp)
	{
		return;
	}

	if (mObjectp->isDead())
	{
		mObjectp = NULL;
		return;
	}

	updateGain();
	if (mObjectp->isHUDAttachment())
	{
		mPositionGlobal = gAgent.getCameraPositionGlobal();
	}
	else
	{
		mPositionGlobal = mObjectp->getPositionGlobal();
	}
	if (mObjectp->getSubParent())
	{
		mVelocity = mObjectp->getSubParent()->getVelocity();
	}
	else
	{
		mVelocity = mObjectp->getVelocity();
	}

	LLAudioSource::update();
}
