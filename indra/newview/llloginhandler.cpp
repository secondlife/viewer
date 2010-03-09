/** 
 * @file llloginhandler.cpp
 * @brief Handles filling in the login panel information from a SLURL
 * such as secondlife:///app/login?first=Bob&last=Dobbs
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

#include "llloginhandler.h"

// viewer includes
#include "llsecapi.h"
#include "llpanellogin.h"			// save_password_to_disk()
#include "llstartup.h"				// getStartupState()
#include "llslurl.h"
#include "llviewercontrol.h"		// gSavedSettings
#include "llviewernetwork.h"		// EGridInfo
#include "llviewerwindow.h"                    // getWindow()

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
							LLMediaCtrl* web)
{
	if (tokens.size() == 1
		&& tokens[0].asString() == "show")
	{
		// We're using reg-in-client, so show the XUI login widgets
		LLPanelLogin::showLoginWidgets();
		return true;
	}

	if (tokens.size() == 1
		&& tokens[0].asString() == "reg")
	{
		LLWindow* window = gViewerWindow->getWindow();
		window->incBusyCount();
		window->setCursor(UI_CURSOR_ARROW);

		// Do this first, as it may be slow and we want to keep something
		// on the user's screen as long as possible
		LLWeb::loadURLExternal( "http://join.eniac15.lindenlab.com/" );

		window->decBusyCount();
		window->setCursor(UI_CURSOR_ARROW);

		// Then hide the window
		window->minimize();
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
		result =  gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());                       
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
		gSavedSettings.setBOOL("AutoLogin", TRUE);
		return gSecAPIHandler->createCredential(LLGridManager::getInstance()->getGrid(), 
													   identifier, authenticator);
	}
	return NULL;
}
