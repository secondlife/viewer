/** 
* @file   llfloaterfacebook.h
* @brief  Header file for llfloaterfacebook
* @author Gilbert@lindenlab.com
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
#ifndef LL_LLFLOATERFACEBOOK_H
#define LL_LLFLOATERFACEBOOK_H

#include "llfloater.h"
#include "lltextbox.h"
#include "llviewertexture.h"

class LLIconCtrl;
class LLCheckBoxCtrl;
class LLSnapshotLivePreview;

class LLFacebookStatusPanel : public LLPanel
{
public:
    LLFacebookStatusPanel();
	BOOL postBuild();
	void draw();
    void onSend();
	bool onFacebookConnectStateChange(const LLSD& data);

	void sendStatus();
	void clearAndClose();

private:
	LLUICtrl* mMessageTextEditor;
	LLUICtrl* mPostButton;
	LLUICtrl* mCancelButton;
};

class LLFacebookPhotoPanel : public LLPanel
{
public:
	LLFacebookPhotoPanel();
	~LLFacebookPhotoPanel();

	BOOL postBuild();
	void draw();

	LLSnapshotLivePreview* getPreviewView();
	void onVisibilityChange(const LLSD& new_visibility);
	void onClickNewSnapshot();
	void onSend();
	bool onFacebookConnectStateChange(const LLSD& data);

	void sendPhoto();
	void clearAndClose();

	void updateControls();
	void updateResolution(BOOL do_update);
	void checkAspectRatio(S32 index);
	LLUICtrl* getRefreshBtn();

private:
	LLHandle<LLView> mPreviewHandle;

	LLUICtrl * mSnapshotPanel;
	LLUICtrl * mResolutionComboBox;
	LLUICtrl * mFilterComboBox;
	LLUICtrl * mRefreshBtn;
	LLUICtrl * mWorkingLabel;
	LLUICtrl * mThumbnailPlaceholder;
	LLUICtrl * mCaptionTextBox;
	LLUICtrl * mLocationCheckbox;
	LLUICtrl * mPostButton;
	LLUICtrl* mCancelButton;
    
    S32 mQuality;       // Compression quality
};

class LLFacebookCheckinPanel : public LLPanel
{
public:
    LLFacebookCheckinPanel();
	BOOL postBuild();
	void draw();
    void onSend();
	bool onFacebookConnectStateChange(const LLSD& data);

	void sendCheckin();
	void clearAndClose();

private:
    std::string mMapUrl;
    LLPointer<LLViewerFetchedTexture> mMapTexture;
	LLUICtrl* mPostButton;
	LLUICtrl* mCancelButton;
	LLUICtrl* mMessageTextEditor;
    LLUICtrl* mMapLoadingIndicator;
    LLIconCtrl* mMapPlaceholder;
    LLIconCtrl* mMapDefault;
    LLCheckBoxCtrl* mMapCheckBox;
    bool mReloadingMapTexture;
};

class LLFacebookAccountPanel : public LLPanel
{
public:
	LLFacebookAccountPanel();
	BOOL postBuild();
	void draw();

private:
	void onVisibilityChange(const LLSD& new_visibility);
	bool onFacebookConnectStateChange(const LLSD& data);
	bool onFacebookConnectInfoChange();
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


class LLFloaterFacebook : public LLFloater
{
public:
	LLFloaterFacebook(const LLSD& key);
	BOOL postBuild();
	void draw();
	void onCancel();
	
	void showPhotoPanel();

	static void preUpdate();
	static void postUpdate();

private:
	LLFacebookPhotoPanel* mFacebookPhotoPanel;
    LLTextBox* mStatusErrorText;
    LLTextBox* mStatusLoadingText;
    LLUICtrl*  mStatusLoadingIndicator;
};

#endif // LL_LLFLOATERFACEBOOK_H

