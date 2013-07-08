/** 
* @file   llfloatersocial.h
* @brief  Header file for llfloatersocial
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
#ifndef LL_LLFLOATERSOCIAL_H
#define LL_LLFLOATERSOCIAL_H

#include "llfloater.h"
#include "llloadingindicator.h"
#include "lltextbox.h"
#include "llviewertexture.h"

class LLIconCtrl;
class LLCheckBoxCtrl;
class LLSnapshotLivePreview;

class LLSocialStatusPanel : public LLPanel
{
public:
    LLSocialStatusPanel();
	BOOL postBuild();
	void draw();
    void onSend();
	bool onFacebookConnectStateChange(const LLSD& data);

	void sendStatus();
	void clearAndClose();

private:
	LLUICtrl* mMessageTextEditor;
	LLUICtrl* mPostStatusButton;
};

class LLSocialPhotoPanel : public LLPanel
{
public:
	LLSocialPhotoPanel();
	~LLSocialPhotoPanel();

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
	void setNeedRefresh(bool need);
	LLUICtrl* getRefreshBtn();

private:
	LLHandle<LLView> mPreviewHandle;

	LLUICtrl * mSnapshotPanel;
	LLUICtrl * mResolutionComboBox;
	LLUICtrl * mRefreshBtn;
	LLUICtrl * mRefreshLabel;
	LLLoadingIndicator * mWorkingIndicator;
	LLUICtrl * mWorkingLabel;
	LLUICtrl * mThumbnailPlaceholder;
	LLUICtrl * mCaptionTextBox;
	LLUICtrl * mLocationCheckbox;
	LLUICtrl * mPostButton;

	bool mNeedRefresh;
};

class LLSocialCheckinPanel : public LLPanel
{
public:
    LLSocialCheckinPanel();
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
    LLUICtrl* mMapLoadingIndicator;
    LLIconCtrl* mMapPlaceholder;
    LLCheckBoxCtrl* mMapCheckBox;
    bool mReloadingMapTexture;
    bool mMapCheckBoxValue;
};

class LLFloaterSocial : public LLFloater
{
public:
	LLFloaterSocial(const LLSD& key);
	BOOL postBuild();
	void draw();
	void onCancel();

	static void preUpdate();
	static void postUpdate();

private:
	LLSocialPhotoPanel* mSocialPhotoPanel;
    LLTextBox* mStatusErrorText;
    LLTextBox* mStatusLoadingText;
    LLUICtrl* mStatusLoadingIndicator;
};

#endif // LL_LLFLOATERSOCIAL_H

