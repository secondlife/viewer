/** 
 * @file llpathfindingcharacterlist.cpp
 * @author William Todd Stinson
 * @brief Class to implement the list of a set of pathfinding characters
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
#include "llpathfindingcharacter.h"
#include "llpathfindingcharacterlist.h"

//---------------------------------------------------------------------------
// LLPathfindingCharacterList
//---------------------------------------------------------------------------

LLPathfindingCharacterList::LLPathfindingCharacterList()
	: LLPathfindingCharacterMap()
{
}

LLPathfindingCharacterList::LLPathfindingCharacterList(const LLSD& pCharacterItems)
	: LLPathfindingCharacterMap()
{
	for (LLSD::map_const_iterator characterItemIter = pCharacterItems.beginMap();
		characterItemIter != pCharacterItems.endMap(); ++characterItemIter)
	{
		const std::string& uuid(characterItemIter->first);
		const LLSD& characterData = characterItemIter->second;
		LLPathfindingCharacterPtr character(new LLPathfindingCharacter(uuid, characterData));
		insert(std::pair<std::string, LLPathfindingCharacterPtr>(uuid, character));
	}
}

LLPathfindingCharacterList::~LLPathfindingCharacterList()
{
	clear();
}
