/** 
 * @file llfloatertos.cpp
 * @brief Terms of Service Agreement dialog
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

		static LLIamHere* build( LLFloaterTOS* parent )
		{
			return new LLIamHere( parent );
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
	LLPointer< LLIamHere > gResponsePtr = 0;
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
		else
		{
			LL_INFOS("TOS") << "ToS page: ToS page unavailable!" << LL_ENDL;
			// normally this is set when navigation to TOS page navigation completes (so you can't accept before TOS loads)
			// but if the page is unavailable, we need to do this now
			LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			tos_agreement->setEnabled( true );
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
	bool agree = self->getChild<LLUICtrl>("agree_chk")->getValue().asBoolean(); 
	self->getChildView("Continue")->setEnabled(agree);
}

// static
void LLFloaterTOS::onContinue( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	LL_INFOS("TOS") << "User agrees with TOS." << LL_ENDL;

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
	LL_INFOS("TOS") << "User disagrees with TOS." << LL_ENDL;
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

			gResponsePtr = LLIamHere::build( this );
			LLHTTPClient::get( getString( "real_url" ), gResponsePtr );
		}
		else if(mRealNavigateBegun)
		{
			LL_INFOS("TOS") << "TOS: NAVIGATE COMPLETE" << LL_ENDL;
			// enable Agree to TOS radio button now that page has loaded
			LLCheckBoxCtrl * tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			tos_agreement->setEnabled( true );
		}
	}
}

