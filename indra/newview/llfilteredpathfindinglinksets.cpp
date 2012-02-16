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
	mLinksetUseFilter(LLPathfindingLinkset::kUnknown)
{
}

LLFilteredPathfindingLinksets::LLFilteredPathfindingLinksets(const LLSD& pLinksetItems)
	: mAllLinksets(),
	mFilteredLinksets(),
	mIsFiltersDirty(false),
	mNameFilter(),
	mDescriptionFilter(),
	mLinksetUseFilter(LLPathfindingLinkset::kUnknown)
{
	setPathfindingLinksets(pLinksetItems);
}

LLFilteredPathfindingLinksets::LLFilteredPathfindingLinksets(const LLFilteredPathfindingLinksets& pOther)
	: mAllLinksets(pOther.mAllLinksets),
	mFilteredLinksets(pOther.mFilteredLinksets),
	mIsFiltersDirty(pOther.mIsFiltersDirty),
	mNameFilter(pOther.mNameFilter),
	mDescriptionFilter(pOther.mDescriptionFilter),
	mLinksetUseFilter(pOther.mLinksetUseFilter)
{
}

LLFilteredPathfindingLinksets::~LLFilteredPathfindingLinksets()
{
	clearPathfindingLinksets();
}

void LLFilteredPathfindingLinksets::setPathfindingLinksets(const LLSD& pLinksetItems)
{
	clearPathfindingLinksets();

	for (LLSD::map_const_iterator linksetItemIter = pLinksetItems.beginMap();
		linksetItemIter != pLinksetItems.endMap(); ++linksetItemIter)
	{
		const std::string& uuid(linksetItemIter->first);
		const LLSD& linksetData = linksetItemIter->second;
		LLPathfindingLinkset linkset(uuid, linksetData);

		mAllLinksets.insert(std::pair<std::string, LLPathfindingLinkset>(uuid, linkset));
	}

	mIsFiltersDirty = true;
}

void LLFilteredPathfindingLinksets::updatePathfindingLinksets(const LLSD& pLinksetItems)
{
	for (LLSD::map_const_iterator linksetItemIter = pLinksetItems.beginMap();
		linksetItemIter != pLinksetItems.endMap(); ++linksetItemIter)
	{
		const std::string& uuid(linksetItemIter->first);
		const LLSD& linksetData = linksetItemIter->second;
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
	return (mNameFilter.isActive() || mDescriptionFilter.isActive() ||
		(mLinksetUseFilter != LLPathfindingLinkset::kUnknown));
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

void LLFilteredPathfindingLinksets::setLinksetUseFilter(LLPathfindingLinkset::ELinksetUse pLinksetUse)
{
	mIsFiltersDirty = (mIsFiltersDirty || (mLinksetUseFilter == pLinksetUse));
	mLinksetUseFilter = pLinksetUse;
}

LLPathfindingLinkset::ELinksetUse LLFilteredPathfindingLinksets::getLinksetUseFilter() const
{
	return mLinksetUseFilter;
}

void LLFilteredPathfindingLinksets::clearFilters()
{
	mNameFilter.clear();
	mDescriptionFilter.clear();
	mLinksetUseFilter = LLPathfindingLinkset::kUnknown;
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
	return (((mLinksetUseFilter == LLPathfindingLinkset::kUnknown) || (mLinksetUseFilter == pLinkset.getLinksetUse())) &&
		(!mNameFilter.isActive() || mNameFilter.doesMatch(pLinkset.getName())) &&
		(!mDescriptionFilter.isActive() || mDescriptionFilter.doesMatch(pLinkset.getDescription())));
}
