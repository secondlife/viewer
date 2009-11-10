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

//LLPanelPrimMediaControls
#include "llagent.h"
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
#include "llslider.h"
#include "llhudview.h"
#include "lliconctrl.h"
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llpanelprimmediacontrols.h"
#include "llpluginclassmedia.h"
#include "llprogressbar.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llvovolume.h"
#include "llweb.h"
#include "llwindow.h"

glh::matrix4f glh_get_current_modelview();
glh::matrix4f glh_get_current_projection();

const F32 ZOOM_NEAR_PADDING		= 1.0f;
const F32 ZOOM_MEDIUM_PADDING	= 1.15f;
const F32 ZOOM_FAR_PADDING		= 1.5f;

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
	mUpdatePercent(0)
{
	mCommitCallbackRegistrar.add("MediaCtrl.Close",		boost::bind(&LLPanelPrimMediaControls::onClickClose, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Back",		boost::bind(&LLPanelPrimMediaControls::onClickBack, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Forward",	boost::bind(&LLPanelPrimMediaControls::onClickForward, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Home",		boost::bind(&LLPanelPrimMediaControls::onClickHome, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Stop",		boost::bind(&LLPanelPrimMediaControls::onClickStop, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Reload",	boost::bind(&LLPanelPrimMediaControls::onClickReload, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Play",		boost::bind(&LLPanelPrimMediaControls::onClickPlay, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Pause",		boost::bind(&LLPanelPrimMediaControls::onClickPause, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Open",		boost::bind(&LLPanelPrimMediaControls::onClickOpen, this));
	mCommitCallbackRegistrar.add("MediaCtrl.Zoom",		boost::bind(&LLPanelPrimMediaControls::onClickZoom, this));
	mCommitCallbackRegistrar.add("MediaCtrl.CommitURL",	boost::bind(&LLPanelPrimMediaControls::onCommitURL, this));
	mCommitCallbackRegistrar.add("MediaCtrl.JumpProgress",		boost::bind(&LLPanelPrimMediaControls::onCommitSlider, this));
	mCommitCallbackRegistrar.add("MediaCtrl.CommitVolumeUp",	boost::bind(&LLPanelPrimMediaControls::onCommitVolumeUp, this));
	mCommitCallbackRegistrar.add("MediaCtrl.CommitVolumeDown",	boost::bind(&LLPanelPrimMediaControls::onCommitVolumeDown, this));
	mCommitCallbackRegistrar.add("MediaCtrl.ToggleMute",		boost::bind(&LLPanelPrimMediaControls::onToggleMute, this));
	
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_prim_media_controls.xml");
	mInactivityTimer.reset();
	mFadeTimer.stop();
	mCurrentZoom = ZOOM_NONE;
	mScrollState = SCROLL_NONE;

	mPanelHandle.bind(this);
}
LLPanelPrimMediaControls::~LLPanelPrimMediaControls()
{
}

BOOL LLPanelPrimMediaControls::postBuild()
{
	LLButton* scroll_up_ctrl = getChild<LLButton>("scrollup");
	scroll_up_ctrl->setClickedCallback(onScrollUp, this);
	scroll_up_ctrl->setHeldDownCallback(onScrollUpHeld, this);
	scroll_up_ctrl->setMouseUpCallback(onScrollStop, this);
	LLButton* scroll_left_ctrl = getChild<LLButton>("scrollleft");
	scroll_left_ctrl->setClickedCallback(onScrollLeft, this);
	scroll_left_ctrl->setHeldDownCallback(onScrollLeftHeld, this);
	scroll_left_ctrl->setMouseUpCallback(onScrollStop, this);
	LLButton* scroll_right_ctrl = getChild<LLButton>("scrollright");
	scroll_right_ctrl->setClickedCallback(onScrollRight, this);
	scroll_right_ctrl->setHeldDownCallback(onScrollRightHeld, this);
	scroll_right_ctrl->setMouseUpCallback(onScrollStop, this);
	LLButton* scroll_down_ctrl = getChild<LLButton>("scrolldown");
	scroll_down_ctrl->setClickedCallback(onScrollDown, this);
	scroll_down_ctrl->setHeldDownCallback(onScrollDownHeld, this);
	scroll_down_ctrl->setMouseUpCallback(onScrollStop, this);
	
	LLUICtrl* media_address	= getChild<LLUICtrl>("media_address");
	media_address->setFocusReceivedCallback(boost::bind(&LLPanelPrimMediaControls::onInputURL, _1, this ));
	mInactiveTimeout = gSavedSettings.getF32("MediaControlTimeout");
	mControlFadeTime = gSavedSettings.getF32("MediaControlFadeTime");

	mCurrentZoom = ZOOM_NONE;
	// clicks on HUD buttons do not remove keyboard focus from media
	setIsChrome(TRUE);
	return TRUE;
}

void LLPanelPrimMediaControls::setMediaFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal)
{
	if (media_impl.notNull() && objectp.notNull())
	{
		mTargetImplID = media_impl->getMediaTextureID();
		mTargetObjectID = objectp->getID();
		mTargetObjectFace = face;
		mTargetObjectNormal = pick_normal;
		mClearFaceOnFade = false;
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
	const S32 MIN_HUD_WIDTH=400;
	const S32 MIN_HUD_HEIGHT=120;

	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	LLViewerObject* objectp = getTargetObject();
	
	if(!media_impl)
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
	bool is_zoomed = (mCurrentZoom != ZOOM_NONE);
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
			// Don't show the media HUD if we do not have permissions
			enabled = dynamic_cast<LLVOVolume*>(objectp)->hasMediaPermission(media_data, LLVOVolume::MEDIA_PERM_CONTROL);
			mini_controls = (LLMediaEntry::MINI == media_data->getControls());
		}
		
		//
		// Set the state of the buttons
		//
		LLUICtrl* back_ctrl					= getChild<LLUICtrl>("back");
		LLUICtrl* fwd_ctrl					= getChild<LLUICtrl>("fwd");
		LLUICtrl* reload_ctrl				= getChild<LLUICtrl>("reload");
		LLUICtrl* play_ctrl					= getChild<LLUICtrl>("play");
		LLUICtrl* pause_ctrl				= getChild<LLUICtrl>("pause");
		LLUICtrl* stop_ctrl					= getChild<LLUICtrl>("stop");
		LLUICtrl* media_stop_ctrl			= getChild<LLUICtrl>("media_stop");
		LLUICtrl* home_ctrl					= getChild<LLUICtrl>("home");
		LLUICtrl* unzoom_ctrl				= getChild<LLUICtrl>("close"); // This is actually "unzoom"
		LLUICtrl* open_ctrl					= getChild<LLUICtrl>("new_window");
        LLUICtrl* zoom_ctrl					= getChild<LLUICtrl>("zoom_frame");
		LLPanel* media_loading_panel		= getChild<LLPanel>("media_progress_indicator");
		LLUICtrl* media_address_ctrl		= getChild<LLUICtrl>("media_address");
		LLUICtrl* media_play_slider_panel	= getChild<LLUICtrl>("media_play_position");
		LLUICtrl* media_play_slider_ctrl	= getChild<LLUICtrl>("media_play_slider");
		LLUICtrl* volume_ctrl				= getChild<LLUICtrl>("media_volume");
		LLButton* volume_btn				= getChild<LLButton>("media_volume_button");
		LLUICtrl* volume_up_ctrl			= getChild<LLUICtrl>("volume_up");
		LLUICtrl* volume_down_ctrl			= getChild<LLUICtrl>("volume_down");
		LLIconCtrl* whitelist_icon			= getChild<LLIconCtrl>("media_whitelist_flag");
		LLIconCtrl* secure_lock_icon		= getChild<LLIconCtrl>("media_secure_lock_flag");
		
		LLUICtrl* media_panel_scroll		= getChild<LLUICtrl>("media_panel_scroll");
		LLUICtrl* scroll_up_ctrl			= getChild<LLUICtrl>("scrollup");
		LLUICtrl* scroll_left_ctrl			= getChild<LLUICtrl>("scrollleft");
		LLUICtrl* scroll_right_ctrl			= getChild<LLUICtrl>("scrollright");
		LLUICtrl* scroll_down_ctrl			= getChild<LLUICtrl>("scrolldown");		
				
		// XXX RSP: TODO: FIXME: clean this up so that it is clearer what mode we are in,
		// and that only the proper controls get made visible/enabled according to that mode. 
		back_ctrl->setVisible(has_focus);
		fwd_ctrl->setVisible(has_focus);
		reload_ctrl->setVisible(has_focus);
		stop_ctrl->setVisible(false);
		home_ctrl->setVisible(has_focus);
		zoom_ctrl->setVisible(!is_zoomed);
		unzoom_ctrl->setVisible(has_focus && is_zoomed);
		open_ctrl->setVisible(true);
		media_address_ctrl->setVisible(has_focus && !mini_controls);
		media_play_slider_panel->setVisible(has_focus && !mini_controls);
		volume_ctrl->setVisible(false);
		volume_up_ctrl->setVisible(false);
		volume_down_ctrl->setVisible(false);
		
		whitelist_icon->setVisible(!mini_controls && (media_data)?media_data->getWhiteListEnable():false);
		// Disable zoom if HUD
		zoom_ctrl->setEnabled(!objectp->isHUDAttachment());
		unzoom_ctrl->setEnabled(!objectp->isHUDAttachment());
		secure_lock_icon->setVisible(false);
		mCurrentURL = media_impl->getCurrentMediaURL();
		
		back_ctrl->setEnabled((media_impl != NULL) && media_impl->canNavigateBack() && can_navigate);
		fwd_ctrl->setEnabled((media_impl != NULL) && media_impl->canNavigateForward() && can_navigate);
		stop_ctrl->setEnabled(has_focus && can_navigate);
		home_ctrl->setEnabled(has_focus && can_navigate);
		LLPluginClassMediaOwner::EMediaStatus result = ((media_impl != NULL) && media_impl->hasMedia()) ? media_plugin->getStatus() : LLPluginClassMediaOwner::MEDIA_NONE;

		if(media_plugin && media_plugin->pluginSupportsMediaTime())
		{
			reload_ctrl->setEnabled(FALSE);
			reload_ctrl->setVisible(FALSE);
			media_stop_ctrl->setVisible(has_focus);
			home_ctrl->setVisible(FALSE);
			back_ctrl->setEnabled(has_focus);
			fwd_ctrl->setEnabled(has_focus);
			media_address_ctrl->setVisible(false);
			media_address_ctrl->setEnabled(false);
			media_play_slider_panel->setVisible(has_focus && !mini_controls);
			media_play_slider_panel->setEnabled(has_focus && !mini_controls);
				
			volume_ctrl->setVisible(has_focus);
			volume_up_ctrl->setVisible(has_focus);
			volume_down_ctrl->setVisible(has_focus);
			volume_ctrl->setEnabled(has_focus);

			whitelist_icon->setVisible(false);
			secure_lock_icon->setVisible(false);
			scroll_up_ctrl->setVisible(false);
			scroll_left_ctrl->setVisible(false);
			scroll_right_ctrl->setVisible(false);
			scroll_down_ctrl->setVisible(false);
			media_panel_scroll->setVisible(false);
				
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
				media_play_slider_ctrl->setValue(0);
				media_play_slider_ctrl->setEnabled(false);
			}
			// TODO: What if it's not fully loaded
					
			if(mUpdateSlider && mMovieDuration!= 0)
			{
				F64 current_time =  media_plugin->getCurrentTime();
				F32 percent = current_time / mMovieDuration;
				media_play_slider_ctrl->setValue(percent);
				media_play_slider_ctrl->setEnabled(true);
			}
				
			// video vloume
			if(volume <= 0.0)
			{
				volume_up_ctrl->setEnabled(TRUE);
				volume_down_ctrl->setEnabled(FALSE);
				media_impl->setVolume(0.0);
				volume_btn->setToggleState(true);
			}
			else if (volume >= 1.0)
			{
				volume_up_ctrl->setEnabled(FALSE);
				volume_down_ctrl->setEnabled(TRUE);
				media_impl->setVolume(1.0);
				volume_btn->setToggleState(false);
			}
			else
			{
				volume_up_ctrl->setEnabled(TRUE);
				volume_down_ctrl->setEnabled(TRUE);
			}
				
			switch(result)
			{
				case LLPluginClassMediaOwner::MEDIA_PLAYING:
					play_ctrl->setEnabled(FALSE);
					play_ctrl->setVisible(FALSE);
					pause_ctrl->setEnabled(TRUE);
					pause_ctrl->setVisible(has_focus);
					media_stop_ctrl->setEnabled(TRUE);
					
					break;
				case LLPluginClassMediaOwner::MEDIA_PAUSED:
				default:
					pause_ctrl->setEnabled(FALSE);
					pause_ctrl->setVisible(FALSE);
					play_ctrl->setEnabled(TRUE);
					play_ctrl->setVisible(has_focus);
					media_stop_ctrl->setEnabled(FALSE);
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
				
			play_ctrl->setVisible(FALSE);
			pause_ctrl->setVisible(FALSE);
			media_stop_ctrl->setVisible(FALSE);
			media_address_ctrl->setVisible(has_focus && !mini_controls);
			media_address_ctrl->setEnabled(has_focus && !mini_controls);
			media_play_slider_panel->setVisible(FALSE);
			media_play_slider_panel->setEnabled(FALSE);
				
			volume_ctrl->setVisible(FALSE);
			volume_up_ctrl->setVisible(FALSE);
			volume_down_ctrl->setVisible(FALSE);
			volume_ctrl->setEnabled(FALSE);
			volume_up_ctrl->setEnabled(FALSE);
			volume_down_ctrl->setEnabled(FALSE);
				
			scroll_up_ctrl->setVisible(has_focus);
			scroll_left_ctrl->setVisible(has_focus);
			scroll_right_ctrl->setVisible(has_focus);
			scroll_down_ctrl->setVisible(has_focus);
			media_panel_scroll->setVisible(has_focus);
			// TODO: get the secure lock bool from media plug in
			std::string prefix =  std::string("https://");
			std::string test_prefix = mCurrentURL.substr(0, prefix.length());
			LLStringUtil::toLower(test_prefix);
			if(test_prefix == prefix)
			{
				secure_lock_icon->setVisible(has_focus);
			}
				
			if(mCurrentURL!=mPreviousURL)
			{
				setCurrentURL();
				mPreviousURL = mCurrentURL;
			}

			if(result == LLPluginClassMediaOwner::MEDIA_LOADING)
			{
				reload_ctrl->setEnabled(FALSE);
				reload_ctrl->setVisible(FALSE);
				stop_ctrl->setEnabled(TRUE);
				stop_ctrl->setVisible(has_focus);
			}
			else
			{
				reload_ctrl->setEnabled(TRUE);
				reload_ctrl->setVisible(has_focus);
				stop_ctrl->setEnabled(FALSE);
				stop_ctrl->setVisible(FALSE);
			}
		}

		
		if(media_plugin)
		{
			//
			// Handle progress bar
			//
			mUpdatePercent = media_plugin->getProgressPercent();
			if(mUpdatePercent<100.0f)
			{
				media_loading_panel->setVisible(true);
				getChild<LLProgressBar>("media_progress_bar")->setPercent(mUpdatePercent);
				gFocusMgr.setTopCtrl(media_loading_panel);
			}
			else
			{
				media_loading_panel->setVisible(false);
				gFocusMgr.setTopCtrl(NULL);
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
		glh::matrix4f mat = glh_get_current_projection()*glh_get_current_modelview();
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

        LLCoordGL screen_min;
		screen_min.mX = llround((F32)gViewerWindow->getWorldViewWidthRaw() * (min.mV[VX] + 1.f) * 0.5f);
		screen_min.mY = llround((F32)gViewerWindow->getWorldViewHeightRaw() * (min.mV[VY] + 1.f) * 0.5f);

		LLCoordGL screen_max;
		screen_max.mX = llround((F32)gViewerWindow->getWorldViewWidthRaw() * (max.mV[VX] + 1.f) * 0.5f);
		screen_max.mY = llround((F32)gViewerWindow->getWorldViewHeightRaw() * (max.mV[VY] + 1.f) * 0.5f);

		// grow panel so that screenspace bounding box fits inside "media_region" element of HUD
		LLRect media_controls_rect;
		getParent()->screenRectToLocal(LLRect(screen_min.mX, screen_max.mY, screen_max.mX, screen_min.mY), &media_controls_rect);
		LLView* media_region = getChild<LLView>("media_region");
		media_controls_rect.mLeft -= media_region->getRect().mLeft;
		media_controls_rect.mBottom -= media_region->getRect().mBottom;
		media_controls_rect.mTop += getRect().getHeight() - media_region->getRect().mTop;
		media_controls_rect.mRight += getRect().getWidth() - media_region->getRect().mRight;

		LLRect old_hud_rect = media_controls_rect;
		// keep all parts of HUD on-screen
		media_controls_rect.intersectWith(getParent()->getLocalRect());

		// clamp to minimum size, keeping centered
		media_controls_rect.setCenterAndSize(media_controls_rect.getCenterX(), media_controls_rect.getCenterY(),
			llmax(MIN_HUD_WIDTH, media_controls_rect.getWidth()), llmax(MIN_HUD_HEIGHT, media_controls_rect.getHeight()));

		setShape(media_controls_rect, true);

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
	F32 alpha = 1.f;
	if(mFadeTimer.getStarted())
	{
		F32 time = mFadeTimer.getElapsedTimeF32();
		alpha = llmax(lerp(1.0, 0.0, time / mControlFadeTime), 0.0f);

		if(mFadeTimer.getElapsedTimeF32() >= mControlFadeTime)
		{
			if(mClearFaceOnFade)
			{
				// Hiding this object makes scroll events go missing after it fades out 
				// (see DEV-41755 for a full description of the train wreck).
				// Only hide the controls when we're untargeting.
				setVisible(FALSE);

				mClearFaceOnFade = false;
				mTargetImplID = LLUUID::null;
				mTargetObjectID = LLUUID::null;
				mTargetObjectFace = 0;
			}
		}
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
		
		LLView* controls_view = NULL;
		controls_view = getChild<LLView>("media_controls");
		
		//FIXME: rewrite as LLViewQuery or get hover set from LLViewerWindow?
		if(controls_view && controls_view->getVisible())
		{
			controls_view->screenPointToLocal(cursor_pos_gl.mX, cursor_pos_gl.mY, &x, &y);

			LLView *hit_child = controls_view->childFromPoint(x, y);
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
	LLViewerMediaFocus::getInstance()->clearFocus();
	resetZoomLevel();
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
		impl->stop();
	}
}

void LLPanelPrimMediaControls::onClickZoom()
{
	focusOnTarget();

	nextZoomLevel();
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

void LLPanelPrimMediaControls::resetZoomLevel()
{
	if(mCurrentZoom != ZOOM_NONE)
	{
		mCurrentZoom = ZOOM_NONE;
		updateZoom();
	}
}

void LLPanelPrimMediaControls::updateZoom()
{
	F32 zoom_padding = 0.0f;
	switch (mCurrentZoom)
	{
	case ZOOM_NONE:
		{
			gAgent.setFocusOnAvatar(TRUE, ANIMATE);
			break;
		}
	case ZOOM_FAR:
		{
			zoom_padding = ZOOM_FAR_PADDING;
			break;
		}
	case ZOOM_MEDIUM:
		{
			zoom_padding = ZOOM_MEDIUM_PADDING;
			break;
		}
	case ZOOM_NEAR:
		{
			zoom_padding = ZOOM_NEAR_PADDING;
			break;
		}
	default:
		{
			gAgent.setFocusOnAvatar(TRUE, ANIMATE);
			break;
		}
	}

	if (zoom_padding > 0.0f)		
		LLViewerMediaFocus::setCameraZoom(getTargetObject(), mTargetObjectNormal, zoom_padding);
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

	LLUICtrl *media_address_ctrl = getChild<LLUICtrl>("media_address_url");
	std::string url = media_address_ctrl->getValue().asString();
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
	LLComboBox* media_address_combo	= getChild<LLComboBox>("media_address_combo");
	// redirects will navigate momentarily to about:blank, don't add to history
	if (media_address_combo && mCurrentURL != "about:blank")
	{
		media_address_combo->remove(mCurrentURL);
		media_address_combo->add(mCurrentURL, ADD_SORTED);
		media_address_combo->selectByValue(mCurrentURL);
	}
#else   // USE_COMBO_BOX_FOR_MEDIA_URL
	LLLineEditor* media_address_url = getChild<LLLineEditor>("media_address_url");
	if (media_address_url && mCurrentURL != "about:blank")
	{
		media_address_url->setValue(mCurrentURL);
	}
#endif	// USE_COMBO_BOX_FOR_MEDIA_URL
}

void LLPanelPrimMediaControls::onCommitSlider()
{
	focusOnTarget();

	LLSlider* media_play_slider_ctrl	= getChild<LLSlider>("media_play_slider");
	LLViewerMediaImpl* media_impl = getTargetMediaImpl();
	if (media_impl) 
	{
		// get slider value
		F64 slider_value = media_play_slider_ctrl->getValue().asReal();
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
		getChild<LLButton>("media_volume")->setToggleState(false);
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
		getChild<LLButton>("media_volume")->setToggleState(false);
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
		else 
		{
			media_impl->setVolume(0.5);
		}
	}
}

