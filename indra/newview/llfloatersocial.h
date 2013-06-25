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
#include "llviewertexture.h"

#include "llsnapshotlivepreview.h"

class LLSocialStatusPanel : public LLPanel
{
public:
    LLSocialStatusPanel();
	BOOL postBuild();
	void draw();
    void onSend();

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
		void onSend();

		const LLRect& getThumbnailPlaceholderRect() { return mThumbnailPlaceholder->getRect(); }
		void onResolutionComboCommit();
		void onClickNewSnapshot();

		LLHandle<LLView> mPreviewHandle;

		void updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update = TRUE);
		void setNeedRefresh(bool need);
		void checkAspectRatio(LLFloaterSnapshot *view, S32 index);
		LLSnapshotLivePreview* getPreviewView();

		void updateControls();

		LLUICtrl * mResolutionComboBox;
		LLUICtrl *mRefreshBtn, *mRefreshLabel;
		LLUICtrl *mSucceessLblPanel, *mFailureLblPanel;
		LLUICtrl* mThumbnailPlaceholder;

		bool mNeedRefresh;
};

class LLSocialCheckinPanel : public LLPanel
{
public:
    LLSocialCheckinPanel();
    void onSend();
};

class LLFloaterSocial : public LLFloater
{
public:
	LLFloaterSocial(const LLSD& key);
	BOOL postBuild();
	void onCancel();
	void onOpen(const LLSD& key);
	/*virtual*/ void draw();


	static void preUpdate();
	static void postUpdate();

private:
	LLSocialPhotoPanel * mSocialPhotoPanel;
    std::string mMapUrl;
    LLPointer<LLViewerFetchedTexture> mMapTexture;
	LLPointer<LLUIImage> mMapPlaceholder;
    bool mReloadingMapTexture;
    bool mMapCheckBoxValue;
};

#endif // LL_LLFLOATERSOCIAL_H

