/** 
 * @file llpanelprimmediacontrols.cpp
 * @brief media controls popup panel
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llparcel.h"
#include "llpanel.h"
#include "llselectmgr.h"
#include "llmediaentry.h"
#include "llrender.h"
#include "lldrawable.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llface.h"
#include "llcombobox.h"
#include "lllayoutstack.h"
#include "llslider.h"
#include "llhudview.h"
#include "lliconctrl.h"
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llpanelprimmediacontrols.h"
#include "llpluginclassmedia.h"
#include "llprogressbar.h"
#include "llsliderctrl.h"
#include "llstring.h"
#include "llviewercontrol.h"
#include "llviewerdisplay.h"
#include "llviewerparcelmgr.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llvovolume.h"
#include "llweb.h"
#include "llwindow.h"
#include "llfloatertools.h"  // to enable hide if build tools are up

#if defined(LL_DARWIN) || (LL_WINDOWS && !LL_RELEASE_FOR_DOWNLOAD )
#define PER_MEDIA_VOLUME
#endif

// Functions pulled from pipeline.cpp
glh::matrix4f glh_get_current_modelview();
glh::matrix4f glh_get_current_projection();
// Functions pulled from llviewerdisplay.cpp
bool get_hud_matrices(glh::matrix4f &proj, glh::matrix4f &model);

// Warning: make sure these two match!
const LLPanelPrimMediaControls::EZoomLevel LLPanelPrimMediaControls::kZoomLevels[] = { ZOOM_NONE, ZOOM_MEDIUM };
const int LLPanelPrimMediaControls::kNumZoomLevels = 2;

//
// LLPanelPrimMediaControls
//

LLPanelPrimMediaControls::LLPanelPrimMediaControls() : 
	mAlpha(1.f),
	mCurrentURL(""),
	mPreviousURL(""),
	mPauseFadeout(false),
	mUpdateSlider(true),
	mClearFaceOnFade(false),
	mCurrentRate(0.0),
	mMovieDuration(0.0),
	mTargetObjectID(LLUUID::null),
	mTargetObjectFace(0),
	mTargetImplID(LLUUID::null),
	mTargetObjectNormal(LLVector3::zero),
	mZoomObjectID(LLUUID::null),
	mZoomObjectFace(0),
	mVolumeSliderVisible(0)
{
	mCommitCallbackRegistrar.add("MediaCtrl.Close",		boost::bind(&LLPanelPrimMediaControls::onClickClose, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Back",		boost::bind(&LLPanelPrimMediaControls::onClickBack, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Forward",	boost::bind(&LLPanelPrimMediaControls::onClickForward, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Home",		boost::bind(&LLPanelPrimMediaControls::onClickHome, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Stop",		boost::bind(&LLPanelPrimMediaControls::onClickStop, this));
	mCommitCallbackRegistrar.add("MediaCtrl.MediaStop",		boost::bind(&LLPanelPrimMediaControls::onClickMediaStop, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Reload",	boost::bind(&LLPanelPrimMediaControls::onClickReload, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Play",		boost::bind(&LLPanelPrimMediaControls::onClickPlay, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Pause",		boost::bind(&LLPanelPrimMediaControls::onClickPause, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Open",		boost::bind(&LLPanelPrimMediaControls::onClickOpen, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Zoom",		boost::bind(&LLPanelPrimMediaControls::onClickZoom, this));
	mCommitCallbackRegistrar.add("MediaCtrl.CommitURL",	boost::bind(&LLPanelPrimMediaControls::onCommitURL, this));
	mCommitCallbackRegistrar.add("MediaCtrl.JumpProgress",		boost::bind(&LLPanelPrimMediaControls::onCommitSlider, this));
	mCommitCallbackRegistrar.add("MediaCtrl.CommitVolumeUp",	boost::bind(&LLPanelPrimMediaControls::onCommitVolumeUp, this));
	mCommitCallbackRegistrar.add("MediaCtrl.CommitVolumeDown",	boost::bind(&LLPanelPrimMediaControls::onCommitVolumeDown, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Volume",	boost::bind(&LLPanelPrimMediaControls::onCommitVolumeSlider, this));
	mCommitCallbackRegistrar.add("MediaCtrl.ToggleMute",		boost::bind(&LLPanelPrimMediaControls::onToggleMute, this));
	mCommitCallbackRegistrar.add("MediaCtrl.ShowVolumeSlider",		boost::bind(&LLPanelPrimMediaControls::showVolumeSlider, this));
	mCommitCallbackRegistrar.add("MediaCtrl.HideVolumeSlider",		boost::bind(&LLPanelPrimMediaControls::hideVolumeSlider, this));
	mCommitCallbackRegistrar.add("MediaCtrl.SkipBack",		boost::bind(&LLPanelPrimMediaControls::onClickSkipBack, this));
	mCommitCallbackRegistrar.add("MediaCtrl.SkipForward",	boost::bind(&LLPanelPrimMediaControls::onClickSkipForward, this));
	
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_prim_media_controls.xml");
	mInactivityTimer.reset();
	mFadeTimer.stop();
	mCurrentZoom = ZOOM_NONE;
	mScrollState = SCROLL_NONE;

	mPanelHandle.bind(this);
	
	mInactiveTimeout = gSavedSettings.getF32("MediaControlTimeout");
	mControlFadeTime = gSavedSettings.getF32("MediaControlFadeTime");
}

LLPanelPrimMediaControls::~LLPanelPrimMediaControls()
{
}

BOOL LLPanelPrimMediaControls::postBuild()
{
	mMediaRegion			= getChild<LLView>("media_region");
	mBackCtrl				= getChild<LLUICtrl>("back");
	mFwdCtrl				= getChild<LLUICtrl>("fwd");
	mReloadCtrl				= getChild<LLUICtrl>("reload");
	mPlayCtrl				= getChild<LLUICtrl>("play");
	mPauseCtrl				= getChild<LLUICtrl>("pause");
	mStopCtrl				= getChild<LLUICtrl>("stop");
	mMediaStopCtrl			= getChild<LLUICtrl>("media_stop");
	mHomeCtrl				= getChild<LLUICtrl>("home");
	mUnzoomCtrl				= getChild<LLUICtrl>("close"); // This is actually "unzoom" 
	mOpenCtrl				= getChild<LLUICtrl>("new_window");
	mZoomCtrl				= getChild<LLUICtrl>("zoom_frame");
	mMediaProgressPanel		= getChild<LLPanel>("media_progress_indicator");
	mMediaProgressBar		= getChild<LLProgressBar>("media_progress_bar");
	mMediaAddressCtrl		= getChild<LLUICtrl>("media_address");
	mMediaAddress			= getChild<LLUICtrl>("media_address_url");
	mMediaPlaySliderPanel	= getChild<LLUICtrl>("media_play_position");
	mMediaPlaySliderCtrl	= getChild<LLUICtrl>("media_play_slider");
	mSkipFwdCtrl			= getChild<LLUICtrl>("skip_forward");
	mSkipBackCtrl			= getChild<LLUICtrl>("skip_back");
	mVolumeCtrl				= getChild<LLUICtrl>("media_volume");
	mMuteBtn				= getChild<LLButton>("media_mute_button");
	mVolumeSliderCtrl       = getChild<LLSliderCtrl>("volume_slider");
	mWhitelistIcon			= getChild<LLIconCtrl>("media_whitelist_flag");
	mSecureLockIcon			= getChild<LLIconCtrl>("media_secure_lock_flag");
	mMediaControlsStack		= getChild<LLLayoutStack>("media_controls");
	mLeftBookend			= getChild<LLUICtrl>("left_bookend");
	mRightBookend			= getChild<LLUICtrl>("right_bookend");
	mBackgroundImage		= LLUI::getUIImage(getString("control_background_image_name"));
	mVolumeSliderBackgroundImage		= LLUI::getUIImage(getString("control_background_image_name"));
	LLStringUtil::convertToF32(getString("skip_step"), mSkipStep);
	LLStringUtil::convertToS32(getString("min_width"), mMinWidth);
	LLStringUtil::convertToS32(getString("min_height"), mMinHeight);
	LLStringUtil::convertToF32(getString("zoom_near_padding"), mZoomNearPadding);
	LLStringUtil::convertToF32(getString("zoom_medium_padding"), mZoomMediumPadding);
	LLStringUtil::convertToF32(getString("zoom_far_padding"), mZoomFarPadding);
	LLStringUtil::convertToS32(getString("top_world_view_avoid_zone"), mTopWorldViewAvoidZone);

	// These are currently removed...but getChild creates a "dummy" widget.
	// This class handles them missing.
	mMediaPanelScroll		= findChild<LLUICtrl>("media_panel_scroll");
	mScrollUpCtrl			= findChild<LLButton>("scrollup");
	mScrollLeftCtrl			= findChild<LLButton>("scrollleft");
	mScrollRightCtrl		= findChild<LLButton>("scrollright");
	mScrollDownCtrl			= findChild<LLButton>("scrolldown");	
	
	if (mScrollUpCtrl)
	{
		mScrollUpCtrl->setClickedCallback(onScrollUp, this);
		mScrollUpCtrl->setHeldDownCallback(onScrollUpHeld, this);
		mScrollUpCtrl->setMouseUpCallback(onScrollStop, this);
	}
	if (mScrollLeftCtrl)
	{
		mScrollLeftCtrl->setClickedCallback(onScrollLeft, this);
		mScrollLeftCtrl->setHeldDownCallback(onScrollLeftHeld, this);
		mScrollLeftCtrl->setMouseUpCallback(onScrollStop, this);
	}
	if (mScrollRightCtrl)
	{
		mScrollRightCtrl->setClickedCallback(onScrollRight, this);
		mScrollRightCtrl->setHeldDownCallback(onScrollRightHeld, this);
		mScrollRightCtrl->setMouseUpCallback(onScrollStop, this);
	}
	if (mScrollDownCtrl)
	{
		mScrollDownCtrl->setClickedCallback(onScrollDown, this);
		mScrollDownCtrl->setHeldDownCallback(onScrollDownHeld, this);
		mScrollDownCtrl->setMouseUpCallback(onScrollStop, this);
	}
		
	mMediaAddress->setFocusReceivedCallback(boost::bind(&LLPanelPrimMediaControls::onInputURL, _1, this ));
	
	mCurrentZoom = ZOOM_NONE;
	// clicks on buttons do not remove keyboard focus from media
	setIsChrome(TRUE);
	return TRUE;
}

void LLPanelPrimMediaControls::setMediaFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal)
{
	if (media_impl.notNull() && objectp.notNull())
	{
		LLUUID prev_id = mTargetImplID;
		mTargetImplID = media_impl->getMediaTextureID();
		mTargetObjectID = objectp->getID();
		mTargetObjectFace = face;
		mTargetObjectNormal = pick_normal;
		mClearFaceOnFade = false;
		
		if (prev_id != mTargetImplID)
			mVolumeSliderCtrl->setValue(media_impl->getVolume());
	}
	else
	{
		// This happens on a timer now.
//		mTargetImplID = LLUUID::null;
//		mTargetObjectID = LLUUID::null;
//		mTargetObjectFace = 0;
		mClearFaceOnFade = true;
	}

	updateShape();
}

void LLPanelPrimMediaControls::focusOnTarget()
{
	// Sets the media focus to the current target of the LLPanelPrimMediaControls.
	// This is how we transition from hover to focus when the user clicks on a control.
	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	if(media_impl)
	{
		if(!media_impl->hasFocus())
		{	
			// The current target doesn't have media focus -- focus on it.
			LLViewerObject* objectp = getTargetObject();
			LLViewerMediaFocus::getInstance()->setFocusFace(objectp, mTargetObjectFace, media_impl, mTargetObjectNormal);
		}
	}	
}

LLViewerMediaImpl* LLPanelPrimMediaControls::getTargetMediaImpl()
{
	return LLViewerMedia::getMediaImplFromTextureID(mTargetImplID);
}

LLViewerObject* LLPanelPrimMediaControls::getTargetObject()
{
	return gObjectList.findObject(mTargetObjectID);
}

LLPluginClassMedia* LLPanelPrimMediaControls::getTargetMediaPlugin()
{
	LLViewerMediaImpl* impl = getTargetMediaImpl();
	if(impl && impl->hasMedia())
	{
		return impl->getMediaPlugin();
	}
	
	return NULL;
}

void LLPanelPrimMediaControls::updateShape()
{
	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	LLViewerObject* objectp = getTargetObject();
	
	if(!media_impl || gFloaterTools->getVisible())
	{
		setVisible(FALSE);
		return;
	}

	LLPluginClassMedia* media_plugin = NULL;
	if(media_impl->hasMedia())
	{
		media_plugin = media_impl->getMediaPlugin();
	}
	
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	bool can_navigate = parcel->getMediaAllowNavigate();
	bool enabled = false;
	bool is_zoomed = (mCurrentZoom != ZOOM_NONE) && (mTargetObjectID == mZoomObjectID) && (mTargetObjectFace == mZoomObjectFace);
	// There is no such thing as "has_focus" being different from normal controls set
	// anymore (as of user feedback from bri 10/09).  So we cheat here and force 'has_focus'
	// to 'true' (or, actually, we use a setting)
	bool has_focus = (gSavedSettings.getBOOL("PrimMediaControlsUseHoverControlSet")) ? media_impl->hasFocus() : true;
	setVisible(enabled);

	if (objectp)
	{
		bool mini_controls = false;
		LLMediaEntry *media_data = objectp->getTE(mTargetObjectFace)->getMediaData();
		if (media_data && NULL != dynamic_cast<LLVOVolume*>(objectp))
		{
			// Don't show the media controls if we do not have permissions
			enabled = dynamic_cast<LLVOVolume*>(objectp)->hasMediaPermission(media_data, LLVOVolume::MEDIA_PERM_CONTROL);
			mini_controls = (LLMediaEntry::MINI == media_data->getControls());
		}
		const bool is_hud = objectp->isHUDAttachment();
		
		//
		// Set the state of the buttons
		//
		
		// XXX RSP: TODO: FIXME: clean this up so that it is clearer what mode we are in,
		// and that only the proper controls get made visible/enabled according to that mode. 
		mBackCtrl->setVisible(has_focus);
		mFwdCtrl->setVisible(has_focus);
		mReloadCtrl->setVisible(has_focus);
		mStopCtrl->setVisible(false);
		mHomeCtrl->setVisible(has_focus);
		mZoomCtrl->setVisible(!is_zoomed);
		mUnzoomCtrl->setVisible(is_zoomed);
		mOpenCtrl->setVisible(true);
		mMediaAddressCtrl->setVisible(has_focus && !mini_controls);
		mMediaPlaySliderPanel->setVisible(has_focus && !mini_controls);
		mVolumeCtrl->setVisible(false);
		
		mWhitelistIcon->setVisible(!mini_controls && (media_data)?media_data->getWhiteListEnable():false);
		// Disable zoom if HUD
		mZoomCtrl->setEnabled(!is_hud);
		mUnzoomCtrl->setEnabled(!is_hud);
		mSecureLockIcon->setVisible(false);
		mCurrentURL = media_impl->getCurrentMediaURL();
		
		mBackCtrl->setEnabled((media_impl != NULL) && media_impl->canNavigateBack() && can_navigate);
		mFwdCtrl->setEnabled((media_impl != NULL) && media_impl->canNavigateForward() && can_navigate);
		mStopCtrl->setEnabled(has_focus && can_navigate);
		mHomeCtrl->setEnabled(has_focus && can_navigate);
		LLPluginClassMediaOwner::EMediaStatus result = ((media_impl != NULL) && media_impl->hasMedia()) ? media_plugin->getStatus() : LLPluginClassMediaOwner::MEDIA_NONE;
		
		if(media_plugin && media_plugin->pluginSupportsMediaTime())
		{
			mReloadCtrl->setEnabled(false);
			mReloadCtrl->setVisible(false);
			mMediaStopCtrl->setVisible(has_focus);
			mHomeCtrl->setVisible(has_focus);
			mBackCtrl->setVisible(false);
			mFwdCtrl->setVisible(false);
			mMediaAddressCtrl->setVisible(false);
			mMediaAddressCtrl->setEnabled(false);
			mMediaPlaySliderPanel->setVisible(has_focus && !mini_controls);
			mMediaPlaySliderPanel->setEnabled(has_focus && !mini_controls);
			mSkipFwdCtrl->setVisible(has_focus && !mini_controls);
			mSkipFwdCtrl->setEnabled(has_focus && !mini_controls);
			mSkipBackCtrl->setVisible(has_focus && !mini_controls);
			mSkipBackCtrl->setEnabled(has_focus && !mini_controls);
			
			mVolumeCtrl->setVisible(has_focus);
			mVolumeCtrl->setEnabled(has_focus);
			mVolumeSliderCtrl->setEnabled(has_focus && shouldVolumeSliderBeVisible());
			mVolumeSliderCtrl->setVisible(has_focus && shouldVolumeSliderBeVisible());
			
			mWhitelistIcon->setVisible(false);
			mSecureLockIcon->setVisible(false);
			if (mMediaPanelScroll)
			{
				mMediaPanelScroll->setVisible(false);
				mScrollUpCtrl->setVisible(false);
				mScrollDownCtrl->setVisible(false);
				mScrollRightCtrl->setVisible(false);
				mScrollDownCtrl->setVisible(false);
			}
			
			F32 volume = media_impl->getVolume();
			// movie's url changed
			if(mCurrentURL!=mPreviousURL)
			{
				mMovieDuration = media_plugin->getDuration();
				mPreviousURL = mCurrentURL;
			}
			
			if(mMovieDuration == 0) 
			{
				mMovieDuration = media_plugin->getDuration();
				mMediaPlaySliderCtrl->setValue(0);
				mMediaPlaySliderCtrl->setEnabled(false);
			}
			// TODO: What if it's not fully loaded
			
			if(mUpdateSlider && mMovieDuration!= 0)
			{
				F64 current_time =  media_plugin->getCurrentTime();
				F32 percent = current_time / mMovieDuration;
				mMediaPlaySliderCtrl->setValue(percent);
				mMediaPlaySliderCtrl->setEnabled(true);
			}
			
			// video vloume
			if(volume <= 0.0)
			{
				mMuteBtn->setToggleState(true);
			}
			else if (volume >= 1.0)
			{
				mMuteBtn->setToggleState(false);
			}
			else
			{
				mMuteBtn->setToggleState(false);
			}
			
			switch(result)
			{
				case LLPluginClassMediaOwner::MEDIA_PLAYING:
					mPlayCtrl->setEnabled(FALSE);
					mPlayCtrl->setVisible(FALSE);
					mPauseCtrl->setEnabled(TRUE);
					mPauseCtrl->setVisible(has_focus);
					
					break;
				case LLPluginClassMediaOwner::MEDIA_PAUSED:
				default:
					mPauseCtrl->setEnabled(FALSE);
					mPauseCtrl->setVisible(FALSE);
					mPlayCtrl->setEnabled(TRUE);
					mPlayCtrl->setVisible(has_focus);
					break;
			}
		}
		else   // web based
		{
			if(media_plugin)
			{
				mCurrentURL = media_plugin->getLocation();
			}
			else
			{
				mCurrentURL.clear();
			}
			
			mPlayCtrl->setVisible(FALSE);
			mPauseCtrl->setVisible(FALSE);
			mMediaStopCtrl->setVisible(FALSE);
			mMediaAddressCtrl->setVisible(has_focus && !mini_controls);
			mMediaAddressCtrl->setEnabled(has_focus && !mini_controls);
			mMediaPlaySliderPanel->setVisible(FALSE);
			mMediaPlaySliderPanel->setEnabled(FALSE);
			mSkipFwdCtrl->setVisible(FALSE);
			mSkipFwdCtrl->setEnabled(FALSE);
			mSkipBackCtrl->setVisible(FALSE);
			mSkipBackCtrl->setEnabled(FALSE);
			
#ifdef PER_MEDIA_VOLUME
			mVolumeCtrl->setVisible(has_focus);
			mVolumeCtrl->setEnabled(has_focus);
			mVolumeSliderCtrl->setEnabled(has_focus && shouldVolumeSliderBeVisible());
			mVolumeSliderCtrl->setVisible(has_focus && shouldVolumeSliderBeVisible());
#else
			mVolumeCtrl->setVisible(FALSE);
			mVolumeSliderCtrl->setVisible(FALSE);
			mVolumeCtrl->setEnabled(FALSE);
			mVolumeSliderCtrl->setEnabled(FALSE);
#endif

			if (mMediaPanelScroll)
			{
				mMediaPanelScroll->setVisible(has_focus);
				mScrollUpCtrl->setVisible(has_focus);
				mScrollDownCtrl->setVisible(has_focus);
				mScrollRightCtrl->setVisible(has_focus);
				mScrollDownCtrl->setVisible(has_focus);
			}
			// TODO: get the secure lock bool from media plug in
			std::string prefix =  std::string("https://");
			std::string test_prefix = mCurrentURL.substr(0, prefix.length());
			LLStringUtil::toLower(test_prefix);
			if(test_prefix == prefix)
			{
				mSecureLockIcon->setVisible(has_focus);
			}
			
			if(mCurrentURL!=mPreviousURL)
			{
				setCurrentURL();
				mPreviousURL = mCurrentURL;
			}
			
			if(result == LLPluginClassMediaOwner::MEDIA_LOADING)
			{
				mReloadCtrl->setEnabled(FALSE);
				mReloadCtrl->setVisible(FALSE);
				mStopCtrl->setEnabled(TRUE);
				mStopCtrl->setVisible(has_focus);
			}
			else
			{
				mReloadCtrl->setEnabled(TRUE);
				mReloadCtrl->setVisible(has_focus);
				mStopCtrl->setEnabled(FALSE);
				mStopCtrl->setVisible(FALSE);
			}
		}
		
		
		if(media_plugin)
		{
			//
			// Handle progress bar
			//
			if(LLPluginClassMediaOwner::MEDIA_LOADING == media_plugin->getStatus())
			{	
				mMediaProgressPanel->setVisible(true);
				mMediaProgressBar->setPercent(media_plugin->getProgressPercent());
			}
			else
			{
				mMediaProgressPanel->setVisible(false);
			}
		}
		
		if(media_impl)
		{
			//
			// Handle Scrolling
			//
			switch (mScrollState) 
			{
				case SCROLL_UP:
					media_impl->scrollWheel(0, -1, MASK_NONE);
					break;
				case SCROLL_DOWN:
					media_impl->scrollWheel(0, 1, MASK_NONE);
					break;
				case SCROLL_LEFT:
					media_impl->scrollWheel(1, 0, MASK_NONE);
					//				media_impl->handleKeyHere(KEY_LEFT, MASK_NONE);
					break;
				case SCROLL_RIGHT:
					media_impl->scrollWheel(-1, 0, MASK_NONE);
					//				media_impl->handleKeyHere(KEY_RIGHT, MASK_NONE);
					break;
				case SCROLL_NONE:
				default:
					break;
			}
		}
		
		setVisible(enabled);
		
		//
		// Calculate position and shape of the controls
		//
		std::vector<LLVector3>::iterator vert_it;
		std::vector<LLVector3>::iterator vert_end;
		std::vector<LLVector3> vect_face;
		
		LLVolume* volume = objectp->getVolume();
		
		if (volume)
		{
			const LLVolumeFace& vf = volume->getVolumeFace(mTargetObjectFace);
			
			const LLVector3* ext = vf.mExtents;
			
			LLVector3 center = (ext[0]+ext[1])*0.5f;
			LLVector3 size = (ext[1]-ext[0])*0.5f;
			LLVector3 vert[] =
			{
				center + size.scaledVec(LLVector3(1,1,1)),
				center + size.scaledVec(LLVector3(-1,1,1)),
				center + size.scaledVec(LLVector3(1,-1,1)),
				center + size.scaledVec(LLVector3(-1,-1,1)),
				center + size.scaledVec(LLVector3(1,1,-1)),
				center + size.scaledVec(LLVector3(-1,1,-1)),
				center + size.scaledVec(LLVector3(1,-1,-1)),
				center + size.scaledVec(LLVector3(-1,-1,-1)),
			};
			
			LLVOVolume* vo = (LLVOVolume*) objectp;
			
			for (U32 i = 0; i < 8; i++)
			{
				vect_face.push_back(vo->volumePositionToAgent(vert[i]));
			}
		}
		vert_it = vect_face.begin();
		vert_end = vect_face.end();
		
		glh::matrix4f mat;
		if (!is_hud) 
		{
			mat = glh_get_current_projection() * glh_get_current_modelview();
		}
		else {
			glh::matrix4f proj, modelview;
			if (get_hud_matrices(proj, modelview))
				mat = proj * modelview;
		}
		LLVector3 min = LLVector3(1,1,1);
		LLVector3 max = LLVector3(-1,-1,-1);
		for(; vert_it != vert_end; ++vert_it)
		{
			// project silhouette vertices into screen space
			glh::vec3f screen_vert = glh::vec3f(vert_it->mV); 
			mat.mult_matrix_vec(screen_vert);
			
			// add to screenspace bounding box
			update_min_max(min, max, LLVector3(screen_vert.v));
		}
		
		// convert screenspace bbox to pixels (in screen coords)
		LLRect window_rect = gViewerWindow->getWorldViewRectScaled();
		LLCoordGL screen_min;
		screen_min.mX = llround((F32)window_rect.getWidth() * (min.mV[VX] + 1.f) * 0.5f);
		screen_min.mY = llround((F32)window_rect.getHeight() * (min.mV[VY] + 1.f) * 0.5f);
		
		LLCoordGL screen_max;
		screen_max.mX = llround((F32)window_rect.getWidth() * (max.mV[VX] + 1.f) * 0.5f);
		screen_max.mY = llround((F32)window_rect.getHeight() * (max.mV[VY] + 1.f) * 0.5f);
		
		// grow panel so that screenspace bounding box fits inside "media_region" element of panel
		LLRect media_panel_rect;
		// Get the height of the controls (less the volume slider)
		S32 controls_height = mMediaControlsStack->getRect().getHeight() - mVolumeSliderCtrl->getRect().getHeight();
		getParent()->screenRectToLocal(LLRect(screen_min.mX, screen_max.mY, screen_max.mX, screen_min.mY), &media_panel_rect);
		media_panel_rect.mTop += controls_height;
		
		// keep all parts of panel on-screen
		// Area of the top of the world view to avoid putting the controls
		window_rect.mTop -= mTopWorldViewAvoidZone;
		// Don't include "spacing" bookends on left & right of the media controls
		window_rect.mLeft -= mLeftBookend->getRect().getWidth();
		window_rect.mRight += mRightBookend->getRect().getWidth();
		// Don't include the volume slider
		window_rect.mBottom -= mVolumeSliderCtrl->getRect().getHeight();
		media_panel_rect.intersectWith(window_rect);
		
		// clamp to minimum size, keeping rect inside window
		S32 centerX = media_panel_rect.getCenterX();
		S32 centerY = media_panel_rect.getCenterY();
		// Shrink screen rect by min width and height, to ensure containment
		window_rect.stretch(-mMinWidth/2, -mMinHeight/2);
		window_rect.clampPointToRect(centerX, centerY);
		media_panel_rect.setCenterAndSize(centerX, centerY, 
										  llmax(mMinWidth, media_panel_rect.getWidth()),
										  llmax(mMinHeight, media_panel_rect.getHeight()));
		
		// Finally set the size of the panel
		setShape(media_panel_rect, true);
		
		// Test mouse position to see if the cursor is stationary
		LLCoordWindow cursor_pos_window;
		getWindow()->getCursorPosition(&cursor_pos_window);
		
		// If last pos is not equal to current pos, the mouse has moved
		// We need to reset the timer, and make sure the panel is visible
		if(cursor_pos_window.mX != mLastCursorPos.mX ||
		   cursor_pos_window.mY != mLastCursorPos.mY ||
		   mScrollState != SCROLL_NONE)
		{
			mInactivityTimer.start();
			mLastCursorPos = cursor_pos_window;
		}
		
		if(isMouseOver() || hasFocus())
		{
			// Never fade the controls if the mouse is over them or they have keyboard focus.
			mFadeTimer.stop();
		}
		else if(!mClearFaceOnFade && (mInactivityTimer.getElapsedTimeF32() < mInactiveTimeout))
		{
			// Mouse is over the object, but has not been stationary for long enough to fade the UI
			mFadeTimer.stop();
		}
		else if(! mFadeTimer.getStarted() )
		{
			// we need to start fading the UI (and we have not already started)
			mFadeTimer.reset();
			mFadeTimer.start();
		}
		else
		{
			// I don't think this is correct anymore.  This is done in draw() after the fade has completed.
			//			setVisible(FALSE);
		}
	}
}

/*virtual*/
void LLPanelPrimMediaControls::draw()
{
	F32 alpha = getDrawContext().mAlpha;
	if(mFadeTimer.getStarted())
	{
		F32 time = mFadeTimer.getElapsedTimeF32();
		alpha *= llmax(lerp(1.0, 0.0, time / mControlFadeTime), 0.0f);

		if(time >= mControlFadeTime)
		{
			if(mClearFaceOnFade)
			{
				// Hiding this object makes scroll events go missing after it fades out 
				// (see DEV-41755 for a full description of the train wreck).
				// Only hide the controls when we're untargeting.
				setVisible(FALSE);

				mClearFaceOnFade = false;
				mVolumeSliderVisible = 0;
				mTargetImplID = LLUUID::null;
				mTargetObjectID = LLUUID::null;
				mTargetObjectFace = 0;
			}
		}
	}
	
	// Build rect for icon area in coord system of this panel
	// Assumes layout_stack is a direct child of this panel
	mMediaControlsStack->updateLayout();
	
	// adjust for layout stack spacing
	S32 space = mMediaControlsStack->getPanelSpacing() + 2;
	LLRect controls_bg_area = mMediaControlsStack->getRect();
	
	controls_bg_area.mTop += space + 2;
	
	// adjust to ignore space from volume slider
	controls_bg_area.mBottom += mVolumeSliderCtrl->getRect().getHeight();
	
	// adjust to ignore space from left bookend padding
	controls_bg_area.mLeft += mLeftBookend->getRect().getWidth() - space;
	
	// ignore space from right bookend padding
	controls_bg_area.mRight -= mRightBookend->getRect().getWidth() - space - 2;
		
	// draw control background UI image
	mBackgroundImage->draw( controls_bg_area, UI_VERTEX_COLOR % alpha);
	
	// draw volume slider background UI image
	if (mVolumeSliderCtrl->getVisible())
	{
		LLRect volume_slider_rect;
		screenRectToLocal(mVolumeSliderCtrl->calcScreenRect(), &volume_slider_rect);
		mVolumeSliderBackgroundImage->draw(volume_slider_rect, UI_VERTEX_COLOR % alpha);
	}
	
	{
		LLViewDrawContext context(alpha);
		LLPanel::draw();
	}
}

