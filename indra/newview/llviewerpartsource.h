/** 
 * @file llviewerpartsource.h
 * @brief LLViewerPartSource class header file
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERPARTSOURCE_H
#define LL_LLVIEWERPARTSOURCE_H

#include "llmemory.h"
#include "llpartdata.h"
#include "llquaternion.h"
#include "v3math.h"

////////////////////
//
// A particle source - subclassed to generate particles with different behaviors
//
//

class LLViewerImage;
class LLViewerObject;
class LLViewerPart;

class LLViewerPartSource : public LLRefCount
{
public:
	enum
	{
		LL_PART_SOURCE_NULL,
		LL_PART_SOURCE_SCRIPT,
		LL_PART_SOURCE_SPIRAL,
		LL_PART_SOURCE_BEAM,
		LL_PART_SOURCE_CHAT
	};

	LLViewerPartSource(const U32 type);

	virtual void update(const F32 dt); // Return FALSE if this source is dead...

	virtual void setDead();
	BOOL isDead() const				{ return mIsDead; }
	void setSuspended( BOOL state )	{ mIsSuspended = state; }
	BOOL isSuspended() const		{ return mIsSuspended; }
	U32 getType() const				{ return mType; }
	static void updatePart(LLViewerPart &part, const F32 dt);
	void setOwnerUUID(const LLUUID& owner_id) { mOwnerUUID = owner_id; }
	LLUUID getOwnerUUID() const { return mOwnerUUID; }

	LLVector3	mPosAgent; // Location of the particle source
	LLVector3	mTargetPosAgent; // Location of the target position
	LLVector3	mLastUpdatePosAgent;
	LLPointer<LLViewerObject>	mSourceObjectp;
	U32 mID;

protected:
	U32			mType;
	BOOL		mIsDead;
	BOOL		mIsSuspended;
	F32			mLastUpdateTime;
	F32			mLastPartTime;
	LLUUID		mOwnerUUID;
	
	// Particle information
	U32			mPartFlags; // Flags for the particle
};



///////////////////////////////
//
// LLViewerPartSourceScript
//
// Particle source that handles the "generic" script-drive particle source
// attached to objects
//


class LLViewerPartSourceScript : public LLViewerPartSource
{
public:
	LLViewerPartSourceScript(LLViewerObject *source_objp);
	/*virtual*/ void update(const F32 dt);

	/*virtual*/ void setDead();

	BOOL updateFromMesg();

	// Returns a new particle source to attach to an object...
	static LLPointer<LLViewerPartSourceScript> unpackPSS(LLViewerObject *source_objp, LLPointer<LLViewerPartSourceScript> pssp, const S32 block_num);
	static LLPointer<LLViewerPartSourceScript> unpackPSS(LLViewerObject *source_objp, LLPointer<LLViewerPartSourceScript> pssp, LLDataPacker &dp);

	LLViewerImage *getImage() const				{ return mImagep; }
	void setImage(LLViewerImage *imagep);
	LLPartSysData				mPartSysData;

	void setTargetObject(LLViewerObject *objp);

protected:
	LLQuaternion				mRotation;			// Current rotation for particle source
	LLPointer<LLViewerImage>	mImagep;			// Cached image pointer of the mPartSysData UUID
	LLPointer<LLViewerObject>	mTargetObjectp;		// Target object for the particle source
};


////////////////////////////
//
// Particle source for spiral effect (customize avatar, mostly)
//

class LLViewerPartSourceSpiral : public LLViewerPartSource
{
public:
	LLViewerPartSourceSpiral(const LLVector3 &pos);

	/*virtual*/ void setDead();

	/*virtual*/ void update(const F32 dt);

	void setSourceObject(LLViewerObject *objp);
	void setColor(const LLColor4 &color);

	static void updatePart(LLViewerPart &part, const F32 dt);
	LLColor4 mColor;
protected:
	LLPointer<LLViewerImage>	mImagep;
	LLVector3d mLKGSourcePosGlobal;
};


////////////////////////////
//
// Particle source for tractor(editing) beam
//

class LLViewerPartSourceBeam : public LLViewerPartSource
{
public:
	LLViewerPartSourceBeam();

	/*virtual*/ void setDead();

	/*virtual*/ void update(const F32 dt);

	void setSourceObject(LLViewerObject *objp);
	void setTargetObject(LLViewerObject *objp);
	void setSourcePosGlobal(const LLVector3d &pos_global);
	void setTargetPosGlobal(const LLVector3d &pos_global);
	void setColor(const LLColor4 &color);

	static void updatePart(LLViewerPart &part, const F32 dt);
	LLPointer<LLViewerImage>	mImagep;
	LLPointer<LLViewerObject>	mTargetObjectp;
	LLVector3d		mLKGTargetPosGlobal;
	LLColor4 mColor;
protected:
	~LLViewerPartSourceBeam();
};



//////////////////////////
//
// Particle source for chat effect
//

class LLViewerPartSourceChat : public LLViewerPartSource
{
public:
	LLViewerPartSourceChat(const LLVector3 &pos);

	/*virtual*/ void setDead();

	/*virtual*/ void update(const F32 dt);

	void setSourceObject(LLViewerObject *objp);
	void setColor(const LLColor4 &color);

	static void updatePart(LLViewerPart &part, const F32 dt);
	LLColor4 mColor;
protected:
	LLPointer<LLViewerImage>	mImagep;
	LLVector3d mLKGSourcePosGlobal;
};


#endif // LL_LLVIEWERPARTSOURCE_H
