/** 
 * @file llheadrotmotion.cpp
 * @brief Implementation of LLHeadRotMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llheadrotmotion.h"
#include "llcharacter.h"
#include "llrand.h"
#include "m3math.h"
#include "v3dmath.h"
#include "llcriticaldamp.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const F32 TORSO_LAG	= 0.35f;	// torso rotation factor
const F32 NECK_LAG = 0.5f;		// neck rotation factor
const F32 HEAD_LOOKAT_LAG_HALF_LIFE	= 0.15f;		// half-life of lookat targeting for head
const F32 TORSO_LOOKAT_LAG_HALF_LIFE	= 0.27f;		// half-life of lookat targeting for torso
const F32 EYE_LOOKAT_LAG_HALF_LIFE = 0.06f;		// half-life of lookat targeting for eye
const F32 HEAD_ROTATION_CONSTRAINT = F_PI_BY_TWO * 0.8f;	// limit angle for head rotation

const F32 MIN_HEAD_LOOKAT_DISTANCE = 0.3f;	// minimum distance from head before we turn to look at it
const F32 MAX_TIME_DELTA = 2.f; //max two seconds a frame for calculating interpolation
const F32 EYE_JITTER_MIN_TIME = 0.3f; // min amount of time between eye "jitter" motions
const F32 EYE_JITTER_MAX_TIME = 2.5f; // max amount of time between eye "jitter" motions
const F32 EYE_JITTER_MAX_YAW = 0.08f; // max yaw of eye jitter motion
const F32 EYE_JITTER_MAX_PITCH = 0.015f; // max pitch of eye jitter motion
const F32 EYE_LOOK_AWAY_MIN_TIME = 5.f; // min amount of time between eye "look away" motions
const F32 EYE_LOOK_AWAY_MAX_TIME = 15.f; // max amount of time between eye "look away" motions
const F32 EYE_LOOK_BACK_MIN_TIME = 1.f; // min amount of time before looking back after looking away
const F32 EYE_LOOK_BACK_MAX_TIME = 5.f; // max amount of time before looking back after looking away
const F32 EYE_LOOK_AWAY_MAX_YAW = 0.15f; // max yaw of eye look away motion
const F32 EYE_LOOK_AWAY_MAX_PITCH = 0.12f; // max pitch of look away motion
const F32 EYE_ROT_LIMIT_ANGLE = F_PI_BY_TWO * 0.3f; //max angle in radians for eye rotation

const F32 EYE_BLINK_MIN_TIME = 0.5f; // minimum amount of time between blinks
const F32 EYE_BLINK_MAX_TIME = 8.f;	// maximum amount of time between blinks
const F32 EYE_BLINK_CLOSE_TIME = 0.03f; // how long the eye stays closed in a blink
const F32 EYE_BLINK_SPEED = 0.015f;		// seconds it takes for a eye open/close movement
const F32 EYE_BLINK_TIME_DELTA = 0.005f; // time between one eye starting a blink and the other following

//-----------------------------------------------------------------------------
// LLHeadRotMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLHeadRotMotion::LLHeadRotMotion(const LLUUID &id) : 
	LLMotion(id),
	mCharacter(NULL),
	mTorsoJoint(NULL),
	mHeadJoint(NULL)
{
	mName = "head_rot";
}


//-----------------------------------------------------------------------------
// ~LLHeadRotMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLHeadRotMotion::~LLHeadRotMotion()
{
}

//-----------------------------------------------------------------------------
// LLHeadRotMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLHeadRotMotion::onInitialize(LLCharacter *character)
{
	if (!character)
		return STATUS_FAILURE;
	mCharacter = character;

	mPelvisJoint = character->getJoint("mPelvis");
	if ( ! mPelvisJoint )
	{
		llinfos << getName() << ": Can't get pelvis joint." << llendl;
		return STATUS_FAILURE;
	}

	mRootJoint = character->getJoint("mRoot");
	if ( ! mRootJoint )
	{
		llinfos << getName() << ": Can't get root joint." << llendl;
		return STATUS_FAILURE;
	}

	mTorsoJoint = character->getJoint("mTorso");
	if ( ! mTorsoJoint )
	{
		llinfos << getName() << ": Can't get torso joint." << llendl;
		return STATUS_FAILURE;
	}

	mHeadJoint = character->getJoint("mHead");
	if ( ! mHeadJoint )
	{
		llinfos << getName() << ": Can't get head joint." << llendl;
		return STATUS_FAILURE;
	}

	mTorsoState.setJoint( character->getJoint("mTorso") );
	if ( ! mTorsoState.getJoint() )
	{
		llinfos << getName() << ": Can't get torso joint." << llendl;
		return STATUS_FAILURE;
	}

	mNeckState.setJoint( character->getJoint("mNeck") );
	if ( ! mNeckState.getJoint() )
	{
		llinfos << getName() << ": Can't get neck joint." << llendl;
		return STATUS_FAILURE;
	}

	mHeadState.setJoint( character->getJoint("mHead") );
	if ( ! mHeadState.getJoint() )
	{
		llinfos << getName() << ": Can't get head joint." << llendl;
		return STATUS_FAILURE;
	}

	mTorsoState.setUsage(LLJointState::ROT);
	mNeckState.setUsage(LLJointState::ROT);
	mHeadState.setUsage(LLJointState::ROT);

	addJointState( &mTorsoState );
	addJointState( &mNeckState );
	addJointState( &mHeadState );

	mLastHeadRot.loadIdentity();

	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// LLHeadRotMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLHeadRotMotion::onActivate()
{
	return TRUE;
}


//-----------------------------------------------------------------------------
// LLHeadRotMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLHeadRotMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLQuaternion	targetHeadRotWorld;
	LLQuaternion	currentRootRotWorld = mRootJoint->getWorldRotation();
	LLQuaternion	currentInvRootRotWorld = ~currentRootRotWorld;

	F32 head_slerp_amt = LLCriticalDamp::getInterpolant(HEAD_LOOKAT_LAG_HALF_LIFE);
	F32 torso_slerp_amt = LLCriticalDamp::getInterpolant(TORSO_LOOKAT_LAG_HALF_LIFE);

	LLVector3* targetPos = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");

	if (targetPos)
	{
		LLVector3 headLookAt = *targetPos;

//		llinfos << "Look At: " << headLookAt + mHeadJoint->getWorldPosition() << llendl;

		F32 lookatDistance = headLookAt.normVec();

		if (lookatDistance < MIN_HEAD_LOOKAT_DISTANCE)
		{
			targetHeadRotWorld = mPelvisJoint->getWorldRotation();
		}
		else
		{
			LLVector3 root_up = LLVector3(0.f, 0.f, 1.f) * currentRootRotWorld;
			LLVector3 left(root_up % headLookAt);
			// if look_at has zero length, fail
			// if look_at and skyward are parallel, fail
			//
			// Test both of these conditions with a cross product.

			if (left.magVecSquared() < 0.15f)
			{
				LLVector3 root_at = LLVector3(1.f, 0.f, 0.f) * currentRootRotWorld;
				root_at.mV[VZ] = 0.f;
				root_at.normVec();

				headLookAt = lerp(headLookAt, root_at, 0.4f);
				headLookAt.normVec();

				left = root_up % headLookAt;
			}
			
			// Make sure look_at and skyward and not parallel
			// and neither are zero length
			LLVector3 up(headLookAt % left);

			targetHeadRotWorld = LLQuaternion(headLookAt, left, up);
		}
	}
	else
	{
		targetHeadRotWorld = currentRootRotWorld;
	}

	LLQuaternion head_rot_local = targetHeadRotWorld * currentInvRootRotWorld;
	head_rot_local.constrain(HEAD_ROTATION_CONSTRAINT);

	// set final torso rotation
	// Set torso target rotation such that it lags behind the head rotation
	// by a fixed amount.
	LLQuaternion torso_rot_local = nlerp(TORSO_LAG, LLQuaternion::DEFAULT, head_rot_local );
	mTorsoState.setRotation( nlerp(torso_slerp_amt, mTorsoState.getRotation(), torso_rot_local) );

	head_rot_local = nlerp(head_slerp_amt, mLastHeadRot, head_rot_local);
	mLastHeadRot = head_rot_local;

	// Set the head rotation.
	LLQuaternion torsoRotLocal =  mNeckState.getJoint()->getParent()->getWorldRotation() * currentInvRootRotWorld;
	head_rot_local = head_rot_local * ~torsoRotLocal;
	mNeckState.setRotation( nlerp(NECK_LAG, LLQuaternion::DEFAULT, head_rot_local) );
	mHeadState.setRotation( nlerp(1.f - NECK_LAG, LLQuaternion::DEFAULT, head_rot_local));

	return TRUE;
}


//-----------------------------------------------------------------------------
// LLHeadRotMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLHeadRotMotion::onDeactivate()
{
}


//-----------------------------------------------------------------------------
// LLEyeMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLEyeMotion::LLEyeMotion(const LLUUID &id) : LLMotion(id)
{
	mCharacter = NULL;
	mEyeJitterTime = 0.f;
	mEyeJitterYaw = 0.f;
	mEyeJitterPitch = 0.f;

	mEyeLookAwayTime = 0.f;
	mEyeLookAwayYaw = 0.f;
	mEyeLookAwayPitch = 0.f;

	mEyeBlinkTime = 0.f;
	mEyesClosed = FALSE;
	
	mHeadJoint = NULL;

	mName = "eye_rot";
}


//-----------------------------------------------------------------------------
// ~LLEyeMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLEyeMotion::~LLEyeMotion()
{
}

//-----------------------------------------------------------------------------
// LLEyeMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLEyeMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;

	mHeadJoint = character->getJoint("mHead");
	if ( ! mHeadJoint )
	{
		llinfos << getName() << ": Can't get head joint." << llendl;
		return STATUS_FAILURE;
	}

	mLeftEyeState.setJoint( character->getJoint("mEyeLeft") );
	if ( ! mLeftEyeState.getJoint() )
	{
		llinfos << getName() << ": Can't get left eyeball joint." << llendl;
		return STATUS_FAILURE;
	}

	mRightEyeState.setJoint( character->getJoint("mEyeRight") );
	if ( ! mRightEyeState.getJoint() )
	{
		llinfos << getName() << ": Can't get Right eyeball joint." << llendl;
		return STATUS_FAILURE;
	}

	mLeftEyeState.setUsage(LLJointState::ROT);
	mRightEyeState.setUsage(LLJointState::ROT);

	addJointState( &mLeftEyeState );
	addJointState( &mRightEyeState );

	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// LLEyeMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLEyeMotion::onActivate()
{
	return TRUE;
}


//-----------------------------------------------------------------------------
// LLEyeMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLEyeMotion::onUpdate(F32 time, U8* joint_mask)
{
	// Compute eye rotation.
	LLQuaternion	target_eye_rot;
	LLVector3		eye_look_at;
	F32				vergence;

	//calculate jitter
	if (mEyeJitterTimer.getElapsedTimeF32() > mEyeJitterTime)
	{
		mEyeJitterTime = EYE_JITTER_MIN_TIME + ll_frand(EYE_JITTER_MAX_TIME - EYE_JITTER_MIN_TIME);
		mEyeJitterYaw = (ll_frand(2.f) - 1.f) * EYE_JITTER_MAX_YAW;
		mEyeJitterPitch = (ll_frand(2.f) - 1.f) * EYE_JITTER_MAX_PITCH;
		// make sure lookaway time count gets updated, because we're resetting the timer
		mEyeLookAwayTime -= llmax(0.f, mEyeJitterTimer.getElapsedTimeF32());
		mEyeJitterTimer.reset();
	} 
	else if (mEyeJitterTimer.getElapsedTimeF32() > mEyeLookAwayTime)
	{
		if (ll_frand() > 0.1f)
		{
			// blink while moving eyes some percentage of the time
			mEyeBlinkTime = mEyeBlinkTimer.getElapsedTimeF32();
		}
		if (mEyeLookAwayYaw == 0.f && mEyeLookAwayPitch == 0.f)
		{
			mEyeLookAwayYaw = (ll_frand(2.f) - 1.f) * EYE_LOOK_AWAY_MAX_YAW;
			mEyeLookAwayPitch = (ll_frand(2.f) - 1.f) * EYE_LOOK_AWAY_MAX_PITCH;
			mEyeLookAwayTime = EYE_LOOK_BACK_MIN_TIME + ll_frand(EYE_LOOK_BACK_MAX_TIME - EYE_LOOK_BACK_MIN_TIME);
		}
		else
		{
			mEyeLookAwayYaw = 0.f;
			mEyeLookAwayPitch = 0.f;
			mEyeLookAwayTime = EYE_LOOK_AWAY_MIN_TIME + ll_frand(EYE_LOOK_AWAY_MAX_TIME - EYE_LOOK_AWAY_MIN_TIME);
		}
	}

	// do blinking
	if (!mEyesClosed && mEyeBlinkTimer.getElapsedTimeF32() >= mEyeBlinkTime)
	{
		F32 leftEyeBlinkMorph = mEyeBlinkTimer.getElapsedTimeF32() - mEyeBlinkTime;
		F32 rightEyeBlinkMorph = leftEyeBlinkMorph - EYE_BLINK_TIME_DELTA;

		leftEyeBlinkMorph = llclamp(leftEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
		rightEyeBlinkMorph = llclamp(rightEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
		mCharacter->setVisualParamWeight("Blink_Left", leftEyeBlinkMorph);
		mCharacter->setVisualParamWeight("Blink_Right", rightEyeBlinkMorph);
		mCharacter->updateVisualParams();

		if (rightEyeBlinkMorph == 1.f)
		{
			mEyesClosed = TRUE;
			mEyeBlinkTime = EYE_BLINK_CLOSE_TIME;
			mEyeBlinkTimer.reset();
		}
	}
	else if (mEyesClosed)
	{
		if (mEyeBlinkTimer.getElapsedTimeF32() >= mEyeBlinkTime)
		{
			F32 leftEyeBlinkMorph = mEyeBlinkTimer.getElapsedTimeF32() - mEyeBlinkTime;
			F32 rightEyeBlinkMorph = leftEyeBlinkMorph - EYE_BLINK_TIME_DELTA;

			leftEyeBlinkMorph = 1.f - llclamp(leftEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
			rightEyeBlinkMorph = 1.f - llclamp(rightEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
			mCharacter->setVisualParamWeight("Blink_Left", leftEyeBlinkMorph);
			mCharacter->setVisualParamWeight("Blink_Right", rightEyeBlinkMorph);
			mCharacter->updateVisualParams();

			if (rightEyeBlinkMorph == 0.f)
			{
				mEyesClosed = FALSE;
				mEyeBlinkTime = EYE_BLINK_MIN_TIME + ll_frand(EYE_BLINK_MAX_TIME - EYE_BLINK_MIN_TIME);
				mEyeBlinkTimer.reset();
			}
		}
	}

	BOOL has_eye_target = FALSE;
	LLVector3* targetPos = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");

	if (targetPos)
	{
		LLVector3		skyward(0.f, 0.f, 1.f);
		LLVector3		left;
		LLVector3		up;

		eye_look_at = *targetPos;
		has_eye_target = TRUE;
		F32 lookAtDistance = eye_look_at.normVec();

		left.setVec(skyward % eye_look_at);
		up.setVec(eye_look_at % left);

		target_eye_rot = LLQuaternion(eye_look_at, left, up);
		// convert target rotation to head-local coordinates
		target_eye_rot *= ~mHeadJoint->getWorldRotation();
		// constrain target orientation to be in front of avatar's face
		target_eye_rot.constrain(EYE_ROT_LIMIT_ANGLE);

		// calculate vergence
		F32 interocular_dist = (mLeftEyeState.getJoint()->getWorldPosition() - mRightEyeState.getJoint()->getWorldPosition()).magVec();
		vergence = -atan2((interocular_dist / 2.f), lookAtDistance);
		llclamp(vergence, -F_PI_BY_TWO, 0.f);
	}
	else
	{
		target_eye_rot = LLQuaternion::DEFAULT;
		vergence = 0.f;
	}

	//RN: subtract 4 degrees to account for foveal angular offset relative to pupil
	vergence += 4.f * DEG_TO_RAD;

	// calculate eye jitter
	LLQuaternion eye_jitter_rot;

	// vergence not too high...
	if (vergence > -0.05f)
	{
		//...go ahead and jitter
		eye_jitter_rot.setQuat(0.f, mEyeJitterPitch + mEyeLookAwayPitch, mEyeJitterYaw + mEyeLookAwayYaw);
	}
	else
	{
		//...or don't
		eye_jitter_rot.loadIdentity();
	}

	// calculate vergence of eyes as an object gets closer to the avatar's head
	LLQuaternion vergence_quat;
		
	if (has_eye_target)
	{
		vergence_quat.setQuat(vergence, LLVector3(0.f, 0.f, 1.f));
	}
	else
	{
		vergence_quat.loadIdentity();
	}

	// calculate eye rotations
	LLQuaternion left_eye_rot = target_eye_rot;
	left_eye_rot = vergence_quat * eye_jitter_rot * left_eye_rot;

	LLQuaternion right_eye_rot = target_eye_rot;
	vergence_quat.transQuat();
	right_eye_rot = vergence_quat * eye_jitter_rot * right_eye_rot;

	mLeftEyeState.setRotation( left_eye_rot );
	mRightEyeState.setRotation( right_eye_rot );

	return TRUE;
}


//-----------------------------------------------------------------------------
// LLEyeMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLEyeMotion::onDeactivate()
{
	LLJoint* joint = mLeftEyeState.getJoint();
	if (joint)
	{
		joint->setRotation(LLQuaternion::DEFAULT);
	}

	joint = mRightEyeState.getJoint();
	if (joint)
	{
		joint->setRotation(LLQuaternion::DEFAULT);
	}
}

// End
