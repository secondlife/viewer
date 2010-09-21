/** 
 * @file llregionposition.cpp
 * @brief Region position storing class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
	mRegionp = LLWorld::getInstance()->getRegionFromPosGlobal(position_global);
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
