/** 
 * @file llpanellogin.cpp
 * @brief Login dialog and logo display
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

#include "llviewerprecompiledheaders.h"

#include "llpanellogin.h"
#include "llpanelgeneral.h"

#include "indra_constants.h"		// for key and mask constants
#include "llfontgl.h"
#include "llmd5.h"
#include "llsecondlifeurls.h"
#include "llwindow.h"			// shell_open()
#include "llversionviewer.h"
#include "v4color.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcommandhandler.h"
#include "llcombobox.h"
#include "llcurl.h"
#include "llviewercontrol.h"
#include "llfloaterabout.h"
#include "llfloatertest.h"
#include "llfloaterpreference.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llurlsimstring.h"
#include "llviewerbuild.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"			// for handle_preferences()
#include "llviewernetwork.h"
#include "llviewerwindow.h"			// to link into child list
#include "llnotify.h"
#include "llappviewer.h"					// for gHideLinks
#include "llurlsimstring.h"
#include "llvieweruictrlfactory.h"
#include "llhttpclient.h"
#include "llweb.h"
#include "llwebbrowserctrl.h"

#include "llfloaterhtml.h"
#include "llfloatertos.h"

#include "llglheaders.h"


LLString load_password_from_disk(void);
void save_password_to_disk(const char* hashed_password);

const S32 BLACK_BORDER_HEIGHT = 160;
const S32 MAX_PASSWORD = 16;

LLPanelLogin *LLPanelLogin::sInstance = NULL;


//parses the input url and returns true if afterwards
//a web-login-key, firstname and lastname  is set
bool LLLoginHandler::parseDirectLogin(std::string url)
{
	LLURI uri(url);
	parse(uri.queryMap());

	if (mWebLoginKey.isNull() ||
		mFirstName.empty() ||
		mLastName.empty())
	{
		return false;
	}
	else
	{
		return true;
	}
}


void LLLoginHandler::parse(const LLSD& queryMap)
{
	mWebLoginKey = queryMap["web_login_key"].asUUID();
	mFirstName = queryMap["first_name"].asString();
	mLastName = queryMap["last_name"].asString();
	
	if (queryMap["grid"].asString() == "aditi")
	{
		gGridChoice = GRID_INFO_ADITI;
	}
	else if (queryMap["grid"].asString() == "agni")
	{
		gGridChoice = GRID_INFO_AGNI;
	}
	else if (queryMap["grid"].asString() == "siva")
	{
		gGridChoice = GRID_INFO_SIVA;
	}
	else if (queryMap["grid"].asString() == "durga")
	{
		gGridChoice = GRID_INFO_DURGA;
	}
	else if (queryMap["grid"].asString() == "shakti")
	{
		gGridChoice = GRID_INFO_SHAKTI;
	}
	else if (queryMap["grid"].asString() == "soma")
	{
		gGridChoice = GRID_INFO_SOMA;
	}
	else if (queryMap["grid"].asString() == "ganga")
	{
		gGridChoice = GRID_INFO_GANGA;
	}
	else if (queryMap["grid"].asString() == "vaak")
	{
		gGridChoice = GRID_INFO_VAAK;
	}
	else if (queryMap["grid"].asString() == "uma")
	{
		gGridChoice = GRID_INFO_UMA;
	}
	
	snprintf(gGridName, MAX_STRING, "%s", gGridInfo[gGridChoice].mName);		/* Flawfinder: ignore */
	LLAppViewer::instance()->resetURIs();
	
	LLString startLocation = queryMap["location"].asString();

	if (startLocation == "specify")
	{
		LLURLSimString::setString(queryMap["region"].asString());
	}
	else if (startLocation == "home")
	{
		gSavedSettings.setBOOL("LoginLastLocation", FALSE);
		LLURLSimString::setString("");
	}
	else if (startLocation == "last")
	{
		gSavedSettings.setBOOL("LoginLastLocation", TRUE);
		LLURLSimString::setString("");
	}
}

