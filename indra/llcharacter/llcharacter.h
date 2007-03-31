/** 
 * @file llcharacter.h
 * @brief Implementation of LLCharacter class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCHARACTER_H
#define LL_LLCHARACTER_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include <string>

#include "lljoint.h"
#include "llmotioncontroller.h"
#include "llassoclist.h"
#include "llvisualparam.h"
#include "linked_lists.h"
#include "string_table.h"
#include "llmemory.h"
#include "llthread.h"

class LLPolyMesh;

class LLPauseRequestHandle : public LLThreadSafeRefCount
{
public:
	LLPauseRequestHandle() {};
};

typedef LLPointer<LLPauseRequestHandle> LLAnimPauseRequest;

//-----------------------------------------------------------------------------
// class LLCharacter
//-----------------------------------------------------------------------------
class LLCharacter
{
public:
	// Constructor
	LLCharacter();

	// Destructor
	virtual ~LLCharacter();

	//-------------------------------------------------------------------------
	// LLCharacter Interface
	// These functions must be implemented by subclasses.
	//-------------------------------------------------------------------------

	// get the prefix to be used to lookup motion data files
	// from the viewer data directory
	virtual const char *getAnimationPrefix() = 0;

	// get the root joint of the character
	virtual LLJoint *getRootJoint() = 0;

	// get the specified joint
	// default implementation does recursive search,
	// subclasses may optimize/cache results.
	virtual LLJoint *getJoint( const std::string &name );

	// get the position of the character
	virtual LLVector3 getCharacterPosition() = 0;

	// get the rotation of the character
	virtual LLQuaternion getCharacterRotation() = 0;

	// get the velocity of the character
	virtual LLVector3 getCharacterVelocity() = 0;

	// get the angular velocity of the character
	virtual LLVector3 getCharacterAngularVelocity() = 0;

	// get the height & normal of the ground under a point
	virtual void getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm) = 0;

	// allocate an array of joints for the character skeleton
	// this must be overloaded to support joint subclasses,
	// and is called implicitly from buildSkeleton().
	// Note this must handle reallocation as it will be called
	// each time buildSkeleton() is called.
	virtual BOOL allocateCharacterJoints( U32 num ) = 0;

	// skeleton joint accessor to support joint subclasses
	virtual LLJoint *getCharacterJoint( U32 i ) = 0;

	// get the physics time dilation for the simulator
	virtual F32 getTimeDilation() = 0;

	// gets current pixel area of this character
	virtual F32 getPixelArea() const = 0;

	// gets the head mesh of the character
	virtual LLPolyMesh*	getHeadMesh() = 0;

	// gets the upper body mesh of the character
	virtual LLPolyMesh*	getUpperBodyMesh() = 0;

	// gets global coordinates from agent local coordinates
	virtual LLVector3d	getPosGlobalFromAgent(const LLVector3 &position) = 0;

	// gets agent local coordinates from global coordinates
	virtual LLVector3	getPosAgentFromGlobal(const LLVector3d &position) = 0;

	// updates all visual parameters for this character
	virtual void updateVisualParams();

	virtual void addDebugText( const std::string& text ) = 0;

	virtual const LLUUID&	getID() = 0;
	//-------------------------------------------------------------------------
	// End Interface
	//-------------------------------------------------------------------------
	// registers a motion with the character
	// returns true if successfull
	BOOL addMotion( const LLUUID& id, LLMotionConstructor create );

	void removeMotion( const LLUUID& id );

	// returns an instance of a registered motion
	LLMotion* createMotion( const LLUUID &id );

	// start a motion
	// returns true if successful, false if an error occurred
	virtual BOOL startMotion( const LLUUID& id, F32 start_offset = 0.f);

	// stop a motion
	virtual BOOL stopMotion( const LLUUID& id, BOOL stop_immediate = FALSE );

	// is this motion active?
	BOOL isMotionActive( const LLUUID& id );

	// Event handler for motion deactivation.
	// Called when a motion has completely stopped and has been deactivated.
	// Subclasses may optionally override this.
	// The default implementation does nothing.
	virtual void requestStopMotion( LLMotion* motion );
	
	// periodic update function, steps the motion controller
	void updateMotion(BOOL force_update = FALSE);

	LLAnimPauseRequest requestPause();
	BOOL areAnimationsPaused() { return mMotionController.isPaused(); }
	void setAnimTimeFactor(F32 factor) { mMotionController.setTimeFactor(factor); }
	void setTimeStep(F32 time_step) { mMotionController.setTimeStep(time_step); }
	// Releases all motion instances which should result in
	// no cached references to character joint data.  This is 
	// useful if a character wants to rebuild it's skeleton.
	virtual void flushAllMotions();

	// dumps information for debugging
	virtual void dumpCharacter( LLJoint *joint = NULL );

	virtual F32 getPreferredPelvisHeight() { return mPreferredPelvisHeight; }

	virtual LLVector3 getVolumePos(S32 joint_index, LLVector3& volume_offset) { return LLVector3::zero; }
	
	virtual LLJoint* findCollisionVolume(U32 volume_id) { return NULL; }

	virtual S32 getCollisionVolumeID(std::string &name) { return -1; }

	void setAnimationData(std::string name, void *data);
	
	void *getAnimationData(std::string name);

	void removeAnimationData(std::string name);
	
	void addVisualParam(LLVisualParam *param);
	void addSharedVisualParam(LLVisualParam *param);

	BOOL setVisualParamWeight(LLVisualParam *which_param, F32 weight, BOOL set_by_user = FALSE );
	BOOL setVisualParamWeight(const char* param_name, F32 weight, BOOL set_by_user = FALSE );
	BOOL setVisualParamWeight(S32 index, F32 weight, BOOL set_by_user = FALSE );

	// get visual param weight by param or name
	F32 getVisualParamWeight(LLVisualParam *distortion);
	F32 getVisualParamWeight(const char* param_name);
	F32 getVisualParamWeight(S32 index);

	// set all morph weights to 0
	void clearVisualParamWeights();

	// visual parameter accessors
	LLVisualParam*	getFirstVisualParam()
	{
		mCurIterator = mVisualParamIndexMap.begin();
		return getNextVisualParam();
	}
	LLVisualParam*	getNextVisualParam()
	{
		if (mCurIterator == mVisualParamIndexMap.end())
			return 0;
		return (mCurIterator++)->second;
	}

	LLVisualParam*	getVisualParam(S32 id)
	{
		VisualParamIndexMap_t::iterator iter = mVisualParamIndexMap.find(id);
		return (iter == mVisualParamIndexMap.end()) ? 0 : iter->second;
	}
	S32 getVisualParamID(LLVisualParam *id)
	{
		VisualParamIndexMap_t::iterator iter;
		for (iter = mVisualParamIndexMap.begin(); iter != mVisualParamIndexMap.end(); iter++)
		{
			if (iter->second == id)
				return iter->first;
		}
		return 0;
	}
	S32				getVisualParamCount() { return (S32)mVisualParamIndexMap.size(); }
	LLVisualParam*	getVisualParam(const char *name);


	ESex getSex()				{ return mSex; }
	void setSex( ESex sex )		{ mSex = sex; }

	U32				getAppearanceSerialNum() const		{ return mAppearanceSerialNum; }
	void			setAppearanceSerialNum( U32 num )	{ mAppearanceSerialNum = num; }
	
	U32				getSkeletonSerialNum() const		{ return mSkeletonSerialNum; }
	void			setSkeletonSerialNum( U32 num )	{ mSkeletonSerialNum = num; }

	static std::vector< LLCharacter* > sInstances;

protected:
	LLMotionController	mMotionController;

	LLAssocList<std::string, void *> mAnimationData;

	F32					mPreferredPelvisHeight;
	ESex				mSex;
	U32					mAppearanceSerialNum;
	U32					mSkeletonSerialNum;
	LLAnimPauseRequest	mPauseRequest;


private:
	// visual parameter stuff
	typedef std::map<S32, LLVisualParam *>    VisualParamIndexMap_t;
	VisualParamIndexMap_t mVisualParamIndexMap;
	VisualParamIndexMap_t::iterator mCurIterator;
	typedef std::map<char *, LLVisualParam *> VisualParamNameMap_t;
	VisualParamNameMap_t  mVisualParamNameMap;

	static LLStringTable sVisualParamNames;	
};

#endif // LL_LLCHARACTER_H

