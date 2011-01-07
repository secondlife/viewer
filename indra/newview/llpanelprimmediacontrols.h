/** 
 * @file llpanelprimmediacontrols.h
 * @brief Pop-up media controls panel
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_PANELPRIMMEDIACONTROLS_H
#define LL_PANELPRIMMEDIACONTROLS_H

#include "llpanel.h"
#include "llviewermedia.h"
#include "llnotificationptr.h"

class LLButton;
class LLCoordWindow;
class LLIconCtrl;
class LLLayoutStack;
class LLProgressBar;
class LLSliderCtrl;
class LLViewerMediaImpl;
class LLWindowShade;

class LLPanelPrimMediaControls : public LLPanel
{
public:
	LLPanelPrimMediaControls();
	virtual ~LLPanelPrimMediaControls();
	/*virtual*/ BOOL postBuild();
	virtual void draw();
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	
	void updateShape();
	bool isMouseOver();
	
	void showNotification(LLNotificationPtr notify);
	void hideNotification();


	enum EZoomLevel
	{
		ZOOM_NONE = 0,
		ZOOM_FAR,
		ZOOM_MEDIUM,
		ZOOM_NEAR
	};
	
	EZoomLevel getZoomLevel() const { return mCurrentZoom; }
	void nextZoomLevel();
	void resetZoomLevel(bool reset_camera = true);
	void close();

	LLHandle<LLPanelPrimMediaControls>	getHandle() const { return mPanelHandle; }
	void setMediaFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);


	static const EZoomLevel kZoomLevels[];
	static const int kNumZoomLevels;
	
	enum EScrollDir
	{
		SCROLL_UP = 0,
		SCROLL_DOWN,
		SCROLL_LEFT,
		SCROLL_RIGHT,
		SCROLL_NONE
	};

private:
	void onClickClose();
	void onClickBack();
	void onClickForward();
	void onClickHome();
	void onClickOpen();
	void onClickReload();
	void onClickPlay();
	void onClickPause();
	void onClickStop();
	void onClickZoom();
	void onClickSkipBack();
	void onClickSkipForward();
	void onClickMediaStop();
	void onCommitURL();
	
	void updateZoom();
	void setCurrentURL();
	void onCommitSlider();
	
	void onCommitVolumeUp();
	void onCommitVolumeDown();
	void onCommitVolumeSlider();
	void onToggleMute();
	void showVolumeSlider();
	void hideVolumeSlider();
	bool shouldVolumeSliderBeVisible();
	
	static void onScrollUp(void* user_data);
	static void onScrollUpHeld(void* user_data);
	static void onScrollLeft(void* user_data);
	static void onScrollLeftHeld(void* user_data);
	static void onScrollRight(void* user_data);
	static void onScrollRightHeld(void* user_data);
	static void onScrollDown(void* user_data);
	static void onScrollDownHeld(void* user_data);
	static void onScrollStop(void* user_data);
	
	static void onInputURL(LLFocusableElement* caller, void *userdata);
	static bool hasControlsPermission(LLViewerObject *obj, const LLMediaEntry *media_entry);
	
	void focusOnTarget();
	
	LLViewerMediaImpl* getTargetMediaImpl();
	LLViewerObject* getTargetObject();
	LLPluginClassMedia* getTargetMediaPlugin();
	
private:

	void clearFaceOnFade();

	void onMouselookModeIn();

	LLView *mMediaRegion;
	LLUICtrl *mBackCtrl;
	LLUICtrl *mFwdCtrl;
	LLUICtrl *mReloadCtrl;
	LLUICtrl *mPlayCtrl;
	LLUICtrl *mPauseCtrl;
	LLUICtrl *mStopCtrl;
	LLUICtrl *mMediaStopCtrl;
	LLUICtrl *mHomeCtrl;
	LLUICtrl *mUnzoomCtrl;
	LLUICtrl *mOpenCtrl;
	LLUICtrl *mSkipBackCtrl;
	LLUICtrl *mSkipFwdCtrl;
	LLUICtrl *mZoomCtrl;
	LLPanel  *mMediaProgressPanel;
	LLProgressBar *mMediaProgressBar;
	LLUICtrl *mMediaAddressCtrl;
	LLUICtrl *mMediaAddress;
	LLUICtrl *mMediaPlaySliderPanel;
	LLUICtrl *mMediaPlaySliderCtrl;
	LLUICtrl *mVolumeCtrl;
	LLButton *mMuteBtn;
	LLSliderCtrl *mVolumeSliderCtrl;
	LLIconCtrl *mWhitelistIcon;
	LLIconCtrl *mSecureLockIcon;
	LLLayoutStack *mMediaControlsStack;
	LLUICtrl *mLeftBookend;
	LLUICtrl *mRightBookend;
	LLUIImage* mBackgroundImage;
	LLUIImage* mVolumeSliderBackgroundImage;
	LLWindowShade* mWindowShade;
	F32 mSkipStep;
	S32 mMinWidth;
	S32 mMinHeight;
	F32 mZoomNearPadding;
	F32 mZoomMediumPadding;
	F32 mZoomFarPadding;
	S32 mTopWorldViewAvoidZone;
	
	LLUICtrl *mMediaPanelScroll;
	LLButton *mScrollUpCtrl;
	LLButton *mScrollLeftCtrl;
	LLButton *mScrollRightCtrl;
	LLButton *mScrollDownCtrl;
	
	bool mPauseFadeout;
	bool mUpdateSlider;
	bool mClearFaceOnFade;
	bool mHideImmediately;

	LLMatrix4 mLastCameraMat;
	EZoomLevel mCurrentZoom;
	EScrollDir mScrollState;
	LLCoordWindow mLastCursorPos;
	LLFrameTimer mInactivityTimer;
	LLFrameTimer mFadeTimer;
	F32 mInactiveTimeout;
	F32 mControlFadeTime;
	LLRootHandle<LLPanelPrimMediaControls> mPanelHandle;
	F32 mAlpha;
	std::string mCurrentURL;
	std::string mPreviousURL;
	F64 mCurrentRate;
	F64 mMovieDuration;
	
	LLUUID mTargetObjectID;
	S32 mTargetObjectFace;
	LLUUID mTargetImplID;
	LLVector3 mTargetObjectNormal;
	
	LLUUID mZoomObjectID;
	S32 mZoomObjectFace;
	
	S32 mVolumeSliderVisible;

	LLNotificationPtr mActiveNotification;
};

#endif // LL_PANELPRIMMEDIACONTROLS_H
