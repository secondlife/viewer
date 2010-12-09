/** 
 * @file llpanellogin.cpp
 * @brief Login dialog and logo display
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

#include "llpanellogin.h"

#include "indra_constants.h"		// for key and mask constants
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llmd5.h"
#include "llsecondlifeurls.h"
#include "v4color.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcommandhandler.h"		// for secondlife:///app/login/
#include "llcombobox.h"
#include "llcurl.h"
#include "llviewercontrol.h"
#include "llfloaterpreference.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llsecapi.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llslurl.h"
#include "llversioninfo.h"
#include "llviewerhelp.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"			// for handle_preferences()
#include "llviewernetwork.h"
#include "llviewerwindow.h"			// to link into child list
#include "lluictrlfactory.h"
#include "llhttpclient.h"
#include "llweb.h"
#include "llmediactrl.h"
#include "llrootview.h"

#include "llfloatertos.h"
#include "lltrans.h"
#include "llglheaders.h"
#include "llpanelloginlistener.h"

#if LL_WINDOWS
#pragma warning(disable: 4355)      // 'this' used in initializer list
#endif  // LL_WINDOWS

#include "llsdserialize.h"
#define USE_VIEWER_AUTH 0

const S32 BLACK_BORDER_HEIGHT = 160;
const S32 MAX_PASSWORD = 16;

LLPanelLogin *LLPanelLogin::sInstance = NULL;
BOOL LLPanelLogin::sCapslockDidNotification = FALSE;


class LLLoginRefreshHandler : public LLCommandHandler
{
public:
	// don't allow from external browsers
	LLLoginRefreshHandler() : LLCommandHandler("login_refresh", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{	
		if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
		{
			LLPanelLogin::loadLoginPage();
		}	
		return true;
	}
};

LLLoginRefreshHandler gLoginRefreshHandler;


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

		// We don't actually expect LLSD back, so need to override completedRaw
		virtual void completedRaw(U32 status, const std::string& reason,
								  const LLChannelDescriptors& channels,
								  const LLIOPipe::buffer_ptr_t& buffer)
		{
			completed(status, reason, LLSD()); // will call result() or error()
		}
	
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
:	LLPanel(),
	mLogoImage(),
	mCallback(callback),
	mCallbackData(cb_data),
	mHtmlAvailable( TRUE ),
	mListener(new LLPanelLoginListener(this))
{
	setFocusRoot(TRUE);

	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);

	// instance management
	if (LLPanelLogin::sInstance)
	{
		llwarns << "Duplicate instance of login view deleted" << llendl;
		// Don't leave bad pointer in gFocusMgr
		gFocusMgr.setDefaultKeyboardFocus(NULL);

		delete LLPanelLogin::sInstance;
	}

	mPasswordModified = FALSE;
	LLPanelLogin::sInstance = this;

	// add to front so we are the bottom-most child
	gViewerWindow->getRootView()->addChildInBack(this);

	// Logo
	mLogoImage = LLUI::getUIImage("startup_logo");

	buildFromFile( "panel_login.xml");
	
#if USE_VIEWER_AUTH
	//leave room for the login menu bar
	setRect(LLRect(0, rect.getHeight()-18, rect.getWidth(), 0)); 
#endif
	// Legacy login web page is hidden under the menu bar.
	// Adjust reg-in-client web browser widget to not be hidden.
	if (gSavedSettings.getBOOL("RegInClient"))
	{
		reshape(rect.getWidth(), rect.getHeight() - MENU_BAR_HEIGHT);
	}
	else
	{
		reshape(rect.getWidth(), rect.getHeight());
	}

#if !USE_VIEWER_AUTH
	getChild<LLLineEditor>("password_edit")->setKeystrokeCallback(onPassKey, this);

	// change z sort of clickable text to be behind buttons
	//sendChildToBack(getChildView("channel_text"));
	sendChildToBack(getChildView("forgot_password_text"));

	LLLineEditor* edit = getChild<LLLineEditor>("password_edit");
	if (edit) edit->setDrawAsterixes(TRUE);

	if(LLStartUp::getStartSLURL().getType() != LLSLURL::LOCATION)
	{
		LLSLURL slurl(gSavedSettings.getString("LoginLocation"));
		LLStartUp::setStartSLURL(slurl);
	}
	updateLocationCombo(false);

	LLComboBox* server_choice_combo = sInstance->getChild<LLComboBox>("server_combo");
	server_choice_combo->setCommitCallback(onSelectServer, NULL);
	server_choice_combo->setFocusLostCallback(boost::bind(onServerComboLostFocus, _1));
	updateServerCombo();

	childSetAction("connect_btn", onClickConnect, this);

	getChild<LLPanel>("login")->setDefaultBtn("connect_btn");

	std::string channel = LLVersionInfo::getChannel();
	std::string version = llformat("%s (%d)",
								   LLVersionInfo::getShortVersion().c_str(),
								   LLVersionInfo::getBuild());
	//LLTextBox* channel_text = getChild<LLTextBox>("channel_text");
	//channel_text->setTextArg("[CHANNEL]", channel); // though not displayed
	//channel_text->setTextArg("[VERSION]", version);
	//channel_text->setClickedCallback(onClickVersion, this);
	
	LLTextBox* forgot_password_text = getChild<LLTextBox>("forgot_password_text");
	forgot_password_text->setClickedCallback(onClickForgotPassword, NULL);

	LLTextBox* create_new_account_text = getChild<LLTextBox>("create_new_account_text");
	create_new_account_text->setClickedCallback(onClickNewAccount, NULL);

	LLTextBox* need_help_text = getChild<LLTextBox>("login_help");
	need_help_text->setClickedCallback(onClickHelp, NULL);
#endif    
	
	// get the web browser control
	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("login_html");
	web_browser->addObserver(this);
	
	// Clear the browser's cache to avoid any potential for the cache messing up the login screen.
	web_browser->clearCache();

	reshapeBrowser();

	// kick off a request to grab the url manually
	gResponsePtr = LLIamHereLogin::build( this );

	LLHTTPClient::head( LLGridManager::getInstance()->getLoginPage(), gResponsePtr );

	// Show last logged in user favorites in "Start at" combo if corresponding option is enabled.
	if (gSavedSettings.getBOOL("ShowFavoritesOnLogin"))
	{
	    addUsersWithFavoritesToUsername();
		getChild<LLComboBox>("username_combo")->setTextChangedCallback(boost::bind(&LLPanelLogin::addFavoritesToStartLocation, this));
	}

	updateLocationCombo(false);

}

void LLPanelLogin::addUsersWithFavoritesToUsername()
{
	LLComboBox* combo = getChild<LLComboBox>("username_combo");
	if (!combo) return;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	LLSD fav_llsd;
	llifstream file;
	file.open(filename);
	if (!file.is_open()) return;
	LLSDSerialize::fromXML(fav_llsd, file);
	for (LLSD::map_const_iterator iter = fav_llsd.beginMap();
		iter != fav_llsd.endMap(); ++iter)
	{
		combo->add(iter->first);
	}
}

void LLPanelLogin::addFavoritesToStartLocation()
{
	LLComboBox* combo = getChild<LLComboBox>("start_location_combo");
	if (!combo) return;
	int num_items = combo->getItemCount();
	for (int i = num_items - 1; i > 2; i--)
	{
		combo->remove(i);
	}
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	LLSD fav_llsd;
	llifstream file;
	file.open(filename);
	if (!file.is_open()) return;
	LLSDSerialize::fromXML(fav_llsd, file);
	for (LLSD::map_const_iterator iter = fav_llsd.beginMap();
		iter != fav_llsd.endMap(); ++iter)
	{
		if(iter->first != getChild<LLComboBox>("username_combo")->getSimple()) continue;
		combo->addSeparator();
		LLSD user_llsd = iter->second;
		for (LLSD::array_const_iterator iter1 = user_llsd.beginArray();
			iter1 != user_llsd.endArray(); ++iter1)
		{
			std::string label = (*iter1)["name"].asString();
			std::string value = (*iter1)["slurl"].asString();
			if(label != "" && value != "")
			{
				combo->add(label, value);
			}
		}
		break;
	}
}

// force the size to be correct (XML doesn't seem to be sufficient to do this)
// (with some padding so the other login screen doesn't show through)
void LLPanelLogin::reshapeBrowser()
{
	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("login_html");
	LLRect rect = gViewerWindow->getWindowRectScaled();
	LLRect html_rect;
#if USE_VIEWER_AUTH
	html_rect.setCenterAndSize( 
		rect.getCenterX() - 2, rect.getCenterY(), 
		rect.getWidth() + 6, rect.getHeight());
#else
	html_rect.setCenterAndSize(
		rect.getCenterX() - 2, rect.getCenterY() + 40,
		rect.getWidth() + 6, rect.getHeight() - 78 );
#endif
	web_browser->setRect( html_rect );
	web_browser->reshape( html_rect.getWidth(), html_rect.getHeight(), TRUE );
	reshape( rect.getWidth(), rect.getHeight(), 1 );
}

void LLPanelLogin::setSiteIsAlive( bool alive )
{
	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("login_html");
	// if the contents of the site was retrieved
	if ( alive )
	{
		if ( web_browser )
		{
			loadLoginPage();
			
			// mark as available
			mHtmlAvailable = TRUE;
		}
	}
	else
	// the site is not available (missing page, server down, other badness)
	{
#if !USE_VIEWER_AUTH
		if ( web_browser )
		{
			// hide browser control (revealing default one)
			web_browser->setVisible( FALSE );

			// mark as unavailable
			mHtmlAvailable = FALSE;
		}
#else

		if ( web_browser )
		{	
			web_browser->navigateToLocalPage( "loading-error" , "index.html" );

			// mark as available
			mHtmlAvailable = TRUE;
		}
#endif
	}
}


LLPanelLogin::~LLPanelLogin()
{
	LLPanelLogin::sInstance = NULL;

	// tell the responder we're not here anymore
	if ( gResponsePtr )
		gResponsePtr->setParent( 0 );

	//// We know we're done with the image, so be rid of it.
	//gTextureList.deleteImage( mLogoImage );

	// Controls having keyboard focus by default
	// must reset it on destroy. (EXT-2748)
	gFocusMgr.setDefaultKeyboardFocus(NULL);
}

// virtual
void LLPanelLogin::draw()
{
	glPushMatrix();
	{
		F32 image_aspect = 1.333333f;
		F32 view_aspect = (F32)getRect().getWidth() / (F32)getRect().getHeight();
		// stretch image to maintain aspect ratio
		if (image_aspect > view_aspect)
		{
			glTranslatef(-0.5f * (image_aspect / view_aspect - 1.f) * getRect().getWidth(), 0.f, 0.f);
			glScalef(image_aspect / view_aspect, 1.f, 1.f);
		}

		S32 width = getRect().getWidth();
		S32 height = getRect().getHeight();

		if ( mHtmlAvailable )
		{
#if !USE_VIEWER_AUTH
			if (getChild<LLView>("login_widgets")->getVisible())
			{
				// draw a background box in black
				gl_rect_2d( 0, height - 264, width, 264, LLColor4::black );
				// draw the bottom part of the background image
				// just the blue background to the native client UI
				mLogoImage->draw(0, -264, width + 8, mLogoImage->getHeight());
			}
#endif
		}
		else
		{
			// the HTML login page is not available so default to the original screen
			S32 offscreen_part = height / 3;
			mLogoImage->draw(0, -offscreen_part, width, height+offscreen_part);
		};
	}
	glPopMatrix();

	LLPanel::draw();
}

// virtual
BOOL LLPanelLogin::handleKeyHere(KEY key, MASK mask)
{
	if ( KEY_F1 == key )
	{
		LLViewerHelp* vhelp = LLViewerHelp::getInstance();
		vhelp->showTopic(vhelp->f1HelpTopic());
		return TRUE;
	}

	return LLPanel::handleKeyHere(key, mask);
}

// virtual 
void LLPanelLogin::setFocus(BOOL b)
{
	if(b != hasFocus())
	{
		if(b)
		{
			LLPanelLogin::giveFocus();
		}
		else
		{
			LLPanel::setFocus(b);
		}
	}
}

// static
void LLPanelLogin::giveFocus()
{
#if USE_VIEWER_AUTH
	if (sInstance)
	{
		sInstance->setFocus(TRUE);
	}
#else
	if( sInstance )
	{
		// Grab focus and move cursor to first blank input field
		std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
		std::string pass = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();

		BOOL have_username = !username.empty();
		BOOL have_pass = !pass.empty();

		LLLineEditor* edit = NULL;
		LLComboBox* combo = NULL;
		if (have_username && !have_pass)
		{
			// User saved his name but not his password.  Move
			// focus to password field.
			edit = sInstance->getChild<LLLineEditor>("password_edit");
		}
		else
		{
			// User doesn't have a name, so start there.
			combo = sInstance->getChild<LLComboBox>("username_combo");
		}

		if (edit)
		{
			edit->setFocus(TRUE);
			edit->selectAll();
		}
		else if (combo)
		{
			combo->setFocus(TRUE);
		}
	}
#endif
}

// static
void LLPanelLogin::showLoginWidgets()
{
	sInstance->getChildView("login_widgets")->setVisible( true);
	LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");
	sInstance->reshapeBrowser();
	// *TODO: Append all the usual login parameters, like first_login=Y etc.
	std::string splash_screen_url = sInstance->getString("real_url");
	web_browser->navigateTo( splash_screen_url, "text/html" );
	LLUICtrl* username_combo = sInstance->getChild<LLUICtrl>("username_combo");
	username_combo->setFocus(TRUE);
}

// static
void LLPanelLogin::show(const LLRect &rect,
						BOOL show_server,
						void (*callback)(S32 option, void* user_data),
						void* callback_data)
{
	new LLPanelLogin(rect, show_server, callback, callback_data);

	if( !gFocusMgr.getKeyboardFocus() )
	{
		// Grab focus and move cursor to first enabled control
		sInstance->setFocus(TRUE);
	}

	// Make sure that focus always goes here (and use the latest sInstance that was just created)
	gFocusMgr.setDefaultKeyboardFocus(sInstance);
}

// static
void LLPanelLogin::setFields(LLPointer<LLCredential> credential,
							 BOOL remember)
{
	if (!sInstance)
	{
		llwarns << "Attempted fillFields with no login view shown" << llendl;
		return;
	}
	LL_INFOS("Credentials") << "Setting login fields to " << *credential << LL_ENDL;

	LLSD identifier = credential->getIdentifier();
	if((std::string)identifier["type"] == "agent") 
	{
		std::string firstname = identifier["first_name"].asString();
		std::string lastname = identifier["last_name"].asString();
	    std::string login_id = firstname;
	    if (!lastname.empty() && lastname != "Resident")
	    {
		    // support traditional First Last name SLURLs
		    login_id += " ";
		    login_id += lastname;
	    }
		sInstance->getChild<LLComboBox>("username_combo")->setLabel(login_id);	
	}
	else if((std::string)identifier["type"] == "account")
	{
		sInstance->getChild<LLComboBox>("username_combo")->setLabel((std::string)identifier["account_name"]);		
	}
	else
	{
	  sInstance->getChild<LLComboBox>("username_combo")->setLabel(std::string());	
	}
	if (gSavedSettings.getBOOL("ShowFavoritesOnLogin"))
	{
		sInstance->addFavoritesToStartLocation();
	}
	// if the password exists in the credential, set the password field with
	// a filler to get some stars
	LLSD authenticator = credential->getAuthenticator();
	LL_INFOS("Credentials") << "Setting authenticator field " << authenticator["type"].asString() << LL_ENDL;
	if(authenticator.isMap() && 
	   authenticator.has("secret") && 
	   (authenticator["secret"].asString().size() > 0))
	{
		
		// This is a MD5 hex digest of a password.
		// We don't actually use the password input field, 
		// fill it with MAX_PASSWORD characters so we get a 
		// nice row of asterixes.
		const std::string filler("123456789!123456");
		sInstance->getChild<LLUICtrl>("password_edit")->setValue(std::string("123456789!123456"));
	}
	else
	{
		sInstance->getChild<LLUICtrl>("password_edit")->setValue(std::string());		
	}
	sInstance->getChild<LLUICtrl>("remember_check")->setValue(remember);
}


// static
void LLPanelLogin::getFields(LLPointer<LLCredential>& credential,
							 BOOL& remember)
{
	if (!sInstance)
	{
		llwarns << "Attempted getFields with no login view shown" << llendl;
		return;
	}
	
	// load the credential so we can pass back the stored password or hash if the user did
	// not modify the password field.
	
	credential = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());

	LLSD identifier = LLSD::emptyMap();
	LLSD authenticator = LLSD::emptyMap();
	
	if(credential.notNull())
	{
		authenticator = credential->getAuthenticator();
	}

	std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
	LLStringUtil::trim(username);
	std::string password = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();

	LL_INFOS2("Credentials", "Authentication") << "retrieving username:" << username << LL_ENDL;
	// determine if the username is a first/last form or not.
	size_t separator_index = username.find_first_of(' ');
	if (separator_index == username.npos
		&& !LLGridManager::getInstance()->isSystemGrid())
	{
		LL_INFOS2("Credentials", "Authentication") << "account: " << username << LL_ENDL;
		// single username, so this is a 'clear' identifier
		identifier["type"] = CRED_IDENTIFIER_TYPE_ACCOUNT;
		identifier["account_name"] = username;
		
		if (LLPanelLogin::sInstance->mPasswordModified)
		{
			authenticator = LLSD::emptyMap();
			// password is plaintext
			authenticator["type"] = CRED_AUTHENTICATOR_TYPE_CLEAR;
			authenticator["secret"] = password;
		}
	}
	else
	{
		// Be lenient in terms of what separators we allow for two-word names
		// and allow legacy users to login with firstname.lastname
		separator_index = username.find_first_of(" ._");
		std::string first = username.substr(0, separator_index);
		std::string last;
		if (separator_index != username.npos)
		{
			last = username.substr(separator_index+1, username.npos);
		LLStringUtil::trim(last);
		}
		else
		{
			// ...on Linden grids, single username users as considered to have
			// last name "Resident"
			// *TODO: Make login.cgi support "account_name" like above
			last = "Resident";
		}
		
		if (last.find_first_of(' ') == last.npos)
		{
			LL_INFOS2("Credentials", "Authentication") << "agent: " << username << LL_ENDL;
			// traditional firstname / lastname
			identifier["type"] = CRED_IDENTIFIER_TYPE_AGENT;
			identifier["first_name"] = first;
			identifier["last_name"] = last;
		
			if (LLPanelLogin::sInstance->mPasswordModified)
			{
				authenticator = LLSD::emptyMap();
				authenticator["type"] = CRED_AUTHENTICATOR_TYPE_HASH;
				authenticator["algorithm"] = "md5";
				LLMD5 pass((const U8 *)password.c_str());
				char md5pass[33];               /* Flawfinder: ignore */
				pass.hex_digest(md5pass);
				authenticator["secret"] = md5pass;
			}
		}
	}
	credential = gSecAPIHandler->createCredential(LLGridManager::getInstance()->getGrid(), identifier, authenticator);
	remember = sInstance->getChild<LLUICtrl>("remember_check")->getValue();
}