BOOL LLPanelPrimMediaControls::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	mInactivityTimer.start();
	return LLViewerMediaFocus::getInstance()->handleScrollWheel(x, y, clicks);
}

BOOL LLPanelPrimMediaControls::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mInactivityTimer.start();
	return LLPanel::handleMouseDown(x, y, mask);
}

BOOL LLPanelPrimMediaControls::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mInactivityTimer.start();
	return LLPanel::handleMouseUp(x, y, mask);
}

BOOL LLPanelPrimMediaControls::handleKeyHere( KEY key, MASK mask )
{
	mInactivityTimer.start();
	return LLPanel::handleKeyHere(key, mask);
}

bool LLPanelPrimMediaControls::isMouseOver()
{
	bool result = false;
	
	if( getVisible() )
	{
		LLCoordWindow cursor_pos_window;
		LLCoordScreen cursor_pos_screen;
		LLCoordGL cursor_pos_gl;
		S32 x, y;
		getWindow()->getCursorPosition(&cursor_pos_window);
		getWindow()->convertCoords(cursor_pos_window, &cursor_pos_gl);
				
		if(mMediaControlsStack->getVisible())
		{
			mMediaControlsStack->screenPointToLocal(cursor_pos_gl.mX, cursor_pos_gl.mY, &x, &y);

			LLView *hit_child = mMediaControlsStack->childFromPoint(x, y);
			if(hit_child && hit_child->getVisible())
			{
				// This was useful for debugging both coordinate translation and view hieararchy problems...
				// llinfos << "mouse coords: " << x << ", " << y << " hit child " << hit_child->getName() << llendl;

				// This will be a direct child of the LLLayoutStack, which should be a layout_panel.
				// These may not shown/hidden by the logic in updateShape(), so we need to do another hit test on the children of the layout panel,
				// which are the actual controls.
				hit_child->screenPointToLocal(cursor_pos_gl.mX, cursor_pos_gl.mY, &x, &y);
				
				LLView *hit_child_2 = hit_child->childFromPoint(x, y);
				if(hit_child_2 && hit_child_2->getVisible())
				{
					// This was useful for debugging both coordinate translation and view hieararchy problems...
					// llinfos << "    mouse coords: " << x << ", " << y << " hit child 2 " << hit_child_2->getName() << llendl;
					result = true;
				}
			}
		}
	}

	return result;
}


