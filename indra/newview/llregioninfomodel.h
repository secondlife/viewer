/** 
 * @file llregioninfomodel.h
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

#ifndef LL_LLREGIONINFOMODEL_H
#define LL_LLREGIONINFOMODEL_H

class LLMessageSystem;

#include "llsingleton.h"

/**
 * Contains region info, notifies interested parties of its changes.
 */
class LLRegionInfoModel : public LLSingleton<LLRegionInfoModel>
{
	LOG_CLASS(LLRegionInfoModel);

public:
	typedef boost::signals2::signal<void()> update_signal_t;
	boost::signals2::connection setUpdateCallback(const update_signal_t::slot_type& cb);

	void sendRegionTerrain(const LLUUID& invoice) const; /// upload region terrain data

	bool getUseFixedSun() const;

	void setUseFixedSun(bool fixed);

	// *TODO: Add getters and make the data private.
	U8			mSimAccess;
	U8			mAgentLimit;

	U64			mRegionFlags;
	U32			mEstateID;
	U32			mParentEstateID;

	S32			mPricePerMeter;
	S32			mRedirectGridX;
	S32			mRedirectGridY;

	F32			mBillableFactor;
	F32			mObjectBonusFactor;
	F32			mWaterHeight;
	F32			mTerrainRaiseLimit;
	F32			mTerrainLowerLimit;
	F32			mSunHour; // 6..30

	BOOL		mUseEstateSun;

	std::string	mSimName;
	std::string	mSimType;

protected:
	friend class LLSingleton<LLRegionInfoModel>;
	friend class LLViewerRegion;

	LLRegionInfoModel();

	/**
	 * Refresh model with data from the incoming server message.
	 */
	void update(LLMessageSystem* msg);

private:
	void reset();

	// *FIXME: Duplicated code from LLPanelRegionInfo
	static void sendEstateOwnerMessage(
		LLMessageSystem* msg,
		const std::string& request,
		const LLUUID& invoice,
		const std::vector<std::string>& strings);

	update_signal_t mUpdateSignal;
};

#endif // LL_LLREGIONINFOMODEL_H
