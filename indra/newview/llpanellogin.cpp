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
#include "lllayoutstack.h"

#include "indra_constants.h"		// for key and mask constants
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llmd5.h"
#include "v4color.h"

#include "llappviewer.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcommandhandler.h"		// for secondlife:///app/login/
#include "llcombobox.h"
#include "llviewercontrol.h"
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

LLPanelLogin *LLPanelLogin::sInstance = NULL;
BOOL LLPanelLogin::sCapslockDidNotification = FALSE;
BOOL LLPanelLogin::sCredentialSet = FALSE;

// Helper functions

LLPointer<LLCredential> load_user_credentials(std::string &user_key)
{
    if (gSecAPIHandler->hasCredentialMap("login_list", LLGridManager::getInstance()->getGrid()))
    {
        // user_key should be of "name Resident" format
        return gSecAPIHandler->loadFromCredentialMap("login_list", LLGridManager::getInstance()->getGrid(), user_key);
    }
    else
    {
        // legacy (or legacy^2, since it also tries to load from settings)
        return gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
    }
}

class LLLoginLocationAutoHandler : public LLCommandHandler
{
public:
	// don't allow from external browsers
	LLLoginLocationAutoHandler() : LLCommandHandler("location_login", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{	
		if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
		{
			if ( tokens.size() == 0 || tokens.size() > 4 ) 
				return false;

			// unescape is important - uris with spaces are escaped in this code path
			// (e.g. space -> %20) and the code to log into a region doesn't support that.
			const std::string region = LLURI::unescape( tokens[0].asString() );

			// just region name as payload 
			if ( tokens.size() == 1 )
			{
				// region name only - slurl will end up as center of region
				LLSLURL slurl(region);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			// region name and x coord as payload 
			if ( tokens.size() == 2 )
			{
				// invalid to only specify region and x coordinate
				// slurl code will revert to same as region only, so do this anyway
				LLSLURL slurl(region);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			// region name and x/y coord as payload 
			if ( tokens.size() == 3 )
			{
				// region and x/y specified - default z to 0
				F32 xpos;
				std::istringstream codec(tokens[1].asString());
				codec >> xpos;

				F32 ypos;
				codec.clear();
				codec.str(tokens[2].asString());
				codec >> ypos;

				const LLVector3 location(xpos, ypos, 0.0f);
				LLSLURL slurl(region, location);

				LLPanelLogin::autologinToLocation(slurl);
			}
			else
			// region name and x/y/z coord as payload 
			if ( tokens.size() == 4 )
			{
				// region and x/y/z specified - ok
				F32 xpos;
				std::istringstream codec(tokens[1].asString());
				codec >> xpos;

				F32 ypos;
				codec.clear();
				codec.str(tokens[2].asString());
				codec >> ypos;

				F32 zpos;
				codec.clear();
				codec.str(tokens[3].asString());
				codec >> zpos;

				const LLVector3 location(xpos, ypos, zpos);
				LLSLURL slurl(region, location);

				LLPanelLogin::autologinToLocation(slurl);
			};
		}	
		return true;
	}
};
LLLoginLocationAutoHandler gLoginLocationAutoHandler;

//---------------------------------------------------------------------------
// Public methods
//---------------------------------------------------------------------------
LLPanelLogin::LLPanelLogin(const LLRect &rect,
						 void (*callback)(S32 option, void* user_data),
						 void *cb_data)
:	LLPanel(),
	mCallback(callback),
	mCallbackData(cb_data),
	mListener(new LLPanelLoginListener(this)),
	mFirstLoginThisInstall(gSavedSettings.getBOOL("FirstLoginThisInstall")),
	mUsernameLength(0),
	mPasswordLength(0),
	mLocationLength(0),
	mShowFavorites(false)
{
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);

	mPasswordModified = FALSE;

	sInstance = this;

	LLView* login_holder = gViewerWindow->getLoginPanelHolder();
	if (login_holder)
	{
		login_holder->addChild(this);
	}

	if (mFirstLoginThisInstall)
	{
		buildFromFile( "panel_login_first.xml");
	}
	else
	{
		buildFromFile( "panel_login.xml");
	}

	reshape(rect.getWidth(), rect.getHeight());

	LLLineEditor* password_edit(getChild<LLLineEditor>("password_edit"));
	password_edit->setKeystrokeCallback(onPassKey, this);
	// STEAM-14: When user presses Enter with this field in focus, initiate login
	password_edit->setCommitCallback(boost::bind(&LLPanelLogin::onClickConnect, false));

	// change z sort of clickable text to be behind buttons
	sendChildToBack(getChildView("forgot_password_text"));
	sendChildToBack(getChildView("sign_up_text"));

    std::string current_grid = LLGridManager::getInstance()->getGrid();
    if (!mFirstLoginThisInstall)
    {
        LLComboBox* favorites_combo = getChild<LLComboBox>("start_location_combo");
        updateLocationSelectorsVisibility(); // separate so that it can be called from preferences
        favorites_combo->setReturnCallback(boost::bind(&LLPanelLogin::onClickConnect, false));
        favorites_combo->setFocusLostCallback(boost::bind(&LLPanelLogin::onLocationSLURL, this));

        LLComboBox* server_choice_combo = getChild<LLComboBox>("server_combo");
        server_choice_combo->setCommitCallback(boost::bind(&LLPanelLogin::onSelectServer, this));

        // Load all of the grids, sorted, and then add a bar and the current grid at the top
        server_choice_combo->removeall();

        std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
        for (std::map<std::string, std::string>::iterator grid_choice = known_grids.begin();
            grid_choice != known_grids.end();
            grid_choice++)
        {
            if (!grid_choice->first.empty() && current_grid != grid_choice->first)
            {
                LL_DEBUGS("AppInit") << "adding " << grid_choice->first << LL_ENDL;
                server_choice_combo->add(grid_choice->second, grid_choice->first);
            }
        }
        server_choice_combo->sortByName();

        LL_DEBUGS("AppInit") << "adding current " << current_grid << LL_ENDL;
        server_choice_combo->add(LLGridManager::getInstance()->getGridLabel(),
            current_grid,
            ADD_TOP);
        server_choice_combo->selectFirstItem();
    }

	LLSLURL start_slurl(LLStartUp::getStartSLURL());
	// The StartSLURL might have been set either by an explicit command-line
	// argument (CmdLineLoginLocation) or by default.
	// current_grid might have been set either by an explicit command-line
	// argument (CmdLineGridChoice) or by default.
	// If the grid specified by StartSLURL is the same as current_grid, the
	// distinction is moot.
	// If we have an explicit command-line SLURL, use that.
	// If we DON'T have an explicit command-line SLURL but we DO have an
	// explicit command-line grid, which is different from the default SLURL's
	// -- do NOT override the explicit command-line grid with the grid from
	// the default SLURL!
	bool force_grid{ start_slurl.getGrid() != current_grid &&
					 gSavedSettings.getString("CmdLineLoginLocation").empty() &&
				   ! gSavedSettings.getString("CmdLineGridChoice").empty() };
	if ( !start_slurl.isSpatial() ) // has a start been established by the command line or NextLoginLocation ? 
	{
		// no, so get the preference setting
		std::string defaultStartLocation = gSavedSettings.getString("LoginLocation");
		LL_INFOS("AppInit")<<"default LoginLocation '"<<defaultStartLocation<<"'"<<LL_ENDL;
		LLSLURL defaultStart(defaultStartLocation);
		if ( defaultStart.isSpatial() && ! force_grid )
		{
			LLStartUp::setStartSLURL(defaultStart);
		}
		else
		{
			LL_INFOS("AppInit") << (force_grid? "--grid specified" : "no valid LoginLocation")
								<< ", using home" << LL_ENDL;
			LLSLURL homeStart(LLSLURL::SIM_LOCATION_HOME);
			LLStartUp::setStartSLURL(homeStart);
		}
	}
	else if (! force_grid)
	{
		onUpdateStartSLURL(start_slurl); // updates grid if needed
	}
	
	childSetAction("connect_btn", onClickConnect, this);

	LLButton* def_btn = getChild<LLButton>("connect_btn");
	setDefaultBtn(def_btn);

	std::string channel = LLVersionInfo::instance().getChannel();
	std::string version = llformat("%s (%d)",
								   LLVersionInfo::instance().getShortVersion().c_str(),
								   LLVersionInfo::instance().getBuild());
	
	LLTextBox* forgot_password_text = getChild<LLTextBox>("forgot_password_text");
	forgot_password_text->setClickedCallback(onClickForgotPassword, NULL);

	LLTextBox* sign_up_text = getChild<LLTextBox>("sign_up_text");
	sign_up_text->setClickedCallback(onClickSignUp, NULL);

	// get the web browser control
	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("login_html");
	web_browser->addObserver(this);

	loadLoginPage();

	LLComboBox* username_combo(getChild<LLComboBox>("username_combo"));
	username_combo->setTextChangedCallback(boost::bind(&LLPanelLogin::onUserNameTextEnty, this));
	// STEAM-14: When user presses Enter with this field in focus, initiate login
	username_combo->setCommitCallback(boost::bind(&LLPanelLogin::onUserListCommit, this));
	username_combo->setReturnCallback(boost::bind(&LLPanelLogin::onClickConnect, this));
	username_combo->setKeystrokeOnEsc(TRUE);


    LLCheckBoxCtrl* remember_name = getChild<LLCheckBoxCtrl>("remember_name");
    remember_name->setCommitCallback(boost::bind(&LLPanelLogin::onRememberUserCheck, this));
    getChild<LLCheckBoxCtrl>("remember_password")->setCommitCallback(boost::bind(&LLPanelLogin::onRememberPasswordCheck, this));
}

void LLPanelLogin::addFavoritesToStartLocation()
{
    if (mFirstLoginThisInstall)
    {
        // first login panel has no favorites, just update name length and buttons
        std::string user_defined_name = getChild<LLComboBox>("username_combo")->getSimple();
        mUsernameLength = user_defined_name.length();
        updateLoginButtons();
        return;
    }

	// Clear the combo.
	LLComboBox* combo = getChild<LLComboBox>("start_location_combo");
	if (!combo) return;
	int num_items = combo->getItemCount();
	for (int i = num_items - 1; i > 1; i--)
	{
		combo->remove(i);
	}

	// Load favorites into the combo.
	std::string user_defined_name = getChild<LLComboBox>("username_combo")->getSimple();
	LLStringUtil::trim(user_defined_name);
	LLStringUtil::toLower(user_defined_name);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites_" + LLGridManager::getInstance()->getGrid() + ".xml");
	std::string old_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	mUsernameLength = user_defined_name.length();
	updateLoginButtons();

	std::string::size_type index = user_defined_name.find_first_of(" ._");
	if (index != std::string::npos)
	{
		std::string username = user_defined_name.substr(0, index);
		std::string lastname = user_defined_name.substr(index+1);
		if (lastname == "resident")
		{
			user_defined_name = username;
		}
		else
		{
			user_defined_name = username + " " + lastname;
		}
	}

	LLSD fav_llsd;
	llifstream file;
	file.open(filename.c_str());
	if (!file.is_open())
	{
		file.open(old_filename.c_str());
		if (!file.is_open()) return;
	}
	LLSDSerialize::fromXML(fav_llsd, file);

	for (LLSD::map_const_iterator iter = fav_llsd.beginMap();
		iter != fav_llsd.endMap(); ++iter)
	{
		// The account name in stored_favorites.xml has Resident last name even if user has
		// a single word account name, so it can be compared case-insensitive with the
		// user defined "firstname lastname".
		S32 res = LLStringUtil::compareInsensitive(user_defined_name, iter->first);
		if (res != 0)
		{
			LL_DEBUGS() << "Skipping favorites for " << iter->first << LL_ENDL;
			continue;
		}

		combo->addSeparator();
		LL_DEBUGS() << "Loading favorites for " << iter->first << LL_ENDL;
		LLSD user_llsd = iter->second;
        bool update_password_setting = true;
		for (LLSD::array_const_iterator iter1 = user_llsd.beginArray();
			iter1 != user_llsd.endArray(); ++iter1)
		{
            if ((*iter1).has("save_password"))
            {
                bool save_password = (*iter1)["save_password"].asBoolean();
                gSavedSettings.setBOOL("RememberPassword", save_password);
                if (!save_password)
                {
                    getChild<LLButton>("connect_btn")->setEnabled(false);
                }
                update_password_setting = false;
            }

            std::string label = (*iter1)["name"].asString();
			std::string value = (*iter1)["slurl"].asString();
			if(label != "" && value != "")
			{
				mShowFavorites = true;
				combo->add(label, value);
				if ( LLStartUp::getStartSLURL().getSLURLString() == value)
				{
					combo->selectByValue(value);
				}
			}
		}
        if (update_password_setting)
        {
            gSavedSettings.setBOOL("UpdateRememberPasswordSetting", TRUE);
        }
		break;
	}
	if (combo->getValue().asString().empty())
	{
		combo->selectFirstItem();
        // Value 'home' or 'last' should have been taken from NextLoginLocation
        // but NextLoginLocation was not set, so init it from combo explicitly
        onLocationSLURL();
	}
}

LLPanelLogin::~LLPanelLogin()
{
	LLPanelLogin::sInstance = NULL;

	// Controls having keyboard focus by default
	// must reset it on destroy. (EXT-2748)
	gFocusMgr.setDefaultKeyboardFocus(NULL);
}

// virtual
void LLPanelLogin::setFocus(BOOL b)
{
	if(b != hasFocus())
	{
		if(b)
		{
			giveFocus();
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
}

// static
void LLPanelLogin::show(const LLRect &rect,
						void (*callback)(S32 option, void* user_data),
						void* callback_data)
{
    if (!LLPanelLogin::sInstance)
    {
        new LLPanelLogin(rect, callback, callback_data);
    }

	if( !gFocusMgr.getKeyboardFocus() )
	{
		// Grab focus and move cursor to first enabled control
		sInstance->setFocus(TRUE);
	}

	// Make sure that focus always goes here (and use the latest sInstance that was just created)
	gFocusMgr.setDefaultKeyboardFocus(sInstance);
}

//static
void LLPanelLogin::reshapePanel()
{
    if (sInstance)
    {
        LLRect rect = sInstance->getRect();
        sInstance->reshape(rect.getWidth(), rect.getHeight());
    }
}

//static
void LLPanelLogin::populateFields(LLPointer<LLCredential> credential, bool remember_user, bool remember_psswrd)
{
    if (!sInstance)
    {
        LL_WARNS() << "Attempted fillFields with no login view shown" << LL_ENDL;
        return;
    }

    sInstance->getChild<LLUICtrl>("remember_name")->setValue(remember_user);
    LLUICtrl* remember_password = sInstance->getChild<LLUICtrl>("remember_password");
    remember_password->setValue(remember_user && remember_psswrd);
    remember_password->setEnabled(remember_user);
    sInstance->populateUserList(credential);
}

//static
void LLPanelLogin::resetFields()
{
    if (!sInstance)
    {
        // class not existing at this point might happen since this
        // function is used to reset list in case of changes by external sources
        return;
    }
    if (sInstance->mFirstLoginThisInstall)
    {
        // no list to populate
        LL_WARNS() << "Shouldn't happen, user should have no ability to modify list on first install" << LL_ENDL;
    }
    else
    {
        LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
        sInstance->populateUserList(cred);
    }
}

// static
void LLPanelLogin::setFields(LLPointer<LLCredential> credential)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted fillFields with no login view shown" << LL_ENDL;
		return;
	}
	sCredentialSet = TRUE;
	LL_INFOS("Credentials") << "Setting login fields to " << *credential << LL_ENDL;

	LLSD identifier = credential.notNull() ? credential->getIdentifier() : LLSD();

	if(identifier.has("type") && (std::string)identifier["type"] == "agent") 
	{
		// not nessesary for panel_login.xml, needed for panel_login_first.xml
		std::string firstname = identifier["first_name"].asString();
		std::string lastname = identifier["last_name"].asString();
	    std::string login_id = firstname;
	    if (!lastname.empty() && lastname != "Resident" && lastname != "resident")
	    {
		    // support traditional First Last name SLURLs
		    login_id += " ";
		    login_id += lastname;
	    }
		sInstance->getChild<LLComboBox>("username_combo")->setLabel(login_id);
		sInstance->mUsernameLength = login_id.length();
	}
	else if(identifier.has("type") && (std::string)identifier["type"] == "account")
	{
		std::string login_id = identifier["account_name"].asString();
		sInstance->getChild<LLComboBox>("username_combo")->setLabel(login_id);
		sInstance->mUsernameLength = login_id.length();
	}
	else
	{
		sInstance->getChild<LLComboBox>("username_combo")->setLabel(std::string());
		sInstance->mUsernameLength = 0;
	}

	sInstance->addFavoritesToStartLocation();
	// if the password exists in the credential, set the password field with
	// a filler to get some stars
	LLSD authenticator = credential.notNull() ? credential->getAuthenticator() : LLSD();
	LL_INFOS("Credentials") << "Setting authenticator field " << authenticator["type"].asString() << LL_ENDL;
	if(authenticator.isMap() && 
	   authenticator.has("secret") && 
	   (authenticator["secret"].asString().size() > 0))
	{
		
		// This is a MD5 hex digest of a password.
		// We don't actually use the password input field, 
		// fill it with MAX_PASSWORD characters so we get a 
		// nice row of asterisks.
		const std::string filler("123456789!123456");
		sInstance->getChild<LLUICtrl>("password_edit")->setValue(filler);
		sInstance->mPasswordLength = filler.length();
		sInstance->updateLoginButtons();
	}
	else
	{
		sInstance->getChild<LLUICtrl>("password_edit")->setValue(std::string());
		sInstance->mPasswordLength = 0;
	}
}

// static
void LLPanelLogin::getFields(LLPointer<LLCredential>& credential,
							 bool& remember_user,
							 bool& remember_psswrd)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted getFields with no login view shown" << LL_ENDL;
		return;
	}

