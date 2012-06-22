/** 
 * @file llviewerpartsim.h
 * @brief LLViewerPart class header file
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLVIEWERPARTSIM_H
#define LL_LLVIEWERPARTSIM_H

#include "lldarrayptr.h"
#include "llframetimer.h"
#include "llpointer.h"
#include "llpartdata.h"
#include "llviewerpartsource.h"

class LLViewerTexture;
class LLViewerPart;
class LLViewerRegion;
class LLViewerTexture;
class LLVOPartGroup;

#define LL_MAX_PARTICLE_COUNT 8192

typedef void (*LLVPCallback)(LLViewerPart &part, const F32 dt);

///////////////////
//
// An individual particle
//


class LLViewerPart : public LLPartData
{
public:
	~LLViewerPart();
public:
	LLViewerPart();

	void init(LLPointer<LLViewerPartSource> sourcep, LLViewerTexture *imagep, LLVPCallback cb);


	U32					mPartID;					// Particle ID used primarily for moving between groups
	F32					mLastUpdateTime;			// Last time the particle was updated
	F32					mSkipOffset;				// Offset against current group mSkippedTime

	LLVPCallback		mVPCallback;				// Callback function for more complicated behaviors
	LLPointer<LLViewerPartSource> mPartSourcep;		// Particle source used for this object
	

	// Current particle state (possibly used for rendering)
	LLPointer<LLViewerTexture>	mImagep;
	LLVector3		mPosAgent;
	LLVector3		mVelocity;
	LLVector3		mAccel;
	LLColor4		mColor;
	LLVector2		mScale;

	static U32		sNextPartID;
};



class LLViewerPartGroup
{
public:
	LLViewerPartGroup(const LLVector3 &center,
					  const F32 box_radius,
					  bool hud);
	virtual ~LLViewerPartGroup();

	void cleanup();

	BOOL addPart(LLViewerPart* part, const F32 desired_size = -1.f);
	
	void updateParticles(const F32 lastdt);

	BOOL posInGroup(const LLVector3 &pos, const F32 desired_size = -1.f);

	void shift(const LLVector3 &offset);

	typedef std::vector<LLViewerPart*>  part_list_t;
	part_list_t mParticles;

	const LLVector3 &getCenterAgent() const		{ return mCenterAgent; }
	S32 getCount() const					{ return (S32) mParticles.size(); }
	LLViewerRegion *getRegion() const		{ return mRegionp; }

	void removeParticlesByID(const U32 source_id);
	
	LLPointer<LLVOPartGroup> mVOPartGroupp;

	BOOL mUniformParticles;
	U32 mID;

	F32 mSkippedTime;
	bool mHud;

protected:
	LLVector3 mCenterAgent;
	F32 mBoxRadius;
	LLVector3 mMinObjPos;
	LLVector3 mMaxObjPos;

	LLViewerRegion *mRegionp;
};

class LLViewerPartSim : public LLSingleton<LLViewerPartSim>
{
public:
	LLViewerPartSim();
	virtual ~LLViewerPartSim(){}
	void destroyClass();

	typedef std::vector<LLViewerPartGroup *> group_list_t;
	typedef std::vector<LLPointer<LLViewerPartSource> > source_list_t;

	void shift(const LLVector3 &offset);

	void updateSimulation();

	void addPartSource(LLPointer<LLViewerPartSource> sourcep);

	void cleanupRegion(LLViewerRegion *regionp);

	BOOL shouldAddPart(); // Just decides whether this particle should be added or not (for particle count capping)
	F32 maxRate() // Return maximum particle generation rate
	{
		if (sParticleCount >= MAX_PART_COUNT)
		{
			return 1.f;
		}
		if (sParticleCount > PART_THROTTLE_THRESHOLD*sMaxParticleCount)
		{
			return (((F32)sParticleCount/(F32)sMaxParticleCount)-PART_THROTTLE_THRESHOLD)*PART_THROTTLE_RESCALE;
		}
		return 0.f;
	}
	F32 getRefRate() { return sParticleAdaptiveRate; }
	F32 getBurstRate() {return sParticleBurstRate; }
	void addPart(LLViewerPart* part);
	void updatePartBurstRate() ;
	void clearParticlesByID(const U32 system_id);
	void clearParticlesByOwnerID(const LLUUID& task_id);
	void removeLastCreatedSource();

	const source_list_t* getParticleSystemList() const { return &mViewerPartSources; }

	friend class LLViewerPartGroup;

	BOOL aboveParticleLimit() const { return sParticleCount > sMaxParticleCount; }

	static void setMaxPartCount(const S32 max_parts)	{ sMaxParticleCount = max_parts; }
	static S32  getMaxPartCount()						{ return sMaxParticleCount; }
	static void incPartCount(const S32 count)			{ sParticleCount += count; }
	static void decPartCount(const S32 count)			{ sParticleCount -= count; }
	
	U32 mID;

protected:
	LLViewerPartGroup *createViewerPartGroup(const LLVector3 &pos_agent, const F32 desired_size, bool hud);
	LLViewerPartGroup *put(LLViewerPart* part);

	group_list_t mViewerPartGroups;
	source_list_t mViewerPartSources;
	LLFrameTimer mSimulationTimer;

	static S32 sMaxParticleCount;
	static S32 sParticleCount;
	static F32 sParticleAdaptiveRate;
	static F32 sParticleBurstRate;

	static const S32 MAX_PART_COUNT;
	static const F32 PART_THROTTLE_THRESHOLD;
	static const F32 PART_THROTTLE_RESCALE;
	static const F32 PART_ADAPT_RATE_MULT;
	static const F32 PART_ADAPT_RATE_MULT_RECIP;

//debug use only
public:
	static S32 sParticleCount2;

	static void checkParticleCount(U32 size = 0) ;
};

#endif // LL_LLVIEWERPARTSIM_H
