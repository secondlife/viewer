/** 
 * @file llfloatertos.cpp
 * @brief Terms of Service Agreement dialog
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatertos.h"

#include "llbutton.h"
#include "llradiogroup.h"
#include "llvfile.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "viewer.h"
#include "llstartup.h"
#include "message.h"
#include "llagent.h"
#include "llvieweruictrlfactory.h"
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
		gUICtrlFactory->buildFloater(LLFloaterTOS::sInstance, "floater_tos.xml");
	}
	else
	{
		gUICtrlFactory->buildFloater(LLFloaterTOS::sInstance, "floater_critical.xml");
	}

	return LLFloaterTOS::sInstance;
}


LLFloaterTOS::LLFloaterTOS(ETOSType type, const std::string & message)
:	LLModalDialog( " ", 100, 100 ),
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
	childSetCommitCallback("tos_agreement", updateAgree, this);

	if ( mType != TOS_TOS )
	{
		// this displays the critical message
		LLTextEditor *Editor = LLUICtrlFactory::getTextEditorByName(this, "tos_text");
		if (Editor)
		{
			Editor->setHandleEditKeysDirectly( TRUE );
			Editor->setEnabled( FALSE );
			Editor->setReadOnlyFgColor(LLColor4::white);
			Editor->setWordWrap(TRUE);
			Editor->setFocus(TRUE);
		}
		childSetValue("tos_text", LLSD(mMessage));
		return TRUE;
	}

#if LL_LIBXUL_ENABLED
	// disable Agree to TOS radio button until the page has fully loaded
	LLRadioGroup* tos_agreement = LLUICtrlFactory::getRadioGroupByName(this, "tos_agreement");
	if ( tos_agreement )
	{
		tos_agreement->setEnabled( false );
	};

	// hide the SL text widget if we're displaying TOS with using a browser widget.
	LLTextEditor *editor = LLUICtrlFactory::getTextEditorByName(this, "tos_text");
	if ( editor )
	{
		editor->setVisible( FALSE );
	};

	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(this, "tos_html");
	if ( web_browser )
	{
		// start to observe it so we see navigate complete events
		if ( web_browser )
		{
			web_browser->addObserver( this );
		};

		gResponsePtr = LLIamHere::build( this );
		LLHTTPClient::get( childGetValue( "real_url" ).asString(), gResponsePtr );
	};
#else
	LLTextEditor *Editor = LLUICtrlFactory::getTextEditorByName(this, "tos_text");
	if (Editor)
	{
		Editor->setHandleEditKeysDirectly( TRUE );
		Editor->setEnabled( FALSE );
		Editor->setReadOnlyFgColor(LLColor4::white);
		Editor->setWordWrap(TRUE);
		Editor->setFocus(TRUE);
	}
	childSetValue("tos_text", LLSD(mMessage));	
#endif

	return TRUE;
}

void LLFloaterTOS::setSiteIsAlive( bool alive )
{
	// only do this for TOS pages
	if ( mType == TOS_TOS )
	{
#if LL_LIBXUL_ENABLED
		LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(this, "tos_html");
		// if the contents of the site was retrieved
		if ( alive )
		{
			if ( web_browser )
			{
				// navigate to the "real" page 
				web_browser->navigateTo( childGetValue( "real_url" ).asString() );
			};
		}
		else
		{
			// normally this is set when navigation to TOS page navigation completes (so you can't accept before TOS loads)
			// but if the page is unavailable, we need to do this now
			LLRadioGroup* tos_agreement = LLUICtrlFactory::getRadioGroupByName(this, "tos_agreement");
			if ( tos_agreement )
			{
				tos_agreement->setEnabled( true );
			};

			if ( web_browser )
			{
				// hide browser control (revealing default text message)
				web_browser->setVisible( FALSE );
			};
		};
#endif // LL_LIBXUL_ENABLED
	};
}

LLFloaterTOS::~LLFloaterTOS()
{
#if LL_LIBXUL_ENABLED
	// stop obsaerving events
	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(this, "tos_html");
	if ( web_browser )
	{
		web_browser->addObserver( this );		
	};
#endif // LL_LIBXUL_ENABLED

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
	std::string agree = self->childGetValue("tos_agreement").asString();
	self->childSetEnabled("Continue", (agree == "radio_agree") );
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
		LLRadioGroup* tos_agreement = LLUICtrlFactory::getRadioGroupByName(this, "tos_agreement");
		if ( tos_agreement )
		{
			tos_agreement->setEnabled( true );
		};
	};
}
