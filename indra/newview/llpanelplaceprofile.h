/**
 * @file llpanelplaceprofile.h
 * @brief Displays place profile in Side Tray.
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

#ifndef LL_LLPANELPLACEPROFILE_H
#define LL_LLPANELPLACEPROFILE_H

#include "llpanelplaceinfo.h"

class LLIconCtrl;
class LLTextEditor;

class LLPanelPlaceProfile : public LLPanelPlaceInfo
{
public:
	LLPanelPlaceProfile();
	/*virtual*/ ~LLPanelPlaceProfile();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void resetLocation();

	/*virtual*/ void setInfoType(INFO_TYPE type);

	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);

	/*virtual*/ void handleVisibilityChange(BOOL new_visibility);

	// Displays information about the currently selected parcel
	// without sending a request to the server.
	// If is_current_parcel true shows "You Are Here" banner.
	void displaySelectedParcelInfo(LLParcel* parcel,
								   LLViewerRegion* region,
								   const LLVector3d& pos_global,
								   bool is_current_parcel);

	void updateEstateName(const std::string& name);
	void updateEstateOwnerName(const std::string& name);
	void updateCovenantText(const std::string &text);

private:
	void onForSaleBannerClick();

	static void updateYouAreHereBanner(void*);// added to gIdleCallbacks

	/**
	 * Holds last displayed parcel. Needed for YouAreHere banner.
	 */
	S32					mSelectedParcelID;
	LLUUID				mLastSelectedRegionID;

	LLPanel*			mForSalePanel;
	LLPanel*			mYouAreHerePanel;

	LLTextBox*			mParcelOwner;

	LLIconCtrl*			mParcelRatingIcon;
	LLTextBox*			mParcelRatingText;
	LLIconCtrl*			mVoiceIcon;
	LLTextBox*			mVoiceText;
	LLIconCtrl*			mFlyIcon;
	LLTextBox*			mFlyText;
	LLIconCtrl*			mPushIcon;
	LLTextBox*			mPushText;
	LLIconCtrl*			mBuildIcon;
	LLTextBox*			mBuildText;
	LLIconCtrl*			mScriptsIcon;
	LLTextBox*			mScriptsText;
	LLIconCtrl*			mDamageIcon;
	LLTextBox*			mDamageText;

	LLTextBox*			mRegionNameText;
	LLTextBox*			mRegionTypeText;
	LLIconCtrl*			mRegionRatingIcon;
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
};

#endif // LL_LLPANELPLACEPROFILE_H
