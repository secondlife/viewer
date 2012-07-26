/** 
* @file llpathfindinglinkset.cpp
* @brief Definition of a pathfinding linkset that contains various properties required for havok pathfinding.
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include <string>

#include "llpathfindingobject.h"
#include "llsd.h"

#define LINKSET_LAND_IMPACT_FIELD   "landimpact"
#define LINKSET_MODIFIABLE_FIELD    "modifiable"
#define LINKSET_CATEGORY_FIELD      "navmesh_category"
#define LINKSET_CAN_BE_VOLUME       "can_be_volume"
#define LINKSET_PHANTOM_FIELD       "phantom"
#define LINKSET_WALKABILITY_A_FIELD "A"
#define LINKSET_WALKABILITY_B_FIELD "B"
#define LINKSET_WALKABILITY_C_FIELD "C"
#define LINKSET_WALKABILITY_D_FIELD "D"

#define LINKSET_CATEGORY_VALUE_INCLUDE 0
#define LINKSET_CATEGORY_VALUE_EXCLUDE 1
#define LINKSET_CATEGORY_VALUE_IGNORE  2

//---------------------------------------------------------------------------
// LLPathfindingLinkset
//---------------------------------------------------------------------------

const S32 LLPathfindingLinkset::MIN_WALKABILITY_VALUE(0);
const S32 LLPathfindingLinkset::MAX_WALKABILITY_VALUE(100);

LLPathfindingLinkset::LLPathfindingLinkset(const LLSD& pTerrainData)
	: LLPathfindingObject(),
	mIsTerrain(true),
	mLandImpact(0U),
	mIsModifiable(FALSE),
	mCanBeVolume(FALSE),
	mLinksetUse(kUnknown),
	mWalkabilityCoefficientA(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientB(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientC(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientD(MIN_WALKABILITY_VALUE)
{
	parsePathfindingData(pTerrainData);
}

LLPathfindingLinkset::LLPathfindingLinkset(const std::string &pUUID, const LLSD& pLinksetData)
	: LLPathfindingObject(pUUID, pLinksetData),
	mIsTerrain(false),
	mLandImpact(0U),
	mIsModifiable(TRUE),
	mCanBeVolume(TRUE),
	mLinksetUse(kUnknown),
	mWalkabilityCoefficientA(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientB(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientC(MIN_WALKABILITY_VALUE),
	mWalkabilityCoefficientD(MIN_WALKABILITY_VALUE)
{
	parseLinksetData(pLinksetData);
	parsePathfindingData(pLinksetData);
}

LLPathfindingLinkset::LLPathfindingLinkset(const LLPathfindingLinkset& pOther)
	: LLPathfindingObject(pOther),
	mIsTerrain(pOther.mIsTerrain),
	mLandImpact(pOther.mLandImpact),
	mIsModifiable(pOther.mIsModifiable),
	mCanBeVolume(pOther.mCanBeVolume),
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
	dynamic_cast<LLPathfindingObject &>(*this) = pOther;

	mIsTerrain = pOther.mIsTerrain;
	mLandImpact = pOther.mLandImpact;
	mIsModifiable = pOther.mIsModifiable;
	mCanBeVolume = pOther.mCanBeVolume;
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

LLPathfindingLinkset::ELinksetUse LLPathfindingLinkset::getLinksetUseWithToggledPhantom(ELinksetUse pLinksetUse)
{
	BOOL isPhantom = LLPathfindingLinkset::isPhantom(pLinksetUse);
	ENavMeshGenerationCategory navMeshGenerationCategory = getNavMeshGenerationCategory(pLinksetUse);

	return getLinksetUse(!isPhantom, navMeshGenerationCategory);
}

bool LLPathfindingLinkset::isShowUnmodifiablePhantomWarning(ELinksetUse pLinksetUse) const
{
	return (!isModifiable() && (isPhantom() != isPhantom(pLinksetUse)));
}

bool LLPathfindingLinkset::isShowCannotBeVolumeWarning(ELinksetUse pLinksetUse) const
{
	return (!canBeVolume() && ((pLinksetUse == kMaterialVolume) || (pLinksetUse == kExclusionVolume)));
}

LLSD LLPathfindingLinkset::encodeAlteredFields(ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const
{
	LLSD itemData;

	if (!isTerrain() && (pLinksetUse != kUnknown) && (getLinksetUse() != pLinksetUse) &&
		(canBeVolume() || ((pLinksetUse != kMaterialVolume) && (pLinksetUse != kExclusionVolume))))
	{
		if (isModifiable())
		{
			itemData[LINKSET_PHANTOM_FIELD] = static_cast<bool>(isPhantom(pLinksetUse));
		}

		itemData[LINKSET_CATEGORY_FIELD] = convertCategoryToLLSD(getNavMeshGenerationCategory(pLinksetUse));
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

void LLPathfindingLinkset::parseLinksetData(const LLSD &pLinksetData)
{
	llassert(pLinksetData.has(LINKSET_LAND_IMPACT_FIELD));
	llassert(pLinksetData.get(LINKSET_LAND_IMPACT_FIELD).isInteger());
	llassert(pLinksetData.get(LINKSET_LAND_IMPACT_FIELD).asInteger() >= 0);
	mLandImpact = pLinksetData.get(LINKSET_LAND_IMPACT_FIELD).asInteger();
	
	llassert(pLinksetData.has(LINKSET_MODIFIABLE_FIELD));
	llassert(pLinksetData.get(LINKSET_MODIFIABLE_FIELD).isBoolean());
	mIsModifiable = pLinksetData.get(LINKSET_MODIFIABLE_FIELD).asBoolean();
}

void LLPathfindingLinkset::parsePathfindingData(const LLSD &pLinksetData)
{
	bool isPhantom = false;
	if (pLinksetData.has(LINKSET_PHANTOM_FIELD))
	{
		llassert(pLinksetData.get(LINKSET_PHANTOM_FIELD).isBoolean());
		isPhantom = pLinksetData.get(LINKSET_PHANTOM_FIELD).asBoolean();
	}
	
	llassert(pLinksetData.has(LINKSET_CATEGORY_FIELD));
	mLinksetUse = getLinksetUse(isPhantom, convertCategoryFromLLSD(pLinksetData.get(LINKSET_CATEGORY_FIELD)));

	if (pLinksetData.has(LINKSET_CAN_BE_VOLUME))
	{
		llassert(pLinksetData.get(LINKSET_CAN_BE_VOLUME).isBoolean());
		mCanBeVolume = pLinksetData.get(LINKSET_CAN_BE_VOLUME).asBoolean();
	}

	llassert(pLinksetData.has(LINKSET_WALKABILITY_A_FIELD));
	llassert(pLinksetData.get(LINKSET_WALKABILITY_A_FIELD).isInteger());
	mWalkabilityCoefficientA = pLinksetData.get(LINKSET_WALKABILITY_A_FIELD).asInteger();
	llassert(mWalkabilityCoefficientA >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientA <= MAX_WALKABILITY_VALUE);
	
	llassert(pLinksetData.has(LINKSET_WALKABILITY_B_FIELD));
	llassert(pLinksetData.get(LINKSET_WALKABILITY_B_FIELD).isInteger());
	mWalkabilityCoefficientB = pLinksetData.get(LINKSET_WALKABILITY_B_FIELD).asInteger();
	llassert(mWalkabilityCoefficientB >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientB <= MAX_WALKABILITY_VALUE);
	
	llassert(pLinksetData.has(LINKSET_WALKABILITY_C_FIELD));
	llassert(pLinksetData.get(LINKSET_WALKABILITY_C_FIELD).isInteger());
	mWalkabilityCoefficientC = pLinksetData.get(LINKSET_WALKABILITY_C_FIELD).asInteger();
	llassert(mWalkabilityCoefficientC >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientC <= MAX_WALKABILITY_VALUE);
	
	llassert(pLinksetData.has(LINKSET_WALKABILITY_D_FIELD));
	llassert(pLinksetData.get(LINKSET_WALKABILITY_D_FIELD).isInteger());
	mWalkabilityCoefficientD = pLinksetData.get(LINKSET_WALKABILITY_D_FIELD).asInteger();
	llassert(mWalkabilityCoefficientD >= MIN_WALKABILITY_VALUE);
	llassert(mWalkabilityCoefficientD <= MAX_WALKABILITY_VALUE);
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

LLPathfindingLinkset::ELinksetUse LLPathfindingLinkset::getLinksetUse(bool pIsPhantom, ENavMeshGenerationCategory pNavMeshGenerationCategory)
{
	ELinksetUse linksetUse = kUnknown;

	if (pIsPhantom)
	{
		switch (pNavMeshGenerationCategory)
		{
		case kNavMeshGenerationIgnore :
			linksetUse = kDynamicPhantom;
			break;
		case kNavMeshGenerationInclude :
			linksetUse = kMaterialVolume;
			break;
		case kNavMeshGenerationExclude :
			linksetUse = kExclusionVolume;
			break;
		default :
			linksetUse = kUnknown;
			llassert(0);
			break;
		}
	}
	else
	{
		switch (pNavMeshGenerationCategory)
		{
		case kNavMeshGenerationIgnore :
			linksetUse = kDynamicObstacle;
			break;
		case kNavMeshGenerationInclude :
			linksetUse = kWalkable;
			break;
		case kNavMeshGenerationExclude :
			linksetUse = kStaticObstacle;
			break;
		default :
			linksetUse = kUnknown;
			llassert(0);
			break;
		}
	}

	return linksetUse;
}

LLPathfindingLinkset::ENavMeshGenerationCategory LLPathfindingLinkset::getNavMeshGenerationCategory(ELinksetUse pLinksetUse)
{
	ENavMeshGenerationCategory navMeshGenerationCategory;
	switch (pLinksetUse)
	{
	case kWalkable :
	case kMaterialVolume :
		navMeshGenerationCategory = kNavMeshGenerationInclude;
		break;
	case kStaticObstacle :
	case kExclusionVolume :
		navMeshGenerationCategory = kNavMeshGenerationExclude;
		break;
	case kDynamicObstacle :
	case kDynamicPhantom :
		navMeshGenerationCategory = kNavMeshGenerationIgnore;
		break;
	case kUnknown :
	default :
		navMeshGenerationCategory = kNavMeshGenerationIgnore;
		llassert(0);
		break;
	}

	return navMeshGenerationCategory;
}

LLSD LLPathfindingLinkset::convertCategoryToLLSD(ENavMeshGenerationCategory pNavMeshGenerationCategory)
{
	LLSD llsd;

	switch (pNavMeshGenerationCategory)
	{
		case kNavMeshGenerationIgnore :
			llsd = static_cast<S32>(LINKSET_CATEGORY_VALUE_IGNORE);
			break;
		case kNavMeshGenerationInclude :
			llsd = static_cast<S32>(LINKSET_CATEGORY_VALUE_INCLUDE);
			break;
		case kNavMeshGenerationExclude :
			llsd = static_cast<S32>(LINKSET_CATEGORY_VALUE_EXCLUDE);
			break;
		default :
			llsd = static_cast<S32>(LINKSET_CATEGORY_VALUE_IGNORE);
			llassert(0);
			break;
	}

	return llsd;
}

LLPathfindingLinkset::ENavMeshGenerationCategory LLPathfindingLinkset::convertCategoryFromLLSD(const LLSD &llsd)
{
	ENavMeshGenerationCategory navMeshGenerationCategory;

	llassert(llsd.isInteger());
	switch (llsd.asInteger())
	{
		case LINKSET_CATEGORY_VALUE_IGNORE :
			navMeshGenerationCategory = kNavMeshGenerationIgnore;
			break;
		case LINKSET_CATEGORY_VALUE_INCLUDE :
			navMeshGenerationCategory = kNavMeshGenerationInclude;
			break;
		case LINKSET_CATEGORY_VALUE_EXCLUDE :
			navMeshGenerationCategory = kNavMeshGenerationExclude;
			break;
		default :
			navMeshGenerationCategory = kNavMeshGenerationIgnore;
			llassert(0);
			break;
	}

	return navMeshGenerationCategory;
}