// static
BOOL LLPanelLogin::isGridComboDirty()
{
	BOOL user_picked = FALSE;
	if (!sInstance)
	{
		llwarns << "Attempted getServer with no login view shown" << llendl;
	}
	else
	{
		LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
		user_picked = combo->isDirty();
	}
	return user_picked;
}

// static
BOOL LLPanelLogin::areCredentialFieldsDirty()
{
	if (!sInstance)
	{
		llwarns << "Attempted getServer with no login view shown" << llendl;
	}
	else
	{
		std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
		LLStringUtil::trim(username);
		std::string password = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();
		LLComboBox* combo = sInstance->getChild<LLComboBox>("username_combo");
		if(combo && combo->isDirty())
		{
			return true;
		}
		LLLineEditor* ctrl = sInstance->getChild<LLLineEditor>("password_edit");
		if(ctrl && ctrl->isDirty()) 
		{
			return true;
		}
	}
	return false;	
}


// static
void LLPanelLogin::updateLocationCombo( bool force_visible )
{
	if (!sInstance) 
	{
		return;
	}	
	
	LLComboBox* combo = sInstance->getChild<LLComboBox>("start_location_combo");
	
	switch(LLStartUp::getStartSLURL().getType())
	{
		case LLSLURL::LOCATION:
		{
			
			combo->setCurrentByIndex( 2 );	
			combo->setTextEntry(LLStartUp::getStartSLURL().getLocationString());	
			break;
		}
		case LLSLURL::HOME_LOCATION:
			combo->setCurrentByIndex(1);
			break;
		default:
			combo->setCurrentByIndex(0);
			break;
	}
	
	BOOL show_start = TRUE;
	
	if ( ! force_visible )
		show_start = gSavedSettings.getBOOL("ShowStartLocation");
	
	sInstance->getChildView("start_location_combo")->setVisible( show_start);
	sInstance->getChildView("start_location_text")->setVisible( show_start);
	
	BOOL show_server = gSavedSettings.getBOOL("ForceShowGrid");
	sInstance->getChildView("server_combo_text")->setVisible( show_server);	
	sInstance->getChildView("server_combo")->setVisible( show_server);
}

