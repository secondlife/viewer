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
	mA(MIN_WALKABILITY_VALUE),
	mB(MIN_WALKABILITY_VALUE),
	mC(MIN_WALKABILITY_VALUE),
	mD(MIN_WALKABILITY_VALUE)
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
		mA = llround(pNavMeshItem.get("A").asReal() * 100.0f);

		llassert(pNavMeshItem.has("B"));
		llassert(pNavMeshItem.get("B").isReal());
		mB = llround(pNavMeshItem.get("B").asReal() * 100.0f);

		llassert(pNavMeshItem.has("C"));
		llassert(pNavMeshItem.get("C").isReal());
		mC = llround(pNavMeshItem.get("C").asReal() * 100.0f);

		llassert(pNavMeshItem.has("D"));
		llassert(pNavMeshItem.get("D").isReal());
		mD = llround(pNavMeshItem.get("D").asReal() * 100.0f);
	}
	else
	{
		// New server-side storage will be integer
		llassert(pNavMeshItem.get("A").isInteger());
		mA = pNavMeshItem.get("A").asInteger();
		llassert(mA >= MIN_WALKABILITY_VALUE);
		llassert(mA <= MAX_WALKABILITY_VALUE);

		llassert(pNavMeshItem.has("B"));
		llassert(pNavMeshItem.get("B").isInteger());
		mB = pNavMeshItem.get("B").asInteger();
		llassert(mB >= MIN_WALKABILITY_VALUE);
		llassert(mB <= MAX_WALKABILITY_VALUE);

		llassert(pNavMeshItem.has("C"));
		llassert(pNavMeshItem.get("C").isInteger());
		mC = pNavMeshItem.get("C").asInteger();
		llassert(mC >= MIN_WALKABILITY_VALUE);
		llassert(mC <= MAX_WALKABILITY_VALUE);

		llassert(pNavMeshItem.has("D"));
		llassert(pNavMeshItem.get("D").isInteger());
		mD = pNavMeshItem.get("D").asInteger();
		llassert(mD >= MIN_WALKABILITY_VALUE);
		llassert(mD <= MAX_WALKABILITY_VALUE);
	}
#else // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	llassert(pNavMeshItem.get("A").isInteger());
	mA = pNavMeshItem.get("A").asInteger();
	llassert(mA >= MIN_WALKABILITY_VALUE);
	llassert(mA <= MAX_WALKABILITY_VALUE);

	llassert(pNavMeshItem.has("B"));
	llassert(pNavMeshItem.get("B").isInteger());
	mB = pNavMeshItem.get("B").asInteger();
	llassert(mB >= MIN_WALKABILITY_VALUE);
	llassert(mB <= MAX_WALKABILITY_VALUE);

	llassert(pNavMeshItem.has("C"));
	llassert(pNavMeshItem.get("C").isInteger());
	mC = pNavMeshItem.get("C").asInteger();
	llassert(mC >= MIN_WALKABILITY_VALUE);
	llassert(mC <= MAX_WALKABILITY_VALUE);

	llassert(pNavMeshItem.has("D"));
	llassert(pNavMeshItem.get("D").isInteger());
	mD = pNavMeshItem.get("D").asInteger();
	llassert(mD >= MIN_WALKABILITY_VALUE);
	llassert(mD <= MAX_WALKABILITY_VALUE);
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
	mA(pOther.mA),
	mB(pOther.mB),
	mC(pOther.mC),
	mD(pOther.mD)
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
	mA = pOther.mA;
	mB = pOther.mB;
	mC = pOther.mC;
	mD = pOther.mD;

	return *this;
}

const LLUUID& LLPathfindingLinkset::getUUID() const
{
	return mUUID;
}

const std::string& LLPathfindingLinkset::getName() const
{
	return mName;
}

const std::string& LLPathfindingLinkset::getDescription() const
{
	return mDescription;
}

U32 LLPathfindingLinkset::getLandImpact() const
{
	return mLandImpact;
}

const LLVector3& LLPathfindingLinkset::getPositionAgent() const
{
	return mLocation;
}

LLPathfindingLinkset::EPathState LLPathfindingLinkset::getPathState() const
{
	return mPathState;
}

void LLPathfindingLinkset::setPathState(EPathState pPathState)
{
	mPathState = pPathState;
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

BOOL LLPathfindingLinkset::isPhantom() const
{
	return mIsPhantom;
}

void LLPathfindingLinkset::setPhantom(BOOL pIsPhantom)
{
	mIsPhantom = pIsPhantom;
}

S32 LLPathfindingLinkset::getA() const
{
	return mA;
}

void LLPathfindingLinkset::setA(S32 pA)
{
	mA = pA;
}

S32 LLPathfindingLinkset::getB() const
{
	return mB;
}

void LLPathfindingLinkset::setB(S32 pB)
{
	mB = pB;
}

S32 LLPathfindingLinkset::getC() const
{
	return mC;
}

void LLPathfindingLinkset::setC(S32 pC)
{
	mC = pC;
}

S32 LLPathfindingLinkset::getD() const
{
	return mD;
}

void LLPathfindingLinkset::setD(S32 pD)
{
	mD = pD;
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
		if (mA != pA)
		{
			itemData["A"] = llclamp(static_cast<F32>(pA) / 100.0f, 0.0f, 1.0f);
		}
		if (mB != pB)
		{
			itemData["B"] = llclamp(static_cast<F32>(pB) / 100.0f, 0.0f, 1.0f);
		}
		if (mC != pC)
		{
			itemData["C"] = llclamp(static_cast<F32>(pC) / 100.0f, 0.0f, 1.0f);
		}
		if (mD != pD)
		{
			itemData["D"] = llclamp(static_cast<F32>(pD) / 100.0f, 0.0f, 1.0f);
		}
	}
	else
	{
		if (mA != pA)
		{
			itemData["A"] = llclamp(pA, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
		if (mB != pB)
		{
			itemData["B"] = llclamp(pB, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
		if (mC != pC)
		{
			itemData["C"] = llclamp(pC, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
		if (mD != pD)
		{
			itemData["D"] = llclamp(pD, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
		}
	}
#else // XXX_STINSON_WALKABILITY_COEFFICIENTS_TYPE_CHANGE
	if (mA != pA)
	{
		itemData["A"] = llclamp(pA, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}
	if (mB != pB)
	{
		itemData["B"] = llclamp(pB, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}
	if (mC != pC)
	{
		itemData["C"] = llclamp(pC, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}
	if (mD != pD)
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
