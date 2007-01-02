/** 
 * @file llregionposition.h
 * @brief Region position storing class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLREGIONPOSITION_H
#define LL_LLREGIONPOSITION_H

/**
 * This class maintains a region, offset pair to store position, so when our "global"
 * coordinate frame shifts, all calculations are still correct.
 */

#include "v3math.h"
#include "v3dmath.h"

class LLViewerRegion;

class LLRegionPosition
{
private:
	LLViewerRegion *mRegionp;
public:
	LLVector3		mPositionRegion;
	LLRegionPosition();
	LLRegionPosition(LLViewerRegion *regionp, const LLVector3 &position_local);
	LLRegionPosition(const LLVector3d &global_position); // From global coords ONLY!

	LLViewerRegion*		getRegion() const;
	void				setPositionGlobal(const LLVector3d& global_pos);
	LLVector3d			getPositionGlobal() const;
	const LLVector3&	getPositionRegion() const;
	const LLVector3		getPositionAgent() const;


	void clear() { mRegionp = NULL; mPositionRegion.clearVec(); }
//	LLRegionPosition operator+(const LLRegionPosition &pos) const;
};

#endif // LL_REGION_POSITION_H
