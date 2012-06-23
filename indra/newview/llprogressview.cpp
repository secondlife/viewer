/** 
 * @file llprogressview.cpp
 * @brief LLProgressView class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llprogressview.h"

#include "indra_constants.h"
#include "llmath.h"
#include "llgl.h"
#include "llrender.h"
#include "llui.h"
#include "llfontgl.h"
#include "lltimer.h"
#include "lltextbox.h"
#include "llglheaders.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcallbacklist.h"
#include "llfocusmgr.h"
#include "llnotifications.h"
#include "llprogressbar.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llweb.h"
#include "lluictrlfactory.h"
#include "llpanellogin.h"

LLProgressView* LLProgressView::sInstance = NULL;

S32 gStartImageWidth = 1;
S32 gStartImageHeight = 1;
const F32 FADE_TO_WORLD_TIME = 1.0f;

static LLRegisterPanelClassWrapper<LLProgressView> r("progress_view");

// XUI: Translate
LLProgressView::LLProgressView() 
:	LLPanel(),
	mPercentDone( 0.f ),
	mMediaCtrl( NULL ),
	mMouseDownInActiveArea( false ),
	mUpdateEvents("LLProgressView"),
	mFadeToWorldTimer(),
	mFadeFromLoginTimer(),
	mStartupComplete(false)
{
	mUpdateEvents.listen("self", boost::bind(&LLProgressView::handleUpdate, this, _1));
	mFadeToWorldTimer.stop();
	mFadeFromLoginTimer.stop();
}

BOOL LLProgressView::postBuild()
{
	mProgressBar = getChild<LLProgressBar>("login_progress_bar");

	// media control that is used to play intro video
	mMediaCtrl = getChild<LLMediaCtrl>("login_media_panel");
	mMediaCtrl->setVisible( false );		// hidden initially
	mMediaCtrl->addObserver( this );		// watch events
	
	LLViewerMedia::setOnlyAudibleMediaTextureID(mMediaCtrl->getTextureID());

	mCancelBtn = getChild<LLButton>("cancel_btn");
	mCancelBtn->setClickedCallback(  LLProgressView::onCancelButtonClicked, NULL );

	getChild<LLTextBox>("title_text")->setText(LLStringExplicit(LLAppViewer::instance()->getSecondLifeTitle()));

	getChild<LLTextBox>("message_text")->setClickedCallback(onClickMessage, this);

	// hidden initially, until we need it
	LLPanel::setVisible(FALSE);

	LLNotifications::instance().getChannel("AlertModal")->connectChanged(boost::bind(&LLProgressView::onAlertModal, this, _1));

	sInstance = this;
	return TRUE;
}


LLProgressView::~LLProgressView()
{
	// Just in case something went wrong, make sure we deregister our idle callback.
	gIdleCallbacks.deleteFunction(onIdle, this);

	gFocusMgr.releaseFocusIfNeeded( this );

	sInstance = NULL;
}

BOOL LLProgressView::handleHover(S32 x, S32 y, MASK mask)
{
	if( childrenHandleHover( x, y, mask ) == NULL )
	{
		gViewerWindow->setCursor(UI_CURSOR_WAIT);
	}
	return TRUE;
}


BOOL LLProgressView::handleKeyHere(KEY key, MASK mask)
{
	// Suck up all keystokes except CTRL-Q.
	if( ('Q' == key) && (MASK_CONTROL == mask) )
	{
		LLAppViewer::instance()->userQuit();
	}
	return TRUE;
}

void LLProgressView::revealIntroPanel()
{
	// if user hasn't yet seen intro video
	std::string intro_url = gSavedSettings.getString("PostFirstLoginIntroURL");
	if ( intro_url.length() > 0 && 
			gSavedSettings.getBOOL("BrowserJavascriptEnabled") &&
			gSavedSettings.getBOOL("PostFirstLoginIntroViewed" ) == FALSE )
	{
		// hide the progress bar
		getChild<LLView>("stack1")->setVisible(false);
		
		// navigate to intro URL and reveal widget 
		mMediaCtrl->navigateTo( intro_url );	
		mMediaCtrl->setVisible( TRUE );


		// flag as having seen the new user post login intro
		gSavedSettings.setBOOL("PostFirstLoginIntroViewed", TRUE );

		mMediaCtrl->setFocus(TRUE);
	}

	mFadeFromLoginTimer.start();
	gIdleCallbacks.addFunction(onIdle, this);
}

void LLProgressView::setStartupComplete()
{
	mStartupComplete = true;

	// if we are not showing a video, fade into world
	if (!mMediaCtrl->getVisible())
	{
		mFadeFromLoginTimer.stop();
		mFadeToWorldTimer.start();
	}
}

void LLProgressView::setVisible(BOOL visible)
{
	// hiding progress view
	if (getVisible() && !visible)
	{
		LLPanel::setVisible(FALSE);
	}
	// showing progress view
	else if (visible && (!getVisible() || mFadeToWorldTimer.getStarted()))
	{
		setFocus(TRUE);
		mFadeToWorldTimer.stop();
		LLPanel::setVisible(TRUE);
	} 
}


void LLProgressView::drawStartTexture(F32 alpha)
{
	gGL.pushMatrix();	
	if (gStartTexture)
	{
		LLGLSUIDefault gls_ui;
		gGL.getTexUnit(0)->bind(gStartTexture.get());
		gGL.color4f(1.f, 1.f, 1.f, alpha);
		F32 image_aspect = (F32)gStartImageWidth / (F32)gStartImageHeight;
		S32 width = getRect().getWidth();
		S32 height = getRect().getHeight();
		F32 view_aspect = (F32)width / (F32)height;
		// stretch image to maintain aspect ratio
		if (image_aspect > view_aspect)
		{
			gGL.translatef(-0.5f * (image_aspect / view_aspect - 1.f) * width, 0.f, 0.f);
			gGL.scalef(image_aspect / view_aspect, 1.f, 1.f);
		}
		else
		{
			gGL.translatef(0.f, -0.5f * (view_aspect / image_aspect - 1.f) * height, 0.f);
			gGL.scalef(1.f, view_aspect / image_aspect, 1.f);
		}
		gl_rect_2d_simple_tex( getRect().getWidth(), getRect().getHeight() );
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}
	else
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f(0.f, 0.f, 0.f, 1.f);
		gl_rect_2d(getRect());
	}
	gGL.popMatrix();
}


void LLProgressView::draw()
{
	static LLTimer timer;

	if (mFadeFromLoginTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mFadeFromLoginTimer.getElapsedTimeF32(), 0.f, FADE_TO_WORLD_TIME, 0.f, 1.f);
		LLViewDrawContext context(alpha);

		if (!mMediaCtrl->getVisible())
		{
			drawStartTexture(alpha);
		}
		
		LLPanel::draw();
		return;
	}

	// handle fade out to world view when we're asked to
	if (mFadeToWorldTimer.getStarted())
	{
		// draw fading panel
		F32 alpha = clamp_rescale(mFadeToWorldTimer.getElapsedTimeF32(), 0.f, FADE_TO_WORLD_TIME, 1.f, 0.f);
		LLViewDrawContext context(alpha);
				
		drawStartTexture(alpha);
		LLPanel::draw();

		// faded out completely - remove panel and reveal world
		if (mFadeToWorldTimer.getElapsedTimeF32() > FADE_TO_WORLD_TIME )
		{
			mFadeToWorldTimer.stop();

			LLViewerMedia::setOnlyAudibleMediaTextureID(LLUUID::null);

			// Fade is complete, release focus
			gFocusMgr.releaseFocusIfNeeded( this );

			// turn off panel that hosts intro so we see the world
			LLPanel::setVisible(FALSE);

			// stop observing events since we no longer care
			mMediaCtrl->remObserver( this );

			// hide the intro
			mMediaCtrl->setVisible( false );

			// navigate away from intro page to something innocuous since 'unload' is broken right now
			//mMediaCtrl->navigateTo( "about:blank" );

			// FIXME: this causes a crash that i haven't been able to fix
			mMediaCtrl->unloadMediaSource();	

			gStartTexture = NULL;
		}
		return;
	}

	drawStartTexture(1.0f);
	// draw children
	LLPanel::draw();
}

void LLProgressView::setText(const std::string& text)
{
	getChild<LLUICtrl>("progress_text")->setValue(text);
}

void LLProgressView::setPercent(const F32 percent)
{
	mProgressBar->setValue(percent);
}

void LLProgressView::setMessage(const std::string& msg)
{
	mMessage = msg;
	getChild<LLUICtrl>("message_text")->setValue(mMessage);
}

void LLProgressView::setCancelButtonVisible(BOOL b, const std::string& label)
{
	mCancelBtn->setVisible( b );
	mCancelBtn->setEnabled( b );
	mCancelBtn->setLabelSelected(label);
	mCancelBtn->setLabelUnselected(label);
}

// static
void LLProgressView::onCancelButtonClicked(void*)
{
	// Quitting viewer here should happen only when "Quit" button is pressed while starting up.
	// Check for startup state is used here instead of teleport state to avoid quitting when
	// cancel is pressed while teleporting inside region (EXT-4911)
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		LLAppViewer::instance()->requestQuit();
	}
	else
	{
		gAgent.teleportCancel();
		sInstance->mCancelBtn->setEnabled(FALSE);
		sInstance->setVisible(FALSE);
	}
}

// static
void LLProgressView::onClickMessage(void* data)
{
	LLProgressView* viewp = (LLProgressView*)data;
	if ( viewp != NULL && ! viewp->mMessage.empty() )
	{
		std::string url_to_open( "" );

		size_t start_pos;
		start_pos = viewp->mMessage.find( "https://" );
		if (start_pos == std::string::npos)
			start_pos = viewp->mMessage.find( "http://" );
		if (start_pos == std::string::npos)
			start_pos = viewp->mMessage.find( "ftp://" );
			
		if ( start_pos != std::string::npos )
		{
			size_t end_pos = viewp->mMessage.find_first_of( " \n\r\t", start_pos );
			if ( end_pos != std::string::npos )
				url_to_open = viewp->mMessage.substr( start_pos, end_pos - start_pos );
			else
				url_to_open = viewp->mMessage.substr( start_pos );

			LLWeb::loadURLExternal( url_to_open );
		}
	}
}

bool LLProgressView::handleUpdate(const LLSD& event_data)
{
	LLSD message = event_data.get("message");
	LLSD desc = event_data.get("desc");
	LLSD percent = event_data.get("percent");

	if(message.isDefined())
	{
		setMessage(message.asString());
	}

	if(desc.isDefined())
	{
		setText(desc.asString());
	}
	
	if(percent.isDefined())
	{
		setPercent(percent.asReal());
	}
	return false;
}

bool LLProgressView::onAlertModal(const LLSD& notify)
{
	// if the progress view is visible, it will obscure the notification window
	// in this case, we want to auto-accept WebLaunchExternalTarget notifications
	if (isInVisibleChain() && notify["sigtype"].asString() == "add")
	{
		LLNotificationPtr notifyp = LLNotifications::instance().find(notify["id"].asUUID());
		if (notifyp && notifyp->getName() == "WebLaunchExternalTarget")
		{
			notifyp->respondWithDefault();
		}
	}
	return false;
}

void LLProgressView::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	// the intro web content calls javascript::window.close() when it's done
	if( event == MEDIA_EVENT_CLOSE_REQUEST )
	{
		if (mStartupComplete)
		{
			//make sure other timer has stopped
			mFadeFromLoginTimer.stop();
			mFadeToWorldTimer.start();
		}
		else
		{
			// hide the media ctrl and wait for startup to be completed before fading to world
			mMediaCtrl->setVisible(false);
			if (mMediaCtrl->getMediaPlugin())
			{
				mMediaCtrl->getMediaPlugin()->stop();
			}

			// show the progress bar
			getChild<LLView>("stack1")->setVisible(true);
		}
	}
}


// static
void LLProgressView::onIdle(void* user_data)
{
	LLProgressView* self = (LLProgressView*) user_data;

	// Close login panel on mFadeToWorldTimer expiration.
	if (self->mFadeFromLoginTimer.getStarted() &&
		self->mFadeFromLoginTimer.getElapsedTimeF32() > FADE_TO_WORLD_TIME)
	{
		self->mFadeFromLoginTimer.stop();
		LLPanelLogin::closePanel();

		// Nothing to do anymore.
		gIdleCallbacks.deleteFunction(onIdle, user_data);
	}
}
