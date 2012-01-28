/** 
 * @file llpathfindinglinksets.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llpathfindinglinksets.h"
#include "llsd.h"
#include "v3math.h"
#include "lluuid.h"

//---------------------------------------------------------------------------
// LLPathfindingLinkset
//---------------------------------------------------------------------------

const S32 LLPathfindingLinkset::MIN_WALKABILITY_VALUE(0);
const S32 LLPathfindingLinkset::MAX_WALKABILITY_VALUE(100);

LLPathfindingLinkset::LLPathfindingLinkset(const std::string &pUUID, const LLSD& pNavMeshItem)
	: mUUID(pUUID),
	mName(),
	mDescription(),
	mLandImpact(0U),
	mLocation(),
	mPathState(kIgnored),
	mIsPhantom(false),
#ifdef XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	mIsWalkabilityCoefficientsF32(false),
#endif // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	mWalkabilityCoefficientA(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientB(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientC(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientD(MIN_WALKABILITY_VALUE)
{
	llassert(pNavMeshItem.has("name"));
	llassert(pNavMeshItem.get("name").isString());
	mName = pNavMeshItem.get("name").asString();

	llassert(pNavMeshItem.has("description"));
	llassert(pNavMeshItem.get("description").isString());
	mDescription = pNavMeshItem.get("description").asString();

	llassert(pNavMeshItem.has("landimpact"));
	llassert(pNavMeshItem.get("landimpact").isInteger());
	llassert(pNavMeshItem.get("landimpact").asInteger() >= 0);
	mLandImpact = pNavMeshItem.get("landimpact").asInteger();

	llassert(pNavMeshItem.has("permanent"));
	llassert(pNavMeshItem.get("permanent").isBoolean());
	bool isPermanent = pNavMeshItem.get("permanent").asBoolean();

	llassert(pNavMeshItem.has("walkable"));
	llassert(pNavMeshItem.get("walkable").isBoolean());
	bool isWalkable = pNavMeshItem.get("walkable").asBoolean();

	mPathState = getPathState(isPermanent, isWalkable);

	llassert(pNavMeshItem.has("phantom"));
	llassert(pNavMeshItem.get("phantom").isBoolean());
	mIsPhantom = pNavMeshItem.get("phantom").asBoolean();

	llassert(pNavMeshItem.has("A"));
#ifdef XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	mIsWalkabilityCoefficientsF32 = pNavMeshItem.get("A").isReal();
	if (mIsWalkabilityCoefficientsF32)
	{
		// Old server-side storage was real
		mWalkabilityCoefficientA = llround(pNavMeshItem.get("A").asReal() * 100.0f);

		llassert(pNavMeshItem.has("B"));
		llassert(pNavMeshItem.get("B").isReal());
		mWalkabilityCoefficientB = llround(pNavMeshItem.get("B").asReal() * 100.0f);

		llassert(pNavMeshItem.has("C"));
		llassert(pNavMeshItem.get("C").isReal());
		mWalkabilityCoefficientC = llround(pNavMeshItem.get("C").asReal() * 100.0f);

		llassert(pNavMeshItem.has("D"));
		llassert(pNavMeshItem.get("D").isReal());
		mWalkabilityCoefficientD = llround(pNavMeshItem.get("D").asReal() * 100.0f);
	}
	else
	{
		// New server-side storage will be integer
		llassert(pNavMeshItem.get("A").isInteger());
		mWalkabilityCoefficientA = pNavMeshItem.get("A").asInteger();
		llassert(mWalkabilityCoefficientA >= MIN_WALKABILITY_VALUE);
		llassert(mWalkabilityCoefficientA <= MAX_WALKABILITY_VALUE);

		llassert(pNavMeshItem.has("B"));
		llassert(pNavMeshItem.get("B").isInteger());
		mWalkabilityCoefficientB = pNavMeshItem.get("B").asInteger();
		llassert(mWalkabilityCoefficientB >= MIN_WALKABILITY_VALUE);
		llassert(mWalkabilityCoefficientB <= MAX_WALKABILITY_VALUE);

		llassert(pNavMeshItem.has("C"));
		llassert(pNavMeshItem.get("C").isInteger());
		mWalkabilityCoefficientC = pNavMeshItem.get("C").asInteger();
		llassert(mWalkabilityCoefficientC >= MIN_WALKABILITY_VALUE);
		llassert(mWalkabilityCoefficientC <= MAX_WALKABILITY_VALUE);

		llassert(pNavMeshItem.has("D"));
		llassert(pNavMeshItem.get("D").isInteger());
		mWalkabilityCoefficientD = pNavMeshItem.get("D").asInteger();
		llassert(mWalkabilityCoefficientD >= MIN_WALKABILITY_VALUE);
		llassert(mWalkabilityCoefficientD <= MAX_WALKABILITY_VALUE);
	}
#else // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	llassert(pNavMeshItem.get("A").isInteger());
	mWalkabilityCoefficientA = pNavMeshItem.get("A").asInteger();
	llassert(mWalkabilityCoefficientA >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientA <= MAX_WALKABILITY_VALUE);

	llassert(pNavMeshItem.has("B"));
	llassert(pNavMeshItem.get("B").isInteger());
	mWalkabilityCoefficientB = pNavMeshItem.get("B").asInteger();
	llassert(mWalkabilityCoefficientB >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientB <= MAX_WALKABILITY_VALUE);

	llassert(pNavMeshItem.has("C"));
	llassert(pNavMeshItem.get("C").isInteger());
	mWalkabilityCoefficientC = pNavMeshItem.get("C").asInteger();
	llassert(mWalkabilityCoefficientC >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientC <= MAX_WALKABILITY_VALUE);

	llassert(pNavMeshItem.has("D"));
	llassert(pNavMeshItem.get("D").isInteger());
	mWalkabilityCoefficientD = pNavMeshItem.get("D").asInteger();
	llassert(mWalkabilityCoefficientD >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientD <= MAX_WALKABILITY_VALUE);
#endif // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE

	llassert(pNavMeshItem.has("position"));
	llassert(pNavMeshItem.get("position").isArray());
	mLocation.setValue(pNavMeshItem.get("position"));
}

LLPathfindingLinkset::LLPathfindingLinkset(const LLPathfindingLinkset& pOther)
	: mUUID(pOther.mUUID),
	mName(pOther.mName),
	mDescription(pOther.mDescription),
	mLandImpact(pOther.mLandImpact),
	mLocation(pOther.mLocation),
	mPathState(pOther.mPathState),
	mIsPhantom(pOther.mIsPhantom),
#ifdef XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	mIsWalkabilityCoefficientsF32(pOther.mIsWalkabilityCoefficientsF32),
#endif // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	mWalkabilityCoefficientA(pOther.mWalkabilityCoefficientA),
	mWalkabilityCoefficientB(pOther.mWalkabilityCoefficientB),
	mWalkabilityCoefficientC(pOther.mWalkabilityCoefficientC),
	mWalkabilityCoefficientD(pOther.mWalkabilityCoefficientD)
{
}

LLPathfindingLinkset::~LLPathfindingLinkset()
{
}

LLPathfindingLinkset& LLPathfindingLinkset::operator =(const LLPathfindingLinkset& pOther)
{
	mUUID = pOther.mUUID;
	mName = pOther.mName;
	mDescription = pOther.mDescription;
	mLandImpact = pOther.mLandImpact;
	mLocation = pOther.mLocation;
	mPathState = pOther.mPathState;
	mIsPhantom = pOther.mIsPhantom;
#ifdef XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	mIsWalkabilityCoefficientsF32 = pOther.mIsWalkabilityCoefficientsF32;
#endif // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	mWalkabilityCoefficientA = pOther.mWalkabilityCoefficientA;
	mWalkabilityCoefficientB = pOther.mWalkabilityCoefficientB;
	mWalkabilityCoefficientC = pOther.mWalkabilityCoefficientC;
	mWalkabilityCoefficientD = pOther.mWalkabilityCoefficientD;

	return *this;
}


LLPathfindingLinkset::EPathState LLPathfindingLinkset::getPathState(bool pIsPermanent, bool pIsWalkable)
{
	return (pIsPermanent ? (pIsWalkable ? kWalkable : kObstacle) : kIgnored);
}

BOOL LLPathfindingLinkset::isPermanent(EPathState pPathState)
{
	BOOL retVal;

	switch (pPathState)
	{
	case kWalkable :
	case kObstacle :
		retVal = true;
		break;
	case kIgnored :
		retVal = false;
		break;
	default :
		retVal = false;
		llassert(0);
		break;
	}

	return retVal;
}

BOOL LLPathfindingLinkset::isWalkable(EPathState pPathState)
{
	BOOL retVal;

	switch (pPathState)
	{
	case kWalkable :
		retVal = true;
		break;
	case kObstacle :
	case kIgnored :
		retVal = false;
		break;
	default :
		retVal = false;
		llassert(0);
		break;
	}

	return retVal;
}

void LLPathfindingLinkset::setWalkabilityCoefficientA(S32 pA)
{
	mWalkabilityCoefficientA = llclamp(pA, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
}

void LLPathfindingLinkset::setWalkabilityCoefficientB(S32 pB)
{
	mWalkabilityCoefficientB = llclamp(pB, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
}

void LLPathfindingLinkset::setWalkabilityCoefficientC(S32 pC)
{
	mWalkabilityCoefficientC = llclamp(pC, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
}

void LLPathfindingLinkset::setWalkabilityCoefficientD(S32 pD)
{
	mWalkabilityCoefficientD = llclamp(pD, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
}

LLSD LLPathfindingLinkset::getAlteredFields(EPathState pPathState, S32 pA, S32 pB, S32 pC, S32 pD, BOOL pIsPhantom) const
{
	LLSD itemData;

	if (mPathState != pPathState)
	{
		itemData["permanent"] = static_cast<bool>(LLPathfindingLinkset::isPermanent(pPathState));
		itemData["walkable"] = static_cast<bool>(LLPathfindingLinkset::isWalkable(pPathState));
	}
#ifdef XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	if (mIsWalkabilityCoefficientsF32)
	{
		if (mWalkabilityCoefficientA != pA)
		{
			itemData["A"] = llclamp(static_cast<F32>(pA) / 100.0f, 0.0f, 1.0f);
		}
		if (mWalkabilityCoefficientB != pB)
		{
			itemData["B"] = llclamp(static_cast<F32>(pB) / 100.0f, 0.0f, 1.0f);
		}
		if (mWalkabilityCoefficientC != pC)
		{
			itemData["C"] = llclamp(static_cast<F32>(pC) / 100.0f, 0.0f, 1.0f);
		}
		if (mWalkabilityCoefficientD != pD)
		{
			itemData["D"] = llclamp(static_cast<F32>(pD) / 100.0f, 0.0f, 1.0f);
		}
	}
	else
	{
		if (mWalkabilityCoefficientA != pA)
		{
			itemData["A"] = llclamp(pA, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
		if (mWalkabilityCoefficientB != pB)
		{
			itemData["B"] = llclamp(pB, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
		if (mWalkabilityCoefficientC != pC)
		{
			itemData["C"] = llclamp(pC, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
		if (mWalkabilityCoefficientD != pD)
		{
			itemData["D"] = llclamp(pD, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
	}
#else // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	if (mWalkabilityCoefficientA != pA)
	{
		itemData["A"] = llclamp(pA, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}
	if (mWalkabilityCoefficientB != pB)
	{
		itemData["B"] = llclamp(pB, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}
	if (mWalkabilityCoefficientC != pC)
	{
		itemData["C"] = llclamp(pC, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}
	if (mWalkabilityCoefficientD != pD)
	{
		itemData["D"] = llclamp(pD, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}
#endif // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	if (mIsPhantom != pIsPhantom)
	{
		itemData["phantom"] = static_cast<bool>(pIsPhantom);
	}

	return itemData;
}
