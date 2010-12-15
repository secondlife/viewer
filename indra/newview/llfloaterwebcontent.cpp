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
#include "llviewercontrol.h"
#include "llweb.h"
#include "llwindow.h"

#include "llfloaterwebcontent.h"

LLFloaterWebContent::LLFloaterWebContent( const LLSD& key )
	: LLFloater( key )
{
	mCommitCallbackRegistrar.add( "WebContent.Back", boost::bind( &LLFloaterWebContent::onClickBack, this ));
	mCommitCallbackRegistrar.add( "WebContent.Forward", boost::bind( &LLFloaterWebContent::onClickForward, this ));
	mCommitCallbackRegistrar.add( "WebContent.Reload", boost::bind( &LLFloaterWebContent::onClickReload, this ));
	mCommitCallbackRegistrar.add( "WebContent.EnterAddress", boost::bind( &LLFloaterWebContent::onEnterAddress, this ));
	mCommitCallbackRegistrar.add( "WebContent.PopExternal", boost::bind( &LLFloaterWebContent::onPopExternal, this ));
}

BOOL LLFloaterWebContent::postBuild()
{
	// these are used in a bunch of places so cache them
	mWebBrowser = getChild< LLMediaCtrl >( "webbrowser" );
	mAddressCombo = getChild< LLComboBox >( "address" );
	mStatusBarText = getChild< LLTextBox >( "statusbartext" );
	mStatusBarProgress = getChild<LLProgressBar>("statusbarprogress" );

	// observe browser events
	mWebBrowser->addObserver( this );

	// these buttons are always enabled
	getChildView("reload")->setEnabled( true );
	getChildView("popexternal")->setEnabled( true );

	// cache image for secure browsing
	mSecureLockIcon = getChild< LLIconCtrl >("media_secure_lock_flag");

	return TRUE;
}

//static
void LLFloaterWebContent::create( const std::string &url, const std::string& target, const std::string& uuid )
{
	lldebugs << "url = " << url << ", target = " << target << ", uuid = " << uuid << llendl;

	std::string tag = target;

	if(target.empty() || target == "_blank")
	{
		if(!uuid.empty())
		{
			tag = uuid;
		}
		else
		{
			// create a unique tag for this instance
			LLUUID id;
			id.generate();
			tag = id.asString();
		}
	}

	S32 browser_window_limit = gSavedSettings.getS32("WebContentWindowLimit");

	if(LLFloaterReg::findInstance("web_content", tag) != NULL)
	{
		// There's already a web browser for this tag, so we won't be opening a new window.
	}
	else if(browser_window_limit != 0)
	{
		// showInstance will open a new window.  Figure out how many web browsers are already open,
		// and close the least recently opened one if this will put us over the limit.

		LLFloaterReg::const_instance_list_t &instances = LLFloaterReg::getFloaterList("web_content");
		lldebugs << "total instance count is " << instances.size() << llendl;

		for(LLFloaterReg::const_instance_list_t::const_iterator iter = instances.begin(); iter != instances.end(); iter++)
		{
			lldebugs << "    " << (*iter)->getKey() << llendl;
		}

		if(instances.size() >= (size_t)browser_window_limit)
		{
			// Destroy the least recently opened instance
			(*instances.begin())->closeFloater();
		}
	}

	LLFloaterWebContent *browser = dynamic_cast<LLFloaterWebContent*> (LLFloaterReg::showInstance("web_content", tag));
	llassert(browser);
	if(browser)
	{
		browser->mUUID = uuid;

		// tell the browser instance to load the specified URL
		browser->open_media(url, target);
		LLViewerMedia::proxyWindowOpened(target, uuid);
	}
}

//static
void LLFloaterWebContent::closeRequest(const std::string &uuid)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("web_content");
	lldebugs << "instance list size is " << inst_list.size() << ", incoming uuid is " << uuid << llendl;
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterWebContent* i = dynamic_cast<LLFloaterWebContent*>(*iter);
		lldebugs << "    " << i->mUUID << llendl;
		if (i && i->mUUID == uuid)
		{
			i->closeFloater(false);
			return;
 		}
 	}
}

//static
void LLFloaterWebContent::geometryChanged(const std::string &uuid, S32 x, S32 y, S32 width, S32 height)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("web_content");
	lldebugs << "instance list size is " << inst_list.size() << ", incoming uuid is " << uuid << llendl;
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterWebContent* i = dynamic_cast<LLFloaterWebContent*>(*iter);
		lldebugs << "    " << i->mUUID << llendl;
		if (i && i->mUUID == uuid)
		{
			i->geometryChanged(x, y, width, height);
			return;
		}
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
	width += getRect().getWidth() - mWebBrowser->getRect().getWidth();
	height += getRect().getHeight() - mWebBrowser->getRect().getHeight();

	LLRect geom;
	geom.setOriginAndSize(x, window_size.mY - (y + height), width, height);

	lldebugs << "geometry change: " << geom << llendl;

	handleReshape(geom,false);
}

void LLFloaterWebContent::open_media(const std::string& web_url, const std::string& target)
{
	// Specifying a mime type of text/html here causes the plugin system to skip the MIME type probe and just open a browser plugin.
	mWebBrowser->setHomePageUrl(web_url, "text/html");
	mWebBrowser->setTarget(target);
	mWebBrowser->navigateTo(web_url, "text/html");
	set_current_url(web_url);
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
	// this is asychronous so we need to keep checking
	getChildView( "back" )->setEnabled( mWebBrowser->canNavigateBack() );
	getChildView( "forward" )->setEnabled( mWebBrowser->canNavigateForward() );

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
		getChildView("back")->setEnabled( self->getHistoryBackAvailable() );
		getChildView("forward")->setEnabled( self->getHistoryForwardAvailable() );

		// toggle visibility of these buttons based on browser state
		getChildView("reload")->setVisible( false );
		getChildView("stop")->setVisible( true );

		// turn "on" progress bar now we're about to start loading
		mStatusBarProgress->setVisible( true );
	}
	else if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		// flags are sent with this event
		getChildView("back")->setEnabled( self->getHistoryBackAvailable() );
		getChildView("forward")->setEnabled( self->getHistoryForwardAvailable() );

		// toggle visibility of these buttons based on browser state
		getChildView("reload")->setVisible( true );
		getChildView("stop")->setVisible( false );

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
		mStatusBarProgress->setPercent( percent );
	}
	else if(event == MEDIA_EVENT_NAME_CHANGED )
	{
		std::string page_title = self->getMediaName();
		// simulate browser behavior - title is empty, use the current URL
		if ( page_title.length() > 0 )
			setTitle( page_title );
		else
			setTitle( mCurrentURL );
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

	// redirects will navigate momentarily to about:blank, don't add to history
	if ( mCurrentURL != "about:blank" )
	{
		mAddressCombo->remove( mCurrentURL );
		mAddressCombo->add( mCurrentURL );
		mAddressCombo->selectByValue( mCurrentURL );
	}
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
	mAddressCombo->remove(0);

	if( mWebBrowser->getMediaPlugin() )
	{
		bool ignore_cache = true;
		mWebBrowser->getMediaPlugin()->browse_reload( ignore_cache );
	};
}

void LLFloaterWebContent::onClickStop()
{
	if( mWebBrowser->getMediaPlugin() )
		mWebBrowser->getMediaPlugin()->stop();
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
