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

#include "llviewerprecompiledheaders.h"

#include <string>
#include <map>

#include "llsd.h"
#include "lluuid.h"
#include "llpathfindinglinkset.h"
#include "llfilteredpathfindinglinksets.h"

//---------------------------------------------------------------------------
// FilterString
//---------------------------------------------------------------------------

FilterString::FilterString()
	: mFilter(),
	mUpperFilter()
{
}

FilterString::FilterString(const std::string& pFilter)
	: mFilter(pFilter),
	mUpperFilter()
{
	LLStringUtil::trim(mFilter);
	mUpperFilter = mFilter;
	if (!mUpperFilter.empty())
	{
		LLStringUtil::toUpper(mUpperFilter);
	}
}

FilterString::~FilterString()
{
}

const std::string& FilterString::get() const
{
	return mFilter;
}

bool FilterString::set(const std::string& pFilter)
{
	std::string newFilter(pFilter);
	LLStringUtil::trim(newFilter);
	bool didFilterChange = (mFilter.compare(newFilter) != 0);
	if (didFilterChange)
	{
		mFilter = newFilter;
		mUpperFilter = newFilter;
		LLStringUtil::toUpper(mUpperFilter);
	}

	return didFilterChange;
}

void FilterString::clear()
{
	mFilter.clear();
	mUpperFilter.clear();
}

bool FilterString::isActive() const
{
	return !mFilter.empty();
}

bool FilterString::doesMatch(const std::string& pTestString) const
{
	bool doesMatch = true;

	if (isActive())
	{
		std::string upperTestString(pTestString);
		LLStringUtil::toUpper(upperTestString);
		doesMatch = (upperTestString.find(mUpperFilter) != std::string::npos);
	}

	return doesMatch;
}

//---------------------------------------------------------------------------
// LLFilteredPathfindingLinksets
//---------------------------------------------------------------------------

LLFilteredPathfindingLinksets::LLFilteredPathfindingLinksets()
	: mAllLinksets(),
	mFilteredLinksets(),
	mIsFiltersDirty(false),
	mNameFilter(),
	mDescriptionFilter(),
	mIsWalkableFilter(true),
	mIsObstacleFilter(true),
	mIsIgnoredFilter(true)
{
}

LLFilteredPathfindingLinksets::LLFilteredPathfindingLinksets(const LLSD& pNavMeshData)
	: mAllLinksets(),
	mFilteredLinksets(),
	mIsFiltersDirty(false),
	mNameFilter(),
	mDescriptionFilter(),
	mIsWalkableFilter(true),
	mIsObstacleFilter(true),
	mIsIgnoredFilter(true)
{
	setPathfindingLinksets(pNavMeshData);
}

LLFilteredPathfindingLinksets::LLFilteredPathfindingLinksets(const LLFilteredPathfindingLinksets& pOther)
	: mAllLinksets(pOther.mAllLinksets),
	mFilteredLinksets(pOther.mFilteredLinksets),
	mIsFiltersDirty(pOther.mIsFiltersDirty),
	mNameFilter(pOther.mNameFilter),
	mDescriptionFilter(pOther.mDescriptionFilter),
	mIsWalkableFilter(pOther.mIsWalkableFilter),
	mIsObstacleFilter(pOther.mIsObstacleFilter),
	mIsIgnoredFilter(pOther.mIsIgnoredFilter)
{
}

LLFilteredPathfindingLinksets::~LLFilteredPathfindingLinksets()
{
	clearPathfindingLinksets();
}

void LLFilteredPathfindingLinksets::setPathfindingLinksets(const LLSD& pNavMeshData)
{
	clearPathfindingLinksets();

	for (LLSD::map_const_iterator navMeshDataIter = pNavMeshData.beginMap();
		navMeshDataIter != pNavMeshData.endMap(); ++navMeshDataIter)
	{
		const std::string& uuid(navMeshDataIter->first);
		const LLSD& linksetData = navMeshDataIter->second;
		LLPathfindingLinkset linkset(uuid, linksetData);

		mAllLinksets.insert(std::pair<std::string, LLPathfindingLinkset>(uuid, linkset));
	}

	mIsFiltersDirty = true;
}

void LLFilteredPathfindingLinksets::updatePathfindingLinksets(const LLSD& pNavMeshData)
{
	for (LLSD::map_const_iterator navMeshDataIter = pNavMeshData.beginMap();
		navMeshDataIter != pNavMeshData.endMap(); ++navMeshDataIter)
	{
		const std::string& uuid(navMeshDataIter->first);
		const LLSD& linksetData = navMeshDataIter->second;
		LLPathfindingLinkset linkset(uuid, linksetData);

		PathfindingLinksetMap::iterator linksetIter = mAllLinksets.find(uuid);
		if (linksetIter == mAllLinksets.end())
		{
			mAllLinksets.insert(std::pair<std::string, LLPathfindingLinkset>(uuid, linkset));
		}
		else
		{
			linksetIter->second = linkset;
		}
	}

	mIsFiltersDirty = true;
}

