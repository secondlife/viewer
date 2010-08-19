/** 
 * @file llpanelplace.h
 * @brief Display of a place in the Find directory.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLPANELPLACE_H
#define LL_LLPANELPLACE_H

#include "llpanel.h"

#include "v3dmath.h"
#include "lluuid.h"

#include "llremoteparcelrequest.h"

class LLButton;
class LLTextBox;
class LLLineEditor;
class LLTextEditor;
class LLTextureCtrl;
class LLMessageSystem;
class LLInventoryItem;

class LLPanelPlace : public LLPanel, LLRemoteParcelInfoObserver
{
public:
	LLPanelPlace();
	/*virtual*/ ~LLPanelPlace();

	/*virtual*/ BOOL postBuild();

	void resetLocation();
		// Ignore all old location information, useful if you are 
		// recycling an existing dialog and need to clear it.

	/*virtual*/ void setParcelID(const LLUUID& parcel_id);
		// Sends a request for data about the given parcel, which will
		// only update the location if there is none already available.

	void displayItemInfo(const LLInventoryItem* pItem);
	void setRegionID(const LLUUID& region_id) { mRegionID = region_id; }
	void setSnapshot(const LLUUID& snapshot_id);
	void setLocationString(const std::string& location);
	void setLandTypeString(const std::string& land_type);
	/*virtual*/ void setErrorStatus(U32 status, const std::string& reason);
	void resetName(const std::string& name);

	void sendParcelInfoRequest();
	void displayParcelInfo(const LLVector3& pos_region,
			const LLUUID& landmark_asset_id,
			const LLUUID& region_id,
			const LLVector3d& pos_global);
	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);

	LLTextureCtrl *getSnapshotCtrl() const { return mSnapshotCtrl; }

protected:
	static void onClickTeleport(void* data);
	static void onClickMap(void* data);
	//static void onClickLandmark(void* data);
	static void onClickAuction(void* data);

	// Go to auction web page if user clicked OK
	static bool callbackAuctionWebPage(const LLSD& notification, const LLSD& response);

protected:
	LLUUID			mParcelID;
	LLUUID			mRequestedID;
	LLUUID			mRegionID;
	LLUUID			mLandmarkAssetID;
	// Absolute position of the location for teleport, may not
	// be available (hence zero)
	LLVector3d		mPosGlobal;
	// Region-local position for teleport, always available.
	LLVector3		mPosRegion;
	// Zero if this is not an auction
	S32				mAuctionID;

	LLTextureCtrl* mSnapshotCtrl;

	LLTextBox* mNameEditor;
	LLTextEditor* mDescEditor;
	LLTextBox* mInfoEditor;
	LLTextBox* mLandTypeEditor;
	LLTextBox* mLocationDisplay; //not calling it "editor" because it isn't

	LLButton*	mTeleportBtn;
	LLButton*	mMapBtn;
	//LLButton*	mLandmarkBtn;
	LLButton*	mAuctionBtn;
};

#endif // LL_LLPANELPLACE_H
