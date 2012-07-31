/** 
* @file llpathfindingnavmeshstatus.cpp
* @brief Implementation of llpathfindingnavmeshstatus
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

#include "llpathfindingnavmeshstatus.h"

#include <string>

#include "llsd.h"
#include "llstring.h"
#include "lluuid.h"

#define REGION_FIELD  "region_id"
#define STATUS_FIELD  "status"
#define VERSION_FIELD "version"

const std::string LLPathfindingNavMeshStatus::sStatusPending("pending");
const std::string LLPathfindingNavMeshStatus::sStatusBuilding("building");
const std::string LLPathfindingNavMeshStatus::sStatusComplete("complete");
const std::string LLPathfindingNavMeshStatus::sStatusRepending("repending");


//---------------------------------------------------------------------------
// LLPathfindingNavMeshStatus
//---------------------------------------------------------------------------

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus()
	: mIsValid(false),
	mRegionUUID(),
	mVersion(0U),
	mStatus(kComplete)
{
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLUUID &pRegionUUID)
	: mIsValid(false),
	mRegionUUID(pRegionUUID),
	mVersion(0U),
	mStatus(kComplete)
{
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLUUID &pRegionUUID, const LLSD &pContent)
	: mIsValid(true),
	mRegionUUID(pRegionUUID),
	mVersion(0U),
	mStatus(kComplete)
{
	parseStatus(pContent);
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLSD &pContent)
	: mIsValid(true),
	mRegionUUID(),
	mVersion(0U),
	mStatus(kComplete)
{
	llassert(pContent.has(REGION_FIELD));
	llassert(pContent.get(REGION_FIELD).isUUID());
	mRegionUUID = pContent.get(REGION_FIELD).asUUID();

	parseStatus(pContent);
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLPathfindingNavMeshStatus &pOther)
	: mIsValid(pOther.mIsValid),
	mRegionUUID(pOther.mRegionUUID),
	mVersion(pOther.mVersion),
	mStatus(pOther.mStatus)
{
}

LLPathfindingNavMeshStatus::~LLPathfindingNavMeshStatus()
{
}

LLPathfindingNavMeshStatus &LLPathfindingNavMeshStatus::operator =(const LLPathfindingNavMeshStatus &pOther)
{
	mIsValid = pOther.mIsValid;
	mRegionUUID = pOther.mRegionUUID;
	mVersion = pOther.mVersion;
	mStatus = pOther.mStatus;

	return *this;
}

void LLPathfindingNavMeshStatus::parseStatus(const LLSD &pContent)
{
	llassert(pContent.has(VERSION_FIELD));
	llassert(pContent.get(VERSION_FIELD).isInteger());
	llassert(pContent.get(VERSION_FIELD).asInteger() >= 0);
	mVersion = static_cast<U32>(pContent.get(VERSION_FIELD).asInteger());

	llassert(pContent.has(STATUS_FIELD));
	llassert(pContent.get(STATUS_FIELD).isString());
	std::string status = pContent.get(STATUS_FIELD).asString();

	if (LLStringUtil::compareStrings(status, sStatusPending) == 0)
	{
		mStatus = kPending;
	}
	else if (LLStringUtil::compareStrings(status, sStatusBuilding) == 0)
	{
		mStatus = kBuilding;
	}
	else if (LLStringUtil::compareStrings(status, sStatusComplete) == 0)
	{
		mStatus = kComplete;
	}
	else if (LLStringUtil::compareStrings(status, sStatusRepending) == 0)
	{
		mStatus = kRepending;
	}
	else
	{
		mStatus = kComplete;
		llassert(0);
	}
}
