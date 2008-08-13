/** 
 * @file llfloatertos.cpp
 * @brief Terms of Service Agreement dialog
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

#include "llfloatertos.h"

#include "llbutton.h"
#include "llradiogroup.h"
#include "llvfile.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "message.h"
#include "llagent.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llviewerstats.h"
#include "llui.h"
#include "llhttpclient.h"
#include "llradiogroup.h"

// static 
LLFloaterTOS* LLFloaterTOS::sInstance = NULL;

// static
LLFloaterTOS* LLFloaterTOS::show(ETOSType type, const std::string & message)
{
	if( !LLFloaterTOS::sInstance )
	{
		LLFloaterTOS::sInstance = new LLFloaterTOS(type, message);
	}

	if (type == TOS_TOS)
	{
		LLUICtrlFactory::getInstance()->buildFloater(LLFloaterTOS::sInstance, "floater_tos.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildFloater(LLFloaterTOS::sInstance, "floater_critical.xml");
	}

	return LLFloaterTOS::sInstance;
}


LLFloaterTOS::LLFloaterTOS(ETOSType type, const std::string & message)
:	LLModalDialog( std::string(" "), 100, 100 ),
	mType(type),
	mMessage(message),
	mWebBrowserWindowId( 0 ),
	mLoadCompleteCount( 0 )
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
				mParent->setSiteIsAlive( false );
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

	if ( mType != TOS_TOS )
	{
		// this displays the critical message
		LLTextEditor *editor = getChild<LLTextEditor>("tos_text");
		editor->setHandleEditKeysDirectly( TRUE );
		editor->setEnabled( FALSE );
		editor->setWordWrap(TRUE);
		editor->setFocus(TRUE);
		editor->setValue(LLSD(mMessage));

		return TRUE;
	}

	// disable Agree to TOS radio button until the page has fully loaded
	LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
	tos_agreement->setEnabled( false );

	// hide the SL text widget if we're displaying TOS with using a browser widget.
	LLTextEditor *editor = getChild<LLTextEditor>("tos_text");
	editor->setVisible( FALSE );

	LLWebBrowserCtrl* web_browser = getChild<LLWebBrowserCtrl>("tos_html");
	if ( web_browser )
	{
		// start to observe it so we see navigate complete events
		web_browser->addObserver( this );

		gResponsePtr = LLIamHere::build( this );
		LLHTTPClient::get( getString( "real_url" ), gResponsePtr );
	}

	return TRUE;
}

void LLFloaterTOS::setSiteIsAlive( bool alive )
{
	// only do this for TOS pages
	if ( mType == TOS_TOS )
	{
		LLWebBrowserCtrl* web_browser = getChild<LLWebBrowserCtrl>("tos_html");
		// if the contents of the site was retrieved
		if ( alive )
		{
			if ( web_browser )
			{
				// navigate to the "real" page 
				web_browser->navigateTo( getString( "real_url" ) );
			};
		}
		else
		{
			// normally this is set when navigation to TOS page navigation completes (so you can't accept before TOS loads)
			// but if the page is unavailable, we need to do this now
			LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			tos_agreement->setEnabled( true );

			if ( web_browser )
			{
				// hide browser control (revealing default text message)
				web_browser->setVisible( FALSE );
			};
		};
	};
}

LLFloaterTOS::~LLFloaterTOS()
{
	// stop obsaerving events
	LLWebBrowserCtrl* web_browser = getChild<LLWebBrowserCtrl>("tos_html");
	if ( web_browser )
	{
		web_browser->remObserver( this );		
	};

	// tell the responder we're not here anymore
	if ( gResponsePtr )
		gResponsePtr->setParent( 0 );

	LLFloaterTOS::sInstance = NULL;
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
	if (self->mType == TOS_TOS)
	{
		gAcceptTOS = TRUE;
	}
	else
	{
		gAcceptCriticalMessage = TRUE;
	}

	// Testing TOS dialog
	#if ! LL_RELEASE_FOR_DOWNLOAD		
	if ( LLStartUp::getStartupState() == STATE_LOGIN_WAIT )
	{
		LLStartUp::setStartupState( STATE_LOGIN_SHOW );
	}
	else 
	#endif

	LLStartUp::setStartupState( STATE_LOGIN_AUTH_INIT );			// Go back and finish authentication
	self->close(); // destroys this object
}

// static
void LLFloaterTOS::onCancel( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	llinfos << "User disagrees with TOS." << llendl;
	gViewerWindow->alertXml("MustAgreeToLogIn", login_alert_done);
	LLStartUp::setStartupState( STATE_LOGIN_SHOW );
	self->mLoadCompleteCount = 0;  // reset counter for next time we come to TOS
	self->close(); // destroys this object
}

//virtual 
void LLFloaterTOS::onNavigateComplete( const EventType& eventIn )
{
	// skip past the loading screen navigate complete
	if ( ++mLoadCompleteCount == 2 )
	{
		llinfos << "NAVIGATE COMPLETE" << llendl;
		// enable Agree to TOS radio button now that page has loaded
		LLCheckBoxCtrl * tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
		tos_agreement->setEnabled( true );
	};
}
