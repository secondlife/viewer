/** 
 * @file llhudeffectlookat.h
 * @brief LLHUDEffectLookAt class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHUDEFFECTLOOKAT_H
#define LL_LLHUDEFFECTLOOKAT_H

#include "llhudeffect.h"
#include "llskiplist.h"

class LLViewerObject;
class LLVOAvatar;
class LLAttentionSet;

typedef enum e_lookat_type
{
	LOOKAT_TARGET_NONE,
	LOOKAT_TARGET_IDLE,
	LOOKAT_TARGET_AUTO_LISTEN,
	LOOKAT_TARGET_FREELOOK,
	LOOKAT_TARGET_RESPOND,
	LOOKAT_TARGET_HOVER,
	LOOKAT_TARGET_CONVERSATION,
	LOOKAT_TARGET_SELECT,
	LOOKAT_TARGET_FOCUS,
	LOOKAT_TARGET_MOUSELOOK,
	LOOKAT_TARGET_CLEAR,
	LOOKAT_NUM_TARGETS
} ELookAtType;

class LLHUDEffectLookAt : public LLHUDEffect
{
public:
	friend class LLHUDObject;

	/*virtual*/ void markDead();
	/*virtual*/ void setSourceObject(LLViewerObject* objectp);

	BOOL setLookAt(ELookAtType target_type, LLViewerObject *object, LLVector3 position);
	void clearLookAtTarget();

	ELookAtType getLookAtType() { return mTargetType; }
	const LLVector3& getTargetPos() { return mTargetPos; }
	const LLVector3d& getTargetOffset() { return mTargetOffsetGlobal; }
	void calcTargetPosition();

protected:
	LLHUDEffectLookAt(const U8 type);
	~LLHUDEffectLookAt();

	/*virtual*/ void update();
	/*virtual*/ void render();
	/*virtual*/ void packData(LLMessageSystem *mesgsys);
	/*virtual*/ void unpackData(LLMessageSystem *mesgsys, S32 blocknum);
	
	// lookat behavior has either target position or target object with offset
	void setTargetObjectAndOffset(LLViewerObject *objp, LLVector3d offset);
	void setTargetPosGlobal(const LLVector3d &target_pos_global);

public:
	static BOOL sDebugLookAt;

private:
	ELookAtType					mTargetType;
	LLVector3d					mTargetOffsetGlobal;
	LLVector3					mLastSentOffsetGlobal;
	F32							mKillTime;
	LLFrameTimer				mTimer;
	LLVector3					mTargetPos;
	F32							mLastSendTime;
	LLAttentionSet*				mAttentions;
};

#endif // LL_LLHUDEFFECTLOOKAT_H
