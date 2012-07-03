/** 
* @file llpathfindingcharacter.cpp
* @brief Definition of a pathfinding character that contains various properties required for havok pathfinding.
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

#include "llpathfindingcharacter.h"

#include <string>

#include "llpathfindingobject.h"
#include "llsd.h"

#define CHARACTER_CPU_TIME_FIELD   "cpu_time"
#define CHARACTER_HORIZONTAL_FIELD "horizontal"
#define CHARACTER_LENGTH_FIELD     "length"
#define CHARACTER_RADIUS_FIELD     "radius"

//---------------------------------------------------------------------------
// LLPathfindingCharacter
//---------------------------------------------------------------------------

LLPathfindingCharacter::LLPathfindingCharacter(const std::string &pUUID, const LLSD& pCharacterData)
	: LLPathfindingObject(pUUID, pCharacterData),
	mCPUTime(0U),
	mIsHorizontal(FALSE),
	mLength(0.0f),
	mRadius(0.0f)
{
	parseCharacterData(pCharacterData);
}

LLPathfindingCharacter::LLPathfindingCharacter(const LLPathfindingCharacter& pOther)
	: LLPathfindingObject(pOther),
	mCPUTime(pOther.mCPUTime),
	mIsHorizontal(pOther.mIsHorizontal),
	mLength(pOther.mLength),
	mRadius(pOther.mRadius)
{
}

LLPathfindingCharacter::~LLPathfindingCharacter()
{
}

LLPathfindingCharacter& LLPathfindingCharacter::operator =(const LLPathfindingCharacter& pOther)
{
	dynamic_cast<LLPathfindingObject &>(*this) = pOther;

	mCPUTime = pOther.mCPUTime;
	mIsHorizontal = pOther.mIsHorizontal;
	mLength = pOther.mLength;
	mRadius = pOther.mRadius;

	return *this;
}

void LLPathfindingCharacter::parseCharacterData(const LLSD &pCharacterData)
{
	llassert(pCharacterData.has(CHARACTER_CPU_TIME_FIELD));
	llassert(pCharacterData.get(CHARACTER_CPU_TIME_FIELD).isReal());
	mCPUTime = pCharacterData.get(CHARACTER_CPU_TIME_FIELD).asReal();

	llassert(pCharacterData.has(CHARACTER_HORIZONTAL_FIELD));
	llassert(pCharacterData.get(CHARACTER_HORIZONTAL_FIELD).isBoolean());
	mIsHorizontal = pCharacterData.get(CHARACTER_HORIZONTAL_FIELD).asBoolean();

	llassert(pCharacterData.has(CHARACTER_LENGTH_FIELD));
	llassert(pCharacterData.get(CHARACTER_LENGTH_FIELD).isReal());
	mLength = pCharacterData.get(CHARACTER_LENGTH_FIELD).asReal();

	llassert(pCharacterData.has(CHARACTER_RADIUS_FIELD));
	llassert(pCharacterData.get(CHARACTER_RADIUS_FIELD).isReal());
	mRadius = pCharacterData.get(CHARACTER_RADIUS_FIELD).asReal();
}
