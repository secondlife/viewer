/** 
 * @file llregionposition.h
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
