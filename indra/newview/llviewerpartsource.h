/** 
 * @file llviewerpartsource.h
 * @brief LLViewerPartSource class header file
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
	U32	getID() const { return mID; }
	LLUUID getImageUUID() const;
	void  setStart() ;

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
	LLPointer<LLViewerImage>	mImagep;

	// Particle information
	U32			mPartFlags; // Flags for the particle
	U32         mDelay ; //delay to start particles
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
	static LLPointer<LLViewerPartSourceScript> createPSS(LLViewerObject *source_objp, const LLPartSysData& particle_parameters);

	LLViewerImage *getImage() const				{ return mImagep; }
	void setImage(LLViewerImage *imagep);
	LLPartSysData				mPartSysData;

	void setTargetObject(LLViewerObject *objp);

protected:
	LLQuaternion				mRotation;			// Current rotation for particle source
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
	LLVector3d mLKGSourcePosGlobal;
};


#endif // LL_LLVIEWERPARTSOURCE_H
