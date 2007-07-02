/** 
 * @file llmotioncontroller.cpp
 * @brief Implementation of LLMotionController class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llmotioncontroller.h"
#include "llkeyframemotion.h"
#include "llmath.h"
#include "lltimer.h"
#include "llanimationstates.h"
#include "llstl.h"

const S32 NUM_JOINT_SIGNATURE_STRIDES = LL_CHARACTER_MAX_JOINTS / 4;
const U32 MAX_MOTION_INSTANCES = 32;

//-----------------------------------------------------------------------------
// Constants and statics
//-----------------------------------------------------------------------------
LLMotionRegistry LLMotionController::sRegistry;

//-----------------------------------------------------------------------------
// LLMotionTableEntry()
//-----------------------------------------------------------------------------
LLMotionTableEntry::LLMotionTableEntry()
{ 
	mConstructor = NULL; 
	mID.setNull(); 
}

LLMotionTableEntry::LLMotionTableEntry(LLMotionConstructor constructor, const LLUUID& id)
		: mConstructor(constructor), mID(id)
{

}

//-----------------------------------------------------------------------------
// create()
//-----------------------------------------------------------------------------
LLMotion* LLMotionTableEntry::create(const LLUUID &id)
{
	LLMotion* motionp = mConstructor(id);

	return motionp;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLMotionRegistry class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLMotionRegistry()
// Class Constructor
//-----------------------------------------------------------------------------
LLMotionRegistry::LLMotionRegistry() : mMotionTable(LLMotionTableEntry::uuidEq, LLMotionTableEntry())
{
	
}


//-----------------------------------------------------------------------------
// ~LLMotionRegistry()
// Class Destructor
//-----------------------------------------------------------------------------
LLMotionRegistry::~LLMotionRegistry()
{
	mMotionTable.removeAll();
}


//-----------------------------------------------------------------------------
// addMotion()
//-----------------------------------------------------------------------------
BOOL LLMotionRegistry::addMotion( const LLUUID& id, LLMotionConstructor constructor )
{
//	llinfos << "Registering motion: " << name << llendl;
	if (!mMotionTable.check(id))
	{
		mMotionTable.set(id, LLMotionTableEntry(constructor, id));
		return TRUE;
	}
	
	return FALSE;
}

//-----------------------------------------------------------------------------
// markBad()
//-----------------------------------------------------------------------------
void LLMotionRegistry::markBad( const LLUUID& id )
{
	mMotionTable.set(id, LLMotionTableEntry());
}

//-----------------------------------------------------------------------------
// createMotion()
//-----------------------------------------------------------------------------
LLMotion *LLMotionRegistry::createMotion( const LLUUID &id )
{
	LLMotionTableEntry motion_entry = mMotionTable.get(id);
	LLMotion* motion = NULL;

	if ( motion_entry.getID().isNull() )
	{
		// *FIX: need to replace with a better default scheme. RN
		motion = LLKeyframeMotion::create(id);
	}
	else
	{
		motion = motion_entry.create(id);
	}

	return motion;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLMotionController class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLMotionController()
// Class Constructor
//-----------------------------------------------------------------------------
LLMotionController::LLMotionController(  )
{
	mTime = 0.f;
	mTimeOffset = 0.f;
	mLastTime = 0.0f;
	mHasRunOnce = FALSE;
	mPaused = FALSE;
	mPauseTime = 0.f;
	mTimeStep = 0.f;
	mTimeStepCount = 0;
	mLastInterp = 0.f;
	mTimeFactor = 1.f;
}


//-----------------------------------------------------------------------------
// ~LLMotionController()
// Class Destructor
//-----------------------------------------------------------------------------
LLMotionController::~LLMotionController()
{
	deleteAllMotions();
}

//-----------------------------------------------------------------------------
// deleteAllMotions()
//-----------------------------------------------------------------------------
void LLMotionController::deleteAllMotions()
{
	mLoadingMotions.clear();
	mLoadedMotions.clear();
	mActiveMotions.clear();

	for_each(mAllMotions.begin(), mAllMotions.end(), DeletePairedPointer());
	mAllMotions.clear();
}

//-----------------------------------------------------------------------------
// addLoadedMotion()
//-----------------------------------------------------------------------------
void LLMotionController::addLoadedMotion(LLMotion* motionp)
{
	if (mLoadedMotions.size() > MAX_MOTION_INSTANCES)
	{
		// too many motions active this frame, kill all blenders
		mPoseBlender.clearBlenders();

		for (U32 i = 0; i < mLoadedMotions.size(); i++)
		{
			LLMotion* cur_motionp = mLoadedMotions.front();
			mLoadedMotions.pop_front();
			
			// motion isn't playing, delete it
			if (!isMotionActive(cur_motionp))
			{
				mCharacter->requestStopMotion(cur_motionp);
				mAllMotions.erase(cur_motionp->getID());
				delete cur_motionp;
				if (mLoadedMotions.size() <= MAX_MOTION_INSTANCES)
				{
					break;
				}
			}
			else
			{
				// put active motion on back
				mLoadedMotions.push_back(cur_motionp);
			}
		}
	}
	mLoadedMotions.push_back(motionp);
}

//-----------------------------------------------------------------------------
// setTimeStep()
//-----------------------------------------------------------------------------
void LLMotionController::setTimeStep(F32 step)
{
	mTimeStep = step;

	if (step != 0.f)
	{
		// make sure timestamps conform to new quantum
		for (motion_list_t::iterator iter = mActiveMotions.begin();
			 iter != mActiveMotions.end(); ++iter)
		{
			LLMotion* motionp = *iter;
			motionp->mActivationTimestamp = (F32)llfloor(motionp->mActivationTimestamp / step) * step;
			BOOL stopped = motionp->isStopped();
			motionp->setStopTime((F32)llfloor(motionp->getStopTime() / step) * step);
			motionp->setStopped(stopped);
			motionp->mSendStopTimestamp = (F32)llfloor(motionp->mSendStopTimestamp / step) * step;
		}
	}
}

//-----------------------------------------------------------------------------
// setTimeFactor()
//-----------------------------------------------------------------------------
void LLMotionController::setTimeFactor(F32 time_factor)
{ 
	mTimeOffset += mTimer.getElapsedTimeAndResetF32() * mTimeFactor; 
	mTimeFactor = time_factor; 
}

//-----------------------------------------------------------------------------
// setCharacter()
//-----------------------------------------------------------------------------
void LLMotionController::setCharacter(LLCharacter *character)
{
	mCharacter = character;
}


//-----------------------------------------------------------------------------
// addMotion()
//-----------------------------------------------------------------------------
BOOL LLMotionController::addMotion( const LLUUID& id, LLMotionConstructor constructor )
{
	return sRegistry.addMotion(id, constructor);
}

//-----------------------------------------------------------------------------
// removeMotion()
//-----------------------------------------------------------------------------
void LLMotionController::removeMotion( const LLUUID& id)
{
	LLMotion* motionp = findMotion(id);
	if (motionp)
	{
		stopMotionLocally(id, TRUE);

		mLoadingMotions.erase(motionp);
		mLoadedMotions.remove(motionp);
		mActiveMotions.remove(motionp);
		mAllMotions.erase(id);
		delete motionp;
	}
}

//-----------------------------------------------------------------------------
// createMotion()
//-----------------------------------------------------------------------------
LLMotion* LLMotionController::createMotion( const LLUUID &id )
{
	// do we have an instance of this motion for this character?
	LLMotion *motion = findMotion(id);

	// if not, we need to create one
	if (!motion)
	{
		// look up constructor and create it
		motion = sRegistry.createMotion(id);
		if (!motion)
		{
			return NULL;
		}

		// look up name for default motions
		const char* motion_name = gAnimLibrary.animStateToString(id);
		if (motion_name)
		{
			motion->setName(motion_name);
		}

		// initialize the new instance
		LLMotion::LLMotionInitStatus stat = motion->onInitialize(mCharacter);
		switch(stat)
		{
		case LLMotion::STATUS_FAILURE:
			llinfos << "Motion " << id << " init failed." << llendl;
			sRegistry.markBad(id);
			delete motion;
			return NULL;
		case LLMotion::STATUS_HOLD:
			mLoadingMotions.insert(motion);
			break;
		case LLMotion::STATUS_SUCCESS:
			// add motion to our list
			addLoadedMotion(motion);
			break;
		default:
			llerrs << "Invalid initialization status" << llendl;
			break;
		}

		mAllMotions[id] = motion;
	}
	return motion;
}

//-----------------------------------------------------------------------------
// startMotion()
//-----------------------------------------------------------------------------
BOOL LLMotionController::startMotion(const LLUUID &id, F32 start_offset)
{
	// look for motion in our list of created motions
	LLMotion *motion = createMotion(id);

	if (!motion)
	{
		return FALSE;
	}
	//if the motion is already active, then we're done
	else if (isMotionActive(motion)) // motion is playing and...
	{	
		if (motion->isStopped()) // motion has been stopped
		{
			deactivateMotion(motion, false);
		}
		else if (mTime < motion->mSendStopTimestamp)	// motion is still active
		{
			return TRUE;
		}
	}

//	llinfos << "Starting motion " << name << llendl;
	return activateMotion(motion, mTime - start_offset);
}


//-----------------------------------------------------------------------------
// stopMotionLocally()
//-----------------------------------------------------------------------------
BOOL LLMotionController::stopMotionLocally(const LLUUID &id, BOOL stop_immediate)
{
	// if already inactive, return false
	LLMotion *motion = findMotion(id);
	if (!motion)
	{
		return FALSE;
	}

	// If on active list, stop it
	if (isMotionActive(motion) && !motion->isStopped())
	{
		// when using timesteps, set stop time to last frame's time, otherwise grab current timer value
		// *FIX: should investigate this inconsistency...hints of obscure bugs

		F32 stop_time = (mTimeStep != 0.f || mPaused) ? (mTime) : mTimeOffset + (mTimer.getElapsedTimeF32() * mTimeFactor);
		motion->setStopTime(stop_time);

		if (stop_immediate)
		{
			deactivateMotion(motion, false);
		}
		return TRUE;
	}
	else if (isMotionLoading(motion))
	{
		motion->setStopped(TRUE);
		return TRUE;
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
// updateRegularMotions()
//-----------------------------------------------------------------------------
void LLMotionController::updateRegularMotions()
{
	updateMotionsByType(LLMotion::NORMAL_BLEND);
}

//-----------------------------------------------------------------------------
// updateAdditiveMotions()
//-----------------------------------------------------------------------------
void LLMotionController::updateAdditiveMotions()
{
	updateMotionsByType(LLMotion::ADDITIVE_BLEND);
}

//-----------------------------------------------------------------------------
// resetJointSignatures()
//-----------------------------------------------------------------------------
void LLMotionController::resetJointSignatures()
{
	memset(&mJointSignature[0][0], 0, sizeof(U8) * LL_CHARACTER_MAX_JOINTS);
	memset(&mJointSignature[1][0], 0, sizeof(U8) * LL_CHARACTER_MAX_JOINTS);
}

//-----------------------------------------------------------------------------
// updateMotionsByType()
//-----------------------------------------------------------------------------
void LLMotionController::updateMotionsByType(LLMotion::LLMotionBlendType anim_type)
{
	BOOL update_result = TRUE;
	U8 last_joint_signature[LL_CHARACTER_MAX_JOINTS];

	memset(&last_joint_signature, 0, sizeof(U8) * LL_CHARACTER_MAX_JOINTS);

	// iterate through active motions in chronological order
	for (motion_list_t::iterator iter = mActiveMotions.begin();
		 iter != mActiveMotions.end(); )
	{
		motion_list_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		if (motionp->getBlendType() != anim_type)
		{
			continue;
		}

		BOOL update_motion = FALSE;

		if (motionp->getPose()->getWeight() < 1.f)
		{
			update_motion = TRUE;
		}
		else
		{
			// NUM_JOINT_SIGNATURE_STRIDES should be multiple of 4
			for (S32 i = 0; i < NUM_JOINT_SIGNATURE_STRIDES; i++)
			{
		 		U32 *current_signature = (U32*)&(mJointSignature[0][i * 4]);
				U32 test_signature = *(U32*)&(motionp->mJointSignature[0][i * 4]);
				
				if ((*current_signature | test_signature) > (*current_signature))
				{
					*current_signature |= test_signature;
					update_motion = TRUE;
				}

				*((U32*)&last_joint_signature[i * 4]) = *(U32*)&(mJointSignature[1][i * 4]);
				current_signature = (U32*)&(mJointSignature[1][i * 4]);
				test_signature = *(U32*)&(motionp->mJointSignature[1][i * 4]);

				if ((*current_signature | test_signature) > (*current_signature))
				{
					*current_signature |= test_signature;
					update_motion = TRUE;
				}
			}
		}

		if (!update_motion)
		{
			if (motionp->isStopped() && mTime > motionp->getStopTime() + motionp->getEaseOutDuration())
			{
				deactivateMotion(motionp, false);
			}
			else if (motionp->isStopped() && mTime > motionp->getStopTime())
			{
				// is this the first iteration in the ease out phase?
				if (mLastTime <= motionp->getStopTime())
				{
					// store residual weight for this motion
					motionp->mResidualWeight = motionp->getPose()->getWeight();
				}
			}
			else if (mTime > motionp->mSendStopTimestamp)
			{
				// notify character of timed stop event on first iteration past sendstoptimestamp
				// this will only be called when an animation stops itself (runs out of time)
				if (mLastTime <= motionp->mSendStopTimestamp)
				{
					mCharacter->requestStopMotion( motionp );
					stopMotionLocally(motionp->getID(), FALSE);
				}
			}
			else if (mTime >= motionp->mActivationTimestamp)
			{
				if (mLastTime < motionp->mActivationTimestamp)
				{
					motionp->mResidualWeight = motionp->getPose()->getWeight();
				}
			}
			continue;
		}

		LLPose *posep = motionp->getPose();

		// only filter by LOD after running every animation at least once (to prime the avatar state)
		if (mHasRunOnce && motionp->getMinPixelArea() > mCharacter->getPixelArea())
		{
			motionp->fadeOut();

			//should we notify the simulator that this motion should be stopped (check even if skipped by LOD logic)
			if (mTime > motionp->mSendStopTimestamp)
			{
				// notify character of timed stop event on first iteration past sendstoptimestamp
				// this will only be called when an animation stops itself (runs out of time)
				if (mLastTime <= motionp->mSendStopTimestamp)
				{
					mCharacter->requestStopMotion( motionp );
					stopMotionLocally(motionp->getID(), FALSE);
				}
			}

			if (motionp->getFadeWeight() < 0.01f)
			{
				if (motionp->isStopped() && mTime > motionp->getStopTime() + motionp->getEaseOutDuration())
				{
					deactivateMotion(motionp, true);
				}
				continue;
			}
		}
		else
		{
			motionp->fadeIn();
		}

		//**********************
		// MOTION INACTIVE
		//**********************
		if (motionp->isStopped() && mTime > motionp->getStopTime() + motionp->getEaseOutDuration())
		{
			// this motion has gone on too long, deactivate it
			// did we have a chance to stop it?
			if (mLastTime <= motionp->getStopTime())
			{
				// if not, let's stop it this time through and deactivate it the next

				posep->setWeight(motionp->getFadeWeight());
				motionp->onUpdate(motionp->getStopTime() - motionp->mActivationTimestamp, last_joint_signature);
			}
			else
			{
				deactivateMotion(motionp, true);
				continue;
			}
		}

		//**********************
		// MOTION EASE OUT
		//**********************
		else if (motionp->isStopped() && mTime > motionp->getStopTime())
		{
			// is this the first iteration in the ease out phase?
			if (mLastTime <= motionp->getStopTime())
			{
				// store residual weight for this motion
				motionp->mResidualWeight = motionp->getPose()->getWeight();
			}

			if (motionp->getEaseOutDuration() == 0.f)
			{
				posep->setWeight(0.f);
			}
			else
			{
				posep->setWeight(motionp->getFadeWeight() * motionp->mResidualWeight * cubic_step(1.f - ((mTime - motionp->getStopTime()) / motionp->getEaseOutDuration())));
			}

			// perform motion update
			update_result = motionp->onUpdate(mTime - motionp->mActivationTimestamp, last_joint_signature);
		}

		//**********************
		// MOTION ACTIVE
		//**********************
		else if (mTime > motionp->mActivationTimestamp + motionp->getEaseInDuration())
		{
			posep->setWeight(motionp->getFadeWeight());

			//should we notify the simulator that this motion should be stopped?
			if (mTime > motionp->mSendStopTimestamp)
			{
				// notify character of timed stop event on first iteration past sendstoptimestamp
				// this will only be called when an animation stops itself (runs out of time)
				if (mLastTime <= motionp->mSendStopTimestamp)
				{
					mCharacter->requestStopMotion( motionp );
					stopMotionLocally(motionp->getID(), FALSE);
				}
			}

			// perform motion update
			update_result = motionp->onUpdate(mTime - motionp->mActivationTimestamp, last_joint_signature);
		}

		//**********************
		// MOTION EASE IN
		//**********************
		else if (mTime >= motionp->mActivationTimestamp)
		{
			if (mLastTime < motionp->mActivationTimestamp)
			{
				motionp->mResidualWeight = motionp->getPose()->getWeight();
			}
			if (motionp->getEaseInDuration() == 0.f)
			{
				posep->setWeight(motionp->getFadeWeight());
			}
			else
			{
				// perform motion update
				posep->setWeight(motionp->getFadeWeight() * motionp->mResidualWeight + (1.f - motionp->mResidualWeight) * cubic_step((mTime - motionp->mActivationTimestamp) / motionp->getEaseInDuration()));
			}
			// perform motion update
			update_result = motionp->onUpdate(mTime - motionp->mActivationTimestamp, last_joint_signature);
		}
		else
		{
			posep->setWeight(0.f);
			update_result = motionp->onUpdate(0.f, last_joint_signature);
		}
		
		// allow motions to deactivate themselves 
		if (!update_result)
		{
			if (!motionp->isStopped() || motionp->getStopTime() > mTime)
			{
				// animation has stopped itself due to internal logic
				// propagate this to the network
				// as not all viewers are guaranteed to have access to the same logic
				mCharacter->requestStopMotion( motionp );
				stopMotionLocally(motionp->getID(), FALSE);
			}

		}

		// even if onupdate returns FALSE, add this motion in to the blend one last time
		mPoseBlender.addMotion(motionp);
	}
}


//-----------------------------------------------------------------------------
// updateMotion()
//-----------------------------------------------------------------------------
void LLMotionController::updateMotion()
{
	BOOL use_quantum = (mTimeStep != 0.f);

	// Update timing info for this time step.
	if (!mPaused)
	{
		F32 update_time = mTimeOffset + (mTimer.getElapsedTimeF32() * mTimeFactor);
		if (use_quantum)
		{
			F32 time_interval = fmodf(update_time, mTimeStep);

			// always animate *ahead* of actual time
			S32 quantum_count = llmax(0, llfloor((update_time - time_interval) / mTimeStep)) + 1;
			if (quantum_count == mTimeStepCount)
			{
				// we're still in same time quantum as before, so just interpolate and exit
				if (!mPaused)
				{
					F32 interp = time_interval / mTimeStep;
					mPoseBlender.interpolate(interp - mLastInterp);
					mLastInterp = interp;
				}
				
				return;
			}
			
			// is calculating a new keyframe pose, make sure the last one gets applied
			mPoseBlender.interpolate(1.f);
			mPoseBlender.clearBlenders();

			mTimeStepCount = quantum_count;
			mLastTime = mTime;
			mTime = (F32)quantum_count * mTimeStep;
			mLastInterp = 0.f;
		}
		else
		{
			mLastTime = mTime;
			mTime = update_time;
		}
	}

	// query pending motions for completion
	for (motion_set_t::iterator iter = mLoadingMotions.begin();
		 iter != mLoadingMotions.end(); )
	{
		motion_set_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		LLMotion::LLMotionInitStatus status = motionp->onInitialize(mCharacter);
		if (status == LLMotion::STATUS_SUCCESS)
		{
			mLoadingMotions.erase(curiter);
			// add motion to our loaded motion list
			addLoadedMotion(motionp);
			// this motion should be playing
			if (!motionp->isStopped())
			{
				activateMotion(motionp, mTime);
			}
		}
		else if (status == LLMotion::STATUS_FAILURE)
		{
			llinfos << "Motion " << motionp->getID() << " init failed." << llendl;
			sRegistry.markBad(motionp->getID());
			mLoadingMotions.erase(curiter);
			mAllMotions.erase(motionp->getID());
			delete motionp;
		}
	}

	resetJointSignatures();

	if (!mPaused)
	{
		// update additive motions
		updateAdditiveMotions();
		resetJointSignatures();

		// update all regular motions
		updateRegularMotions();

		if (use_quantum)
		{
			mPoseBlender.blendAndCache(TRUE);
		}
		else
		{
			mPoseBlender.blendAndApply();
		}
	}

	mHasRunOnce = TRUE;
//	llinfos << "Motion controller time " << motionTimer.getElapsedTimeF32() << llendl;
}


//-----------------------------------------------------------------------------
// activateMotion()
//-----------------------------------------------------------------------------
BOOL LLMotionController::activateMotion(LLMotion *motion, F32 time)
{
	if (mLoadingMotions.find(motion) != mLoadingMotions.end())
	{
		// we want to start this motion, but we can't yet, so flag it as started
		motion->setStopped(FALSE);
		// report pending animations as activated
		return TRUE;
	}

	motion->mResidualWeight = motion->getPose()->getWeight();
	motion->mActivationTimestamp = time;

	// set stop time based on given duration and ease out time
	if (motion->getDuration() != 0.f && !motion->getLoop())
	{
		F32 ease_out_time;
		F32 motion_duration;

		// should we stop at the end of motion duration, or a bit earlier 
		// to allow it to ease out while moving?
		ease_out_time = motion->getEaseOutDuration();

		// is the clock running when the motion is easing in?
		// if not (POSTURE_EASE) then we need to wait that much longer before triggering the stop
		motion_duration = llmax(motion->getDuration() - ease_out_time, 0.f);
		motion->mSendStopTimestamp = time + motion_duration;
	} 
	else
	{
		motion->mSendStopTimestamp = F32_MAX;
	}

	mActiveMotions.remove(motion); // in case it is already in the active list
	mActiveMotions.push_front(motion);

	motion->activate();
	motion->onUpdate(0.f, mJointSignature[1]);

	return TRUE;
}

//-----------------------------------------------------------------------------
// deactivateMotion()
//-----------------------------------------------------------------------------
BOOL LLMotionController::deactivateMotion(LLMotion *motion, bool remove_weight)
{
	if( remove_weight )
	{
		// immediately remove pose weighting instead of letting it time out
		LLPose *posep = motion->getPose();
		posep->setWeight(0.f);
	}
	
	motion->deactivate();
	mActiveMotions.remove(motion);

	return TRUE;
}

//-----------------------------------------------------------------------------
// isMotionActive()
//-----------------------------------------------------------------------------
bool LLMotionController::isMotionActive(LLMotion *motion)
{
	return (motion && motion->isActive());
}

//-----------------------------------------------------------------------------
// isMotionLoading()
//-----------------------------------------------------------------------------
bool LLMotionController::isMotionLoading(LLMotion* motion)
{
	return (mLoadingMotions.find(motion) != mLoadingMotions.end());
}


//-----------------------------------------------------------------------------
// findMotion()
//-----------------------------------------------------------------------------
LLMotion *LLMotionController::findMotion(const LLUUID& id)
{
	return mAllMotions[id];
}

//-----------------------------------------------------------------------------
// deactivateAllMotions()
//-----------------------------------------------------------------------------
void LLMotionController::deactivateAllMotions()
{
	//They must all die, precious.
	for (std::map<LLUUID, LLMotion*>::iterator iter = mAllMotions.begin();
		 iter != mAllMotions.end(); iter++)
	{
		LLMotion* motionp = iter->second;
		if (motionp) motionp->deactivate();
	}

	// delete all motion instances
	deleteAllMotions();
}


//-----------------------------------------------------------------------------
// flushAllMotions()
//-----------------------------------------------------------------------------
void LLMotionController::flushAllMotions()
{
	std::vector<std::pair<LLUUID,F32> > active_motions;
	active_motions.reserve(mActiveMotions.size());
	for (motion_list_t::iterator iter = mActiveMotions.begin();
		 iter != mActiveMotions.end(); )
	{
		motion_list_t::iterator curiter = iter++;
		LLMotion* motionp = *curiter;
		F32 dtime = mTime - motionp->mActivationTimestamp;
		active_motions.push_back(std::make_pair(motionp->getID(),dtime));
		motionp->deactivate();
	}

	// delete all motion instances
	deleteAllMotions();

	// kill current hand pose that was previously called out by
	// keyframe motion
	mCharacter->removeAnimationData("Hand Pose");

	// restart motions
	for (std::vector<std::pair<LLUUID,F32> >::iterator iter = active_motions.begin();
		 iter != active_motions.end(); ++iter)
	{
		startMotion(iter->first, iter->second);
	}
}

//-----------------------------------------------------------------------------
// pause()
//-----------------------------------------------------------------------------
void LLMotionController::pause()
{
	if (!mPaused)
	{
		//llinfos << "Pausing animations..." << llendl;
		mPauseTime = mTimer.getElapsedTimeF32();
		mPaused = TRUE;
	}
	
}

//-----------------------------------------------------------------------------
// unpause()
//-----------------------------------------------------------------------------
void LLMotionController::unpause()
{
	if (mPaused)
	{
		//llinfos << "Unpausing animations..." << llendl;
		mTimer.reset();
		mTimer.setAge(mPauseTime);
		mPaused = FALSE;
	}
}
// End
