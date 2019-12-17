/** 
 * @file llhudeffecttrail.cpp
 * @brief LLHUDEffectSpiral class implementation
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

#include "llhudeffecttrail.h"

#include "llviewercontrol.h"
#include "message.h"

#include "llagent.h"
#include "llbox.h"
#include "lldrawable.h"
#include "llhudrender.h"
#include "llviewertexturelist.h"
#include "llviewerobjectlist.h"
#include "llviewerpartsim.h"
#include "llviewerpartsource.h"
#include "llvoavatar.h"
#include "llworld.h"

LLHUDEffectSpiral::LLHUDEffectSpiral(const U8 type) : LLHUDEffect(type), mbInit(FALSE)
{
	mKillTime = 10.f;
	mVMag = 1.f;
	mVOffset = 0.f;
	mInitialRadius = 1.f;
	mFinalRadius = 1.f;
	mSpinRate = 10.f;
	mFlickerRate = 50.f;
	mScaleBase = 0.1f;
	mScaleVar = 0.f;

	mFadeInterp.setStartTime(0.f);
	mFadeInterp.setEndTime(mKillTime);
	mFadeInterp.setStartVal(1.f);
	mFadeInterp.setEndVal(1.f);
}

LLHUDEffectSpiral::~LLHUDEffectSpiral()
{
}

void LLHUDEffectSpiral::markDead()
{
	if (mPartSourcep)
	{
		mPartSourcep->setDead();
		mPartSourcep = NULL;
	}
	LLHUDEffect::markDead();
}

void LLHUDEffectSpiral::packData(LLMessageSystem *mesgsys)
{
	if (!mSourceObject)
	{
		//LL_WARNS() << "Missing object in trail pack!" << LL_ENDL;
	}
	LLHUDEffect::packData(mesgsys);

	U8 packed_data[56];
	memset(packed_data, 0, 56);

	if (mSourceObject)
	{
		htolememcpy(packed_data, mSourceObject->mID.mData, MVT_LLUUID, 16);
	}
	if (mTargetObject)
	{
		htolememcpy(packed_data + 16, mTargetObject->mID.mData, MVT_LLUUID, 16);
	}
	if (!mPositionGlobal.isExactlyZero())
	{
		htolememcpy(packed_data + 32, mPositionGlobal.mdV, MVT_LLVector3d, 24);
	}
	mesgsys->addBinaryDataFast(_PREHASH_TypeData, packed_data, 56);
}

void LLHUDEffectSpiral::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
	const size_t EFFECT_SIZE = 56;
	U8 packed_data[EFFECT_SIZE];

	LLHUDEffect::unpackData(mesgsys, blocknum);
	LLUUID object_id, target_object_id;
	size_t size = mesgsys->getSizeFast(_PREHASH_Effect, blocknum, _PREHASH_TypeData);
	if (size != EFFECT_SIZE)
	{
		LL_WARNS() << "Spiral effect with bad size " << size << LL_ENDL;
		return;
	}
	mesgsys->getBinaryDataFast(_PREHASH_Effect, _PREHASH_TypeData, 
		packed_data, EFFECT_SIZE, blocknum, EFFECT_SIZE);
	
	htolememcpy(object_id.mData, packed_data, MVT_LLUUID, 16);
	htolememcpy(target_object_id.mData, packed_data + 16, MVT_LLUUID, 16);
	htolememcpy(mPositionGlobal.mdV, packed_data + 32, MVT_LLVector3d, 24);

	LLViewerObject *objp = NULL;

	if (object_id.isNull())
	{
		setSourceObject(NULL);
	}
	else
	{
		LLViewerObject *objp = gObjectList.findObject(object_id);
		if (objp)
		{
			setSourceObject(objp);
		}
		else
		{
			// We don't have this object, kill this effect
			markDead();
			return;
		}
	}

	if (target_object_id.isNull())
	{
		setTargetObject(NULL);
	}
	else
	{
		objp = gObjectList.findObject(target_object_id);
		if (objp)
		{
			setTargetObject(objp);
		}
		else
		{
			// We don't have this object, kill this effect
			markDead();
			return;
		}
	}

	triggerLocal();
}

void LLHUDEffectSpiral::triggerLocal()
{
	mKillTime = mTimer.getElapsedTimeF32() + mDuration;

	BOOL show_beam = gSavedSettings.getBOOL("ShowSelectionBeam");

	LLColor4 color;
	color.setVec(mColor);

	if (!mPartSourcep)
	{
		if (!mTargetObject.isNull() && !mSourceObject.isNull())
		{
			if (show_beam)
			{
				LLPointer<LLViewerPartSourceBeam> psb = new LLViewerPartSourceBeam;
				psb->setColor(color);
				psb->setSourceObject(mSourceObject);
				psb->setTargetObject(mTargetObject);
				psb->setOwnerUUID(gAgent.getID());
				LLViewerPartSim::getInstance()->addPartSource(psb);
				mPartSourcep = psb;
			}
		}
		else
		{
			if (!mSourceObject.isNull() && !mPositionGlobal.isExactlyZero())
			{
				if (show_beam)
				{
					LLPointer<LLViewerPartSourceBeam> psb = new LLViewerPartSourceBeam;
					psb->setSourceObject(mSourceObject);
					psb->setTargetObject(NULL);
					psb->setColor(color);
					psb->mLKGTargetPosGlobal = mPositionGlobal;
					psb->setOwnerUUID(gAgent.getID());
					LLViewerPartSim::getInstance()->addPartSource(psb);
					mPartSourcep = psb;
				}
			}
			else
			{
				LLVector3 pos;
				if (mSourceObject)
				{
					pos = mSourceObject->getPositionAgent();
				}
				else
				{
					pos = gAgent.getPosAgentFromGlobal(mPositionGlobal);
				}
				LLPointer<LLViewerPartSourceSpiral> pss = new LLViewerPartSourceSpiral(pos);
				if (!mSourceObject.isNull())
				{
					pss->setSourceObject(mSourceObject);
				}
				pss->setColor(color);
				pss->setOwnerUUID(gAgent.getID());
				LLViewerPartSim::getInstance()->addPartSource(pss);
				mPartSourcep = pss;
			}
		}
	}
	else
	{
		LLPointer<LLViewerPartSource>& ps = mPartSourcep;
		if (mPartSourcep->getType() == LLViewerPartSource::LL_PART_SOURCE_BEAM)
		{
			LLViewerPartSourceBeam *psb = (LLViewerPartSourceBeam *)ps.get();
			psb->setSourceObject(mSourceObject);
			psb->setTargetObject(mTargetObject);
			psb->setColor(color);
			if (mTargetObject.isNull())
			{
				psb->mLKGTargetPosGlobal = mPositionGlobal;
			}
		}
		else
		{
			LLViewerPartSourceSpiral *pss = (LLViewerPartSourceSpiral *)ps.get();
			pss->setSourceObject(mSourceObject);
		}
	}

	mbInit = TRUE;
}

void LLHUDEffectSpiral::setTargetObject(LLViewerObject *objp)
{
	if (objp == mTargetObject)
	{
		return;
	}

	mTargetObject = objp;
}

void LLHUDEffectSpiral::render()
{
	F32 time = mTimer.getElapsedTimeF32();

	if ((!mSourceObject.isNull() && mSourceObject->isDead()) ||
	    (!mTargetObject.isNull() && mTargetObject->isDead()) ||
	    mKillTime < time ||
		(!mPartSourcep.isNull() && !gSavedSettings.getBOOL("ShowSelectionBeam")) )
	{
		markDead();
		return;
	}
}

void LLHUDEffectSpiral::renderForTimer()
{
	render();
}