bool LLLoginHandler::handle(const LLSD& tokens,
						  const LLSD& queryMap)
{	
	parse(queryMap);
	
	//if we haven't initialized stuff yet, this is 
	//coming in from the GURL handler, just parse
	if (STATE_FIRST == LLStartUp::getStartupState())
	{
		return true;
	}
	
	LLString password = queryMap["password"].asString();

	if (!password.empty())
	{
		gSavedSettings.setBOOL("RememberPassword", TRUE);

		if (password.substr(0,3) != "$1$")
		{
			LLMD5 pass((unsigned char*)password.c_str());
			char md5pass[33];		/* Flawfinder: ignore */
			pass.hex_digest(md5pass);
			password = md5pass;
			save_password_to_disk(password.c_str());
		}
	}
	else
	{
		save_password_to_disk(NULL);
		gSavedSettings.setBOOL("RememberPassword", FALSE);
	}
			

	if  (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)  //on splash page
	{
		if (mWebLoginKey.isNull()) {
			LLPanelLogin::loadLoginPage();
		} else {
			LLStartUp::setStartupState( STATE_LOGIN_CLEANUP );
		}
	}
	return true;
}

LLLoginHandler gLoginHandler;

// helper class that trys to download a URL from a web site and calls a method 
// on parent class indicating if the web server is working or not
class LLIamHereLogin : public LLHTTPClient::Responder
{
	private:
		LLIamHereLogin( LLPanelLogin* parent ) :
		   mParent( parent )
		{}

		LLPanelLogin* mParent;

	public:
		static boost::intrusive_ptr< LLIamHereLogin > build( LLPanelLogin* parent )
		{
			return boost::intrusive_ptr< LLIamHereLogin >( new LLIamHereLogin( parent ) );
		};

		virtual void  setParent( LLPanelLogin* parentIn )
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
	boost::intrusive_ptr< LLIamHereLogin > gResponsePtr = 0;
};

//---------------------------------------------------------------------------
// Public methods
//---------------------------------------------------------------------------
LLPanelLogin::LLPanelLogin(const LLRect &rect,
						 BOOL show_server,
						 void (*callback)(S32 option, void* user_data),
						 void *cb_data)
:	LLPanel("panel_login", LLRect(0,600,800,0), FALSE),		// not bordered
	mLogoImage(),
	mCallback(callback),
	mCallbackData(cb_data),
	mHtmlAvailable( TRUE )
{
	mIsFocusRoot = TRUE;

	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);

	// instance management
	if (LLPanelLogin::sInstance)
	{
		llwarns << "Duplicate instance of login view deleted" << llendl;
		delete LLPanelLogin::sInstance;

		// Don't leave bad pointer in gFocusMgr
		gFocusMgr.setDefaultKeyboardFocus(NULL);
	}

	LLPanelLogin::sInstance = this;

	// add to front so we are the bottom-most child
	gViewerWindow->getRootView()->addChildAtEnd(this);

	// Logo
	mLogoImage = gImageList.getImage("startup_logo.tga", LLUUID::null, MIPMAP_FALSE, TRUE);

	gUICtrlFactory->buildPanel(this, "panel_login.xml");
	
	//leave room for the login menu bar
	setRect(LLRect(0, rect.getHeight()-18, rect.getWidth(), 0)); 
	reshape(rect.getWidth(), rect.getHeight());
	
	// get the web browser control
	#if LL_LIBXUL_ENABLED
	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(this, "login_html");
	if ( web_browser )
	{
		// don't make it a tab stop until SL-27594 is fixed
		web_browser->setTabStop(FALSE);

		// painfully build the path to the loading screen
		std::string loading_path( gDirUtilp->getExpandedFilename( LL_PATH_SKINS, "" ) );
		loading_path.append( gDirUtilp->getDirDelimiter() );
		loading_path.append( "html" );
		loading_path.append( gDirUtilp->getDirDelimiter() );
		loading_path.append( "loading" );
		loading_path.append( gDirUtilp->getDirDelimiter() );
		loading_path.append( "loading.html" );
		web_browser->navigateTo( loading_path.c_str() );

		// make links open in external browser
		web_browser->setOpenInExternalBrowser( true );

		// force the size to be correct (XML doesn't seem to be sufficient to do this) (with some padding so the other login screen doesn't show through)
		LLRect htmlRect = mRect;
		htmlRect.setCenterAndSize( mRect.getCenterX() - 2, mRect.getCenterY(), mRect.getWidth() + 6, mRect.getHeight());
		web_browser->setRect( htmlRect );
		web_browser->reshape( htmlRect.getWidth(), htmlRect.getHeight(), TRUE );
		reshape( mRect.getWidth(), mRect.getHeight(), 1 );    
		
		// kick off a request to grab the url manually
		gResponsePtr = LLIamHereLogin::build( this );
		LLHTTPClient::get( childGetValue( "real_url" ).asString(), gResponsePtr );
	};
	#else
		mHtmlAvailable = FALSE;
	#endif

	// Initialize visibility (and don't force visibility - use prefs)
}

