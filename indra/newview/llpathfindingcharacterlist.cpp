/** 
* @file llpathfindingcharacterlist.cpp
* @brief Implementation of llpathfindingcharacterlist
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

#include "llpathfindingcharacterlist.h"

#include "llpathfindingcharacter.h"
#include "llpathfindingobject.h"
#include "llpathfindingobjectlist.h"
#include "llsd.h"

//---------------------------------------------------------------------------
// LLPathfindingCharacterList
//---------------------------------------------------------------------------

LLPathfindingCharacterList::LLPathfindingCharacterList()
	: LLPathfindingObjectList()
{
}

LLPathfindingCharacterList::LLPathfindingCharacterList(const LLSD& pCharacterListData)
	: LLPathfindingObjectList()
{
	parseCharacterListData(pCharacterListData);
}

LLPathfindingCharacterList::~LLPathfindingCharacterList()
{
}

void LLPathfindingCharacterList::parseCharacterListData(const LLSD& pCharacterListData)
{
	LLPathfindingObjectMap &objectMap = getObjectMap();

	for (LLSD::map_const_iterator characterDataIter = pCharacterListData.beginMap();
		characterDataIter != pCharacterListData.endMap(); ++characterDataIter)
	{
		const std::string& uuid(characterDataIter->first);
		const LLSD& characterData = characterDataIter->second;
		LLPathfindingObjectPtr character(new LLPathfindingCharacter(uuid, characterData));
		objectMap.insert(std::pair<std::string, LLPathfindingObjectPtr>(uuid, character));
	}
}
