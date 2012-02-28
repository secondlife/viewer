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
#include "llpathfindinglinkset.h"
#include "llsd.h"
#include "v3math.h"
#include "lluuid.h"

#define LINKSET_NAME_FIELD          "name"
#define LINKSET_DESCRIPTION_FIELD   "description"
#define LINKSET_LAND_IMPACT_FIELD   "landimpact"
#define LINKSET_MODIFIABLE_FIELD    "modifiable"
#define LINKSET_PERMANENT_FIELD     "permanent"
#define LINKSET_WALKABLE_FIELD      "walkable"
#define LINKSET_PHANTOM_FIELD       "phantom"
#define LINKSET_WALKABILITY_A_FIELD "A"
#define LINKSET_WALKABILITY_B_FIELD "B"
#define LINKSET_WALKABILITY_C_FIELD "C"
#define LINKSET_WALKABILITY_D_FIELD "D"
#define LINKSET_POSITION_FIELD      "position"

//---------------------------------------------------------------------------
// LLPathfindingLinkset
//---------------------------------------------------------------------------

const S32 LLPathfindingLinkset::MIN_WALKABILITY_VALUE(0);
const S32 LLPathfindingLinkset::MAX_WALKABILITY_VALUE(100);

LLPathfindingLinkset::LLPathfindingLinkset(const LLSD& pTerrainLinksetItem)
	: mUUID(),
	mIsTerrain(true),
	mName(),
	mDescription(),
	mLandImpact(0U),
	mLocation(LLVector3::zero),
	mIsModifiable(TRUE),
	mLinksetUse(kUnknown),
	mWalkabilityCoefficientA(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientB(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientC(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientD(MIN_WALKABILITY_VALUE)
{
	parsePathfindingData(pTerrainLinksetItem);
}

LLPathfindingLinkset::LLPathfindingLinkset(const std::string &pUUID, const LLSD& pLinksetItem)
	: mUUID(pUUID),
	mIsTerrain(false),
	mName(),
	mDescription(),
	mLandImpact(0U),
	mLocation(LLVector3::zero),
	mIsModifiable(TRUE),
	mLinksetUse(kUnknown),
	mWalkabilityCoefficientA(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientB(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientC(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientD(MIN_WALKABILITY_VALUE)
{
	parseObjectData(pLinksetItem);
	parsePathfindingData(pLinksetItem);
}

LLPathfindingLinkset::LLPathfindingLinkset(const LLPathfindingLinkset& pOther)
	: mUUID(pOther.mUUID),
	mName(pOther.mName),
	mDescription(pOther.mDescription),
	mLandImpact(pOther.mLandImpact),
	mLocation(pOther.mLocation),
	mIsModifiable(pOther.mIsModifiable),
	mLinksetUse(pOther.mLinksetUse),
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
	mIsModifiable = pOther.mIsModifiable;
	mLinksetUse = pOther.mLinksetUse;
	mWalkabilityCoefficientA = pOther.mWalkabilityCoefficientA;
	mWalkabilityCoefficientB = pOther.mWalkabilityCoefficientB;
	mWalkabilityCoefficientC = pOther.mWalkabilityCoefficientC;
	mWalkabilityCoefficientD = pOther.mWalkabilityCoefficientD;

	return *this;
}

BOOL LLPathfindingLinkset::isPhantom() const
{
	return isPhantom(getLinksetUse());
}

BOOL LLPathfindingLinkset::isPhantom(ELinksetUse pLinksetUse)
{
	BOOL retVal;

	switch (pLinksetUse)
	{
	case kWalkable :
	case kStaticObstacle :
	case kDynamicObstacle :
		retVal = false;
		break;
	case kMaterialVolume :
	case kExclusionVolume :
	case kDynamicPhantom :
		retVal = true;
		break;
	case kUnknown :
	default :
		retVal = false;
		llassert(0);
		break;
	}

	return retVal;
}

LLPathfindingLinkset::ELinksetUse LLPathfindingLinkset::getLinksetUseWithToggledPhantom(ELinksetUse pLinksetUse)
{
	BOOL isPhantom = LLPathfindingLinkset::isPhantom(pLinksetUse);
	BOOL isPermanent = LLPathfindingLinkset::isPermanent(pLinksetUse);
	BOOL isWalkable = LLPathfindingLinkset::isWalkable(pLinksetUse);

	return getLinksetUse(!isPhantom, isPermanent, isWalkable);
}

LLSD LLPathfindingLinkset::encodeAlteredFields(ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const
{
	LLSD itemData;

	if (!isTerrain() && (pLinksetUse != kUnknown) && (mLinksetUse != pLinksetUse))
	{
		if (mIsModifiable)
		{
			itemData[LINKSET_PHANTOM_FIELD] = static_cast<bool>(LLPathfindingLinkset::isPhantom(pLinksetUse));
		}
		itemData[LINKSET_PERMANENT_FIELD] = static_cast<bool>(LLPathfindingLinkset::isPermanent(pLinksetUse));
		itemData[LINKSET_WALKABLE_FIELD] = static_cast<bool>(LLPathfindingLinkset::isWalkable(pLinksetUse));
	}

	if (mWalkabilityCoefficientA != pA)
	{
		itemData[LINKSET_WALKABILITY_A_FIELD] = llclamp(pA, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}

	if (mWalkabilityCoefficientB != pB)
	{
		itemData[LINKSET_WALKABILITY_B_FIELD] = llclamp(pB, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}

	if (mWalkabilityCoefficientC != pC)
	{
		itemData[LINKSET_WALKABILITY_C_FIELD] = llclamp(pC, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}

	if (mWalkabilityCoefficientD != pD)
	{
		itemData[LINKSET_WALKABILITY_D_FIELD] = llclamp(pD, MIN_WALKABILITY_VALUE, MAX_WALKABILITY_VALUE);
	}

	return itemData;
}

void LLPathfindingLinkset::parseObjectData(const LLSD &pLinksetItem)
{
	llassert(pLinksetItem.has(LINKSET_NAME_FIELD));
	llassert(pLinksetItem.get(LINKSET_NAME_FIELD).isString());
	mName = pLinksetItem.get(LINKSET_NAME_FIELD).asString();
	
	llassert(pLinksetItem.has(LINKSET_DESCRIPTION_FIELD));
	llassert(pLinksetItem.get(LINKSET_DESCRIPTION_FIELD).isString());
	mDescription = pLinksetItem.get(LINKSET_DESCRIPTION_FIELD).asString();
	
	llassert(pLinksetItem.has(LINKSET_LAND_IMPACT_FIELD));
	llassert(pLinksetItem.get(LINKSET_LAND_IMPACT_FIELD).isInteger());
	llassert(pLinksetItem.get(LINKSET_LAND_IMPACT_FIELD).asInteger() >= 0);
	mLandImpact = pLinksetItem.get(LINKSET_LAND_IMPACT_FIELD).asInteger();
	
	llassert(pLinksetItem.has(LINKSET_MODIFIABLE_FIELD));
	llassert(pLinksetItem.get(LINKSET_MODIFIABLE_FIELD).isBoolean());
	mIsModifiable = pLinksetItem.get(LINKSET_MODIFIABLE_FIELD).asBoolean();
	
	llassert(pLinksetItem.has(LINKSET_POSITION_FIELD));
	llassert(pLinksetItem.get(LINKSET_POSITION_FIELD).isArray());
	mLocation.setValue(pLinksetItem.get(LINKSET_POSITION_FIELD));
}

void LLPathfindingLinkset::parsePathfindingData(const LLSD &pLinksetItem)
{
	bool isPhantom = false;
	if (pLinksetItem.has(LINKSET_PHANTOM_FIELD))
	{
		llassert(pLinksetItem.get(LINKSET_PHANTOM_FIELD).isBoolean());
		isPhantom = pLinksetItem.get(LINKSET_PHANTOM_FIELD).asBoolean();
	}
	
	llassert(pLinksetItem.has(LINKSET_PERMANENT_FIELD));
	llassert(pLinksetItem.get(LINKSET_PERMANENT_FIELD).isBoolean());
	bool isPermanent = pLinksetItem.get(LINKSET_PERMANENT_FIELD).asBoolean();
	
	llassert(pLinksetItem.has(LINKSET_WALKABLE_FIELD));
	llassert(pLinksetItem.get(LINKSET_WALKABLE_FIELD).isBoolean());
	bool isWalkable = pLinksetItem.get(LINKSET_WALKABLE_FIELD).asBoolean();
	
	mLinksetUse = getLinksetUse(isPhantom, isPermanent, isWalkable);
	
	llassert(pLinksetItem.has(LINKSET_WALKABILITY_A_FIELD));
	llassert(pLinksetItem.get(LINKSET_WALKABILITY_A_FIELD).isInteger());
	mWalkabilityCoefficientA = pLinksetItem.get(LINKSET_WALKABILITY_A_FIELD).asInteger();
	llassert(mWalkabilityCoefficientA >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientA <= MAX_WALKABILITY_VALUE);
	
	llassert(pLinksetItem.has(LINKSET_WALKABILITY_B_FIELD));
	llassert(pLinksetItem.get(LINKSET_WALKABILITY_B_FIELD).isInteger());
	mWalkabilityCoefficientB = pLinksetItem.get(LINKSET_WALKABILITY_B_FIELD).asInteger();
	llassert(mWalkabilityCoefficientB >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientB <= MAX_WALKABILITY_VALUE);
	
	llassert(pLinksetItem.has(LINKSET_WALKABILITY_C_FIELD));
	llassert(pLinksetItem.get(LINKSET_WALKABILITY_C_FIELD).isInteger());
	mWalkabilityCoefficientC = pLinksetItem.get(LINKSET_WALKABILITY_C_FIELD).asInteger();
	llassert(mWalkabilityCoefficientC >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientC <= MAX_WALKABILITY_VALUE);
	
	llassert(pLinksetItem.has(LINKSET_WALKABILITY_D_FIELD));
	llassert(pLinksetItem.get(LINKSET_WALKABILITY_D_FIELD).isInteger());
	mWalkabilityCoefficientD = pLinksetItem.get(LINKSET_WALKABILITY_D_FIELD).asInteger();
	llassert(mWalkabilityCoefficientD >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientD <= MAX_WALKABILITY_VALUE);
}

LLPathfindingLinkset::ELinksetUse LLPathfindingLinkset::getLinksetUse(bool pIsPhantom, bool pIsPermanent, bool pIsWalkable)
{
	return (pIsPhantom ? (pIsPermanent ? (pIsWalkable ? kMaterialVolume : kExclusionVolume) : kDynamicPhantom) :
		(pIsPermanent ? (pIsWalkable ? kWalkable : kStaticObstacle) : kDynamicObstacle));
}

BOOL LLPathfindingLinkset::isPermanent(ELinksetUse pLinksetUse)
{
	BOOL retVal;

	switch (pLinksetUse)
	{
	case kWalkable :
	case kStaticObstacle :
	case kMaterialVolume :
	case kExclusionVolume :
		retVal = true;
		break;
	case kDynamicObstacle :
	case kDynamicPhantom :
		retVal = false;
		break;
	case kUnknown :
	default :
		retVal = false;
		llassert(0);
		break;
	}

	return retVal;
}

BOOL LLPathfindingLinkset::isWalkable(ELinksetUse pLinksetUse)
{
	BOOL retVal;

	switch (pLinksetUse)
	{
	case kWalkable :
	case kMaterialVolume :
		retVal = true;
		break;
	case kStaticObstacle :
	case kDynamicObstacle :
	case kExclusionVolume :
	case kDynamicPhantom :
		retVal = false;
		break;
	case kUnknown :
	default :
		retVal = false;
		llassert(0);
		break;
	}

	return retVal;
}
