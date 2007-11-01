/** 
 * @file lloverlaybar.cpp
 * @brief LLOverlayBar class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

// Temporary buttons that appear at the bottom of the screen when you
// are in a mode.

#include "llviewerprecompiledheaders.h"

#include "lloverlaybar.h"

#include "audioengine.h"
#include "llagent.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llimview.h"
#include "llmediaengine.h"
#include "llmediaremotectrl.h"
#include "llpanelaudiovolume.h"
#include "llparcel.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"	// handle_reset_view()
#include "llviewerparcelmgr.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"
#include "llvoiceclient.h"
#include "llvoavatar.h"
#include "llvoiceremotectrl.h"
#include "llwebbrowserctrl.h"
#include "viewer.h"

//
// Globals
//

LLOverlayBar *gOverlayBar = NULL;

extern S32 MENU_BAR_HEIGHT;

//
// Functions
//


//static
void* LLOverlayBar::createMasterRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;	
	self->mMasterRemote =  new LLMediaRemoteCtrl ( "master_volume",
												   "volume",
												   LLRect(),
												   "panel_master_volume.xml");
	return self->mMasterRemote;
}

void* LLOverlayBar::createMediaRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;	
	self->mMediaRemote =  new LLMediaRemoteCtrl ( "media_remote",
												  "media",
												  LLRect(),
												  "panel_media_remote.xml");
	return self->mMediaRemote;
}

void* LLOverlayBar::createMusicRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;
	self->mMusicRemote =  new LLMediaRemoteCtrl ( "music_remote",
												  "music",
												  LLRect(),
												  "panel_music_remote.xml" );		
	return self->mMusicRemote;
}

void* LLOverlayBar::createVoiceRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;	
	self->mVoiceRemote = new LLVoiceRemoteCtrl("voice_remote");
	return self->mVoiceRemote;
}




LLOverlayBar::LLOverlayBar(const std::string& name, const LLRect& rect)
	:	LLPanel(name, rect, FALSE),		// not bordered
		mMasterRemote(NULL),
		mMusicRemote(NULL),
		mMediaRemote(NULL),
		mVoiceRemote(NULL),
		mMediaState(STOPPED),
		mMusicState(STOPPED)
{
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);

	mBuilt = false;

	LLCallbackMap::map_t factory_map;
	factory_map["master_volume"] = LLCallbackMap(LLOverlayBar::createMasterRemote, this);
	factory_map["media_remote"] = LLCallbackMap(LLOverlayBar::createMediaRemote, this);
	factory_map["music_remote"] = LLCallbackMap(LLOverlayBar::createMusicRemote, this);
	factory_map["voice_remote"] = LLCallbackMap(LLOverlayBar::createVoiceRemote, this);
	
	gUICtrlFactory->buildPanel(this, "panel_overlaybar.xml", &factory_map);
	
	childSetAction("IM Received",onClickIMReceived,this);
	childSetAction("Set Not Busy",onClickSetNotBusy,this);
	childSetAction("Release Keys",onClickReleaseKeys,this);
	childSetAction("Mouselook",onClickMouselook,this);
	childSetAction("Stand Up",onClickStandUp,this);

	mIsFocusRoot = TRUE;
	mBuilt = true;

	// make overlay bar conform to window size
	setRect(rect);
	layoutButtons();
}

LLOverlayBar::~LLOverlayBar()
{
	// LLView destructor cleans up children
}

EWidgetType LLOverlayBar::getWidgetType() const
{
	return WIDGET_TYPE_OVERLAY_BAR;
}

LLString LLOverlayBar::getWidgetTag() const
{
	return LL_OVERLAY_BAR_TAG;
}

// virtual
void LLOverlayBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);

	if (mBuilt) 
	{
		layoutButtons();
	}
}

void LLOverlayBar::layoutButtons()
{
	S32 width = mRect.getWidth();
	if (width > 1024) width = 1024;

	S32 count = getChildCount();
	const S32 PAD = gSavedSettings.getS32("StatusBarPad");

	const S32 num_media_controls = 3;
	S32 media_remote_width = mMediaRemote ? mMediaRemote->getRect().getWidth() : 0;
	S32 music_remote_width = mMusicRemote ? mMusicRemote->getRect().getWidth() : 0;
	S32 voice_remote_width = mVoiceRemote ? mVoiceRemote->getRect().getWidth() : 0;
	S32 master_remote_width = mMasterRemote ? mMasterRemote->getRect().getWidth() : 0;

	// total reserved width for all media remotes
	const S32 ENDPAD = 20;
	S32 remote_total_width = media_remote_width + PAD + music_remote_width + PAD + voice_remote_width + PAD + master_remote_width + ENDPAD;

	// calculate button widths
	F32 segment_width = (F32)(width - remote_total_width) / (F32)(count - num_media_controls);

	S32 btn_width = lltrunc(segment_width - PAD);

	// Evenly space all views
	LLRect r;
	S32 i = 0;
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		r = view->getRect();
		r.mLeft = (width) - llround(remote_total_width + (i-num_media_controls+1)*segment_width);
		r.mRight = r.mLeft + btn_width;
		view->setRect(r);
		i++;
	}

	// Fix up remotes to have constant width because they can't shrink
	S32 right = mRect.getWidth() - remote_total_width - PAD;
	if (mMediaRemote)
	{
		r = mMediaRemote->getRect();
		r.mLeft = right + PAD;
		right = r.mLeft + media_remote_width;
		r.mRight = right;
		mMediaRemote->setRect(r);
	}
	if (mMusicRemote)
	{
		r = mMusicRemote->getRect();
		r.mLeft = right + PAD;
		right = r.mLeft + music_remote_width;
		r.mRight = right;
		mMusicRemote->setRect(r);
	}
	if (mVoiceRemote)
	{
		r = mVoiceRemote->getRect();
		r.mLeft = right + PAD;
		right = r.mLeft + voice_remote_width;
		r.mRight = right;
		mVoiceRemote->setRect(r);
	}
	if (mMasterRemote)
	{
		r = mMasterRemote->getRect();
		r.mLeft = right + PAD;
		right = r.mLeft + master_remote_width;
		r.mRight = right;
		mMasterRemote->setRect(r);
	}
	
	updateRect();
}

void LLOverlayBar::draw()
{
	// retrieve rounded rect image
	LLUUID image_id;
	image_id.set(gViewerArt.getString("rounded_square.tga"));
	LLViewerImage* imagep = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	if (imagep)
	{
		LLGLSTexture texture_enabled;
		LLViewerImage::bindTexture(imagep);

		const S32 PAD = gSavedSettings.getS32("StatusBarPad");

		// draw rounded rect tabs behind all children
		LLRect r;
		// focus highlights
		LLColor4 color = gColors.getColor("FloaterFocusBorderColor");
		glColor4fv(color.mV);
		if(gFocusMgr.childHasKeyboardFocus(gBottomPanel))
		{
			for (child_list_const_iter_t child_iter = getChildList()->begin();
				child_iter != getChildList()->end(); ++child_iter)
			{
				LLView *view = *child_iter;
				if(view->getEnabled() && view->getVisible())
				{
					r = view->getRect();
					gl_segmented_rect_2d_tex(r.mLeft - PAD/3 - 1, 
											r.mTop + 3, 
											r.mRight + PAD/3 + 1,
											r.mBottom, 
											imagep->getWidth(), 
											imagep->getHeight(), 
											16, 
											ROUNDED_RECT_TOP);
				}
			}
		}

		// main tabs
		for (child_list_const_iter_t child_iter = getChildList()->begin();
			child_iter != getChildList()->end(); ++child_iter)
		{
			LLView *view = *child_iter;
			if(view->getEnabled() && view->getVisible())
			{
				r = view->getRect();
				// draw a nice little pseudo-3D outline
				color = gColors.getColor("DefaultShadowDark");
				glColor4fv(color.mV);
				gl_segmented_rect_2d_tex(r.mLeft - PAD/3 + 1, r.mTop + 2, r.mRight + PAD/3, r.mBottom, 
										 imagep->getWidth(), imagep->getHeight(), 16, ROUNDED_RECT_TOP);
				color = gColors.getColor("DefaultHighlightLight");
				glColor4fv(color.mV);
				gl_segmented_rect_2d_tex(r.mLeft - PAD/3, r.mTop + 2, r.mRight + PAD/3 - 3, r.mBottom, 
										 imagep->getWidth(), imagep->getHeight(), 16, ROUNDED_RECT_TOP);
				// here's the main background.  Note that it overhangs on the bottom so as to hide the
				// focus highlight on the bottom panel, thus producing the illusion that the focus highlight
				// continues around the tabs
				color = gColors.getColor("FocusBackgroundColor");
				glColor4fv(color.mV);
				gl_segmented_rect_2d_tex(r.mLeft - PAD/3 + 1, r.mTop + 1, r.mRight + PAD/3 - 1, r.mBottom - 1, 
										 imagep->getWidth(), imagep->getHeight(), 16, ROUNDED_RECT_TOP);
			}
		}
	}

	// draw children on top
	LLPanel::draw();
}


// Per-frame updates of visibility
void LLOverlayBar::refresh()
{
	BOOL im_received = gIMMgr->getIMReceived();
	childSetVisible("IM Received", im_received);
	childSetEnabled("IM Received", im_received);

	BOOL busy = gAgent.getBusy();
	childSetVisible("Set Not Busy", busy);
	childSetEnabled("Set Not Busy", busy);

	BOOL controls_grabbed = gAgent.anyControlGrabbed();
	
	childSetVisible("Release Keys", controls_grabbed);
	childSetEnabled("Release Keys", controls_grabbed);


	BOOL mouselook_grabbed;
	mouselook_grabbed = gAgent.isControlGrabbed(CONTROL_ML_LBUTTON_DOWN_INDEX)
						|| gAgent.isControlGrabbed(CONTROL_ML_LBUTTON_UP_INDEX);

	
	childSetVisible("Mouselook", mouselook_grabbed);
	childSetEnabled("Mouselook", mouselook_grabbed);

	BOOL sitting = FALSE;
	if (gAgent.getAvatarObject())
	{
		sitting = gAgent.getAvatarObject()->mIsSitting;
		childSetVisible("Stand Up", sitting);
		childSetEnabled("Stand Up", sitting);
		
	}

	if ( mMusicRemote && gAudiop )
	{
		LLParcel* parcel = gParcelMgr->getAgentParcel();
		if (!parcel 
			|| parcel->getMusicURL().empty()
			|| !gSavedSettings.getBOOL("AudioStreamingMusic"))
		{
			mMusicRemote->setVisible(FALSE);
			mMusicRemote->setEnabled(FALSE);
		}
		else
		{
			mMusicRemote->setVisible(TRUE);
			mMusicRemote->setEnabled(TRUE);
		}
	}

	// if there is a url and a texture and media is enabled and available and media streaming is on... (phew!)
	if ( mMediaRemote )
	{
		if (LLMediaEngine::getInstance () &&
			LLMediaEngine::getInstance ()->getUrl ().length () && 
			LLMediaEngine::getInstance ()->getImageUUID ().notNull () &&
			LLMediaEngine::getInstance ()->isEnabled () &&
			LLMediaEngine::getInstance ()->isAvailable () &&
			gSavedSettings.getBOOL ( "AudioStreamingVideo" ) )
		{
			// display remote control 
			mMediaRemote->setVisible ( TRUE );
			mMediaRemote->setEnabled ( TRUE );
		}
		else
		{
			mMediaRemote->setVisible ( FALSE );
			mMediaRemote->setEnabled ( FALSE );
		}
	}
	if (mVoiceRemote)
	{
		mVoiceRemote->setVisible(LLVoiceClient::voiceEnabled());
	}
	
	// turn off the whole bar in mouselook
	if (gAgent.cameraMouselook())
	{
		setVisible(FALSE);
	}
	else
	{
		setVisible(TRUE);
	}
}

//-----------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------

// static
void LLOverlayBar::onClickIMReceived(void*)
{
	gIMMgr->setFloaterOpen(TRUE);
}


// static
void LLOverlayBar::onClickSetNotBusy(void*)
{
	gAgent.clearBusy();
}


// static
void LLOverlayBar::onClickReleaseKeys(void*)
{
	gAgent.forceReleaseControls();
}

// static
void LLOverlayBar::onClickResetView(void* data)
{
	handle_reset_view();
}

//static
void LLOverlayBar::onClickMouselook(void*)
{
	gAgent.changeCameraToMouselook();
}

//static
void LLOverlayBar::onClickStandUp(void*)
{
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
}

////////////////////////////////////////////////////////////////////////////////
// static media helpers
// *TODO: Move this into an audio manager abstraction

//static
void LLOverlayBar::mediaPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMediaState = PLAYING; // desired state
	LLParcel* parcel = gParcelMgr->getAgentParcel();
	if (parcel)
	{
		LLString path("");
		LLMediaEngine::getInstance()->convertImageAndLoadUrl( true, false, path );
	}
}
//static
void LLOverlayBar::mediaPause(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMediaState = PAUSED; // desired state
	LLMediaEngine::getInstance()->pause();
}
//static
void LLOverlayBar::mediaStop(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMediaState = STOPPED; // desired state
	LLMediaEngine::getInstance()->stop();
}

//static
void LLOverlayBar::musicPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMusicState = PLAYING; // desired state
	if (gAudiop)
	{
		LLParcel* parcel = gParcelMgr->getAgentParcel();
		if ( parcel )
		{
			// this doesn't work properly when crossing parcel boundaries - even when the 
			// stream is stopped, it doesn't return the right thing - commenting out for now.
// 			if ( gAudiop->isInternetStreamPlaying() == 0 )
			{
				gAudiop->startInternetStream(parcel->getMusicURL().c_str());
			}
		}
	}
}
//static
void LLOverlayBar::musicPause(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMusicState = PAUSED; // desired state
	if (gAudiop)
	{
		gAudiop->pauseInternetStream(1);
	}
}
//static
void LLOverlayBar::musicStop(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMusicState = STOPPED; // desired state
	if (gAudiop)
	{
		gAudiop->stopInternetStream();
	}
}

//static
void LLOverlayBar::enableMusicButtons(LLPanel* panel)
{	
	BOOL play_enabled = FALSE;
	BOOL play_visible = TRUE;
	BOOL pause_visible = FALSE;
	BOOL stop_enabled = FALSE;
	if ( gAudiop && gOverlayBar && gSavedSettings.getBOOL("AudioStreamingMusic"))
	{
		play_enabled = TRUE;
		S32 is_playing = gAudiop->isInternetStreamPlaying();
		if (is_playing == 1)
		{
			play_visible = FALSE;
			pause_visible = TRUE;
			stop_enabled = TRUE;
		}
		else if (is_playing == 2)
		{
			play_visible = TRUE;
			pause_visible = FALSE;
			stop_enabled = TRUE;
		}
	}
	panel->childSetEnabled("music_play", play_enabled);
	panel->childSetEnabled("music_pause", play_enabled);
	panel->childSetVisible("music_play", play_visible);
	panel->childSetVisible("music_pause", pause_visible);
	panel->childSetEnabled("music_stop", stop_enabled);
}

//static
void LLOverlayBar::enableMediaButtons(LLPanel* panel)
{
	// Media
	BOOL play_enabled = FALSE;
	BOOL play_visible = TRUE;
	BOOL pause_visible = FALSE;
	BOOL stop_enabled = FALSE;

	if ( LLMediaEngine::getInstance() && gOverlayBar && gSavedSettings.getBOOL("AudioStreamingVideo") )
	{
		play_enabled = TRUE;
		if (LLMediaEngine::getInstance()->getMediaRenderer())
		{
			if ( LLMediaEngine::getInstance()->getMediaRenderer()->isPlaying() ||
				 LLMediaEngine::getInstance()->getMediaRenderer()->isLooping() )
			{
				play_visible = FALSE;
				pause_visible = TRUE;
				stop_enabled = TRUE;
			}
			else if ( LLMediaEngine::getInstance()->getMediaRenderer()->isPaused() )
			{
				play_visible = TRUE;
				pause_visible = FALSE;
				stop_enabled = TRUE;
			}
		}
	}
	panel->childSetEnabled("media_play", play_enabled);
	panel->childSetEnabled("media_pause", play_enabled);
	panel->childSetVisible("media_play", play_visible);
	panel->childSetVisible("media_pause", pause_visible);
	panel->childSetEnabled("media_stop", stop_enabled);
}

void LLOverlayBar::toggleAudioVolumeFloater(void* user_data)
{
	LLFloaterAudioVolume::toggleInstance(LLSD());
}