void LLPanelPrimMediaControls::onClickClose()
{
	close();
}

void LLPanelPrimMediaControls::close()
{
	resetZoomLevel(true);
	LLViewerMediaFocus::getInstance()->clearFocus();
	setVisible(FALSE);
}


void LLPanelPrimMediaControls::onClickBack()
{
	focusOnTarget();

	LLViewerMediaImpl* impl =getTargetMediaImpl();
	
	if (impl)
	{
		impl->navigateBack();
	}
}

void LLPanelPrimMediaControls::onClickForward()
{
	focusOnTarget();

	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if (impl)
	{
		impl->navigateForward();
	}
}

void LLPanelPrimMediaControls::onClickHome()
{
	focusOnTarget();

	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if(impl)
	{
		impl->navigateHome();
	}
}

void LLPanelPrimMediaControls::onClickOpen()
{
	LLViewerMediaImpl* impl = getTargetMediaImpl();
	if(impl)
	{
		LLWeb::loadURL(impl->getCurrentMediaURL());
	}	
}

void LLPanelPrimMediaControls::onClickReload()
{
	focusOnTarget();

	//LLViewerMedia::navigateHome();
	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if(impl)
	{
		impl->navigateReload();
	}
}

void LLPanelPrimMediaControls::onClickPlay()
{
	focusOnTarget();

	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if(impl)
	{
		impl->play();
	}
}

