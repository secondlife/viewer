/** 
 * @file llvopartgroup.h
 * @brief Group of particle systems
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOPARTGROUP_H
#define LL_LLVOPARTGROUP_H

#include "llviewerobject.h"
#include "v3math.h"
#include "v3color.h"
#include "llframetimer.h"

class LLViewerPartGroup;

class LLVOPartGroup : public LLViewerObject
{
public:
	LLVOPartGroup(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	~LLVOPartGroup();

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent);
	/*virtual*/ void updateTextures(LLAgent &agent);

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);

	void setViewerPartGroup(LLViewerPartGroup *part_groupp)		{ mViewerPartGroupp = part_groupp; }
protected:
	LLViewerPartGroup *mViewerPartGroupp;

	LLColor4 mDebugColor;
};

#endif // LL_LLVOPARTGROUP_H
