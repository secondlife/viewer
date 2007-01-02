/** 
 * @file llhudeffectpointat.h
 * @brief LLHUDEffectPointAt class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHUDEFFECTPOINTAT_H
#define LL_LLHUDEFFECTPOINTAT_H

#include "llhudeffect.h"

class LLViewerObject;
class LLVOAvatar;

typedef enum e_pointat_type
{
	POINTAT_TARGET_NONE,
	POINTAT_TARGET_SELECT,
	POINTAT_TARGET_GRAB,
	POINTAT_TARGET_CLEAR,
	POINTAT_NUM_TARGETS
} EPointAtType;

class LLHUDEffectPointAt : public LLHUDEffect
{
public:
	friend class LLHUDObject;

	/*virtual*/ void markDead();
	/*virtual*/ void setSourceObject(LLViewerObject* objectp);

	BOOL setPointAt(EPointAtType target_type, LLViewerObject *object, LLVector3 position);
	void clearPointAtTarget();

	EPointAtType getPointAtType() { return mTargetType; }
	const LLVector3& getPointAtPosAgent() { return mTargetPos; }
	const LLVector3d getPointAtPosGlobal();
protected:
	LLHUDEffectPointAt(const U8 type);
	~LLHUDEffectPointAt();

	/*virtual*/ void render();
	/*virtual*/ void packData(LLMessageSystem *mesgsys);
	/*virtual*/ void unpackData(LLMessageSystem *mesgsys, S32 blocknum);

	// lookat behavior has either target position or target object with offset
	void setTargetObjectAndOffset(LLViewerObject *objp, LLVector3d offset);
	void setTargetPosGlobal(const LLVector3d &target_pos_global);
	void calcTargetPosition();
	void update();
public:
	static BOOL sDebugPointAt;
private:
	EPointAtType				mTargetType;
	LLVector3d					mTargetOffsetGlobal;
	LLVector3					mLastSentOffsetGlobal;
	F32							mKillTime;
	LLFrameTimer				mTimer;
	LLVector3					mTargetPos;
	F32							mLastSendTime;
};

#endif // LL_LLHUDEFFECTPOINTAT_H