void LLPanelPrimMediaControls::onClickPause()
{
	focusOnTarget();

	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if(impl)
	{
		impl->pause();
	}
}

void LLPanelPrimMediaControls::onClickStop()
{
	focusOnTarget();

	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if(impl)
	{
		impl->navigateStop();
	}
}

void LLPanelPrimMediaControls::onClickMediaStop()
{
	focusOnTarget();

	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if(impl)
	{
		impl->stop();
	}
}

void LLPanelPrimMediaControls::onClickSkipBack()
{
	focusOnTarget();

	LLViewerMediaImpl* impl =getTargetMediaImpl();
	
	if (impl)
	{
		impl->skipBack(mSkipStep);
	}
}

void LLPanelPrimMediaControls::onClickSkipForward()
{
	focusOnTarget();

	LLViewerMediaImpl* impl = getTargetMediaImpl();

	if (impl)
	{
		impl->skipForward(mSkipStep);
	}
}

void LLPanelPrimMediaControls::onClickZoom()
{
	focusOnTarget();
	
	if(mCurrentZoom == ZOOM_NONE)
	{
		nextZoomLevel();
	}
}

void LLPanelPrimMediaControls::nextZoomLevel()
{
	int index = 0;
	while (index < kNumZoomLevels)
	{
		if (kZoomLevels[index] == mCurrentZoom) 
		{
			index++;
			break;
		}
		index++;
	}
	mCurrentZoom = kZoomLevels[index % kNumZoomLevels];
	updateZoom();
}

