/** 
 * @file llviewerpartsim.h
 * @brief LLViewerPart class header file
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERPARTSIM_H
#define LL_LLVIEWERPARTSIM_H

#include "lldarrayptr.h"
#include "llskiplist.h"
#include "llframetimer.h"
#include "llmemory.h"

#include "llpartdata.h"

class LLViewerImage;
class LLViewerPart;
class LLViewerPartSource;
class LLViewerRegion;
class LLViewerImage;
class LLVOPartGroup;

typedef void (*LLVPCallback)(LLViewerPart &part, const F32 dt);

///////////////////
//
// An individual particle
//


class LLViewerPart : public LLPartData, public LLRefCount
{
protected:
	~LLViewerPart();
public:
	LLViewerPart();

	LLViewerPart &operator=(const LLViewerPart &part);
	void init(LLPointer<LLViewerPartSource> sourcep, LLViewerImage *imagep, LLVPCallback cb);


	U32					mPartID;					// Particle ID used primarily for moving between groups
	F32					mLastUpdateTime;			// Last time the particle was updated


	LLVPCallback		mVPCallback;				// Callback function for more complicated behaviors
	LLPointer<LLViewerPartSource> mPartSourcep;		// Particle source used for this object
	

	// Current particle state (possibly used for rendering)
	LLPointer<LLViewerImage>	mImagep;
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
					  const F32 box_radius);
	virtual ~LLViewerPartGroup();

	void cleanup();

	BOOL addPart(LLViewerPart* part, const F32 desired_size = -1.f);
	
	void updateParticles(const F32 dt);

	BOOL posInGroup(const LLVector3 &pos, const F32 desired_size = -1.f);

	void shift(const LLVector3 &offset);

	typedef std::vector<LLPointer<LLViewerPart> > part_list_t;
	part_list_t mParticles;

	const LLVector3 &getCenterAgent() const		{ return mCenterAgent; }
	S32 getCount() const					{ return (S32) mParticles.size(); }
	LLViewerRegion *getRegion() const		{ return mRegionp; }

	LLPointer<LLVOPartGroup> mVOPartGroupp;

	BOOL mUniformParticles;
	U32 mID;

protected:
	void removePart(const S32 part_num);

protected:
	LLVector3 mCenterAgent;
	F32 mBoxRadius;
	LLVector3 mMinObjPos;
	LLVector3 mMaxObjPos;

	LLViewerRegion *mRegionp;
};

class LLViewerPartSim
{
public:
	LLViewerPartSim();
	virtual ~LLViewerPartSim();

	void shift(const LLVector3 &offset);

	void updateSimulation();

	void addPartSource(LLPointer<LLViewerPartSource> sourcep);

	void cleanupRegion(LLViewerRegion *regionp);

	BOOL shouldAddPart(); // Just decides whether this particle should be added or not (for particle count capping)
	void addPart(LLViewerPart* part);
	void cleanMutedParticles(const LLUUID& task_id);

	friend class LLViewerPartGroup;

	BOOL aboveParticleLimit() const { return sParticleCount > sMaxParticleCount; }

	static void setMaxPartCount(const S32 max_parts)	{ sMaxParticleCount = max_parts; }
	static S32  getMaxPartCount()						{ return sMaxParticleCount; }
	static void incPartCount(const S32 count)			{ sParticleCount += count; }
	static void decPartCount(const S32 count)			{ sParticleCount -= count; }
	
	U32 mID;

protected:
	LLViewerPartGroup *createViewerPartGroup(const LLVector3 &pos_agent, const F32 desired_size);
	LLViewerPartGroup *put(LLViewerPart* part);

protected:
	typedef std::vector<LLViewerPartGroup *> group_list_t;
	typedef std::vector<LLPointer<LLViewerPartSource> > source_list_t;
	group_list_t mViewerPartGroups;
	source_list_t mViewerPartSources;
	LLFrameTimer mSimulationTimer;
	static S32 sMaxParticleCount;
	static S32 sParticleCount;
};

#endif // LL_LLVIEWERPARTSIM_H
