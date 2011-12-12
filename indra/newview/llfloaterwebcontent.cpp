/**
 * @file llfloaterwebcontent.cpp
 * @brief floater for displaying web content - e.g. profiles and search (eventually)
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llcombobox.h"
#include "lliconctrl.h"
#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "llpluginclassmedia.h"
#include "llprogressbar.h"
#include "lltextbox.h"
#include "llurlhistory.h"
#include "llviewercontrol.h"
#include "llweb.h"
#include "llwindow.h"

#include "llfloaterwebcontent.h"

LLFloaterWebContent::_Params::_Params()
:	url("url"),
	target("target"),
	id("id"),
	window_class("window_class", "web_content"),
	show_chrome("show_chrome", true),
	allow_address_entry("allow_address_entry", true),
	preferred_media_size("preferred_media_size"),
	trusted_content("trusted_content", false),
	show_page_title("show_page_title", true)
{}

LLFloaterWebContent::LLFloaterWebContent( const Params& params )
:	LLFloater( params ),
	LLInstanceTracker<LLFloaterWebContent, std::string>(params.id()),
	mWebBrowser(NULL),
	mAddressCombo(NULL),
	mSecureLockIcon(NULL),
	mStatusBarText(NULL),
	mStatusBarProgress(NULL),
	mBtnBack(NULL),
	mBtnForward(NULL),
	mBtnReload(NULL),
	mBtnStop(NULL),
	mUUID(params.id()),
	mShowPageTitle(params.show_page_title)
{
	mCommitCallbackRegistrar.add( "WebContent.Back", boost::bind( &LLFloaterWebContent::onClickBack, this ));
	mCommitCallbackRegistrar.add( "WebContent.Forward", boost::bind( &LLFloaterWebContent::onClickForward, this ));
	mCommitCallbackRegistrar.add( "WebContent.Reload", boost::bind( &LLFloaterWebContent::onClickReload, this ));
	mCommitCallbackRegistrar.add( "WebContent.Stop", boost::bind( &LLFloaterWebContent::onClickStop, this ));
	mCommitCallbackRegistrar.add( "WebContent.EnterAddress", boost::bind( &LLFloaterWebContent::onEnterAddress, this ));
	mCommitCallbackRegistrar.add( "WebContent.PopExternal", boost::bind( &LLFloaterWebContent::onPopExternal, this ));
}

BOOL LLFloaterWebContent::postBuild()
{
	// these are used in a bunch of places so cache them
	mWebBrowser        = getChild< LLMediaCtrl >( "webbrowser" );
	mAddressCombo      = getChild< LLComboBox >( "address" );
	mStatusBarText     = getChild< LLTextBox >( "statusbartext" );
	mStatusBarProgress = getChild<LLProgressBar>("statusbarprogress" );

	mBtnBack           = getChildView( "back" );
	mBtnForward        = getChildView( "forward" );
	mBtnReload         = getChildView( "reload" );
	mBtnStop           = getChildView( "stop" );

	// observe browser events
	mWebBrowser->addObserver( this );

	// these buttons are always enabled
	mBtnReload->setEnabled( true );
	getChildView("popexternal")->setEnabled( true );

	// cache image for secure browsing
	mSecureLockIcon = getChild< LLIconCtrl >("media_secure_lock_flag");

	// initialize the URL history using the system URL History manager
	initializeURLHistory();

	return TRUE;
}

void LLFloaterWebContent::initializeURLHistory()
{
	// start with an empty list
	LLCtrlListInterface* url_list = childGetListInterface("address");
	if (url_list)
	{
		url_list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}

	// Get all of the entries in the "browser" collection
	LLSD browser_history = LLURLHistory::getURLHistory("browser");
	LLSD::array_iterator iter_history = browser_history.beginArray();
	LLSD::array_iterator end_history = browser_history.endArray();
	for(; iter_history != end_history; ++iter_history)
	{
		std::string url = (*iter_history).asString();
		if(! url.empty())
			url_list->addSimpleElement(url);
	}
}

bool LLFloaterWebContent::matchesKey(const LLSD& key)
{
	Params p(mKey);
	Params other_p(key);
	if (!other_p.target().empty() && other_p.target() != "_blank")
	{
		return other_p.target() == p.target();
	}
	else
	{
		return other_p.id() == p.id();
	}
}

//static
LLFloater* LLFloaterWebContent::create( Params p)
{
	preCreate(p);
	return new LLFloaterWebContent(p);
}

//static
void LLFloaterWebContent::closeRequest(const std::string &uuid)
{
	LLFloaterWebContent* floaterp = instance_tracker_t::getInstance(uuid);
	if (floaterp)
	{
		floaterp->closeFloater(false);
	}
}

//static
void LLFloaterWebContent::geometryChanged(const std::string &uuid, S32 x, S32 y, S32 width, S32 height)
{
	LLFloaterWebContent* floaterp = instance_tracker_t::getInstance(uuid);
	if (floaterp)
	{
		floaterp->geometryChanged(x, y, width, height);
	}
}

void LLFloaterWebContent::geometryChanged(S32 x, S32 y, S32 width, S32 height)
{
	// Make sure the layout of the browser control is updated, so this calculation is correct.
	LLLayoutStack::updateClass();

	// TODO: need to adjust size and constrain position to make sure floaters aren't moved outside the window view, etc.
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);

	// Adjust width and height for the size of the chrome on the web Browser window.
	LLRect browser_rect;
	mWebBrowser->localRectToOtherView(mWebBrowser->getLocalRect(), &browser_rect, this);

	S32 requested_browser_bottom = window_size.mY - (y + height);
	LLRect geom;
	geom.setOriginAndSize(x - browser_rect.mLeft, 
						requested_browser_bottom - browser_rect.mBottom, 
						width + getRect().getWidth() - browser_rect.getWidth(), 
						height + getRect().getHeight() - browser_rect.getHeight());

	lldebugs << "geometry change: " << geom << llendl;
	
	LLRect new_rect;
	getParent()->screenRectToLocal(geom, &new_rect);
	setShape(new_rect);	
}

// static
void LLFloaterWebContent::preCreate(LLFloaterWebContent::Params& p)
{
	lldebugs << "url = " << p.url() << ", target = " << p.target() << ", uuid = " << p.id() << llendl;

	if (!p.id.isProvided())
	{
		p.id = LLUUID::generateNewID().asString();
	}

	if(p.target().empty() || p.target() == "_blank")
	{
		p.target = p.id();
	}

	S32 browser_window_limit = gSavedSettings.getS32("WebContentWindowLimit");
	if(browser_window_limit != 0)
	{
		// showInstance will open a new window.  Figure out how many web browsers are already open,
		// and close the least recently opened one if this will put us over the limit.

		LLFloaterReg::const_instance_list_t &instances = LLFloaterReg::getFloaterList(p.window_class);
		lldebugs << "total instance count is " << instances.size() << llendl;

		for(LLFloaterReg::const_instance_list_t::const_iterator iter = instances.begin(); iter != instances.end(); iter++)
		{
			lldebugs << "    " << (*iter)->getKey()["target"] << llendl;
		}

		if(instances.size() >= (size_t)browser_window_limit)
		{
			// Destroy the least recently opened instance
			(*instances.begin())->closeFloater();
		}
	}
}

void LLFloaterWebContent::open_media(const Params& p)
{
	// Specifying a mime type of text/html here causes the plugin system to skip the MIME type probe and just open a browser plugin.
	LLViewerMedia::proxyWindowOpened(p.target(), p.id());
	mWebBrowser->setHomePageUrl(p.url, "text/html");
	mWebBrowser->setTarget(p.target);
	mWebBrowser->navigateTo(p.url, "text/html");
	
	set_current_url(p.url);

	getChild<LLLayoutPanel>("status_bar")->setVisible(p.show_chrome);
	getChild<LLLayoutPanel>("nav_controls")->setVisible(p.show_chrome);
	bool address_entry_enabled = p.allow_address_entry && !p.trusted_content;
	getChildView("address")->setEnabled(address_entry_enabled);
	getChildView("popexternal")->setEnabled(address_entry_enabled);

	if (!address_entry_enabled)
	{
		mWebBrowser->setFocus(TRUE);
	}

	if (!p.show_chrome)
	{
		setResizeLimits(100, 100);
	}

	if (!p.preferred_media_size().isEmpty())
	{
		LLLayoutStack::updateClass();
		LLRect browser_rect = mWebBrowser->calcScreenRect();
		LLCoordWindow window_size;
		getWindow()->getSize(&window_size);
		
		geometryChanged(browser_rect.mLeft, window_size.mY - browser_rect.mTop, p.preferred_media_size().getWidth(), p.preferred_media_size().getHeight());
	}

}

void LLFloaterWebContent::onOpen(const LLSD& key)
{
	Params params(key);

	if (!params.validateBlock())
	{
		closeFloater();
		return;
	}

	mWebBrowser->setTrustedContent(params.trusted_content);

	// tell the browser instance to load the specified URL
	open_media(params);
}

//virtual
void LLFloaterWebContent::onClose(bool app_quitting)
{
	LLViewerMedia::proxyWindowClosed(mUUID);
	destroy();
}

// virtual
void LLFloaterWebContent::draw()
{
	// this is asynchronous so we need to keep checking
	mBtnBack->setEnabled( mWebBrowser->canNavigateBack() );
	mBtnForward->setEnabled( mWebBrowser->canNavigateForward() );

	LLFloater::draw();
}

// virtual
void LLFloaterWebContent::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	if(event == MEDIA_EVENT_LOCATION_CHANGED)
	{
		const std::string url = self->getLocation();

		if ( url.length() )
			mStatusBarText->setText( url );

		set_current_url( url );
	}
	else if(event == MEDIA_EVENT_NAVIGATE_BEGIN)
	{
		// flags are sent with this event
		mBtnBack->setEnabled( self->getHistoryBackAvailable() );
		mBtnForward->setEnabled( self->getHistoryForwardAvailable() );

		// toggle visibility of these buttons based on browser state
		mBtnReload->setVisible( false );
		mBtnStop->setVisible( true );

		// turn "on" progress bar now we're about to start loading
		mStatusBarProgress->setVisible( true );
	}
	else if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		// flags are sent with this event
		mBtnBack->setEnabled( self->getHistoryBackAvailable() );
		mBtnForward->setEnabled( self->getHistoryForwardAvailable() );

		// toggle visibility of these buttons based on browser state
		mBtnReload->setVisible( true );
		mBtnStop->setVisible( false );

		// turn "off" progress bar now we're loaded
		mStatusBarProgress->setVisible( false );

		// we populate the status bar with URLs as they change so clear it now we're done
		const std::string end_str = "";
		mStatusBarText->setText( end_str );

		// decide if secure browsing icon should be displayed
		std::string prefix =  std::string("https://");
		std::string test_prefix = mCurrentURL.substr(0, prefix.length());
		LLStringUtil::toLower(test_prefix);
		if(test_prefix == prefix)
		{
			mSecureLockIcon->setVisible(true);
		}
		else
		{
			mSecureLockIcon->setVisible(false);
		}
	}
	else if(event == MEDIA_EVENT_CLOSE_REQUEST)
	{
		// The browser instance wants its window closed.
		closeFloater();
	}
	else if(event == MEDIA_EVENT_GEOMETRY_CHANGE)
	{
		geometryChanged(self->getGeometryX(), self->getGeometryY(), self->getGeometryWidth(), self->getGeometryHeight());
	}
	else if(event == MEDIA_EVENT_STATUS_TEXT_CHANGED )
	{
		const std::string text = self->getStatusText();
		if ( text.length() )
			mStatusBarText->setText( text );
	}
	else if(event == MEDIA_EVENT_PROGRESS_UPDATED )
	{
		int percent = (int)self->getProgressPercent();
		mStatusBarProgress->setValue( percent );
	}
	else if(event == MEDIA_EVENT_NAME_CHANGED )
	{
		std::string page_title = self->getMediaName();
		// simulate browser behavior - title is empty, use the current URL
		if (mShowPageTitle)
		{
			if ( page_title.length() > 0 )
				setTitle( page_title );
			else
				setTitle( mCurrentURL );
		}
	}
	else if(event == MEDIA_EVENT_LINK_HOVERED )
	{
		const std::string link = self->getHoverLink();
		mStatusBarText->setText( link );
	}
}

void LLFloaterWebContent::set_current_url(const std::string& url)
{
	mCurrentURL = url;

	// serialize url history into the system URL History manager
	LLURLHistory::removeURL("browser", mCurrentURL);
	LLURLHistory::addURL("browser", mCurrentURL);

	mAddressCombo->remove( mCurrentURL );
	mAddressCombo->add( mCurrentURL );
	mAddressCombo->selectByValue( mCurrentURL );
}

void LLFloaterWebContent::onClickForward()
{
	mWebBrowser->navigateForward();
}

void LLFloaterWebContent::onClickBack()
{
	mWebBrowser->navigateBack();
}

void LLFloaterWebContent::onClickReload()
{

	if( mWebBrowser->getMediaPlugin() )
	{
		bool ignore_cache = true;
		mWebBrowser->getMediaPlugin()->browse_reload( ignore_cache );
	}
	else
	{
		mWebBrowser->navigateTo(mCurrentURL);
	}
}

void LLFloaterWebContent::onClickStop()
{
	if( mWebBrowser->getMediaPlugin() )
		mWebBrowser->getMediaPlugin()->browse_stop();

	// still should happen when we catch the navigate complete event
	// but sometimes (don't know why) that event isn't sent from Qt
	// and we ghetto a point where the stop button stays active.
	mBtnReload->setVisible( true );
	mBtnStop->setVisible( false );
}

void LLFloaterWebContent::onEnterAddress()
{
	// make sure there is at least something there.
	// (perhaps this test should be for minimum length of a URL)
	std::string url = mAddressCombo->getValue().asString();
	if ( url.length() > 0 )
	{
		mWebBrowser->navigateTo( url, "text/html");
	};
}

void LLFloaterWebContent::onPopExternal()
{
	// make sure there is at least something there.
	// (perhaps this test should be for minimum length of a URL)
	std::string url = mAddressCombo->getValue().asString();
	if ( url.length() > 0 )
	{
		LLWeb::loadURLExternal( url );
	};
}
