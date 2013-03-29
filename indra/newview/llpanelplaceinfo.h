/** 
 * @file llpanelplaceinfo.h
 * @brief Base class for place information in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELPLACEINFO_H
#define LL_LLPANELPLACEINFO_H

#include "llpanel.h"

#include "v3dmath.h"
#include "lluuid.h"

#include "llremoteparcelrequest.h"

class LLAvatarName;
class LLExpandableTextBox;
class LLIconCtrl;
class LLInventoryItem;
class LLPanelPickEdit;
class LLParcel;
class LLScrollContainer;
class LLTextBox;
class LLTextureCtrl;
class LLViewerRegion;
class LLViewerInventoryCategory;

class LLPanelPlaceInfo : public LLPanel, LLRemoteParcelInfoObserver
{
public:
	enum EInfoType
	{
		UNKNOWN,

		AGENT,
		CREATE_LANDMARK,
		LANDMARK,
		PLACE,
		TELEPORT_HISTORY
	};

	LLPanelPlaceInfo();
	/*virtual*/ ~LLPanelPlaceInfo();

	/*virtual*/ BOOL postBuild();

	// Ignore all old location information, useful if you are 
	// recycling an existing dialog and need to clear it.
	virtual void resetLocation();

	// Sends a request for data about the given parcel, which will
	// only update the location if there is none already available.
	/*virtual*/ void setParcelID(const LLUUID& parcel_id);

	// Depending on how the panel was triggered
	// (from landmark or current location, or other)
	// sets a corresponding title and contents.
	virtual void setInfoType(EInfoType type);

	// Requests remote parcel info by parcel ID.
	void sendParcelInfoRequest();

	// Displays information about a remote parcel.
	// Sends a request to the server.
	void displayParcelInfo(const LLUUID& region_id,
						   const LLVector3d& pos_global);

	/*virtual*/ void setErrorStatus(U32 status, const std::string& reason);

	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	// Create a pick for the location specified
	// by global_pos.
	void createPick(const LLVector3d& pos_global, LLPanelPickEdit* pick_panel);

protected:
	static void onNameCache(LLTextBox* text, const std::string& full_name);
	static void onAvatarNameCache(const LLUUID& agent_id,
								  const LLAvatarName& av_name,
								  LLTextBox* text);

	/**
	 * mParcelID is valid only for remote places, in other cases it's null. See resetLocation() 
	 */
	LLUUID					mParcelID;
	LLUUID					mRequestedID;
	LLVector3				mPosRegion;
	std::string				mParcelTitle; // used for pick title without coordinates
	std::string				mCurrentTitle;
	S32						mScrollingPanelMinHeight;
	S32						mScrollingPanelWidth;
	EInfoType 				mInfoType;

	LLScrollContainer*		mScrollContainer;
	LLPanel*				mScrollingPanel;
	LLTextBox*				mTitle;
	LLTextureCtrl*			mSnapshotCtrl;
	LLTextBox*				mRegionName;
	LLTextBox*				mParcelName;
	LLExpandableTextBox*	mDescEditor;
	LLIconCtrl*				mMaturityRatingIcon;
	LLTextBox*				mMaturityRatingText;
};

#endif // LL_LLPANELPLACEINFO_H
