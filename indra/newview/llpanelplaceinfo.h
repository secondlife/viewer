/** 
 * @file llpanelplaceinfo.h
 * @brief Displays place information in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llpanelmedia.h"
#include "llremoteparcelrequest.h"

class LLButton;
class LLComboBox;
class LLInventoryItem;
class LLLineEditor;
class LLPanelPick;
class LLParcel;
class LLIconCtrl;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLViewerRegion;

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
	void resetLocation();

	// Sends a request for data about the given parcel, which will
	// only update the location if there is none already available.
	/*virtual*/ void setParcelID(const LLUUID& parcel_id);

	// Depending on how the panel was triggered 
	// (from landmark or current location, or other) 
	// sets a corresponding title and contents.
	void setInfoType(INFO_TYPE type);

	// Create a landmark for the current location
	// in a folder specified by folder_id.
	void createLandmark(const LLUUID& folder_id);
	
	// Create a pick for the location specified
	// by global_pos.
	void createPick(const LLVector3d& pos_global, LLPanelPick* pick_panel);

	BOOL isMediaPanelVisible();
	void toggleMediaPanel(BOOL visible);
	void displayItemInfo(const LLInventoryItem* pItem);
	/*virtual*/ void setErrorStatus(U32 status, const std::string& reason);

	void sendParcelInfoRequest();

	// Displays information about a remote parcel.
	// Sends a request to the server.
	void displayParcelInfo(const LLUUID& region_id,
						   const LLVector3d& pos_global);

	// Displays information about the currently selected parcel
	// without sending a request to the server.
	void displaySelectedParcelInfo(LLParcel* parcel,
								LLViewerRegion* region,
								const LLVector3d& pos_global);

	void updateEstateName(const std::string& name);
	void updateEstateOwnerName(const std::string& name);
	void updateCovenantText(const std::string &text);
	void updateLastVisitedText(const LLDate &date);

	void nameUpdatedCallback(LLTextBox* text,
							 const std::string& first,
							 const std::string& last);

	void toggleLandmarkEditMode(BOOL enabled);

	const std::string& getLandmarkTitle() const;
	const std::string getLandmarkNotes() const;
	const LLUUID getLandmarkFolder() const;

	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ void handleVisibilityChange (BOOL new_visibility);

private:

	void populateFoldersList();

	LLUUID			mParcelID;
	LLUUID			mRequestedID;
	LLUUID			mLandmarkID;
	LLVector3		mPosRegion;
	std::string		mCurrentTitle;
	S32				mMinHeight;
	INFO_TYPE 		mInfoType;

	LLTextBox*			mTitle;
	LLIconCtrl*			mForSaleIcon;
	LLTextureCtrl*		mSnapshotCtrl;
	LLTextBox*			mRegionName;
	LLTextBox*			mParcelName;
	LLTextEditor*		mDescEditor;
	LLTextBox*			mMaturityRatingText;
	LLTextBox*			mParcelOwner;
	LLTextBox*			mLastVisited;

	LLTextBox*			mRatingText;
	LLTextBox*			mVoiceText;
	LLTextBox*			mFlyText;
	LLTextBox*			mPushText;
	LLTextBox*			mBuildText;
	LLTextBox*			mScriptsText;
	LLTextBox*			mDamageText;

	LLTextBox*			mRegionNameText;
	LLTextBox*			mRegionTypeText;
	LLTextBox*			mRegionRatingText;
	LLTextBox*			mRegionOwnerText;
	LLTextBox*			mRegionGroupText;

	LLTextBox*			mEstateNameText;
	LLTextBox*			mEstateRatingText;
	LLTextBox*			mEstateOwnerText;
	LLTextEditor*		mCovenantText;

	LLTextBox*			mSalesPriceText;
	LLTextBox*			mAreaText;
	LLTextBox*			mTrafficText;
	LLTextBox*			mPrimitivesText;
	LLTextBox*			mParcelScriptsText;
	LLTextBox*			mTerraformLimitsText;
	LLTextEditor*		mSubdivideText;
	LLTextEditor*		mResaleText;
	LLTextBox*			mSaleToText;

	LLTextBox*			mOwner;
	LLTextBox*			mCreator;
	LLTextBox*			mCreated;
	LLLineEditor*		mTitleEditor;
	LLTextEditor*		mNotesEditor;
	LLComboBox*			mFolderCombo;
	LLPanel*            mScrollingPanel;
	LLPanel*			mInfoPanel;
	LLMediaPanel*		mMediaPanel;
};

#endif // LL_LLPANELPLACEINFO_H
