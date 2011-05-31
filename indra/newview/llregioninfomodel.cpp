/** 
 * @file llregioninfomodel.cpp
 * @brief Region info model
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llregioninfomodel.h"

// libs
#include "message.h"

// viewers

void LLRegionInfoModel::reset()
{
	mSimAccess			= 0;
	mAgentLimit			= 0;

	mRegionFlags		= 0;
	mEstateID			= 0;
	mParentEstateID		= 0;

	mPricePerMeter		= 0;
	mRedirectGridX		= 0;
	mRedirectGridY		= 0;

	mBillableFactor		= 0.0f;
	mObjectBonusFactor	= 0.0f;
	mWaterHeight		= 0.0f;
	mTerrainRaiseLimit	= 0.0f;
	mTerrainLowerLimit	= 0.0f;
	mSunHour			= 0.0f;

	mUseEstateSun		= false;

	mSimType.clear();
	mSimName.clear();
}

LLRegionInfoModel::LLRegionInfoModel()
{
	reset();
}

boost::signals2::connection LLRegionInfoModel::setUpdateCallback(const update_signal_t::slot_type& cb)
{
	return mUpdateSignal.connect(cb);
}

void LLRegionInfoModel::update(LLMessageSystem* msg)
{
	reset();

	msg->getStringFast(_PREHASH_RegionInfo, _PREHASH_SimName, mSimName);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_EstateID, mEstateID);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_ParentEstateID, mParentEstateID);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_RegionFlags, mRegionFlags);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_SimAccess, mSimAccess);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_MaxAgents, mAgentLimit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_ObjectBonusFactor, mObjectBonusFactor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_BillableFactor, mBillableFactor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_WaterHeight, mWaterHeight);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainRaiseLimit, mTerrainRaiseLimit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainLowerLimit, mTerrainLowerLimit);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_PricePerMeter, mPricePerMeter);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridX, mRedirectGridX);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridY, mRedirectGridY);

	msg->getBOOL(_PREHASH_RegionInfo, _PREHASH_UseEstateSun, mUseEstateSun);

	// actually the "last set" sun hour, not the current sun hour. JC
	msg->getF32(_PREHASH_RegionInfo, _PREHASH_SunHour, mSunHour);

	// the only reasonable way to decide if we actually have any data is to
	// check to see if any of these fields have nonzero sizes
	if (msg->getSize(_PREHASH_RegionInfo2, _PREHASH_ProductSKU) > 0 ||
		msg->getSize(_PREHASH_RegionInfo2, "ProductName") > 0)
	{
		msg->getString(_PREHASH_RegionInfo2, "ProductName", mSimType);
	}

	// Let interested parties know that region info has been updated.
	mUpdateSignal();
}