// static
void LLPanelLogin::updateStartSLURL()
{
	if (!sInstance) return;
	
	LLComboBox* combo = sInstance->getChild<LLComboBox>("start_location_combo");
	S32 index = combo->getCurrentIndex();
	
	switch (index)
	{
		case 0:
		{
			LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_LAST));
			break;
		}			
		case 1:
		{
			LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_HOME));
			break;
		}
		default:
		{
			LLSLURL slurl = LLSLURL(combo->getValue().asString());
			if(slurl.getType() == LLSLURL::LOCATION)
			{
				// we've changed the grid, so update the grid selection
				LLStartUp::setStartSLURL(slurl);
			}
			break;
		}			
	}
}


void LLPanelLogin::setLocation(const LLSLURL& slurl)
{
	LLStartUp::setStartSLURL(slurl);
	updateServer();
}

// static
void LLPanelLogin::closePanel()
{
	if (sInstance)
	{
		gViewerWindow->getRootView()->removeChild( LLPanelLogin::sInstance );

		delete sInstance;
		sInstance = NULL;
	}
}

// static
void LLPanelLogin::setAlwaysRefresh(bool refresh)
{
	if (LLStartUp::getStartupState() >= STATE_LOGIN_CLEANUP) return;

	LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");

	if (web_browser)
	{
		web_browser->setAlwaysRefresh(refresh);
	}
}



