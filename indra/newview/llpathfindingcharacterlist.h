/** 
 * @file llpathfindingcharacterlist.h
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

#ifndef LL_LLPATHFINDINGCHARACTERLIST_H
#define LL_LLPATHFINDINGCHARACTERLIST_H

#include <string>
#include <map>
#include "llpathfindingcharacter.h"

#include <boost/shared_ptr.hpp>

class LLSD;
class LLPathfindingCharacterList;

typedef boost::shared_ptr<LLPathfindingCharacterList> LLPathfindingCharacterListPtr;
typedef std::map<std::string, LLPathfindingCharacterPtr> LLPathfindingCharacterMap;

class LLPathfindingCharacterList : public LLPathfindingCharacterMap
{
public:
	LLPathfindingCharacterList();
	LLPathfindingCharacterList(const LLSD& pCharacterItems);
	virtual ~LLPathfindingCharacterList();

protected:

private:

};

#endif // LL_LLPATHFINDINGCHARACTERLIST_H
