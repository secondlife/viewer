/** 
 * @file llpanelplaceinfo.h
 * @brief Base class for place information in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLPANELPLACEINFO_H
#define LL_LLPANELPLACEINFO_H

#include "llpanel.h"

#include "v3dmath.h"
#include "lluuid.h"

#include "llremoteparcelrequest.h"

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
	enum INFO_TYPE
	{
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
	virtual void setInfoType(INFO_TYPE type);

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
	INFO_TYPE 				mInfoType;

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
