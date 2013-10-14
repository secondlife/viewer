/** 
* @file llpathfindinglinksetlist.cpp
* @brief Implementation of llpathfindinglinksetlist
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

#include "llpathfindinglinksetlist.h"

#include <string>
#include <map>

#include "llpathfindinglinkset.h"
#include "llpathfindingobject.h"
#include "llpathfindingobjectlist.h"
#include "llsd.h"

//---------------------------------------------------------------------------
// LLPathfindingLinksetList
//---------------------------------------------------------------------------

LLPathfindingLinksetList::LLPathfindingLinksetList()
	: LLPathfindingObjectList()
{
}

LLPathfindingLinksetList::LLPathfindingLinksetList(const LLSD& pLinksetListData)
	: LLPathfindingObjectList()
{
	parseLinksetListData(pLinksetListData);
}

LLPathfindingLinksetList::~LLPathfindingLinksetList()
{
}

LLSD LLPathfindingLinksetList::encodeObjectFields(LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const
{
	LLSD listData;

	for (const_iterator linksetIter = begin(); linksetIter != end(); ++linksetIter)
	{
		const LLPathfindingObjectPtr objectPtr = linksetIter->second;
		const LLPathfindingLinkset *linkset = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());

		if (!linkset->isTerrain())
		{
			LLSD linksetData = linkset->encodeAlteredFields(pLinksetUse, pA, pB, pC, pD);
			if (!linksetData.isUndefined())
			{
				const std::string& uuid(linksetIter->first);
				listData[uuid] = linksetData;
			}
		}
	}

	return listData;
}

LLSD LLPathfindingLinksetList::encodeTerrainFields(LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const
{
	LLSD terrainData;

	for (const_iterator linksetIter = begin(); linksetIter != end(); ++linksetIter)
	{
		const LLPathfindingObjectPtr objectPtr = linksetIter->second;
		const LLPathfindingLinkset *linkset = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());

		if (linkset->isTerrain())
		{
			terrainData = linkset->encodeAlteredFields(pLinksetUse, pA, pB, pC, pD);
			break;
		}
	}
	
	return terrainData;
}

bool LLPathfindingLinksetList::isShowUnmodifiablePhantomWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	bool isShowWarning = false;

	for (const_iterator objectIter = begin(); !isShowWarning && (objectIter != end()); ++objectIter)
	{
		const LLPathfindingObjectPtr objectPtr = objectIter->second;
		const LLPathfindingLinkset *linkset = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());
		isShowWarning = linkset->isShowUnmodifiablePhantomWarning(pLinksetUse);
	}

	return isShowWarning;
}

bool LLPathfindingLinksetList::isShowPhantomToggleWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	bool isShowWarning = false;

	for (const_iterator objectIter = begin(); !isShowWarning && (objectIter != end()); ++objectIter)
	{
		const LLPathfindingObjectPtr objectPtr = objectIter->second;
		const LLPathfindingLinkset *linkset = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());
		isShowWarning = linkset->isShowPhantomToggleWarning(pLinksetUse);
	}

	return isShowWarning;
}

bool LLPathfindingLinksetList::isShowCannotBeVolumeWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	bool isShowWarning = false;

	for (const_iterator objectIter = begin(); !isShowWarning && (objectIter != end()); ++objectIter)
	{
		const LLPathfindingObjectPtr objectPtr = objectIter->second;
		const LLPathfindingLinkset *linkset = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());
		isShowWarning = linkset->isShowCannotBeVolumeWarning(pLinksetUse);
	}

	return isShowWarning;
}

void LLPathfindingLinksetList::determinePossibleStates(BOOL &pCanBeWalkable, BOOL &pCanBeStaticObstacle, BOOL &pCanBeDynamicObstacle,
	BOOL &pCanBeMaterialVolume, BOOL &pCanBeExclusionVolume, BOOL &pCanBeDynamicPhantom) const
{
	pCanBeWalkable = FALSE;
	pCanBeStaticObstacle = FALSE;
	pCanBeDynamicObstacle = FALSE;
	pCanBeMaterialVolume = FALSE;
	pCanBeExclusionVolume = FALSE;
	pCanBeDynamicPhantom = FALSE;

	for (const_iterator objectIter = begin();
		!(pCanBeWalkable && pCanBeStaticObstacle && pCanBeDynamicObstacle && pCanBeMaterialVolume && pCanBeExclusionVolume && pCanBeDynamicPhantom) && (objectIter != end());
		++objectIter)
	{
		const LLPathfindingObjectPtr objectPtr = objectIter->second;
		const LLPathfindingLinkset *linkset = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());

		if (linkset->isTerrain())
		{
			pCanBeWalkable = TRUE;
		}
		else
		{
			if (linkset->isModifiable())
			{
				pCanBeWalkable = TRUE;
				pCanBeStaticObstacle = TRUE;
				pCanBeDynamicObstacle = TRUE;
				pCanBeDynamicPhantom = TRUE;
				if (linkset->canBeVolume())
				{
					pCanBeMaterialVolume = TRUE;
					pCanBeExclusionVolume = TRUE;
				}
			}
			else if (linkset->isPhantom())
			{
				pCanBeDynamicPhantom = TRUE;
				if (linkset->canBeVolume())
				{
					pCanBeMaterialVolume = TRUE;
					pCanBeExclusionVolume = TRUE;
				}
			}
			else
			{
				pCanBeWalkable = TRUE;
				pCanBeStaticObstacle = TRUE;
				pCanBeDynamicObstacle = TRUE;
			}
		}
	}
}

void LLPathfindingLinksetList::parseLinksetListData(const LLSD& pLinksetListData)
{
	LLPathfindingObjectMap &objectMap = getObjectMap();

	for (LLSD::map_const_iterator linksetDataIter = pLinksetListData.beginMap();
		linksetDataIter != pLinksetListData.endMap(); ++linksetDataIter)
	{
		const std::string& uuid(linksetDataIter->first);
		const LLSD& linksetData = linksetDataIter->second;
		LLPathfindingObjectPtr linksetPtr(new LLPathfindingLinkset(uuid, linksetData));
		objectMap.insert(std::pair<std::string, LLPathfindingObjectPtr>(uuid, linksetPtr));
	}
}