void LLPanelLogin::loadLoginPage()
{
	if (!sInstance) return;
	
	std::ostringstream oStr;

	std::string login_page = LLGridManager::getInstance()->getLoginPage();

	oStr << login_page;
	
	// Use the right delimeter depending on how LLURI parses the URL
	LLURI login_page_uri = LLURI(login_page);
	
	std::string first_query_delimiter = "&";
	if (login_page_uri.queryMap().size() == 0)
	{
		first_query_delimiter = "?";
	}

	// Language
	std::string language = LLUI::getLanguage();
	oStr << first_query_delimiter<<"lang=" << language;
	
	// First Login?
	if (gSavedSettings.getBOOL("FirstLoginThisInstall"))
	{
		oStr << "&firstlogin=TRUE";
	}

	// Channel and Version
	std::string version = llformat("%s (%d)",
								   LLVersionInfo::getShortVersion().c_str(),
								   LLVersionInfo::getBuild());

	char* curl_channel = curl_escape(LLVersionInfo::getChannel().c_str(), 0);
	char* curl_version = curl_escape(version.c_str(), 0);

	oStr << "&channel=" << curl_channel;
	oStr << "&version=" << curl_version;

	curl_free(curl_channel);
	curl_free(curl_version);

	// Grid
	char* curl_grid = curl_escape(LLGridManager::getInstance()->getGridLabel().c_str(), 0);
	oStr << "&grid=" << curl_grid;
	curl_free(curl_grid);
	gViewerWindow->setMenuBackgroundColor(false, !LLGridManager::getInstance()->isInProductionGrid());
	gLoginMenuBarView->setBackgroundColor(gMenuBarView->getBackgroundColor());


#if USE_VIEWER_AUTH
	LLURLSimString::sInstance.parse();

	std::string location;
	std::string region;
	std::string password;
	
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
		location = gSavedSettings.getString("LoginLocation");
	}
	
	std::string username;

    if(gSavedSettings.getLLSD("UserLoginInfo").size() == 3)
    {
        LLSD cmd_line_login = gSavedSettings.getLLSD("UserLoginInfo");
		username = cmd_line_login[0].asString() + " " + cmd_line_login[1];
        password = cmd_line_login[2].asString();
    }
    	
	
	char* curl_region = curl_escape(region.c_str(), 0);

	oStr <<"username=" << username <<
		 "&location=" << location <<	"&region=" << curl_region;
	
	curl_free(curl_region);

	if (!password.empty())
	{
		oStr << "&password=" << password;
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
#else
	if (gSavedSettings.getBOOL("ForceShowGrid"))
		oStr << "&show_grid=TRUE";
#endif
#endif
	
	LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");

	// navigate to the "real" page
	if (gSavedSettings.getBOOL("RegInClient"))
	{
		web_browser->setFocus(TRUE);
		login_page = sInstance->getString("reg_in_client_url");
		web_browser->navigateTo(login_page, "text/html");
	}
	else
	{
		web_browser->navigateTo( oStr.str(), "text/html" );
	}
}

