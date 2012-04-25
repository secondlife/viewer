/** 
 * @file llpathfindingcharacter.h
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

#ifndef LL_LLPATHFINDINGCHARACTER_H
#define LL_LLPATHFINDINGCHARACTER_H

#include "v3math.h"
#include "lluuid.h"
#include "llavatarname.h"

#include <boost/shared_ptr.hpp>

class LLSD;
class LLPathfindingCharacter;

typedef boost::shared_ptr<LLPathfindingCharacter> LLPathfindingCharacterPtr;

class LLPathfindingCharacter
{
public:
	LLPathfindingCharacter(const std::string &pUUID, const LLSD &pCharacterItem);
	LLPathfindingCharacter(const LLPathfindingCharacter& pOther);
	virtual ~LLPathfindingCharacter();

	LLPathfindingCharacter& operator = (const LLPathfindingCharacter& pOther);

	inline const LLUUID&      getUUID() const        {return mUUID;};
	inline const std::string& getName() const        {return mName;};
	inline const std::string& getDescription() const {return mDescription;};
	inline const std::string  getOwnerName() const   {return mOwnerName.getCompleteName();};
	inline F32                getCPUTime() const     {return mCPUTime;};
	inline const LLVector3&   getLocation() const    {return mLocation;};

protected:

private:
	LLUUID       mUUID;
	std::string  mName;
	std::string  mDescription;
	LLUUID       mOwnerUUID;
	LLAvatarName mOwnerName;
	F32          mCPUTime;
	LLVector3    mLocation;
};

#endif // LL_LLPATHFINDINGCHARACTER_H
