/** 
* @file   llfloatertwitter.h
* @brief  Header file for llfloatertwitter
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
#ifndef LL_LLFLOATERTWITTER_H
#define LL_LLFLOATERTWITTER_H

#include "llfloater.h"
#include "lltextbox.h"
#include "llviewertexture.h"

class LLIconCtrl;
class LLCheckBoxCtrl;
class LLSnapshotLivePreview;

class LLTwitterPhotoPanel : public LLPanel
{
public:
	LLTwitterPhotoPanel();
	~LLTwitterPhotoPanel();

	BOOL postBuild();
	void draw();

	LLSnapshotLivePreview* getPreviewView();
	void onVisibilityChange(const LLSD& new_visibility);
	void onAddLocationToggled();
	void onAddPhotoToggled();
	void onClickNewSnapshot();
	void onSend();
	bool onTwitterConnectStateChange(const LLSD& data);

	void sendPhoto();
	void clearAndClose();

	void updateStatusTextLength(BOOL restore_old_status_text);
	void updateControls();
	void updateResolution(BOOL do_update);
	void checkAspectRatio(S32 index);
	LLUICtrl* getRefreshBtn();

private:
	LLHandle<LLView> mPreviewHandle;

	LLUICtrl * mSnapshotPanel;
	LLUICtrl * mResolutionComboBox;
	LLUICtrl * mRefreshBtn;
	LLUICtrl * mWorkingLabel;
	LLUICtrl * mThumbnailPlaceholder;
	LLUICtrl * mStatusCounterLabel;
	LLUICtrl * mStatusTextBox;
	LLUICtrl * mLocationCheckbox;
	LLUICtrl * mPhotoCheckbox;
	LLUICtrl * mPostButton;
	LLUICtrl* mCancelButton;

	std::string mOldStatusText;
};

class LLTwitterAccountPanel : public LLPanel
{
public:
	LLTwitterAccountPanel();
	BOOL postBuild();
	void draw();

private:
	void onVisibilityChange(const LLSD& new_visibility);
	bool onTwitterConnectStateChange(const LLSD& data);
	bool onTwitterConnectInfoChange();
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


class LLFloaterTwitter : public LLFloater
{
public:
	LLFloaterTwitter(const LLSD& key);
	BOOL postBuild();
	void draw();
	void onCancel();

	void showPhotoPanel();

	static void preUpdate();
	static void postUpdate();

private:
	LLTwitterPhotoPanel* mSocialPhotoPanel;
    LLTextBox* mStatusErrorText;
    LLTextBox* mStatusLoadingText;
    LLUICtrl*  mStatusLoadingIndicator;
};

#endif // LL_LLFLOATERTWITTER_H