void LLPanelLogin::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
	if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");
		if (web_browser)
		{
			// *HACK HACK HACK HACK!
			/* Stuff a Tab key into the browser now so that the first field will
			** get the focus!  The embedded javascript on the page that properly
			** sets the initial focus in a real web browser is not working inside
			** the viewer, so this is an UGLY HACK WORKAROUND for now.
			*/
			// Commented out as it's not reliable
			//web_browser->handleKey(KEY_TAB, MASK_NONE, false);
		}
	}
}

//---------------------------------------------------------------------------
// Protected methods
//---------------------------------------------------------------------------

// static
void LLPanelLogin::onClickConnect(void *)
{
	if (sInstance && sInstance->mCallback)
	{
		// tell the responder we're not here anymore
		if ( gResponsePtr )
			gResponsePtr->setParent( 0 );

		// JC - Make sure the fields all get committed.
		sInstance->setFocus(FALSE);

		LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
		LLSD combo_val = combo->getSelectedValue();
		if (combo_val.isUndefined())
		{
			combo_val = combo->getValue();
		}
		if(combo_val.isUndefined())
		{
			LLNotificationsUtil::add("StartRegionEmpty");
			return;
		}		
		try
		{
			LLGridManager::getInstance()->setGridChoice(combo_val.asString());
		}
		catch (LLInvalidGridName ex)
		{
			LLSD args;
			args["GRID"] = combo_val.asString();
			LLNotificationsUtil::add("InvalidGrid", args);
			return;
		}
		updateStartSLURL();
		std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();

		
		if(username.empty())
		{
			// user must type in something into the username field
			LLNotificationsUtil::add("MustHaveAccountToLogIn");
		}
		else
		{
			LLPointer<LLCredential> cred;
			BOOL remember;
			getFields(cred, remember);
			std::string identifier_type;
			cred->identifierType(identifier_type);
			LLSD allowed_credential_types;
			LLGridManager::getInstance()->getLoginIdentifierTypes(allowed_credential_types);
			
			// check the typed in credential type against the credential types expected by the server.
			for(LLSD::array_iterator i = allowed_credential_types.beginArray();
				i != allowed_credential_types.endArray();
				i++)
			{
				
				if(i->asString() == identifier_type)
				{
					// yay correct credential type
					sInstance->mCallback(0, sInstance->mCallbackData);
					return;
				}
			}
			
			// Right now, maingrid is the only thing that is picky about
			// credential format, as it doesn't yet allow account (single username)
			// format creds.  - Rox.  James, we wanna fix the message when we change
			// this.
			LLNotificationsUtil::add("InvalidCredentialFormat");			
		}
	}
}