void LLFilteredPathfindingLinksets::clearPathfindingLinksets()
{
	mAllLinksets.clear();
	mFilteredLinksets.clear();
	mIsFiltersDirty = false;
}

const LLFilteredPathfindingLinksets::PathfindingLinksetMap& LLFilteredPathfindingLinksets::getAllLinksets() const
{
	return mAllLinksets;
}

const LLFilteredPathfindingLinksets::PathfindingLinksetMap& LLFilteredPathfindingLinksets::getFilteredLinksets()
{
	if (!isFiltersActive())
	{
		return mAllLinksets;
	}
	else
	{
		applyFilters();
		return mFilteredLinksets;
	}
}

BOOL LLFilteredPathfindingLinksets::isFiltersActive() const
{
	return (mNameFilter.isActive() || mDescriptionFilter.isActive() || !mIsWalkableFilter || !mIsObstacleFilter || !mIsIgnoredFilter);
}

void LLFilteredPathfindingLinksets::setNameFilter(const std::string& pNameFilter)
{
	mIsFiltersDirty = (mNameFilter.set(pNameFilter) || mIsFiltersDirty);
}

const std::string& LLFilteredPathfindingLinksets::getNameFilter() const
{
	return mNameFilter.get();
}

void LLFilteredPathfindingLinksets::setDescriptionFilter(const std::string& pDescriptionFilter)
{
	mIsFiltersDirty = (mDescriptionFilter.set(pDescriptionFilter) || mIsFiltersDirty);
}

const std::string& LLFilteredPathfindingLinksets::getDescriptionFilter() const
{
	return mDescriptionFilter.get();
}

void LLFilteredPathfindingLinksets::setWalkableFilter(BOOL pWalkableFilter)
{
	mIsFiltersDirty = (mIsFiltersDirty || (mIsWalkableFilter == pWalkableFilter));
	mIsWalkableFilter = pWalkableFilter;
}

BOOL LLFilteredPathfindingLinksets::isWalkableFilter() const
{
	return mIsWalkableFilter;
}

void LLFilteredPathfindingLinksets::setObstacleFilter(BOOL pObstacleFilter)
{
	mIsFiltersDirty = (mIsFiltersDirty || (mIsObstacleFilter == pObstacleFilter));
	mIsObstacleFilter = pObstacleFilter;
}

BOOL LLFilteredPathfindingLinksets::isObstacleFilter() const
{
	return mIsObstacleFilter;
}

void LLFilteredPathfindingLinksets::setIgnoredFilter(BOOL pIgnoredFilter)
{
	mIsFiltersDirty = (mIsFiltersDirty || (mIsIgnoredFilter == pIgnoredFilter));
	mIsIgnoredFilter = pIgnoredFilter;
}

BOOL LLFilteredPathfindingLinksets::isIgnoredFilter() const
{
	return mIsIgnoredFilter;
}

void LLFilteredPathfindingLinksets::clearFilters()
{
	mNameFilter.clear();
	mDescriptionFilter.clear();
	mIsWalkableFilter = true;
	mIsObstacleFilter = true;
	mIsIgnoredFilter = true;
	mIsFiltersDirty = false;
}

void LLFilteredPathfindingLinksets::applyFilters()
{
	mFilteredLinksets.clear();

	for (PathfindingLinksetMap::const_iterator linksetIter = mAllLinksets.begin();
		linksetIter != mAllLinksets.end(); ++linksetIter)
	{
		const std::string& uuid(linksetIter->first);
		const LLPathfindingLinkset& linkset(linksetIter->second);
		if (doesMatchFilters(linkset))
		{
			mFilteredLinksets.insert(std::pair<std::string, LLPathfindingLinkset>(uuid, linkset));
		}
	}

	mIsFiltersDirty = false;
}

BOOL LLFilteredPathfindingLinksets::doesMatchFilters(const LLPathfindingLinkset& pLinkset) const
{
	return (((mIsWalkableFilter && (pLinkset.getPathState() == LLPathfindingLinkset::kWalkable)) ||
			 (mIsObstacleFilter && (pLinkset.getPathState() == LLPathfindingLinkset::kObstacle)) ||
			 (mIsIgnoredFilter && (pLinkset.getPathState() == LLPathfindingLinkset::kIgnored))) &&
			(!mNameFilter.isActive() || mNameFilter.doesMatch(pLinkset.getName())) &&
			(!mDescriptionFilter.isActive() || mDescriptionFilter.doesMatch(pLinkset.getDescription())));
}
