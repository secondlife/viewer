/** 
 * @file llfilteredpathfindinglinksets.h
 * @author William Todd Stinson
 * @brief Class to implement the filtering of a set of pathfinding linksets
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

#ifndef LL_FILTEREDPATHFINDINGLINKSETS_H
#define LL_FILTEREDPATHFINDINGLINKSETS_H

#include <string>
#include <map>

class LLSD;
class LLPathfindingLinkset;

class FilterString
{
public:
	FilterString();
	FilterString(const std::string& pFilter);
	virtual ~FilterString();

	const std::string& get() const;
	bool               set(const std::string& pFilter);
	void               clear();

	bool               isActive() const;
	bool               doesMatch(const std::string& pTestString) const;

protected:

private:
	std::string mFilter;
	std::string mUpperFilter;
};

class LLFilteredPathfindingLinksets
{
public:
	typedef std::map<std::string, LLPathfindingLinkset> PathfindingLinksetMap;

	LLFilteredPathfindingLinksets();
	LLFilteredPathfindingLinksets(const LLSD& pNavMeshData);
	LLFilteredPathfindingLinksets(const LLFilteredPathfindingLinksets& pOther);
	virtual ~LLFilteredPathfindingLinksets();

	void                         setPathfindingLinksets(const LLSD& pNavMeshData);
	void                         updatePathfindingLinksets(const LLSD& pNavMeshData);
	void                         clearPathfindingLinksets();

	const PathfindingLinksetMap& getAllLinksets() const;
	const PathfindingLinksetMap& getFilteredLinksets();

	BOOL                         isFiltersActive() const;

	void                         setNameFilter(const std::string& pNameFilter);
	const std::string&           getNameFilter() const;

	void                         setDescriptionFilter(const std::string& pDescriptionFilter);
	const std::string&           getDescriptionFilter() const;

	void                         setWalkableFilter(BOOL pWalkableFilter);
	BOOL                         isWalkableFilter() const;

	void                         setObstacleFilter(BOOL pObstacleFilter);
	BOOL                         isObstacleFilter() const;

	void                         setIgnoredFilter(BOOL pIgnoredFilter);
	BOOL                         isIgnoredFilter() const;

	void                         clearFilters();

protected:

private:
	PathfindingLinksetMap mAllLinksets;
	PathfindingLinksetMap mFilteredLinksets;

	bool                  mIsFiltersDirty;
	FilterString          mNameFilter;
	FilterString          mDescriptionFilter;
	BOOL                  mIsWalkableFilter;
	BOOL                  mIsObstacleFilter;
	BOOL                  mIsIgnoredFilter;

	void applyFilters();
	BOOL doesMatchFilters(const LLPathfindingLinkset& pLinkset) const;
};

#endif // LL_FILTEREDPATHFINDINGLINKSETS_H
