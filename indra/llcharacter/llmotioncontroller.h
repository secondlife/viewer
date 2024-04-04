/** 
 * @file llmotioncontroller.h
 * @brief Implementation of LLMotionController class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLMOTIONCONTROLLER_H
#define LL_LLMOTIONCONTROLLER_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include <string>
#include <map>
#include <deque>

#include "llmotion.h"
#include "llpose.h"
#include "llframetimer.h"
#include "llstatemachine.h"
#include "llstring.h"

//-----------------------------------------------------------------------------
// Class predeclaration
// This is necessary because llcharacter.h includes this file.
//-----------------------------------------------------------------------------
class LLCharacter;

//-----------------------------------------------------------------------------
// LLMotionRegistry
//-----------------------------------------------------------------------------
typedef LLMotion*(*LLMotionConstructor)(const LLUUID &id);

class LLMotionRegistry
{
public:
	// Constructor
	LLMotionRegistry();

	// Destructor
	~LLMotionRegistry();

	// adds motion classes to the registry
	// returns true if successfull
	bool registerMotion( const LLUUID& id, LLMotionConstructor create);

	// creates a new instance of a named motion
	// returns NULL motion is not registered
	LLMotion *createMotion( const LLUUID &id );

	// initialization of motion failed, don't try to create this motion again
	void markBad( const LLUUID& id );


protected:
	typedef std::map<LLUUID, LLMotionConstructor> motion_map_t;
	motion_map_t mMotionTable;
};

//-----------------------------------------------------------------------------
// class LLMotionController
//-----------------------------------------------------------------------------
class LLMotionController
{
public:
	typedef std::list<LLMotion*> motion_list_t;
	typedef std::set<LLMotion*> motion_set_t;
	bool mIsSelf;
	
public:
	// Constructor
	LLMotionController();

	// Destructor
	virtual ~LLMotionController();

	// set associated character
	// this must be called exactly once by the containing character class.
	// this is generally done in the Character constructor
	void setCharacter( LLCharacter *character );

	// registers a motion with the controller
	// (actually just forwards call to motion registry)
	// returns true if successfull
	bool registerMotion( const LLUUID& id, LLMotionConstructor create );

	// creates a motion from the registry
	LLMotion *createMotion( const LLUUID &id );

	// unregisters a motion with the controller
	// (actually just forwards call to motion registry)
	// returns true if successfull
	void removeMotion( const LLUUID& id );

	// start motion
	// begins playing the specified motion
	// returns true if successful
	bool startMotion( const LLUUID &id, F32 start_offset );

	// stop motion
	// stops a playing motion
	// in reality, it begins the ease out transition phase
	// returns true if successful
	bool stopMotionLocally( const LLUUID &id, bool stop_immediate );

	// Move motions from loading to loaded
	void updateLoadingMotions();
	
	// update motions
	// invokes the update handlers for each active motion
	// activates sequenced motions
	// deactivates terminated motions`
	void updateMotions(bool force_update = false);

	// minimal update (e.g. while hidden)
	void updateMotionsMinimal();

	void clearBlenders() { mPoseBlender.clearBlenders(); }

	// flush motions
	// releases all motion instances
	void flushAllMotions();

	//Flush is a liar.
	void deactivateAllMotions();	

	// pause and continue all motions
	void pauseAllMotions();
	void unpauseAllMotions();
	bool isPaused() const { return mPaused; }
    S32 getPausedFrame() const { return mPausedFrame; }

	void setTimeStep(F32 step);
    F32 getTimeStep() const { return mTimeStep; }

	void setTimeFactor(F32 time_factor);
	F32 getTimeFactor() const { return mTimeFactor; }

    F32 getAnimTime() const { return mAnimTime; }
    
	motion_list_t& getActiveMotions() { return mActiveMotions; }

	void incMotionCounts(S32& num_motions, S32& num_loading_motions, S32& num_loaded_motions, S32& num_active_motions, S32& num_deprecated_motions);
	
//protected:
	bool isMotionActive( LLMotion *motion );
	bool isMotionLoading( LLMotion *motion );
	LLMotion *findMotion( const LLUUID& id ) const;

	void dumpMotions();

	const LLFrameTimer& getFrameTimer() { return mTimer; }

	static F32	getCurrentTimeFactor()				{ return sCurrentTimeFactor;	};
	static void setCurrentTimeFactor(F32 factor)	{ sCurrentTimeFactor = factor;	};

protected:
	// internal operations act on motion instances directly
	// as there can be duplicate motions per id during blending overlap
	void deleteAllMotions();
	bool activateMotionInstance(LLMotion *motion, F32 time);
	bool deactivateMotionInstance(LLMotion *motion);
	void deprecateMotionInstance(LLMotion* motion);
	bool stopMotionInstance(LLMotion *motion, bool stop_imemdiate);
	void removeMotionInstance(LLMotion* motion);
	void updateRegularMotions();
	void updateAdditiveMotions();
	void resetJointSignatures();
	void updateMotionsByType(LLMotion::LLMotionBlendType motion_type);
	void updateIdleMotion(LLMotion* motionp);
	void updateIdleActiveMotions();
	void purgeExcessMotions();
	void deactivateStoppedMotions();

protected:
	F32					mTimeFactor;			// 1.f for normal speed
	static F32			sCurrentTimeFactor;		// Value to use for initialization
	static LLMotionRegistry	sRegistry;
	LLPoseBlender		mPoseBlender;

	LLCharacter			*mCharacter;

//	Life cycle of an animation:
//
//	Animations are instantiated and immediately put in the mAllMotions map for their entire lifetime.
//	If the animations depend on any asset data, the appropriate data is fetched from the data server,
//	and the animation is put on the mLoadingMotions list.
//	Once an animations is loaded, it will be initialized and put on the mLoadedMotions list.
//	Any animation that is currently playing also sits in the mActiveMotions list.

	typedef std::map<LLUUID, LLMotion*> motion_map_t;
	motion_map_t	mAllMotions;

	motion_set_t		mLoadingMotions;
	motion_set_t		mLoadedMotions;
	motion_list_t		mActiveMotions;
	motion_set_t		mDeprecatedMotions;
	
	LLFrameTimer		mTimer;
	F32					mPrevTimerElapsed;
	F32					mAnimTime;
	F32					mLastTime;
	bool				mHasRunOnce;
	bool				mPaused;
	S32					mPausedFrame;
	F32					mTimeStep;
	S32					mTimeStepCount;
	F32					mLastInterp;

	U8					mJointSignature[2][LL_CHARACTER_MAX_ANIMATED_JOINTS];
private:
	U32					mLastCountAfterPurge; //for logging and debugging purposes
};

//-----------------------------------------------------------------------------
// Class declaractions
//-----------------------------------------------------------------------------
#include "llcharacter.h"

#endif // LL_LLMOTIONCONTROLLER_H

