/** 
 * @file llaudiosourcevo.cpp
 * @author Douglas Soo, James Cook
 * @brief Audio sources attached to viewer objects
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llaudiosourcevo.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llmutelist.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"

LLAudioSourceVO::LLAudioSourceVO(const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain, LLViewerObject *objectp)
	:	LLAudioSource(sound_id, owner_id, gain, LLAudioEngine::AUDIO_TYPE_SFX), 
	mObjectp(objectp)
{
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
	mGain = llclamp(gain, 0.f, 1.f);
}

void LLAudioSourceVO::checkCutOffRadius()
{
    if (mSourceMuted // already muted by something, will be recalculated on update()
        || !mObjectp)
    {
        return;
    }

    F32 cutoff = mObjectp->getSoundCutOffRadius();
    if (cutoff < 0.1f)
    {
        // consider cutoff below 0.1m as off (to avoid near zero comparison)
        return;
    }

    LLVector3d pos_global = getPosGlobal();
    if (!isInCutOffRadius(pos_global, cutoff))
    {
        mSourceMuted = true;
    }
}

LLVector3d LLAudioSourceVO::getPosGlobal() const
{
    if (mObjectp->isAttachment())
    {
        LLViewerObject* parent = mObjectp;
        while (parent && !parent->isAvatar())
        {
            parent = (LLViewerObject*)parent->getParent();
        }
        if (parent)
        {
            return parent->getPositionGlobal();
        }
    }
    else
    {
        return mObjectp->getPositionGlobal();
    }
    return LLVector3d();
}

bool LLAudioSourceVO::isInCutOffRadius(const LLVector3d pos_global, const F32 cutoff) const
{
    static LLCachedControl<S32> ear_mode(gSavedSettings, "VoiceEarLocation", 0);

    LLVector3d pos_ear;

    switch (ear_mode())
    {
        case 0: // camera
            pos_ear = gAgentCamera.getCameraPositionGlobal();
            break;

        case 1: // avatar
        case 2:
            // voice support 'mixed' in '2' case with agent's position and camera's rotations
            // but it is not defined in settings and uses camera as default
            pos_ear = gAgent.getPositionGlobal();
            break;

        default:
            pos_ear = gAgentCamera.getCameraPositionGlobal();
            break;
    }
    LLVector3d to_vec = pos_global - pos_ear;

    F32 dist = (F32)to_vec.magVec();

    return dist < cutoff;
}

void LLAudioSourceVO::updateMute()
{
	if (!mObjectp || mObjectp->isDead())
	{
	  	mSourceMuted = true;
		return;
	}

	bool mute = false;
	LLVector3d pos_global = getPosGlobal();

	F32 cutoff = mObjectp->getSoundCutOffRadius();
	if ((cutoff > 0.1f && !isInCutOffRadius(pos_global, cutoff)) // consider cutoff below 0.1m as off
		|| !LLViewerParcelMgr::getInstance()->canHearSound(pos_global))
	{
		mute = true;
	}

	if (!mute)
	{
		if (LLMuteList::getInstance()->isMuted(mObjectp->getID()))
		{
			mute = true;
		}
		else if (LLMuteList::getInstance()->isMuted(mOwnerID, LLMute::flagObjectSounds))
		{
			mute = true;
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
				mute = true;
			}
		}
	}

	if (mute != mSourceMuted)
	{
		mSourceMuted = mute;
		if (mSourceMuted)
		{
		  	// Stop the sound.
			this->play(LLUUID::null);
		}
		else
		{
		  	// Muted sounds keep there data at all times, because
			// it's the place where the audio UUID is stored.
			// However, it's possible that mCurrentDatap is
			// NULL when this source did only preload sounds.
			if (mCurrentDatap)
			{
		  		// Restart the sound.
				this->play(mCurrentDatap->getID());
			}
		}
	}
}

void LLAudioSourceVO::update()
{
	updateMute();

	if (!mObjectp)
	{
		return;
	}

	if (mObjectp->isDead())
	{
		mObjectp = NULL;
		return;
	}

	if (mSourceMuted)
	{
	  	return;
	}

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