/*
// static
bool LLPanelLogin::newAccountAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		llinfos << "Going to account creation URL" << llendl;
		LLWeb::loadURLExternal( LLNotifications::instance().getGlobalString("CREATE_ACCOUNT_URL")); 
	}
	else
	{
		sInstance->setFocus(TRUE);
	}
	return false;
}
*/

// static
void LLPanelLogin::onClickNewAccount(void*)
{
	LLWeb::loadURLExternal(sInstance->getString("create_account_url"));
}


// static
void LLPanelLogin::onClickVersion(void*)
{
	LLFloaterReg::showInstance("sl_about"); 
}

//static
void LLPanelLogin::onClickForgotPassword(void*)
{
	if (sInstance )
	{
		LLWeb::loadURLExternal(sInstance->getString( "forgot_password_url" ));
	}
}

//static
void LLPanelLogin::onClickHelp(void*)
{
	if (sInstance)
	{
		LLViewerHelp* vhelp = LLViewerHelp::getInstance();
		vhelp->showTopic(vhelp->preLoginTopic());
	}
}

// static
void LLPanelLogin::onPassKey(LLLineEditor* caller, void* user_data)
{
	LLPanelLogin *This = (LLPanelLogin *) user_data;
	This->mPasswordModified = TRUE;
	if (gKeyboard->getKeyDown(KEY_CAPSLOCK) && sCapslockDidNotification == FALSE)
	{
// *TODO: use another way to notify user about enabled caps lock, see EXT-6858
		sCapslockDidNotification = TRUE;
	}
}


