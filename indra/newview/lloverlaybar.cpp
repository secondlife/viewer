/** 
 * @file lloverlaybar.cpp
 * @brief LLOverlayBar class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Temporary buttons that appear at the bottom of the screen when you
// are in a mode.

#include "llviewerprecompiledheaders.h"

#include "lloverlaybar.h"

#include "audioengine.h"
#include "llparcel.h"

#include "llagent.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "llimview.h"
#include "lltextbox.h"
#include "llvoavatar.h"
#include "llmediaengine.h"
#include "viewer.h"
#include "llui.h"
#include "llviewermenu.h"	// handle_reset_view()
#include "llviewerparcelmgr.h"
#include "llwebbrowserctrl.h"
#include "llvieweruictrlfactory.h"
#include "llviewerimagelist.h"
#include "llviewerwindow.h"
#include "llfocusmgr.h"

//
// Globals
//

LLOverlayBar *gOverlayBar = NULL;

extern S32 MENU_BAR_HEIGHT;

//
// Functions
//


//static
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




LLOverlayBar::LLOverlayBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, rect, FALSE)		// not bordered
{
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);

	isBuilt = FALSE;

	LLCallbackMap::map_t factory_map;
	factory_map["media_remote"] = LLCallbackMap(LLOverlayBar::createMediaRemote, this);
	factory_map["music_remote"] = LLCallbackMap(LLOverlayBar::createMusicRemote, this);
	
	gUICtrlFactory->buildPanel(this, "panel_overlaybar.xml", &factory_map);
	
	childSetAction("IM Received",onClickIMReceived,this);
	childSetAction("Set Not Busy",onClickSetNotBusy,this);
	childSetAction("Release Keys",onClickReleaseKeys,this);
	childSetAction("Mouselook",onClickMouselook,this);
	childSetAction("Stand Up",onClickStandUp,this);

	mMusicRemote->addObserver ( this );
	
	if ( gAudiop )
	{
		mMusicRemote->setVolume ( gSavedSettings.getF32 ( "AudioLevelMusic" ) );
		mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Stop, FALSE );
	};

	mIsFocusRoot = TRUE;

	mMediaRemote->addObserver ( this );
	mMediaRemote->setVolume ( gSavedSettings.getF32 ( "MediaAudioVolume" ) );

	isBuilt = true;

	layoutButtons();
}

LLOverlayBar::~LLOverlayBar()
{
	// LLView destructor cleans up children

	mMusicRemote->remObserver ( this );
	mMediaRemote->remObserver ( this );
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

	if (isBuilt) 
	{
		layoutButtons();
	}
}


void LLOverlayBar::layoutButtons()
{
	S32 width = mRect.getWidth();
	if (width > 800) width = 800;

	S32 count = getChildCount();
	const S32 PAD = gSavedSettings.getS32("StatusBarPad");

	F32 segment_width = (F32)(width) / (F32)count;

	S32 btn_width = lltrunc(segment_width - PAD);

	S32 remote_width = mMusicRemote->getRect().getWidth();

	// Evenly space all views
	LLRect r;
	S32 i = 0;
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		r = view->getRect();
		r.mLeft = (width) - llround((i+1)*segment_width);
		r.mRight = r.mLeft + btn_width;
		view->setRect(r);
		i++;
	}

	// Fix up remotes to have constant width because they can't shrink
	r = mMusicRemote->getRect();
	r.mRight = r.mLeft + remote_width;
	mMusicRemote->setRect(r);

	r = mMediaRemote->getRect();
	r.mLeft = mMusicRemote->getRect().mRight + PAD;
	r.mRight = r.mLeft + remote_width;
	mMediaRemote->setRect(r);

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
	BOOL im_received = gIMView->getIMReceived();
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

	if ( gAudiop )
	{
		LLParcel* parcel = gParcelMgr->getAgentParcel();
		if (!parcel
			|| !parcel->getMusicURL()
			|| !parcel->getMusicURL()[0]
			|| !gSavedSettings.getBOOL("AudioStreamingMusic"))
		{
			mMusicRemote->setVisible(FALSE);
			mMusicRemote->setEnabled(FALSE);
		}
		else
		{
			mMusicRemote->setVisible(TRUE);
			mMusicRemote->setEnabled(TRUE);

			S32 musicPlaying = gAudiop->isInternetStreamPlaying();

			if ( musicPlaying == 0 )	// stopped
			{
				mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Stop, FALSE );
			}
			else
			if ( musicPlaying == 1 )	// playing
			{
				mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Play, FALSE );
				if (gAudiop)
				{
					gAudiop->setInternetStreamGain ( gSavedSettings.getF32 ( "AudioLevelMusic" ) );
				}
			}
			else
			if ( musicPlaying == 2 )	// paused
			{
				mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Stop, FALSE );
			}
		}
	}

	// if there is a url and a texture and media is enabled and available and media streaming is on... (phew!)
	if ( LLMediaEngine::getInstance () &&
		 LLMediaEngine::getInstance ()->getUrl ().length () && 
		 LLMediaEngine::getInstance ()->getImageUUID ().notNull () &&
		 LLMediaEngine::getInstance ()->isEnabled () &&
		 LLMediaEngine::getInstance ()->isAvailable () &&
		 gSavedSettings.getBOOL ( "AudioStreamingVideo" ) )
	{
		// display remote control 
		mMediaRemote->setVisible ( TRUE );
		mMediaRemote->setEnabled ( TRUE );

		if ( LLMediaEngine::getInstance ()->getMediaRenderer () )
		{
			if ( LLMediaEngine::getInstance ()->getMediaRenderer ()->isPlaying () ||
				LLMediaEngine::getInstance ()->getMediaRenderer ()->isLooping () )
			{
				mMediaRemote->setTransportState ( LLMediaRemoteCtrl::Pause, TRUE );
			}
			else
			if ( LLMediaEngine::getInstance ()->getMediaRenderer ()->isPaused () )
			{
				mMediaRemote->setTransportState ( LLMediaRemoteCtrl::Play, TRUE );
			}
			else
			{
				mMediaRemote->setTransportState ( LLMediaRemoteCtrl::Stop, TRUE );
			};
		};
	}
	else
	{
		mMediaRemote->setVisible ( FALSE );
		mMediaRemote->setEnabled ( FALSE );
		mMediaRemote->setTransportState ( LLMediaRemoteCtrl::Stop, TRUE );
	};

	BOOL any_button = (childIsVisible("IM Received")
					   || childIsVisible("Set Not Busy")
					   || childIsVisible("Release Keys")
					   || childIsVisible("Mouselook")
					   || childIsVisible("Stand Up")
					   || mMusicRemote->getVisible()
					   || mMediaRemote->getVisible() );		   
					   
	
	// turn off the whole bar in mouselook
	if (gAgent.cameraMouselook())
	{
		setVisible(FALSE);
	}
	else
	{
		setVisible(any_button);
	};
}

//-----------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------

// static
void LLOverlayBar::onClickIMReceived(void*)
{
	gIMView->setFloaterOpen(TRUE);
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
//
//
void
LLOverlayBar::
onVolumeChange ( const LLMediaRemoteCtrlObserver::EventType& eventIn )
{
	LLUICtrl* control = eventIn.getControl ();
	F32 value = eventIn.getValue ();

	if ( control == mMusicRemote )
	{
		if (gAudiop)
		{
			gAudiop->setInternetStreamGain ( value );
		};
		gSavedSettings.setF32 ( "AudioLevelMusic", value );
	}
	else
	if ( control == mMediaRemote )
	{
		LLMediaEngine::getInstance ()->setVolume ( value );
		gSavedSettings.setF32 ( "MediaAudioVolume", value );

	};
}

////////////////////////////////////////////////////////////////////////////////
//
//
void
LLOverlayBar::
onStopButtonPressed ( const LLMediaRemoteCtrlObserver::EventType& eventIn )
{
	LLUICtrl* control = eventIn.getControl ();

	if ( control == mMusicRemote )
	{
		if ( gAudiop )
		{
			gAudiop->stopInternetStream ();
		};
		mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Stop, FALSE );
	}
	else
	if ( control == mMediaRemote )
	{
		LLMediaEngine::getInstance ()->stop ();
		mMediaRemote->setTransportState ( LLMediaRemoteCtrl::Stop, TRUE );
	};
}

////////////////////////////////////////////////////////////////////////////////
//
//
void LLOverlayBar::onPlayButtonPressed( const LLMediaRemoteCtrlObserver::EventType& eventIn )
{
	LLUICtrl* control = eventIn.getControl ();

	LLParcel* parcel = gParcelMgr->getAgentParcel();
	if ( control == mMusicRemote )
	{
		if (gAudiop)
		{
			if ( parcel )
			{
				// this doesn't work properly when crossing parcel boundaries - even when the 
				// stream is stopped, it doesn't return the right thing - commenting out for now.
				//if ( gAudiop->isInternetStreamPlaying() == 0 )
				//{
					const char* music_url = parcel->getMusicURL();

					gAudiop->startInternetStream(music_url);

					mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Play, FALSE );
				//}
			}
		};

		// CP: this is the old way of doing things (click play each time on a parcel to start stream)
		//if (gAudiop)
		//{
		//	if (gAudiop->isInternetStreamPlaying() > 0)
		//	{
		//		gAudiop->pauseInternetStream ( 0 );
		//	}
		//	else
		//	{
		//		if (parcel)
		//		{
		//			const char* music_url = parcel->getMusicURL();
		//			gAudiop->startInternetStream(music_url);
		//		}
		//	}
		//};
		//mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Stop, FALSE );
	}
	else
	if ( control == mMediaRemote )
	{
		LLParcel* parcel = gParcelMgr->getAgentParcel();
		if (parcel)
		{
			bool web_url = (parcel->getParcelFlag(PF_URL_WEB_PAGE) || parcel->getParcelFlag(PF_URL_RAW_HTML));
			LLString path( "" );
			#if LL_MOZILLA_ENABLED
			LLString mozilla_subdir;
			if (web_url)
			{
				path = get_mozilla_path();
			}
			#endif
			LLMediaEngine::getInstance ()->convertImageAndLoadUrl( true, web_url, path );
			mMediaRemote->setTransportState ( LLMediaRemoteCtrl::Play, TRUE );
		}
	};
}

////////////////////////////////////////////////////////////////////////////////
//
//
void LLOverlayBar::onPauseButtonPressed( const LLMediaRemoteCtrlObserver::EventType& eventIn )
{
	LLUICtrl* control = eventIn.getControl ();

	if ( control == mMusicRemote )
	{
		if (gAudiop)
		{
			gAudiop->pauseInternetStream ( 1 );
		};
		mMusicRemote->setTransportState ( LLMediaRemoteCtrl::Play, FALSE );
	}
	else
	if ( control == mMediaRemote )
	{
		LLMediaEngine::getInstance ()->pause ();
		mMediaRemote->setTransportState ( LLMediaRemoteCtrl::Pause, TRUE );
	};
}
