/** 
* @file   llfloaterflickr.h
* @brief  Header file for llfloaterflickr
* @author cho@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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
#ifndef LL_LLFLOATERFLICKR_H
#define LL_LLFLOATERFLICKR_H

#include "llfloater.h"
#include "lltextbox.h"
#include "llviewertexture.h"

class LLIconCtrl;
class LLCheckBoxCtrl;
class LLSnapshotLivePreview;
class LLFloaterBigPreview;

class LLFlickrPhotoPanel : public LLPanel
{
public:
	LLFlickrPhotoPanel();
	~LLFlickrPhotoPanel();

	BOOL postBuild();
	S32 notify(const LLSD& info);
	void draw();

	LLSnapshotLivePreview* getPreviewView();
	void onVisibilityChange(BOOL new_visibility);
	void onClickNewSnapshot();
    void onClickBigPreview();
	void onSend();
	bool onFlickrConnectStateChange(const LLSD& data);

	void sendPhoto();
	void clearAndClose();

	void updateControls();
	void updateResolution(BOOL do_update);
	void checkAspectRatio(S32 index);
	LLUICtrl* getRefreshBtn();

private:
    bool isPreviewVisible();
    void attachPreview();

	LLHandle<LLView> mPreviewHandle;

	LLUICtrl * mSnapshotPanel;
	LLUICtrl * mResolutionComboBox;
	LLUICtrl * mFilterComboBox;
	LLUICtrl * mRefreshBtn;
	LLUICtrl * mWorkingLabel;
	LLUICtrl * mThumbnailPlaceholder;
	LLUICtrl * mTitleTextBox;
	LLUICtrl * mDescriptionTextBox;
	LLUICtrl * mLocationCheckbox;
	LLUICtrl * mTagsTextBox;
	LLUICtrl * mRatingComboBox;
	LLUICtrl * mPostButton;
	LLUICtrl * mCancelButton;
	LLButton * mBtnPreview;

    LLFloaterBigPreview * mBigPreviewFloater;
};

class LLFlickrAccountPanel : public LLPanel
{
public:
	LLFlickrAccountPanel();
	BOOL postBuild();
	void draw();

private:
	void onVisibilityChange(BOOL new_visibility);
	bool onFlickrConnectStateChange(const LLSD& data);
	bool onFlickrConnectInfoChange();
	void onConnect();
	void onUseAnotherAccount();
	void onDisconnect();

	void showConnectButton();
	void hideConnectButton();
	void showDisconnectedLayout();
	void showConnectedLayout();

	LLTextBox * mAccountCaptionLabel;
	LLTextBox * mAccountNameLabel;
	LLUICtrl * mPanelButtons;
	LLUICtrl * mConnectButton;
	LLUICtrl * mDisconnectButton;
};


class LLFloaterFlickr : public LLFloater
{
public:
	LLFloaterFlickr(const LLSD& key);
	BOOL postBuild();
	void draw();
	void onClose(bool app_quitting);
	void onCancel();
	
	void showPhotoPanel();

private:
	LLFlickrPhotoPanel* mFlickrPhotoPanel;
    LLTextBox* mStatusErrorText;
    LLTextBox* mStatusLoadingText;
    LLUICtrl*  mStatusLoadingIndicator;
};

#endif // LL_LLFLOATERFLICKR_H

