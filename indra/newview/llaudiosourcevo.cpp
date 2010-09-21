/** 
 * @file llaudiosourcevo.cpp
 * @author Douglas Soo, James Cook
 * @brief Audio sources attached to viewer objects
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llaudiosourcevo.h"

#include "llagentcamera.h"
#include "llmutelist.h"
#include "llviewerparcelmgr.h"

LLAudioSourceVO::LLAudioSourceVO(const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain, LLViewerObject *objectp)
	:	LLAudioSource(sound_id, owner_id, gain, LLAudioEngine::AUDIO_TYPE_SFX), 
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
	LLVector3d pos_global;

	if (mObjectp->isAttachment())
	{
		LLViewerObject* parent = mObjectp;
		while (parent && !parent->isAvatar())
		{
			parent = (LLViewerObject*)parent->getParent();
		}
		if (parent)
		{
			pos_global = parent->getPositionGlobal();
		}
	}
	else
	{
		pos_global = mObjectp->getPositionGlobal();
	}
	
	if (!LLViewerParcelMgr::getInstance()->canHearSound(pos_global))
	{
		mute = TRUE;
	}

	if (!mute)
	{
		if (LLMuteList::getInstance()->isMuted(mObjectp->getID()))
		{
			mute = TRUE;
		}
		else if (LLMuteList::getInstance()->isMuted(mOwnerID, LLMute::flagObjectSounds))
		{
			mute = TRUE;
		}
		else if (mObjectp->isAttachment())
		{
			LLViewerObject* parent = mObjectp;
			while (parent && !parent->isAvatar())
			{
				parent = (LLViewerObject*)parent->getParent();
			}
			if (parent 
				&& LLMuteList::getInstance()->isMuted(parent->getID()))
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
		mPositionGlobal = gAgentCamera.getCameraPositionGlobal();
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
