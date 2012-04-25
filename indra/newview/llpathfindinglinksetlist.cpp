/** 
 * @file llpathfindinglinksetlist.cpp
 * @author William Todd Stinson
 * @brief Class to implement the list of a set of pathfinding linksets
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

#include <string>
#include <map>

#include "llsd.h"
#include "lluuid.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"

//---------------------------------------------------------------------------
// LLPathfindingLinksetList
//---------------------------------------------------------------------------

LLPathfindingLinksetList::LLPathfindingLinksetList()
	: LLPathfindingLinksetMap()
{
}

LLPathfindingLinksetList::LLPathfindingLinksetList(const LLSD& pLinksetItems)
	: LLPathfindingLinksetMap()
{
	for (LLSD::map_const_iterator linksetItemIter = pLinksetItems.beginMap();
		linksetItemIter != pLinksetItems.endMap(); ++linksetItemIter)
	{
		const std::string& uuid(linksetItemIter->first);
		const LLSD& linksetData = linksetItemIter->second;
		LLPathfindingLinksetPtr linkset(new LLPathfindingLinkset(uuid, linksetData));
		insert(std::pair<std::string, LLPathfindingLinksetPtr>(uuid, linkset));
	}
}

LLPathfindingLinksetList::~LLPathfindingLinksetList()
{
	clear();
}

void LLPathfindingLinksetList::update(const LLPathfindingLinksetList &pUpdateLinksetList)
{
	for (LLPathfindingLinksetList::const_iterator updateLinksetIter = pUpdateLinksetList.begin();
		updateLinksetIter != pUpdateLinksetList.end(); ++updateLinksetIter)
	{
		const std::string &uuid = updateLinksetIter->first;
		const LLPathfindingLinksetPtr updateLinksetPtr = updateLinksetIter->second;

		LLPathfindingLinksetList::iterator linksetIter = find(uuid);
		if (linksetIter == end())
		{
			insert(std::pair<std::string, LLPathfindingLinksetPtr>(uuid, updateLinksetPtr));
		}
		else
		{
			LLPathfindingLinksetPtr linksetPtr = linksetIter->second;
			*linksetPtr = *updateLinksetPtr;
		}
	}
}

LLSD LLPathfindingLinksetList::encodeObjectFields(LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const
{
	LLSD listData;

	for (LLPathfindingLinksetMap::const_iterator linksetIter = begin(); linksetIter != end(); ++linksetIter)
	{
		const LLPathfindingLinksetPtr linksetPtr = linksetIter->second;
		if (!linksetPtr->isTerrain())
		{
			LLSD linksetData = linksetPtr->encodeAlteredFields(pLinksetUse, pA, pB, pC, pD);
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
	
	for (LLPathfindingLinksetMap::const_iterator linksetIter = begin(); linksetIter != end(); ++linksetIter)
	{
		const LLPathfindingLinksetPtr linksetPtr = linksetIter->second;
		if (linksetPtr->isTerrain())
		{
			terrainData = linksetPtr->encodeAlteredFields(pLinksetUse, pA, pB, pC, pD);
			break;
		}
	}
	
	return terrainData;
}
