/** 
 * @file llhudeffecttrail.cpp
 * @brief LLHUDEffectSpiral class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llhudeffecttrail.h"

#include "llviewercontrol.h"
#include "llimagegl.h"
#include "message.h"

#include "llagent.h"
#include "llbox.h"
#include "lldrawable.h"
#include "llhudrender.h"
#include "llviewerimagelist.h"
#include "llviewerobjectlist.h"
#include "llviewerpartsim.h"
#include "llviewerpartsource.h"
#include "llvoavatar.h"
#include "llworld.h"


const F32 PARTICLE_SPACING = 0.01f;
const F32 MAX_SIZE = 0.025f;
const F32 START_POS_MAG = 1.f;
const F32 END_POS_MAG = 1.2f;


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
		//llwarns << "Missing object in trail pack!" << llendl;
	}
	LLHUDEffect::packData(mesgsys);

	U8 packed_data[56];
	memset(packed_data, 0, 56);

	if (mSourceObject)
	{
		htonmemcpy(packed_data, mSourceObject->mID.mData, MVT_LLUUID, 16);
	}
	if (mTargetObject)
	{
		htonmemcpy(packed_data + 16, mTargetObject->mID.mData, MVT_LLUUID, 16);
	}
	if (!mPositionGlobal.isExactlyZero())
	{
		htonmemcpy(packed_data + 32, mPositionGlobal.mdV, MVT_LLVector3d, 24);
	}
	mesgsys->addBinaryDataFast(_PREHASH_TypeData, packed_data, 56);
}

void LLHUDEffectSpiral::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
	const size_t EFFECT_SIZE = 56;
	U8 packed_data[EFFECT_SIZE];

	LLHUDEffect::unpackData(mesgsys, blocknum);
	LLUUID object_id, target_object_id;
	S32 size = mesgsys->getSizeFast(_PREHASH_Effect, blocknum, _PREHASH_TypeData);
	if (size != EFFECT_SIZE)
	{
		llwarns << "Spiral effect with bad size " << size << llendl;
		return;
	}
	mesgsys->getBinaryDataFast(_PREHASH_Effect, _PREHASH_TypeData, 
		packed_data, EFFECT_SIZE, blocknum, EFFECT_SIZE);
	
	htonmemcpy(object_id.mData, packed_data, MVT_LLUUID, 16);
	htonmemcpy(target_object_id.mData, packed_data + 16, MVT_LLUUID, 16);
	htonmemcpy(mPositionGlobal.mdV, packed_data + 32, MVT_LLVector3d, 24);

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
