/** 
 * @file llfloatertos.cpp
 * @brief Terms of Service Agreement dialog
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llfloatertos.h"

// viewer includes
#include "llviewerstats.h"
#include "llviewerwindow.h"

// linden library includes
#include "llbutton.h"
#include "llevents.h"
#include "llhttpclient.h"
#include "llhttpstatuscodes.h"	// for HTTP_FOUND
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llvfile.h"
#include "message.h"
#include "llstartup.h"              // login_alert_done


LLFloaterTOS::LLFloaterTOS(const LLSD& data)
:	LLModalDialog( data["message"].asString() ),
	mMessage(data["message"].asString()),
	mWebBrowserWindowId( 0 ),
	mLoadingScreenLoaded(false),
	mSiteAlive(false),
	mRealNavigateBegun(false),
	mReplyPumpName(data["reply_pump"].asString())
{
}

// helper class that trys to download a URL from a web site and calls a method 
// on parent class indicating if the web server is working or not
class LLIamHere : public LLHTTPClient::Responder
{
	private:
		LLIamHere( LLFloaterTOS* parent ) :
		   mParent( parent )
		{}

		LLFloaterTOS* mParent;

	public:

		static boost::intrusive_ptr< LLIamHere > build( LLFloaterTOS* parent )
		{
			return boost::intrusive_ptr< LLIamHere >( new LLIamHere( parent ) );
		};
		
		virtual void  setParent( LLFloaterTOS* parentIn )
		{
			mParent = parentIn;
		};
		
		virtual void result( const LLSD& content )
		{
			if ( mParent )
				mParent->setSiteIsAlive( true );
		};

		virtual void error( U32 status, const std::string& reason )
		{
			if ( mParent )
			{
				// *HACK: For purposes of this alive check, 302 Found
				// (aka Moved Temporarily) is considered alive.  The web site
				// redirects this link to a "cache busting" temporary URL. JC
				bool alive = (status == HTTP_FOUND);
				mParent->setSiteIsAlive( alive );
			}
		};
};

// this is global and not a class member to keep crud out of the header file
namespace {
	boost::intrusive_ptr< LLIamHere > gResponsePtr = 0;
};

BOOL LLFloaterTOS::postBuild()
{	
	childSetAction("Continue", onContinue, this);
	childSetAction("Cancel", onCancel, this);
	childSetCommitCallback("agree_chk", updateAgree, this);
	
	if (hasChild("tos_text"))
	{
		// this displays the critical message
		LLUICtrl *tos_text = getChild<LLUICtrl>("tos_text");
		tos_text->setEnabled( FALSE );
		tos_text->setFocus(TRUE);
		tos_text->setValue(LLSD(mMessage));

		return TRUE;
	}

	// disable Agree to TOS radio button until the page has fully loaded
	LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
	tos_agreement->setEnabled( false );

	// hide the SL text widget if we're displaying TOS with using a browser widget.
	LLUICtrl *editor = getChild<LLUICtrl>("tos_text");
	editor->setVisible( FALSE );

	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("tos_html");
	if ( web_browser )
	{
		web_browser->addObserver(this);

		// Don't use the start_url parameter for this browser instance -- it may finish loading before we get to add our observer.
		// Store the URL separately and navigate here instead.
		web_browser->navigateTo( getString( "loading_url" ) );
		
		gResponsePtr = LLIamHere::build( this );
		LLHTTPClient::get( getString( "real_url" ), gResponsePtr );
	}

	return TRUE;
}

void LLFloaterTOS::setSiteIsAlive( bool alive )
{
	mSiteAlive = alive;
	
	// only do this for TOS pages
	if (hasChild("tos_html"))
	{
		// if the contents of the site was retrieved
		if ( alive )
		{
			// navigate to the "real" page 
			loadIfNeeded();
		}
		else
		{
			// normally this is set when navigation to TOS page navigation completes (so you can't accept before TOS loads)
			// but if the page is unavailable, we need to do this now
			LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			tos_agreement->setEnabled( true );
		}
	}
}

void LLFloaterTOS::loadIfNeeded()
{
	if(!mRealNavigateBegun && mSiteAlive)
	{
		LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("tos_html");
		if(web_browser)
		{
			mRealNavigateBegun = true;
			web_browser->navigateTo( getString( "real_url" ) );
		}
	}
}

LLFloaterTOS::~LLFloaterTOS()
{

	// tell the responder we're not here anymore
	if ( gResponsePtr )
		gResponsePtr->setParent( 0 );
}

// virtual
void LLFloaterTOS::draw()
{
	// draw children
	LLModalDialog::draw();
}

// static
void LLFloaterTOS::updateAgree(LLUICtrl*, void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	bool agree = self->childGetValue("agree_chk").asBoolean(); 
	self->childSetEnabled("Continue", agree);
}

// static
void LLFloaterTOS::onContinue( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	llinfos << "User agrees with TOS." << llendl;

	if(self->mReplyPumpName != "")
	{
		LLEventPumps::instance().obtain(self->mReplyPumpName).post(LLSD(true));
	}

	self->closeFloater(); // destroys this object
}

// static
void LLFloaterTOS::onCancel( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	llinfos << "User disagrees with TOS." << llendl;
	LLNotificationsUtil::add("MustAgreeToLogIn", LLSD(), LLSD(), login_alert_done);

	if(self->mReplyPumpName != "")
	{
		LLEventPumps::instance().obtain(self->mReplyPumpName).post(LLSD(false));
	}

	// reset state for next time we come to TOS
	self->mLoadingScreenLoaded = false;
	self->mSiteAlive = false;
	self->mRealNavigateBegun = false;
	
	// destroys this object
	self->closeFloater(); 
}

//virtual 
void LLFloaterTOS::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
	if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		if(!mLoadingScreenLoaded)
		{
			mLoadingScreenLoaded = true;
			loadIfNeeded();
		}
		else if(mRealNavigateBegun)
		{
			llinfos << "NAVIGATE COMPLETE" << llendl;
			// enable Agree to TOS radio button now that page has loaded
			LLCheckBoxCtrl * tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			tos_agreement->setEnabled( true );
		}
	}
}