void LLPanelPrimMediaControls::resetZoomLevel(bool reset_camera)
{
	if(mCurrentZoom != ZOOM_NONE)
	{
		mCurrentZoom = ZOOM_NONE;
		if(reset_camera)
		{
			updateZoom();
		}
	}
}

void LLPanelPrimMediaControls::updateZoom()
{
	F32 zoom_padding = 0.0f;
	switch (mCurrentZoom)
	{
	case ZOOM_NONE:
		{
			gAgentCamera.setFocusOnAvatar(TRUE, ANIMATE);
			break;
		}
	case ZOOM_FAR:
		{
			zoom_padding = mZoomFarPadding;
			break;
		}
	case ZOOM_MEDIUM:
		{
			zoom_padding = mZoomMediumPadding;
			break;
		}
	case ZOOM_NEAR:
		{
			zoom_padding = mZoomNearPadding;
			break;
		}
	default:
		{
			gAgentCamera.setFocusOnAvatar(TRUE, ANIMATE);
			break;
		}
	}

	if (zoom_padding > 0.0f)
	{	
		// since we only zoom into medium for now, always set zoom_in constraint to true
		LLViewerMediaFocus::setCameraZoom(getTargetObject(), mTargetObjectNormal, zoom_padding, true);
	}
	
	// Remember the object ID/face we zoomed into, so we can update the zoom icon appropriately
	mZoomObjectID = mTargetObjectID;
	mZoomObjectFace = mTargetObjectFace;
}