void LLPanelLogin::setSiteIsAlive( bool alive )
{
#if LL_LIBXUL_ENABLED
	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(this, "login_html");
	// if the contents of the site was retrieved
	if ( alive )
	{
		if ( web_browser )
		{
			loadLoginPage();
			
			// mark as available
			mHtmlAvailable = TRUE;
		};
	}
	else
	// the site is not available (missing page, server down, other badness)
	{
		if ( web_browser )
		{
			// hide browser control (revealing default one)
			web_browser->setVisible( FALSE );

			// mark as unavailable
			mHtmlAvailable = FALSE;
		};
	};
#else
	mHtmlAvailable = FALSE;
#endif
}


LLPanelLogin::~LLPanelLogin()
{
	LLPanelLogin::sInstance = NULL;

	// tell the responder we're not here anymore
	if ( gResponsePtr )
		gResponsePtr->setParent( 0 );

	// We know we're done with the image, so be rid of it.
	gImageList.deleteImage( mLogoImage );
}

// virtual
void LLPanelLogin::draw()
{
	if (!getVisible()) return;

	BOOL target_fullscreen;
	S32 target_width;
	S32 target_height;
	gViewerWindow->getTargetWindow(target_fullscreen, target_width, target_height);

	childSetVisible("full_screen_text", target_fullscreen);

	glPushMatrix();
	{
		F32 image_aspect = 1.333333f;
		F32 view_aspect = (F32)mRect.getWidth() / (F32)mRect.getHeight();
		// stretch image to maintain aspect ratio
		if (image_aspect > view_aspect)
		{
			glTranslatef(-0.5f * (image_aspect / view_aspect - 1.f) * mRect.getWidth(), 0.f, 0.f);
			glScalef(image_aspect / view_aspect, 1.f, 1.f);
		}

		S32 width = mRect.getWidth();
		S32 height = mRect.getHeight();

		if ( mHtmlAvailable )
		{
			// draw a background box in black
			gl_rect_2d( 0, height - 264, width, 264, LLColor4( 0.0f, 0.0f, 0.0f, 1.f ) );
		}
		else
		{
			// the HTML login page is not available so default to the original screen
			S32 offscreen_part = height / 3;
			gl_draw_scaled_image(0, -offscreen_part, width, height+offscreen_part, mLogoImage);
		};
	}
	glPopMatrix();

	LLPanel::draw();
}

// virtual
BOOL LLPanelLogin::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if (getVisible() && getEnabled())
	{
		if (( KEY_RETURN == key ) && (MASK_ALT == mask))
		{
			gViewerWindow->toggleFullscreen(FALSE);
			return TRUE;
		}

		if (('P' == key) && (MASK_CONTROL == mask))
		{
			LLFloaterPreference::show(NULL);
			return TRUE;
		}

		if (('T' == key) && (MASK_CONTROL == mask))
		{
			new LLFloaterSimple("floater_test.xml");
			return TRUE;
		}
		
#if LL_LIBXUL_ENABLED
		if ( KEY_F1 == key )
		{
			llinfos << "Spawning HTML help window" << llendl;
			gViewerHtmlHelp.show();
			return TRUE;
		}

# if !LL_RELEASE_FOR_DOWNLOAD
		if ( KEY_F2 == key )
		{
			llinfos << "Spawning floater TOS window" << llendl;
			LLFloaterTOS* tos_dialog = LLFloaterTOS::show(LLFloaterTOS::TOS_TOS,"");
			tos_dialog->startModal();
			return TRUE;
		}
# endif
#endif

		if (!called_from_parent)
		{
			if (KEY_RETURN == key && MASK_NONE == mask)
			{
				// let the panel handle UICtrl processing: calls onClickConnect()
				return LLPanel::handleKeyHere(key, mask, called_from_parent);
			}
		}
	}

	return LLPanel::handleKeyHere(key, mask, called_from_parent);
}
	

// static
void LLPanelLogin::giveFocus()
{

	if (sInstance)
		sInstance->setFocus(TRUE);
}


