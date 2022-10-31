/**
 * @file llpanelplaceprofile.h
 * @brief Displays place profile in Side Tray.
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

#ifndef LL_LLPANELPLACEPROFILE_H
#define LL_LLPANELPLACEPROFILE_H

#include "llpanelplaceinfo.h"

class LLAccordionCtrl;
class LLIconCtrl;
class LLTextEditor;

class LLPanelPlaceProfile : public LLPanelPlaceInfo
{
public:
    LLPanelPlaceProfile();
    /*virtual*/ ~LLPanelPlaceProfile();

    /*virtual*/ BOOL postBuild();

    /*virtual*/ void resetLocation();

    /*virtual*/ void setInfoType(EInfoType type);

    /*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);

    /*virtual*/ void onVisibilityChange(BOOL new_visibility);

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
    S32                 mSelectedParcelID;
    LLUUID              mLastSelectedRegionID;
    F64                 mNextCovenantUpdateTime;  //seconds since client start

    LLPanel*            mForSalePanel;
    LLPanel*            mYouAreHerePanel;

    LLIconCtrl*         mParcelRatingIcon;
    LLTextBox*          mParcelRatingText;
    LLIconCtrl*         mVoiceIcon;
    LLTextBox*          mVoiceText;
    LLIconCtrl*         mFlyIcon;
    LLTextBox*          mFlyText;
    LLIconCtrl*         mPushIcon;
    LLTextBox*          mPushText;
    LLIconCtrl*         mBuildIcon;
    LLTextBox*          mBuildText;
    LLIconCtrl*         mScriptsIcon;
    LLTextBox*          mScriptsText;
    LLIconCtrl*         mDamageIcon;
    LLTextBox*          mDamageText;
    LLIconCtrl*         mSeeAVsIcon;
    LLTextBox*          mSeeAVsText;

    LLTextBox*          mRegionNameText;
    LLTextBox*          mRegionTypeText;
    LLIconCtrl*         mRegionRatingIcon;
    LLTextBox*          mRegionRatingText;
    LLTextBox*          mRegionOwnerText;
    LLTextBox*          mRegionGroupText;

    LLTextBox*          mEstateNameText;
    LLTextBox*          mEstateRatingText;
    LLIconCtrl*         mEstateRatingIcon;
    LLTextBox*          mEstateOwnerText;
    LLTextEditor*       mCovenantText;

    LLTextBox*          mSalesPriceText;
    LLTextBox*          mAreaText;
    LLTextBox*          mTrafficText;
    LLTextBox*          mPrimitivesText;
    LLTextBox*          mParcelScriptsText;
    LLTextBox*          mTerraformLimitsText;
    LLTextEditor*       mSubdivideText;
    LLTextEditor*       mResaleText;
    LLTextBox*          mSaleToText;
    LLAccordionCtrl*    mAccordionCtrl;
};

#endif // LL_LLPANELPLACEPROFILE_H
