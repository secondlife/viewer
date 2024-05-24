/**
 * @file llloginhandler.cpp
 * @brief Handles filling in the login panel information from a SLURL
 * such as secondlife:///app/login?first=Bob&last=Dobbs
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llloginhandler.h"

// viewer includes
#include "llsecapi.h"
#include "lllogininstance.h"        // to check if logged in yet
#include "llpanellogin.h"
#include "llstartup.h"              // getStartupState()
#include "llslurl.h"
#include "llviewercontrol.h"        // gSavedSettings
#include "llviewernetwork.h"        // EGridInfo
#include "llviewerwindow.h"         // getWindow()

// library includes
#include "llmd5.h"
#include "llweb.h"
#include "llwindow.h"


// Must have instance to auto-register with LLCommandDispatcher
LLLoginHandler gLoginHandler;


//parses the input url and returns true if afterwards
//a web-login-key, firstname and lastname  is set
bool LLLoginHandler::parseDirectLogin(std::string url)
{
    LLURI uri(url);
    parse(uri.queryMap());

    // NOTE: Need to add direct login as per identity evolution
    return true;
}

void LLLoginHandler::parse(const LLSD& queryMap)
{

    if (queryMap.has("grid"))
    {
      LLGridManager::getInstance()->setGridChoice(queryMap["grid"].asString());
    }


    std::string startLocation = queryMap["location"].asString();

    if (startLocation == "specify")
    {
      LLStartUp::setStartSLURL(LLSLURL(LLGridManager::getInstance()->getGridLoginID(),
                       queryMap["region"].asString()));
    }
    else if (startLocation == "home")
    {
      LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_HOME));
    }
    else if (startLocation == "last")
    {
      LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_LAST));
    }
}

bool LLLoginHandler::handle(const LLSD& tokens,
                            const LLSD& query_map,
                            const std::string& grid,
                            LLMediaCtrl* web)
{
    // do nothing if we are already logged in
    if (LLLoginInstance::getInstance()->authSuccess())
    {
        LL_WARNS_ONCE("SLURL") << "Already logged in! Ignoring login SLapp." << LL_ENDL;
        return true;
    }

    // Make sure window is visible
    LLWindow* window = gViewerWindow->getWindow();
    if (window->getMinimized())
    {
        window->restore();
    }

    parse(query_map);

    //if we haven't initialized stuff yet, this is
    //coming in from the GURL handler, just parse
    if (STATE_FIRST == LLStartUp::getStartupState())
    {
        return true;
    }

    if  (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)  //on splash page
    {
      // as the login page may change from grid to grid, as well as
      // things like username/password/etc, we simply refresh the
      // login page to make sure everything is set up correctly
      LLPanelLogin::loadLoginPage();
      LLStartUp::setStartupState( STATE_LOGIN_CLEANUP );
    }
    return true;
}



//  Initialize the credentials
// If the passed in URL contains login info, parse
// that into a credential and web login key.  Otherwise
// check the command line.  If the command line
// does not contain any login creds, load the last saved
// ones from the protected credential store.
// This always returns with a credential structure set in the
// login handler
LLPointer<LLCredential> LLLoginHandler::initializeLoginInfo()
{
    LLPointer<LLCredential> result = NULL;
    // so try to load it from the UserLoginInfo
    result = loadSavedUserLoginInfo();
    if (result.isNull())
    {
        // Since legacy viewer store login info one per grid, newer viewers have to
        // reuse same information to remember last user and for compatibility,
        // but otherwise login info is stored in separate map in gSecAPIHandler
        result = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
    }

    return result;
}


LLPointer<LLCredential> LLLoginHandler::loadSavedUserLoginInfo()
{
  // load the saved user login info into a LLCredential.
  // perhaps this should be moved.
    LLSD cmd_line_login = gSavedSettings.getLLSD("UserLoginInfo");
    if (cmd_line_login.size() == 3)
    {

        LLMD5 pass((unsigned char*)cmd_line_login[2].asString().c_str());
        char md5pass[33];               /* Flawfinder: ignore */
        pass.hex_digest(md5pass);
        LLSD identifier = LLSD::emptyMap();
        identifier["type"] = "agent";
        identifier["first_name"] = cmd_line_login[0];
        identifier["last_name"] = cmd_line_login[1];

        LLSD authenticator = LLSD::emptyMap();
        authenticator["type"] = "hash";
        authenticator["algorithm"] = "md5";
        authenticator["secret"] = md5pass;
        // yuck, we'll fix this with mani's changes.
        return gSecAPIHandler->createCredential(LLGridManager::getInstance()->getGrid(),
                                                       identifier, authenticator);
    }
    return NULL;
}