// static
void LLPanelLogin::show(const LLRect &rect,
						BOOL show_server,
						void (*callback)(S32 option, void* user_data),
						void* callback_data)
{
	new LLPanelLogin(rect, show_server, callback, callback_data);

	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(sInstance, "login_html");
	
	if (!web_browser) return;

	if( !gFocusMgr.getKeyboardFocus() )
	{
		// Grab focus and move cursor to first enabled control
		web_browser->setFocus(TRUE);
	}

	// Make sure that focus always goes here (and use the latest sInstance that was just created)
	gFocusMgr.setDefaultKeyboardFocus(web_browser);
}


// static
void LLPanelLogin::close()
{
	if (sInstance)
	{
		gViewerWindow->getRootView()->removeChild( LLPanelLogin::sInstance );
		
		gFocusMgr.setDefaultKeyboardFocus(NULL);

		delete sInstance;
		sInstance = NULL;
	}
}

// static
void LLPanelLogin::setAlwaysRefresh(bool refresh)
{
	if (LLStartUp::getStartupState() >= STATE_LOGIN_CLEANUP) return;

	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(sInstance, "login_html");

	if (web_browser)
	{
		web_browser->setAlwaysRefresh(refresh);
	}
}



void LLPanelLogin::loadLoginPage()
{
	if (!sInstance) return;

	LLURLSimString::sInstance.parse();

	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(sInstance, "login_html");

	std::ostringstream oStr;

	LLString location;
	LLString region;
	LLString password;
	
	if (LLURLSimString::parse())
	{
		std::ostringstream oRegionStr;
		location = "specify";
		oRegionStr << LLURLSimString::sInstance.mSimName << "/" << LLURLSimString::sInstance.mX << "/"
			 << LLURLSimString::sInstance.mY << "/"
			 << LLURLSimString::sInstance.mZ;
		region = oRegionStr.str();
	}
	else
	{
		if (gSavedSettings.getBOOL("LoginLastLocation"))
		{
			location = "last";
		}
		else
		{
			location = "home";
		}
	}
	
	LLString firstname, lastname;
	
	if (gCmdLineFirstName.empty())
	{
		firstname = gSavedSettings.getString("FirstName");
	}
	else
	{
		firstname = gCmdLineFirstName;
	}
	
	if (gCmdLineLastName.empty())
	{
		lastname = gSavedSettings.getString("LastName");
	}
	else
	{
		lastname = gCmdLineLastName;
	}
	
	LLString version = llformat("%d.%d.%d (%d)",
						LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VIEWER_BUILD);

	char* curl_region = curl_escape(region.c_str(), 0);
	char* curl_channel = curl_escape(gChannelName.c_str(), 0);
	char* curl_version = curl_escape(version.c_str(), 0);

	
	oStr << sInstance->childGetValue( "real_url" ).asString()  << "&firstname=" << firstname <<
		"&lastname=" << lastname << "&location=" << location <<	"&region=" << curl_region <<
		"&grid=" << gGridInfo[gGridChoice].mLabel << "&channel=" << curl_channel <<
		"&version=" << curl_version;

	
	curl_free(curl_region);
	curl_free(curl_channel);
	curl_free(curl_version);

	if (!gCmdLinePassword.empty())
	{
		oStr << "&password=" << gCmdLinePassword;
	}
	else if (!(password = load_password_from_disk()).empty())
	{
		oStr << "&password=$1$" << password;
	}
	if (gAutoLogin)
	{
		oStr << "&auto_login=TRUE";
	}
	if (gSavedSettings.getBOOL("ShowStartLocation"))
	{
		oStr << "&show_start_location=TRUE";
	}	
	if (gSavedSettings.getBOOL("RememberPassword"))
	{
		oStr << "&remember_password=TRUE";
	}	
#ifndef	LL_RELEASE_FOR_DOWNLOAD
	oStr << "&show_grid=TRUE";
#endif
	
	// navigate to the "real" page 
	web_browser->navigateTo( oStr.str() );
}


//---------------------------------------------------------------------------
// Protected methods
//---------------------------------------------------------------------------

// static
void LLPanelLogin::onClickQuit(void*)
{
	if (sInstance && sInstance->mCallback)
	{
		// tell the responder we're not here anymore
		if ( gResponsePtr )
			gResponsePtr->setParent( 0 );

		sInstance->mCallback(1, sInstance->mCallbackData);
	}
}


// static
void LLPanelLogin::onClickVersion(void*)
{
	LLFloaterAbout::show(NULL);
}