	LLSD identifier = LLSD::emptyMap();
	LLSD authenticator = LLSD::emptyMap();

	std::string username = sInstance->getChild<LLComboBox>("username_combo")->getSimple();
	std::string password = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();
	LLStringUtil::trim(username);

	LL_INFOS("Credentials", "Authentication") << "retrieving username:" << username << LL_ENDL;
	// determine if the username is a first/last form or not.
	size_t separator_index = username.find_first_of(' ');
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
			LL_INFOS("Credentials", "Authentication") << "agent: " << username << LL_ENDL;
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
            else
            {
                std::string key = first + "_" + last;
                LLStringUtil::toLower(key);
                credential = load_user_credentials(key);
                if (credential.notNull())
                {
                    authenticator = credential->getAuthenticator();
                }
            }
		}
	}
	credential = gSecAPIHandler->createCredential(LLGridManager::getInstance()->getGrid(), identifier, authenticator);

    remember_psswrd = sInstance->getChild<LLUICtrl>("remember_password")->getValue();
    remember_user = sInstance->getChild<LLUICtrl>("remember_name")->getValue();
}


// static
BOOL LLPanelLogin::areCredentialFieldsDirty()
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted getServer with no login view shown" << LL_ENDL;
	}
	else
	{
		LLComboBox* combo = sInstance->getChild<LLComboBox>("username_combo");
		if (combo && combo->getCurrentIndex() == -1 && !combo->getValue().asString().empty())
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
void LLPanelLogin::updateLocationSelectorsVisibility()
{
	if (sInstance) 
	{
		BOOL show_server = gSavedSettings.getBOOL("ForceShowGrid");
		LLComboBox* server_combo = sInstance->getChild<LLComboBox>("server_combo");
		if ( server_combo ) 
		{
			server_combo->setVisible(show_server);
		}
	}	
}

// static - called from LLStartUp::setStartSLURL
void LLPanelLogin::onUpdateStartSLURL(const LLSLURL& new_start_slurl)
{
	if (!sInstance) return;

	LL_DEBUGS("AppInit")<<new_start_slurl.asString()<<LL_ENDL;

	LLComboBox* location_combo = sInstance->getChild<LLComboBox>("start_location_combo");
	/*
	 * Determine whether or not the new_start_slurl modifies the grid.
	 *
	 * Note that some forms that could be in the slurl are grid-agnostic.,
	 * such as "home".  Other forms, such as
	 * https://grid.example.com/region/Party%20Town/20/30/5 
	 * specify a particular grid; in those cases we want to change the grid
	 * and the grid selector to match the new value.
	 */
	enum LLSLURL::SLURL_TYPE new_slurl_type = new_start_slurl.getType();
	switch ( new_slurl_type )
	{
	case LLSLURL::LOCATION:
	  {
		std::string slurl_grid = LLGridManager::getInstance()->getGrid(new_start_slurl.getGrid());
		if ( ! slurl_grid.empty() ) // is that a valid grid?
		{
			if ( slurl_grid != LLGridManager::getInstance()->getGrid() ) // new grid?
			{
				// the slurl changes the grid, so update everything to match
				LLGridManager::getInstance()->setGridChoice(slurl_grid);

				// update the grid selector to match the slurl
				LLComboBox* server_combo = sInstance->getChild<LLComboBox>("server_combo");
				std::string server_label(LLGridManager::getInstance()->getGridLabel(slurl_grid));
				server_combo->setSimple(server_label);

				updateServer(); // to change the links and splash screen
			}
			if ( new_start_slurl.getLocationString().length() )
			{
					
				location_combo->setLabel(new_start_slurl.getLocationString());
				sInstance->mLocationLength = new_start_slurl.getLocationString().length();
				sInstance->updateLoginButtons();
			}
		}
		else
		{
			// the grid specified by the slurl is not known
			LLNotificationsUtil::add("InvalidLocationSLURL");
			LL_WARNS("AppInit")<<"invalid LoginLocation:"<<new_start_slurl.asString()<<LL_ENDL;
			location_combo->setTextEntry(LLStringUtil::null);
		}
	  }
 	break;

	case LLSLURL::HOME_LOCATION:
		//location_combo->setCurrentByIndex(0); // home location
		break;

	default:
		LL_WARNS("AppInit")<<"invalid login slurl, using home"<<LL_ENDL;
		//location_combo->setCurrentByIndex(0); // home location
		break;
	}
}

void LLPanelLogin::setLocation(const LLSLURL& slurl)
{
	LL_DEBUGS("AppInit")<<"setting Location "<<slurl.asString()<<LL_ENDL;
	LLStartUp::setStartSLURL(slurl); // calls onUpdateStartSLURL, above
}

void LLPanelLogin::autologinToLocation(const LLSLURL& slurl)
{
	LL_DEBUGS("AppInit")<<"automatically logging into Location "<<slurl.asString()<<LL_ENDL;
	LLStartUp::setStartSLURL(slurl); // calls onUpdateStartSLURL, above

	if ( LLPanelLogin::sInstance != NULL )
	{
		void* unused_parameter = 0;
		LLPanelLogin::sInstance->onClickConnect(unused_parameter);
	}
}


// static
void LLPanelLogin::closePanel()
{
	if (sInstance)
	{
		if (LLPanelLogin::sInstance->getParent())
		{
			LLPanelLogin::sInstance->getParent()->removeChild(LLPanelLogin::sInstance);
		}

		delete sInstance;
		sInstance = NULL;
	}
}

// static
void LLPanelLogin::setAlwaysRefresh(bool refresh)
{
	if (sInstance && LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
	{
		LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");

		if (web_browser)
		{
			web_browser->setAlwaysRefresh(refresh);
		}
	}
}



void LLPanelLogin::loadLoginPage()
{
	if (!sInstance) return;

	LLURI login_page = LLURI(LLGridManager::getInstance()->getLoginPage());
	LLSD params(login_page.queryMap());

	LL_DEBUGS("AppInit") << "login_page: " << login_page << LL_ENDL;

	// allow users (testers really) to specify a different login content URL
	std::string force_login_url = gSavedSettings.getString("ForceLoginURL");
	if ( force_login_url.length() > 0 )
	{
		login_page = LLURI(force_login_url);
	}

	// Language
	params["lang"] = LLUI::getLanguage();

	// First Login?
	if (gSavedSettings.getBOOL("FirstLoginThisInstall"))
	{
		params["firstlogin"] = "TRUE"; // not bool: server expects string TRUE
	}

	// Channel and Version
	params["version"] = llformat("%s (%d)",
								 LLVersionInfo::instance().getShortVersion().c_str(),
								 LLVersionInfo::instance().getBuild());
	params["channel"] = LLVersionInfo::instance().getChannel();

	// Grid
	params["grid"] = LLGridManager::getInstance()->getGridId();

	// add OS info
	params["os"] = LLOSInfo::instance().getOSStringSimple();

	// sourceid
	params["sourceid"] = gSavedSettings.getString("sourceid");

	// login page (web) content version
	params["login_content_version"] = gSavedSettings.getString("LoginContentVersion");

	// Make an LLURI with this augmented info
	std::string url = login_page.scheme().empty()? login_page.authority() : login_page.scheme() + "://" + login_page.authority();
	LLURI login_uri(LLURI::buildHTTP(url,
									 login_page.path(),
									 params));

	gViewerWindow->setMenuBackgroundColor(false, !LLGridManager::getInstance()->isInProductionGrid());

	LLMediaCtrl* web_browser = sInstance->getChild<LLMediaCtrl>("login_html");
	if (web_browser->getCurrentNavUrl() != login_uri.asString())
	{
		LL_DEBUGS("AppInit") << "loading:    " << login_uri << LL_ENDL;
		web_browser->navigateTo( login_uri.asString(), "text/html" );
	}
}

void LLPanelLogin::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
}

//---------------------------------------------------------------------------
// Protected methods
//---------------------------------------------------------------------------
// static
void LLPanelLogin::onClickConnect(bool commit_fields)
{
	if (sInstance && sInstance->mCallback)
	{
		if (commit_fields)
		{
			// JC - Make sure the fields all get committed.
			sInstance->setFocus(FALSE);
		}

		LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
		LLSD combo_val = combo->getSelectedValue();

		// the grid definitions may come from a user-supplied grids.xml, so they may not be good
		LL_DEBUGS("AppInit")<<"grid "<<combo_val.asString()<<LL_ENDL;
		try
		{
			LLGridManager::getInstance()->setGridChoice(combo_val.asString());
		}
		catch (LLInvalidGridName ex)
		{
			LLSD args;
			args["GRID"] = ex.name();
			LLNotificationsUtil::add("InvalidGrid", args);
			return;
		}

		// The start location SLURL has already been sent to LLStartUp::setStartSLURL

		std::string username = sInstance->getChild<LLUICtrl>("username_combo")->getValue().asString();
		std::string password = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();
		
		if(username.empty())
		{
			// user must type in something into the username field
			LLNotificationsUtil::add("MustHaveAccountToLogIn");
		}
		else if(password.empty())
		{
		    LLNotificationsUtil::add("MustEnterPasswordToLogIn");
		}
		else
		{
			sCredentialSet = FALSE;
			LLPointer<LLCredential> cred;
			bool remember_1, remember_2;
			getFields(cred, remember_1, remember_2);
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
void LLPanelLogin::onClickSignUp(void*)
{
	if (sInstance)
	{
		LLWeb::loadURLExternal(sInstance->getString("sign_up_url"));
	}
}

// static
void LLPanelLogin::onUserNameTextEnty(void*)
{
    sInstance->mPasswordModified = true;
    sInstance->getChild<LLUICtrl>("password_edit")->setValue(std::string());
    sInstance->mPasswordLength = 0;
    sInstance->addFavoritesToStartLocation(); //will call updateLoginButtons()
}

// static
void LLPanelLogin::onUserListCommit(void*)
{
    if (sInstance)
    {
        LLComboBox* username_combo(sInstance->getChild<LLComboBox>("username_combo"));
        static S32 ind = -1;
        if (ind != username_combo->getCurrentIndex())
        {
            std::string user_key = username_combo->getSelectedValue();
            LLPointer<LLCredential> cred = gSecAPIHandler->loadFromCredentialMap("login_list", LLGridManager::getInstance()->getGrid(), user_key);
            setFields(cred);
            sInstance->mPasswordModified = false;
        }
        else
        {
           std::string pass = sInstance->getChild<LLUICtrl>("password_edit")->getValue().asString();
           if (pass.empty())
           {
               sInstance->giveFocus();
           }
           else
           {
               onClickConnect();
           }
        }
    }
}

// static
void LLPanelLogin::onRememberUserCheck(void*)
{
    if (sInstance)
    {
        LLCheckBoxCtrl* remember_name(sInstance->getChild<LLCheckBoxCtrl>("remember_name"));
        LLCheckBoxCtrl* remember_psswrd(sInstance->getChild<LLCheckBoxCtrl>("remember_password"));
        LLComboBox* user_combo(sInstance->getChild<LLComboBox>("username_combo"));

        bool remember = remember_name->getValue().asBoolean();
        if (!sInstance->mFirstLoginThisInstall
            && user_combo->getCurrentIndex() != -1
            && !remember)
        {
            remember = true;
            remember_name->setValue(true);
            LLNotificationsUtil::add("LoginCantRemoveUsername");
        }
        if (!remember)
        {
            remember_psswrd->setValue(false);
        }
        remember_psswrd->setEnabled(remember);        
    }
}

void LLPanelLogin::onRememberPasswordCheck(void*)
{
    if (sInstance)
    {
        gSavedSettings.setBOOL("UpdateRememberPasswordSetting", TRUE);

        LLPointer<LLCredential> cred;
        bool remember_user, remember_password;
        getFields(cred, remember_user, remember_password);

        std::string grid(LLGridManager::getInstance()->getGridId());
        std::string user_id(cred->userID());
        if (!remember_password)
        {
            gSecAPIHandler->removeFromProtectedMap("mfa_hash", grid, user_id);
            gSecAPIHandler->syncProtectedMap();
        }
    }
}

// static
void LLPanelLogin::onPassKey(LLLineEditor* caller, void* user_data)
{
	LLPanelLogin *self = (LLPanelLogin *)user_data;
	self->mPasswordModified = TRUE;
	if (gKeyboard->getKeyDown(KEY_CAPSLOCK) && sCapslockDidNotification == FALSE)
	{
		// *TODO: use another way to notify user about enabled caps lock, see EXT-6858
		sCapslockDidNotification = TRUE;
	}

	LLLineEditor* password_edit(self->getChild<LLLineEditor>("password_edit"));
	self->mPasswordLength = password_edit->getText().length();
	self->updateLoginButtons();
}


void LLPanelLogin::updateServer()
{
	if (sInstance)
	{
		try 
		{
			// if they've selected another grid, we should load the credentials
			// for that grid and set them to the UI. But if there were any modifications to
			// fields, modifications should carry over.
			// Not sure if it should carry over password but it worked like this before login changes
			// Example: you started typing in and found that your are under wrong grid,
			// you switch yet don't lose anything
			if (sInstance->areCredentialFieldsDirty())
			{
				// save modified creds
				LLComboBox* user_combo = sInstance->getChild<LLComboBox>("username_combo");
				LLLineEditor* pswd_edit = sInstance->getChild<LLLineEditor>("password_edit");
				std::string username = user_combo->getSimple();
				LLStringUtil::trim(username);
				std::string password = pswd_edit->getValue().asString();

				// populate dropbox and setFields
				// Note: following call is related to initializeLoginInfo()
				LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
				sInstance->populateUserList(credential);

				// restore creds
				user_combo->setTextEntry(username);
				pswd_edit->setValue(password);
				sInstance->mUsernameLength = username.length();
				sInstance->mPasswordLength = password.length();
			}
			else
			{
				// populate dropbox and setFields
				// Note: following call is related to initializeLoginInfo()
				LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
				sInstance->populateUserList(credential);
			}

			// update the login panel links 
			bool system_grid = LLGridManager::getInstance()->isSystemGrid();

			// Want to vanish not only create_new_account_btn, but also the
			// title text over it, so turn on/off the whole layout_panel element.
			sInstance->getChild<LLLayoutPanel>("links")->setVisible(system_grid);
			sInstance->getChildView("forgot_password_text")->setVisible(system_grid);

			// grid changed so show new splash screen (possibly)
			loadLoginPage();
		}
		catch (LLInvalidGridName ex)
		{
			LL_WARNS("AppInit")<<"server '"<<ex.name()<<"' selection failed"<<LL_ENDL;
			LLSD args;
			args["GRID"] = ex.name();
			LLNotificationsUtil::add("InvalidGrid", args);	
			return;
		}
	}
}

void LLPanelLogin::updateLoginButtons()
{
	LLButton* login_btn = getChild<LLButton>("connect_btn");

	login_btn->setEnabled(mUsernameLength != 0 && mPasswordLength != 0);

	if (!mFirstLoginThisInstall)
	{
		LLComboBox* user_combo = getChild<LLComboBox>("username_combo");
		LLCheckBoxCtrl* remember_name = getChild<LLCheckBoxCtrl>("remember_name");
		if (user_combo->getCurrentIndex() != -1)
		{
			remember_name->setValue(true);
			LLCheckBoxCtrl* remember_pass = getChild<LLCheckBoxCtrl>("remember_password");
			remember_pass->setEnabled(TRUE);
		} // Note: might be good idea to do "else remember_name->setValue(mRememberedState)" but it might behave 'weird' to user
	}
}

void LLPanelLogin::populateUserList(LLPointer<LLCredential> credential)
{
    LLComboBox* user_combo = getChild<LLComboBox>("username_combo");
    user_combo->removeall();
    user_combo->clear();
    user_combo->setValue(std::string());
    getChild<LLUICtrl>("password_edit")->setValue(std::string());
    mUsernameLength = 0;
    mPasswordLength = 0;

    if (gSecAPIHandler->hasCredentialMap("login_list", LLGridManager::getInstance()->getGrid()))
    {
        LLSecAPIHandler::credential_map_t credencials;
        gSecAPIHandler->loadCredentialMap("login_list", LLGridManager::getInstance()->getGrid(), credencials);

        LLSecAPIHandler::credential_map_t::iterator cr_iter = credencials.begin();
        LLSecAPIHandler::credential_map_t::iterator cr_end = credencials.end();
        while (cr_iter != cr_end)
        {
            if (cr_iter->second.notNull()) // basic safety in case of future changes
            {
                // cr_iter->first == user_id , to be able to be find it in case we select it
                user_combo->add(LLPanelLogin::getUserName(cr_iter->second), cr_iter->first, ADD_BOTTOM, TRUE);
            }
            cr_iter++;
        }

        if (credential.isNull() || !user_combo->setSelectedByValue(LLSD(credential->userID()), true))
        {
            // selection failed, fields will be mepty
            updateLoginButtons();
        }
        else
        {
            setFields(credential);
        }
    }
    else
    {
        if (credential.notNull())
        {
            const LLSD &ident = credential->getIdentifier();
            if (ident.isMap() && ident.has("type"))
            {
                // this llsd might hold invalid credencial (failed login), so
                // do not add to the list, just set field.
                setFields(credential);
            }
            else
            {
                updateLoginButtons();
            }
        }
        else
        {
            updateLoginButtons();
        }
    }
}


void LLPanelLogin::onSelectServer()
{
	// The user twiddled with the grid choice ui.
	// apply the selection to the grid setting.
	LLComboBox* server_combo = getChild<LLComboBox>("server_combo");
	LLSD server_combo_val = server_combo->getSelectedValue();
	LL_INFOS("AppInit") << "grid "<<server_combo_val.asString()<< LL_ENDL;
	LLGridManager::getInstance()->setGridChoice(server_combo_val.asString());
	addFavoritesToStartLocation();
	
	/*
	 * Determine whether or not the value in the start_location_combo makes sense
	 * with the new grid value.
	 *
	 * Note that some forms that could be in the location combo are grid-agnostic,
	 * such as "MyRegion/128/128/0".  There could be regions with that name on any
	 * number of grids, so leave them alone.  Other forms, such as
	 * https://grid.example.com/region/Party%20Town/20/30/5 specify a particular
	 * grid; in those cases we want to clear the location.
	 */
	LLComboBox* location_combo = getChild<LLComboBox>("start_location_combo");
	S32 index = location_combo->getCurrentIndex();
	switch (index)
	{
	case 0: // last location
        LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_LAST));
        break;
	case 1: // home location
        LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_HOME));
		break;
		
	default:
		{
			std::string location = location_combo->getValue().asString();
			LLSLURL slurl(location); // generata a slurl from the location combo contents
			if (location.empty()
				|| (slurl.getType() == LLSLURL::LOCATION
				    && slurl.getGrid() != LLGridManager::getInstance()->getGrid())
				   )
			{
				// the grid specified by the location is not this one, so clear the combo
				location_combo->setCurrentByIndex(0); // last location on the new grid
				onLocationSLURL();
			}
		}			
		break;
	}

	updateServer();
}

void LLPanelLogin::onLocationSLURL()
{
	LLComboBox* location_combo = getChild<LLComboBox>("start_location_combo");
	std::string location = location_combo->getValue().asString();
	LL_DEBUGS("AppInit")<<location<<LL_ENDL;

	LLStartUp::setStartSLURL(location); // calls onUpdateStartSLURL, above 
}

// static
bool LLPanelLogin::getShowFavorites()
{
	return gSavedPerAccountSettings.getBOOL("ShowFavoritesOnLogin");
}

// static
std::string LLPanelLogin::getUserName(LLPointer<LLCredential> &cred)
{
    if (cred.isNull())
    {
        return "unknown";
    }
    const LLSD &ident = cred->getIdentifier();

    if (!ident.isMap())
    {
        return "unknown";
    }
    else if ((std::string)ident["type"] == "agent")
    {
        std::string second_name = ident["last_name"];
        if (second_name == "resident" || second_name == "Resident")
        {
            return (std::string)ident["first_name"];
        }
        return (std::string)ident["first_name"] + " " + (std::string)ident["last_name"];
    }
    else if ((std::string)ident["type"] == "account")
    {
        return LLCacheName::cleanFullName((std::string)ident["account_name"]);
    }

    return "unknown";
}