void LLPanelLogin::updateServer()
{
	try 
	{

		updateServerCombo();	
		// if they've selected another grid, we should load the credentials
		// for that grid and set them to the UI.
		if(sInstance && !sInstance->areCredentialFieldsDirty())
		{
			LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());	
			bool remember = sInstance->getChild<LLUICtrl>("remember_check")->getValue();
			sInstance->setFields(credential, remember);
		}
		// grid changed so show new splash screen (possibly)
		loadLoginPage();
		updateLocationCombo(LLStartUp::getStartSLURL().getType() == LLSLURL::LOCATION);
	}
	catch (LLInvalidGridName ex)
	{
		// do nothing
	}
}

void LLPanelLogin::updateServerCombo()
{
	if (!sInstance) 
	{
		return;	
	}
	// We add all of the possible values, sorted, and then add a bar and the current value at the top
	LLComboBox* server_choice_combo = sInstance->getChild<LLComboBox>("server_combo");	
	server_choice_combo->removeall();

	std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids(!gSavedSettings.getBOOL("ShowBetaGrids"));

	for (std::map<std::string, std::string>::iterator grid_choice = known_grids.begin();
		 grid_choice != known_grids.end();
		 grid_choice++)
	{
		if (!grid_choice->first.empty())
		{
			server_choice_combo->add(grid_choice->second, grid_choice->first);
		}
	}
	server_choice_combo->sortByName();
	
	server_choice_combo->addSeparator(ADD_TOP);
	
	server_choice_combo->add(LLGridManager::getInstance()->getGridLabel(), 
		LLGridManager::getInstance()->getGrid(), ADD_TOP);	
	
	server_choice_combo->selectFirstItem();	
}

