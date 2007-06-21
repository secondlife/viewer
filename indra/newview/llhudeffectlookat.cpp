/** 
 * @file llhudeffectlookat.cpp
 * @brief LLHUDEffectLookAt class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llhudeffectlookat.h"

#include "message.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "lldrawable.h"
#include "llviewerobjectlist.h"
#include "llsphere.h"
#include "llselectmgr.h"
#include "llglheaders.h"

BOOL LLHUDEffectLookAt::sDebugLookAt = FALSE;

// packet layout
const S32 SOURCE_AVATAR = 0;
const S32 TARGET_OBJECT = 16;
const S32 TARGET_POS = 32;
const S32 LOOKAT_TYPE = 56;
const S32 PKT_SIZE = 57;

// throttle
const F32 MAX_SENDS_PER_SEC = 4.f;

const F32 MIN_DELTAPOS_FOR_UPDATE = 0.05f;
const F32 MIN_TARGET_OFFSET_SQUARED = 0.0001f;

// timeouts
// can't use actual F32_MAX, because we add this to the current frametime
const F32 MAX_TIMEOUT = F32_MAX / 2.f;

const F32 LOOKAT_TIMEOUTS[LOOKAT_NUM_TARGETS] = 
{
	MAX_TIMEOUT, //LOOKAT_TARGET_NONE
	3.f, //LOOKAT_TARGET_IDLE
	4.f, //LOOKAT_TARGET_AUTO_LISTEN
	2.f, //LOOKAT_TARGET_FREELOOK
	4.f, //LOOKAT_TARGET_RESPOND
	1.f, //LOOKAT_TARGET_HOVER
	MAX_TIMEOUT, //LOOKAT_TARGET_CONVERSATION
	MAX_TIMEOUT, //LOOKAT_TARGET_SELECT
	MAX_TIMEOUT, //LOOKAT_TARGET_FOCUS
	MAX_TIMEOUT, //LOOKAT_TARGET_MOUSELOOK
	0.f, //LOOKAT_TARGET_CLEAR
};

const S32 LOOKAT_PRIORITIES[LOOKAT_NUM_TARGETS] = 
{
	0, //LOOKAT_TARGET_NONE
	1, //LOOKAT_TARGET_IDLE
	3, //LOOKAT_TARGET_AUTO_LISTEN
	2, //LOOKAT_TARGET_FREELOOK
	3, //LOOKAT_TARGET_RESPOND
	4, //LOOKAT_TARGET_HOVER
	5, //LOOKAT_TARGET_CONVERSATION
	6, //LOOKAT_TARGET_SELECT
	6, //LOOKAT_TARGET_FOCUS
	7, //LOOKAT_TARGET_MOUSELOOK
	8, //LOOKAT_TARGET_CLEAR
};

const char *LOOKAT_STRINGS[] = 
{
	"None",			//LOOKAT_TARGET_NONE
	"Idle",			//LOOKAT_TARGET_IDLE
	"AutoListen",	//LOOKAT_TARGET_AUTO_LISTEN
	"FreeLook",		//LOOKAT_TARGET_FREELOOK
	"Respond",		//LOOKAT_TARGET_RESPOND
	"Hover",		//LOOKAT_TARGET_HOVER
	"Conversation", //LOOKAT_TARGET_CONVERSATION
	"Select",		//LOOKAT_TARGET_SELECT
	"Focus",		//LOOKAT_TARGET_FOCUS
	"Mouselook",	//LOOKAT_TARGET_MOUSELOOK
	"Clear",		//LOOKAT_TARGET_CLEAR
};

const LLColor3 LOOKAT_COLORS[LOOKAT_NUM_TARGETS] =
{
	LLColor3(0.3f, 0.3f, 0.3f), //LOOKAT_TARGET_NONE
	LLColor3(0.5f, 0.5f, 0.5f), //LOOKAT_TARGET_IDLE
	LLColor3(0.5f, 0.5f, 0.5f), //LOOKAT_TARGET_AUTO_LISTEN
	LLColor3(0.5f, 0.5f, 0.9f), //LOOKAT_TARGET_FREELOOK
	LLColor3(0.f, 0.f, 0.f),	//LOOKAT_TARGET_RESPOND
	LLColor3(0.5f, 0.9f, 0.5f), //LOOKAT_TARGET_HOVER
	LLColor3(0.1f, 0.1f, 0.5f),	//LOOKAT_TARGET_CONVERSATION
	LLColor3(0.9f, 0.5f, 0.5f), //LOOKAT_TARGET_SELECT
	LLColor3(0.9f, 0.5f, 0.9f), //LOOKAT_TARGET_FOCUS
	LLColor3(0.9f, 0.9f, 0.5f), //LOOKAT_TARGET_MOUSELOOK
	LLColor3(1.f, 1.f, 1.f),	//LOOKAT_TARGET_CLEAR
};

//-----------------------------------------------------------------------------
// LLHUDEffectLookAt()
//-----------------------------------------------------------------------------
LLHUDEffectLookAt::LLHUDEffectLookAt(const U8 type) : 
	LLHUDEffect(type), 
	mKillTime(0.f),
	mLastSendTime(0.f)
{
	clearLookAtTarget();
}

//-----------------------------------------------------------------------------
// ~LLHUDEffectLookAt()
//-----------------------------------------------------------------------------
LLHUDEffectLookAt::~LLHUDEffectLookAt()
{
}

//-----------------------------------------------------------------------------
// packData()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::packData(LLMessageSystem *mesgsys)
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

	U8 lookAtTypePacked = (U8)mTargetType;
	
	htonmemcpy(&(packed_data[LOOKAT_TYPE]), &lookAtTypePacked, MVT_U8, 1);

	mesgsys->addBinaryDataFast(_PREHASH_TypeData, packed_data, PKT_SIZE);

	mLastSendTime = mTimer.getElapsedTimeF32();
}

//-----------------------------------------------------------------------------
// unpackData()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
	LLVector3d new_target;
	U8 packed_data[PKT_SIZE];

	LLUUID dataId;
	mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_ID, dataId, blocknum);

	if (!gAgent.mLookAt.isNull() && dataId == gAgent.mLookAt->getID())
	{
		return;
	}

	LLHUDEffect::unpackData(mesgsys, blocknum);
	LLUUID source_id;
	LLUUID target_id;
	S32 size = mesgsys->getSizeFast(_PREHASH_Effect, blocknum, _PREHASH_TypeData);
	if (size != PKT_SIZE)
	{
		llwarns << "LookAt effect with bad size " << size << llendl;
		return;
	}
	mesgsys->getBinaryDataFast(_PREHASH_Effect, _PREHASH_TypeData, packed_data, PKT_SIZE, blocknum);
	
	htonmemcpy(source_id.mData, &(packed_data[SOURCE_AVATAR]), MVT_LLUUID, 16);

	LLViewerObject *objp = gObjectList.findObject(source_id);
	if (objp && objp->isAvatar())
	{
		setSourceObject(objp);
	}
	else
	{
		//llwarns << "Could not find source avatar for lookat effect" << llendl;
		return;
	}

	htonmemcpy(target_id.mData, &(packed_data[TARGET_OBJECT]), MVT_LLUUID, 16);

	objp = gObjectList.findObject(target_id);

	htonmemcpy(new_target.mdV, &(packed_data[TARGET_POS]), MVT_LLVector3d, 24);

	if (objp)
	{
		setTargetObjectAndOffset(objp, new_target);
	}
	else if (target_id.isNull())
	{
		setTargetPosGlobal(new_target);
	}
	else
	{
		//llwarns << "Could not find target object for lookat effect" << llendl;
	}

	U8 lookAtTypeUnpacked = 0;
	htonmemcpy(&lookAtTypeUnpacked, &(packed_data[LOOKAT_TYPE]), MVT_U8, 1);
	mTargetType = (ELookAtType)lookAtTypeUnpacked;

	if (mTargetType == LOOKAT_TARGET_NONE)
	{
		clearLookAtTarget();
	}
}

//-----------------------------------------------------------------------------
// setTargetObjectAndOffset()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::setTargetObjectAndOffset(LLViewerObject *objp, LLVector3d offset)
{
	mTargetObject = objp;
	mTargetOffsetGlobal = offset;
}

//-----------------------------------------------------------------------------
// setTargetPosGlobal()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::setTargetPosGlobal(const LLVector3d &target_pos_global)
{
	mTargetObject = NULL;
	mTargetOffsetGlobal = target_pos_global;
}

//-----------------------------------------------------------------------------
// setLookAt()
// called by agent logic to set look at behavior locally, and propagate to sim
//-----------------------------------------------------------------------------
BOOL LLHUDEffectLookAt::setLookAt(ELookAtType target_type, LLViewerObject *object, LLVector3 position)
{
	if (!mSourceObject)
	{
		return FALSE;
	}
	
	llassert(target_type < LOOKAT_NUM_TARGETS);

	// must be same or higher priority than existing effect
	if (LOOKAT_PRIORITIES[target_type] < LOOKAT_PRIORITIES[mTargetType])
	{
		return FALSE;
	}

	F32 current_time  = mTimer.getElapsedTimeF32();

	// type of lookat behavior or target object has changed
	BOOL lookAtChanged = (target_type != mTargetType) ||
		(object != mTargetObject);

	// lookat position has moved a certain amount and we haven't just sent an update
	lookAtChanged = lookAtChanged || (dist_vec(position, mLastSentOffsetGlobal) > MIN_DELTAPOS_FOR_UPDATE) && 
		((current_time - mLastSendTime) > (1.f / MAX_SENDS_PER_SEC));

	if (lookAtChanged)
	{
		mLastSentOffsetGlobal = position;
		setDuration(LOOKAT_TIMEOUTS[target_type]);
		setNeedsSendToSim(TRUE);
	}
 
	if (target_type == LOOKAT_TARGET_CLEAR)
	{
		clearLookAtTarget();
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

		update();
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// clearLookAtTarget()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::clearLookAtTarget()
{
	mTargetObject = NULL;
	mTargetOffsetGlobal.clearVec();
	mTargetType = LOOKAT_TARGET_NONE;
	if (mSourceObject.notNull())
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->stopMotion(ANIM_AGENT_HEAD_ROT);
	}
}

//-----------------------------------------------------------------------------
// markDead()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::markDead()
{
	if (mSourceObject.notNull())
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->removeAnimationData("LookAtPoint");
	}

	mSourceObject = NULL;
	clearLookAtTarget();
	LLHUDEffect::markDead();
}

void LLHUDEffectLookAt::setSourceObject(LLViewerObject* objectp)
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
void LLHUDEffectLookAt::render()
{
	if (sDebugLookAt && mSourceObject.notNull())
	{
		LLGLSNoTexture gls_no_texture;

		LLVector3 target = mTargetPos + ((LLVOAvatar*)(LLViewerObject*)mSourceObject)->mHeadp->getWorldPosition();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(target.mV[VX], target.mV[VY], target.mV[VZ]);
		glScalef(0.3f, 0.3f, 0.3f);
		glBegin(GL_LINES);
		{
			LLColor3 color = LOOKAT_COLORS[mTargetType];
			glColor3f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE]);
			glVertex3f(-1.f, 0.f, 0.f);
			glVertex3f(1.f, 0.f, 0.f);

			glVertex3f(0.f, -1.f, 0.f);
			glVertex3f(0.f, 1.f, 0.f);

			glVertex3f(0.f, 0.f, -1.f);
			glVertex3f(0.f, 0.f, 1.f);
		} glEnd();
		glPopMatrix();
	}
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::update()
{
	// If the target object is dead, set the target object to NULL
	if (!mTargetObject.isNull() && mTargetObject->isDead())
	{
		clearLookAtTarget();
	}

	// if source avatar is null or dead, mark self as dead and return
	if (mSourceObject.isNull() || mSourceObject->isDead())
	{
		markDead();
		return;
	}

	F32 time = mTimer.getElapsedTimeF32();

	// clear out the effect if time is up
	if (mKillTime != 0.f && time > mKillTime)
	{
		if (mTargetType != LOOKAT_TARGET_NONE)
		{
			clearLookAtTarget();
			// look at timed out (only happens on own avatar), so tell everyone
			setNeedsSendToSim(TRUE);
		}
	}

	if (mTargetType != LOOKAT_TARGET_NONE)
	{
		calcTargetPosition();

		LLMotion* head_motion = ((LLVOAvatar*)(LLViewerObject*)mSourceObject)->findMotion(ANIM_AGENT_HEAD_ROT);
		if (!head_motion || head_motion->isStopped())
		{
			((LLVOAvatar*)(LLViewerObject*)mSourceObject)->startMotion(ANIM_AGENT_HEAD_ROT);
		}
	}

	if (sDebugLookAt)
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->addDebugText(LOOKAT_STRINGS[mTargetType]);
	}
}

void LLHUDEffectLookAt::calcTargetPosition()
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

	if (gNoRender)
	{
		return;
	}
	LLVector3 head_position = ((LLVOAvatar*)(LLViewerObject*)mSourceObject)->mHeadp->getWorldPosition();

	if (targetObject && targetObject->mDrawable.notNull())
	{
		LLQuaternion objRot;
		if (targetObject->isAvatar())
		{
			LLVOAvatar *avatarp = (LLVOAvatar *)targetObject;

			BOOL looking_at_self = ((LLVOAvatar*)(LLViewerObject*)mSourceObject)->isSelf() && avatarp->isSelf();

			// if selecting self, stare forward
			if (looking_at_self && mTargetOffsetGlobal.magVecSquared() < MIN_TARGET_OFFSET_SQUARED)
			{
				//sets the lookat point in front of the avatar
				mTargetOffsetGlobal.setVec(5.0, 0.0, 0.0);
				local_offset.setVec(mTargetOffsetGlobal);
			}

			mTargetPos = avatarp->mHeadp->getWorldPosition();
			if (mTargetType == LOOKAT_TARGET_MOUSELOOK || mTargetType == LOOKAT_TARGET_FREELOOK)
			{
				// mouselook and freelook target offsets are absolute
				objRot = LLQuaternion::DEFAULT;
			}
			else if (looking_at_self && gAgent.cameraCustomizeAvatar())
			{
				// *NOTE: We have to do this because animation
				// overrides do not set lookat behavior.
				// *TODO: animation overrides for lookat behavior.
				objRot = avatarp->mPelvisp->getWorldRotation();
			}
			else
			{
				objRot = avatarp->mRoot.getWorldRotation();
			}
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

	mTargetPos -= head_position;

	((LLVOAvatar*)(LLViewerObject*)mSourceObject)->setAnimationData("LookAtPoint", (void *)&mTargetPos);
}
