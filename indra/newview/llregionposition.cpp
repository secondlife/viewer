/** 
 * @file llregionposition.cpp
 * @brief Region position storing class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llregionposition.h"

#include "llagent.h"
#include "llworld.h"
#include "llviewerregion.h"

LLRegionPosition::LLRegionPosition()
{
	mRegionp = NULL;
}

LLRegionPosition::LLRegionPosition(LLViewerRegion *regionp, const LLVector3 &position)
{
	mRegionp = regionp;
	mPositionRegion = position;
}

LLRegionPosition::LLRegionPosition(const LLVector3d &global_position)
{
	setPositionGlobal(global_position);
}

LLViewerRegion *LLRegionPosition::getRegion() const
{
	return mRegionp;
}

const LLVector3 &LLRegionPosition::getPositionRegion() const
{
	return mPositionRegion;
}

const LLVector3 LLRegionPosition::getPositionAgent() const
{
	return mRegionp->getPosAgentFromRegion( mPositionRegion );
}

LLVector3d LLRegionPosition::getPositionGlobal() const
{
	if (mRegionp)
	{
		return mRegionp->getPosGlobalFromRegion(mPositionRegion);
	}
	else
	{
		LLVector3d pos_global;
		pos_global.setVec(mPositionRegion);
		return pos_global;
	}
}


void LLRegionPosition::setPositionGlobal(const LLVector3d& position_global )
{
	mRegionp = gWorldPointer->getRegionFromPosGlobal(position_global);
	if (mRegionp)
	{
		mPositionRegion = mRegionp->getPosRegionFromGlobal(position_global);
	}
	else
	{
		mRegionp = gAgent.getRegion();
		llassert(mRegionp);
		mPositionRegion = mRegionp->getPosRegionFromGlobal(position_global);
	}
}
