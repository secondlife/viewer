/** 
 * @file llpathfindinglinksets.h
 * @author William Todd Stinson
 * @brief Definition of a pathfinding linkset that contains various properties required for havok pathfinding.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLPATHFINDINGLINKSETS_H
#define LL_LLPATHFINDINGLINKSETS_H

#include "llsd.h"
#include "v3math.h"
#include "lluuid.h"

// This is a reminder to remove the code regarding the changing of the data type for the
// walkability coefficients from F32 to S32 representing the percentage from 0-100.
#define XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE

class LLPathfindingLinkset
{
public:
	typedef enum
	{
		kWalkable,
		kObstacle,
		kIgnored
	} EPathState;

	LLPathfindingLinkset(const std::string &pUUID, const LLSD &pNavMeshItem);
	LLPathfindingLinkset(const LLPathfindingLinkset& pOther);
	virtual ~LLPathfindingLinkset();

	LLPathfindingLinkset& operator = (const LLPathfindingLinkset& pOther);

	const LLUUID&      getUUID() const;
	const std::string& getName() const;
	const std::string& getDescription() const;
	U32                getLandImpact() const;
	const LLVector3&   getPositionAgent() const;

	EPathState         getPathState() const;
	void               setPathState(EPathState pPathState);
	static EPathState  getPathState(bool pIsPermanent, bool pIsWalkable);
	static BOOL        isPermanent(EPathState pPathState);
	static BOOL        isWalkable(EPathState pPathState);

	BOOL               isPhantom() const;
	void               setPhantom(BOOL pIsPhantom);

	S32                getWalkabilityCoefficientA() const;
	void               setWalkabilityCoefficientA(S32 pA);

	S32                getWalkabilityCoefficientB() const;
	void               setWalkabilityCoefficientB(S32 pB);

	S32                getWalkabilityCoefficientC() const;
	void               setWalkabilityCoefficientC(S32 pC);

	S32                getWalkabilityCoefficientD() const;
	void               setWalkabilityCoefficientD(S32 pD);

	LLSD               getAlteredFields(EPathState pPathState, S32 pA, S32 pB, S32 pC, S32 pD, BOOL pIsPhantom) const;

protected:

private:
	static const S32 MIN_WALKABILITY_VALUE;
	static const S32 MAX_WALKABILITY_VALUE;

	LLUUID      mUUID;
	std::string mName;
	std::string mDescription;
	U32         mLandImpact;
	LLVector3   mLocation;
	EPathState  mPathState;
	BOOL        mIsPhantom;
#ifdef XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	BOOL        mIsWalkabilityCoefficientsF32;
#endif // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	S32         mWalkabilityCoefficientA;
	S32         mWalkabilityCoefficientB;
	S32         mWalkabilityCoefficientC;
	S32         mWalkabilityCoefficientD;
};

#endif // LL_LLFLOATERPATHFINDINGLINKSETS_H