// static
void LLPanelLogin::onSelectServer(LLUICtrl*, void*)
{
	// *NOTE: The paramters for this method are ignored. 
	// LLPanelLogin::onServerComboLostFocus(LLFocusableElement* fe, void*)
	// calls this method.
	LL_INFOS("AppInit") << "onSelectServer" << LL_ENDL;
	// The user twiddled with the grid choice ui.
	// apply the selection to the grid setting.
	LLPointer<LLCredential> credential;
	
	LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
	LLSD combo_val = combo->getSelectedValue();
	if (combo_val.isUndefined())
	{
		combo_val = combo->getValue();
	}
	
	combo = sInstance->getChild<LLComboBox>("start_location_combo");	
	combo->setCurrentByIndex(1);
	LLStartUp::setStartSLURL(LLSLURL(gSavedSettings.getString("LoginLocation")));
	LLGridManager::getInstance()->setGridChoice(combo_val.asString());
	// This new selection will override preset uris
	// from the command line.
	updateServer();
	updateLocationCombo(false);
	updateLoginPanelLinks();
}

void LLPanelLogin::onServerComboLostFocus(LLFocusableElement* fe)
{
	if (!sInstance)
	{
		return;
	}

	LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
	if(fe == combo)
	{
		onSelectServer(combo, NULL);	
	}
}

void LLPanelLogin::updateLoginPanelLinks()
{
	LLSD grid_data;
	LLGridManager::getInstance()->getGridInfo(grid_data);
	bool system_grid = grid_data.has(GRID_IS_SYSTEM_GRID_VALUE);
	
	// need to call through sInstance, as it's called from onSelectServer, which
	// is static.
	sInstance->getChildView("create_new_account_text")->setVisible( system_grid);
	sInstance->getChildView("forgot_password_text")->setVisible( system_grid);
}