void LLPanelPrimMediaControls::onScrollUp(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->focusOnTarget();

	LLViewerMediaImpl* impl = this_panel->getTargetMediaImpl();
	
	if(impl)
	{
		impl->scrollWheel(0, -1, MASK_NONE);
	}
}
void LLPanelPrimMediaControls::onScrollUpHeld(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->mScrollState = SCROLL_UP;
}
void LLPanelPrimMediaControls::onScrollRight(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->focusOnTarget();

	LLViewerMediaImpl* impl = this_panel->getTargetMediaImpl();

	if(impl)
	{
		impl->scrollWheel(-1, 0, MASK_NONE);
//		impl->handleKeyHere(KEY_RIGHT, MASK_NONE);
	}
}
void LLPanelPrimMediaControls::onScrollRightHeld(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->mScrollState = SCROLL_RIGHT;
}

void LLPanelPrimMediaControls::onScrollLeft(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->focusOnTarget();

	LLViewerMediaImpl* impl = this_panel->getTargetMediaImpl();

	if(impl)
	{
		impl->scrollWheel(1, 0, MASK_NONE);
//		impl->handleKeyHere(KEY_LEFT, MASK_NONE);
	}
}
void LLPanelPrimMediaControls::onScrollLeftHeld(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->mScrollState = SCROLL_LEFT;
}

