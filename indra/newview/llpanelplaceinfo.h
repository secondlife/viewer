/** 
 * @file llpanelplace.h
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

#include "lliconctrl.h"

#include "llremoteparcelrequest.h"

class LLButton;
class LLTextBox;
class LLLineEditor;
class LLTextEditor;
class LLTextureCtrl;
class LLInventoryItem;

class LLPanelPlaceInfo : public LLPanel, LLRemoteParcelInfoObserver
{
public:
	LLPanelPlaceInfo();
	/*virtual*/ ~LLPanelPlaceInfo();

	/*virtual*/ BOOL postBuild();

	void resetLocation();
		// Ignore all old location information, useful if you are 
		// recycling an existing dialog and need to clear it.

	/*virtual*/ void setParcelID(const LLUUID& parcel_id);
		// Sends a request for data about the given parcel, which will
		// only update the location if there is none already available.

	void displayItemInfo(const LLInventoryItem* pItem);
	/*virtual*/ void setErrorStatus(U32 status, const std::string& reason);

	void sendParcelInfoRequest();
	void displayParcelInfo(const LLVector3& pos_region,
						   const LLUUID& region_id,
						   const LLVector3d& pos_global);
	void nameUpdatedCallback(LLTextBox* text,
							 const std::string& first,
							 const std::string& last);

	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);

private:
	enum LANDMARK_INFO_TYPE
		{
			TITLE,
			NOTE
		};

	void onCommitTitleOrNote(LANDMARK_INFO_TYPE type);

	LLUUID			mParcelID;
	LLUUID			mRequestedID;
	LLUUID			mLandmarkID;
	LLVector3		mPosRegion;

	LLTextureCtrl*		mSnapshotCtrl;
	LLTextBox*			mRegionName;
	LLTextBox*			mParcelName;
	LLTextEditor*		mDescEditor;
	LLIconCtrl*			mRating;
	LLTextBox*			mOwner;
	LLTextBox*			mCreator;
	LLTextBox*			mCreated;
	LLLineEditor*		mTitleEditor;
	LLTextEditor*		mNotesEditor;
	LLTextBox*			mLocationEditor;	
	LLPanel*			mInfoPanel;
};

#endif // LL_LLPANELPLACEINFO_H
