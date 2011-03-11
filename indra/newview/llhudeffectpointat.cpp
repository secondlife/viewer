/** 
 * @file llhudeffectpointat.cpp
 * @brief LLHUDEffectPointAt class implementation
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

#include "llhudeffectpointat.h"

#include "llgl.h"
#include "llrender.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "lldrawable.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "message.h"

// packet layout
const S32 SOURCE_AVATAR = 0;
const S32 TARGET_OBJECT = 16;
const S32 TARGET_POS = 32;
const S32 POINTAT_TYPE = 56;
const S32 PKT_SIZE = 57;

// throttle
const F32 MAX_SENDS_PER_SEC = 4.f;

const F32 MIN_DELTAPOS_FOR_UPDATE = 0.05f;

// timeouts
// can't use actual F32_MAX, because we add this to the current frametime
const F32 MAX_TIMEOUT = F32_MAX / 4.f;

const F32 POINTAT_TIMEOUTS[POINTAT_NUM_TARGETS] = 
{
	MAX_TIMEOUT, //POINTAT_TARGET_NONE
	MAX_TIMEOUT, //POINTAT_TARGET_SELECT
	MAX_TIMEOUT, //POINTAT_TARGET_GRAB
	0.f, //POINTAT_TARGET_CLEAR
};

const S32 POINTAT_PRIORITIES[POINTAT_NUM_TARGETS] = 
{
	0, //POINTAT_TARGET_NONE
	1, //POINTAT_TARGET_SELECT
	2, //POINTAT_TARGET_GRAB
	3, //POINTAT_TARGET_CLEAR
};

// statics

BOOL LLHUDEffectPointAt::sDebugPointAt;


//-----------------------------------------------------------------------------
// LLHUDEffectPointAt()
//-----------------------------------------------------------------------------
LLHUDEffectPointAt::LLHUDEffectPointAt(const U8 type) : 
	LLHUDEffect(type), 
	mKillTime(0.f),
	mLastSendTime(0.f)
{
	clearPointAtTarget();
}

//-----------------------------------------------------------------------------
// ~LLHUDEffectPointAt()
//-----------------------------------------------------------------------------
LLHUDEffectPointAt::~LLHUDEffectPointAt()
{
}

//-----------------------------------------------------------------------------
// packData()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::packData(LLMessageSystem *mesgsys)
{
	// Pack the default data
	LLHUDEffect::packData(mesgsys);

	// Pack the type-specific data.  Uses a fun packed binary format.  Whee!
	U8 packed_data[PKT_SIZE];
	memset(packed_data, 0, PKT_SIZE);

	if (mSourceObject)
	{
		htonmemcpy(&(packed_data[SOURCE_AVATAR]), mSourceObject->mID.mData, MVT_LLUUID, 16);
	}
	else
	{
		htonmemcpy(&(packed_data[SOURCE_AVATAR]), LLUUID::null.mData, MVT_LLUUID, 16);
	}

	// pack both target object and position
	// position interpreted as offset if target object is non-null
	if (mTargetObject)
	{
		htonmemcpy(&(packed_data[TARGET_OBJECT]), mTargetObject->mID.mData, MVT_LLUUID, 16);
	}
	else
	{
		htonmemcpy(&(packed_data[TARGET_OBJECT]), LLUUID::null.mData, MVT_LLUUID, 16);
	}

	htonmemcpy(&(packed_data[TARGET_POS]), mTargetOffsetGlobal.mdV, MVT_LLVector3d, 24);

	U8 pointAtTypePacked = (U8)mTargetType;
	htonmemcpy(&(packed_data[POINTAT_TYPE]), &pointAtTypePacked, MVT_U8, 1);

	mesgsys->addBinaryDataFast(_PREHASH_TypeData, packed_data, PKT_SIZE);

	mLastSendTime = mTimer.getElapsedTimeF32();
}

//-----------------------------------------------------------------------------
// unpackData()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
	LLVector3d new_target;
	U8 packed_data[PKT_SIZE];

	LLUUID dataId;
	mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_ID, dataId, blocknum);

	// ignore messages from ourselves
	if (!gAgentCamera.mPointAt.isNull() && dataId == gAgentCamera.mPointAt->getID())
	{
		return;
	}

	LLHUDEffect::unpackData(mesgsys, blocknum);
	LLUUID source_id;
	LLUUID target_id;
	U8 pointAtTypeUnpacked = 0;
	S32 size = mesgsys->getSizeFast(_PREHASH_Effect, blocknum, _PREHASH_TypeData);
	if (size != PKT_SIZE)
	{
		llwarns << "PointAt effect with bad size " << size << llendl;
		return;
	}
	mesgsys->getBinaryDataFast(_PREHASH_Effect, _PREHASH_TypeData, packed_data, PKT_SIZE, blocknum);
	
	htonmemcpy(source_id.mData, &(packed_data[SOURCE_AVATAR]), MVT_LLUUID, 16);
	htonmemcpy(target_id.mData, &(packed_data[TARGET_OBJECT]), MVT_LLUUID, 16);
	htonmemcpy(new_target.mdV, &(packed_data[TARGET_POS]), MVT_LLVector3d, 24);
	htonmemcpy(&pointAtTypeUnpacked, &(packed_data[POINTAT_TYPE]), MVT_U8, 1);

	LLViewerObject *objp = gObjectList.findObject(source_id);
	if (objp && objp->isAvatar())
	{
		setSourceObject(objp);
	}
	else
	{
		//llwarns << "Could not find source avatar for pointat effect" << llendl;
		return;
	}

	objp = gObjectList.findObject(target_id);

	if (objp)
	{
		setTargetObjectAndOffset(objp, new_target);
	}
	else if (target_id.isNull())
	{
		setTargetPosGlobal(new_target);
	}

	mTargetType = (EPointAtType)pointAtTypeUnpacked;

//	mKillTime = mTimer.getElapsedTimeF32() + mDuration;
	update();
}

//-----------------------------------------------------------------------------
// setTargetObjectAndOffset()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::setTargetObjectAndOffset(LLViewerObject *objp, LLVector3d offset)
{
	mTargetObject = objp;
	mTargetOffsetGlobal = offset;
}

//-----------------------------------------------------------------------------
// setTargetPosGlobal()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::setTargetPosGlobal(const LLVector3d &target_pos_global)
{
	mTargetObject = NULL;
	mTargetOffsetGlobal = target_pos_global;
}

//-----------------------------------------------------------------------------
// setPointAt()
// called by agent logic to set look at behavior locally, and propagate to sim
//-----------------------------------------------------------------------------
BOOL LLHUDEffectPointAt::setPointAt(EPointAtType target_type, LLViewerObject *object, LLVector3 position)
{
	if (!mSourceObject)
	{
		return FALSE;
	}
	
	if (target_type >= POINTAT_NUM_TARGETS)
	{
		llwarns << "Bad target_type " << (int)target_type << " - ignoring." << llendl;
		return FALSE;
	}

	// must be same or higher priority than existing effect
	if (POINTAT_PRIORITIES[target_type] < POINTAT_PRIORITIES[mTargetType])
	{
		return FALSE;
	}

	F32 current_time  = mTimer.getElapsedTimeF32();
	
	// type of pointat behavior or target object has changed
	BOOL targetTypeChanged = (target_type != mTargetType) ||
		(object != mTargetObject);

	BOOL targetPosChanged = (dist_vec_squared(position, mLastSentOffsetGlobal) > MIN_DELTAPOS_FOR_UPDATE * MIN_DELTAPOS_FOR_UPDATE) && 
		((current_time - mLastSendTime) > (1.f / MAX_SENDS_PER_SEC));

	if (targetTypeChanged || targetPosChanged)
	{
		mLastSentOffsetGlobal = position;
		setDuration(POINTAT_TIMEOUTS[target_type]);
		setNeedsSendToSim(TRUE);
//		llinfos << "Sending pointat data" << llendl;
	}

	if (target_type == POINTAT_TARGET_CLEAR)
	{
		clearPointAtTarget();
	}
	else
	{
		mTargetType = target_type;
		mTargetObject = object;
		if (object)
		{
			mTargetOffsetGlobal.setVec(position);
		}
		else
		{
			mTargetOffsetGlobal = gAgent.getPosGlobalFromAgent(position);
		}

		mKillTime = mTimer.getElapsedTimeF32() + mDuration;

		//set up requisite animation data
		update();
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// clearPointAtTarget()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::clearPointAtTarget()
{
	mTargetObject = NULL;
	mTargetOffsetGlobal.clearVec();
	mTargetType = POINTAT_TARGET_NONE;
}

//-----------------------------------------------------------------------------
// markDead()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::markDead()
{
	if (!mSourceObject.isNull() && mSourceObject->isAvatar())
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->removeAnimationData("PointAtPoint");
	}

	clearPointAtTarget();
	LLHUDEffect::markDead();
}

void LLHUDEffectPointAt::setSourceObject(LLViewerObject* objectp)
{
	// restrict source objects to avatars
	if (objectp && objectp->isAvatar())
	{
		LLHUDEffect::setSourceObject(objectp);
	}
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::render()
{
	update();
	if (sDebugPointAt && mTargetType != POINTAT_TARGET_NONE)
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLVector3 target = mTargetPos + mSourceObject->getRenderPosition();
		gGL.pushMatrix();
		gGL.translatef(target.mV[VX], target.mV[VY], target.mV[VZ]);
		glScalef(0.3f, 0.3f, 0.3f);
		gGL.begin(LLRender::LINES);
		{
			gGL.color3f(1.f, 0.f, 0.f);
			gGL.vertex3f(-1.f, 0.f, 0.f);
			gGL.vertex3f(1.f, 0.f, 0.f);

			gGL.vertex3f(0.f, -1.f, 0.f);
			gGL.vertex3f(0.f, 1.f, 0.f);

			gGL.vertex3f(0.f, 0.f, -1.f);
			gGL.vertex3f(0.f, 0.f, 1.f);
		} gGL.end();
		gGL.popMatrix();
	}
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
void LLHUDEffectPointAt::update()
{
	// If the target object is dead, set the target object to NULL
	if (!mTargetObject.isNull() && mTargetObject->isDead())
	{
		clearPointAtTarget();
	}

	if (mSourceObject.isNull() || mSourceObject->isDead())
	{
		markDead();
		return;
	}
	
	F32 time = mTimer.getElapsedTimeF32();

	// clear out the effect if time is up
	if (mKillTime != 0.f && time > mKillTime)
	{
		mTargetType = POINTAT_TARGET_NONE;
	}

	if (mSourceObject->isAvatar())
	{
		if (mTargetType == POINTAT_TARGET_NONE)
		{
			((LLVOAvatar*)(LLViewerObject*)mSourceObject)->removeAnimationData("PointAtPoint");
		}
		else
		{
			if (calcTargetPosition())
			{
				((LLVOAvatar*)(LLViewerObject*)mSourceObject)->startMotion(ANIM_AGENT_EDITING);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// calcTargetPosition()
// returns whether we successfully calculated a finite target position.
//-----------------------------------------------------------------------------
bool LLHUDEffectPointAt::calcTargetPosition()
{
	LLViewerObject *targetObject = (LLViewerObject *)mTargetObject;
	LLVector3 local_offset;
	
	if (targetObject)
	{
		local_offset.setVec(mTargetOffsetGlobal);
	}
	else
	{
		local_offset = gAgent.getPosAgentFromGlobal(mTargetOffsetGlobal);
	}

	if (targetObject && targetObject->mDrawable.notNull())
	{
		LLQuaternion objRot;
		if (targetObject->isAvatar())
		{
			LLVOAvatar *avatarp = (LLVOAvatar *)targetObject;
			mTargetPos = avatarp->mHeadp->getWorldPosition();
			objRot = avatarp->mPelvisp->getWorldRotation();
		}
		else
		{
			if (targetObject->mDrawable->getGeneration() == -1)
			{
				mTargetPos = targetObject->getPositionAgent();
				objRot = targetObject->getWorldRotation();
			}
			else
			{
				mTargetPos = targetObject->getRenderPosition();
				objRot = targetObject->getRenderRotation();
			}
		}

		mTargetPos += (local_offset * objRot);
	}
	else
	{
		mTargetPos = local_offset;
	}

	mTargetPos -= mSourceObject->getRenderPosition();

	if (!llfinite(mTargetPos.lengthSquared()))
	{
		return false;
	}

	if (mSourceObject->isAvatar())
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->setAnimationData("PointAtPoint", (void *)&mTargetPos);
	}

	return true;
}

const LLVector3d LLHUDEffectPointAt::getPointAtPosGlobal()
{
	LLVector3d global_pos;
	global_pos.setVec(mTargetPos);
	if (mSourceObject.notNull())
	{
		global_pos += mSourceObject->getPositionGlobal();
	}
	
	return global_pos;
}
