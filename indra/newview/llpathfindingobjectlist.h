/** 
* @file   llpathfindingobjectlist.h
* @brief  Header file for llpathfindingobjectlist
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
#ifndef LL_LLPATHFINDINGOBJECTLIST_H
#define LL_LLPATHFINDINGOBJECTLIST_H

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>

#include "llpathfindingobject.h"

class LLPathfindingObjectList;

typedef boost::shared_ptr<LLPathfindingObjectList> LLPathfindingObjectListPtr;
typedef std::map<std::string, LLPathfindingObjectPtr> LLPathfindingObjectMap;

class LLPathfindingObjectList
{
public:
	LLPathfindingObjectList();
	virtual ~LLPathfindingObjectList();

	bool isEmpty() const;

	void clear();

	void update(LLPathfindingObjectPtr pUpdateObjectPtr);
	void update(LLPathfindingObjectListPtr pUpdateObjectListPtr);

	LLPathfindingObjectPtr find(const std::string &pObjectId) const;

	typedef LLPathfindingObjectMap::const_iterator const_iterator;
	const_iterator begin() const;
	const_iterator end() const;

protected:
	LLPathfindingObjectMap &getObjectMap();

private:
	LLPathfindingObjectMap mObjectMap;
};

#endif // LL_LLPATHFINDINGOBJECTLIST_H