void LLPanelPrimMediaControls::onScrollDown(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->focusOnTarget();

	LLViewerMediaImpl* impl = this_panel->getTargetMediaImpl();
	
	if(impl)
	{
		impl->scrollWheel(0, 1, MASK_NONE);
	}
}
void LLPanelPrimMediaControls::onScrollDownHeld(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->mScrollState = SCROLL_DOWN;
}

void LLPanelPrimMediaControls::onScrollStop(void* user_data)
{
	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (user_data);
	this_panel->mScrollState = SCROLL_NONE;
}

void LLPanelPrimMediaControls::onCommitURL()
{
	focusOnTarget();

	std::string url = mMediaAddress->getValue().asString();
	if(getTargetMediaImpl() && !url.empty())
	{
		getTargetMediaImpl()->navigateTo( url, "", true);

		// Make sure keyboard focus is set to the media focus object.
		gFocusMgr.setKeyboardFocus(LLViewerMediaFocus::getInstance());
			
	}
	mPauseFadeout = false;
	mFadeTimer.start();
}


void LLPanelPrimMediaControls::onInputURL(LLFocusableElement* caller, void *userdata)
{

	LLPanelPrimMediaControls* this_panel = static_cast<LLPanelPrimMediaControls*> (userdata);
	this_panel->focusOnTarget();

	this_panel->mPauseFadeout = true;
	this_panel->mFadeTimer.stop();
	this_panel->mFadeTimer.reset();
	
}

