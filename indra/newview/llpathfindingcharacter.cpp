/** 
 * @file llpathfindinglinksets.cpp
 * @author William Todd Stinson
 * @brief Definition of a pathfinding character that contains various properties required for havok pathfinding.
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
#include "llpathfindingcharacter.h"
#include "llsd.h"
#include "v3math.h"
#include "lluuid.h"

#define CHARACTER_NAME_FIELD          "name"
#define CHARACTER_DESCRIPTION_FIELD   "description"
#define CHARACTER_OWNER_FIELD         "description"
#define CHARACTER_CPU_TIME_FIELD      "landimpact"
#define CHARACTER_POSITION_FIELD      "position"

//---------------------------------------------------------------------------
// LLPathfindingCharacter
//---------------------------------------------------------------------------

LLPathfindingCharacter::LLPathfindingCharacter(const std::string &pUUID, const LLSD& pCharacterItem)
	: mUUID(pUUID),
	mName(),
	mDescription(),
	mOwner(),
	mCPUTime(0U),
	mLocation()
{
	llassert(pCharacterItem.has(CHARACTER_NAME_FIELD));
	llassert(pCharacterItem.get(CHARACTER_NAME_FIELD).isString());
	mName = pCharacterItem.get(CHARACTER_NAME_FIELD).asString();

	llassert(pCharacterItem.has(CHARACTER_DESCRIPTION_FIELD));
	llassert(pCharacterItem.get(CHARACTER_DESCRIPTION_FIELD).isString());
	mDescription = pCharacterItem.get(CHARACTER_DESCRIPTION_FIELD).asString();

	llassert(pCharacterItem.has(CHARACTER_OWNER_FIELD));
	llassert(pCharacterItem.get(CHARACTER_OWNER_FIELD).isString());
	mOwner = pCharacterItem.get(CHARACTER_OWNER_FIELD).asString();

	llassert(pCharacterItem.has(CHARACTER_CPU_TIME_FIELD));
	llassert(pCharacterItem.get(CHARACTER_CPU_TIME_FIELD).isInteger());
	llassert(pCharacterItem.get(CHARACTER_CPU_TIME_FIELD).asInteger() >= 0);
	mCPUTime = pCharacterItem.get(CHARACTER_CPU_TIME_FIELD).asInteger();

	llassert(pCharacterItem.has(CHARACTER_POSITION_FIELD));
	llassert(pCharacterItem.get(CHARACTER_POSITION_FIELD).isArray());
	mLocation.setValue(pCharacterItem.get(CHARACTER_POSITION_FIELD));
}

LLPathfindingCharacter::LLPathfindingCharacter(const LLPathfindingCharacter& pOther)
	: mUUID(pOther.mUUID),
	mName(pOther.mName),
	mDescription(pOther.mDescription),
	mOwner(pOther.mOwner),
	mCPUTime(pOther.mCPUTime),
	mLocation(pOther.mLocation)
{
}

LLPathfindingCharacter::~LLPathfindingCharacter()
{
}

LLPathfindingCharacter& LLPathfindingCharacter::operator =(const LLPathfindingCharacter& pOther)
{
	mUUID = pOther.mUUID;
	mName = pOther.mName;
	mDescription = pOther.mDescription;
	mOwner = pOther.mOwner;
	mCPUTime = pOther.mCPUTime;
	mLocation = pOther.mLocation;

	return *this;
}