void LLPanelPrimMediaControls::setCurrentURL()
{	
#ifdef USE_COMBO_BOX_FOR_MEDIA_URL
//	LLComboBox* media_address_combo	= getChild<LLComboBox>("media_address_combo");
//	// redirects will navigate momentarily to about:blank, don't add to history
//	if (media_address_combo && mCurrentURL != "about:blank")
//	{
//		media_address_combo->remove(mCurrentURL);
//		media_address_combo->add(mCurrentURL, ADD_SORTED);
//		media_address_combo->selectByValue(mCurrentURL);
//	}
#else   // USE_COMBO_BOX_FOR_MEDIA_URL
	if (mMediaAddress && mCurrentURL != "about:blank")
	{
		mMediaAddress->setValue(mCurrentURL);
	}
#endif	// USE_COMBO_BOX_FOR_MEDIA_URL
}

void LLPanelPrimMediaControls::onCommitSlider()
{
	focusOnTarget();

	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	if (media_impl) 
	{
		// get slider value
		F64 slider_value = mMediaPlaySliderCtrl->getValue().asReal();
		if(slider_value <= 0.0)
		{	
			media_impl->stop();
		}
		else 
		{
			media_impl->seek(slider_value*mMovieDuration);
			//mUpdateSlider= false;
		}
	}
}		

void LLPanelPrimMediaControls::onCommitVolumeUp()
{
	focusOnTarget();

	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	if (media_impl) 
	{
		F32 volume = media_impl->getVolume();
		
		volume += 0.1f;
		if(volume >= 1.0f)
		{
			volume = 1.0f;
		}
		
		media_impl->setVolume(volume);
		mMuteBtn->setToggleState(false);
	}
}		

void LLPanelPrimMediaControls::onCommitVolumeDown()
{
	focusOnTarget();

	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	if (media_impl) 
	{
		F32 volume = media_impl->getVolume();
		
		volume -= 0.1f;
		if(volume <= 0.0f)
		{
			volume = 0.0f;
		}

		media_impl->setVolume(volume);
		mMuteBtn->setToggleState(false);
	}
}		

void LLPanelPrimMediaControls::onCommitVolumeSlider()
{
	focusOnTarget();

	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	if (media_impl) 
	{
		media_impl->setVolume(mVolumeSliderCtrl->getValueF32());
	}
}

void LLPanelPrimMediaControls::onToggleMute()
{
	focusOnTarget();

	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	if (media_impl) 
	{
		F32 volume = media_impl->getVolume();
		
		if(volume > 0.0)
		{
			media_impl->setVolume(0.0);
		}
		else if (mVolumeSliderCtrl->getValueF32() == 0.0)
		{
			media_impl->setVolume(1.0);
			mVolumeSliderCtrl->setValue(1.0);
		}
		else 
		{
			media_impl->setVolume(mVolumeSliderCtrl->getValueF32());
		}
	}
}

void LLPanelPrimMediaControls::showVolumeSlider()
{
	mVolumeSliderVisible++;
}

void LLPanelPrimMediaControls::hideVolumeSlider()
{
	mVolumeSliderVisible--;
}

bool LLPanelPrimMediaControls::shouldVolumeSliderBeVisible()
{
	return mVolumeSliderVisible > 0;
}
