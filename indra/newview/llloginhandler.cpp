/** 
 * @file llloginhandler.cpp
 * @brief Handles filling in the login panel information from a SLURL
 * such as secondlife:///app/login?first=Bob&last=Dobbs
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
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

#include "llloginhandler.h"

// viewer includes
#include "llpanellogin.h"			// save_password_to_disk()
#include "llstartup.h"				// getStartupState()
#include "llurlsimstring.h"
#include "llviewercontrol.h"		// gSavedSettings
#include "llviewernetwork.h"		// EGridInfo

// library includes
#include "llmd5.h"


// Must have instance to auto-register with LLCommandDispatcher
LLLoginHandler gLoginHandler;


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
	
	EGridInfo grid_choice = GRID_INFO_NONE;
	if (queryMap["grid"].asString() == "aditi")
	{
		grid_choice = GRID_INFO_ADITI;
	}
	else if (queryMap["grid"].asString() == "agni")
	{
		grid_choice = GRID_INFO_AGNI;
	}
	else if (queryMap["grid"].asString() == "siva")
	{
		grid_choice = GRID_INFO_SIVA;
	}
	else if (queryMap["grid"].asString() == "damballah")
	{
		grid_choice = GRID_INFO_DAMBALLAH;
	}
	else if (queryMap["grid"].asString() == "durga")
	{
		grid_choice = GRID_INFO_DURGA;
	}
	else if (queryMap["grid"].asString() == "shakti")
	{
		grid_choice = GRID_INFO_SHAKTI;
	}
	else if (queryMap["grid"].asString() == "soma")
	{
		grid_choice = GRID_INFO_SOMA;
	}
	else if (queryMap["grid"].asString() == "ganga")
	{
		grid_choice = GRID_INFO_GANGA;
	}
	else if (queryMap["grid"].asString() == "vaak")
	{
		grid_choice = GRID_INFO_VAAK;
	}
	else if (queryMap["grid"].asString() == "uma")
	{
		grid_choice = GRID_INFO_UMA;
	}
	else if (queryMap["grid"].asString() == "mohini")
	{
		grid_choice = GRID_INFO_MOHINI;
	}
	else if (queryMap["grid"].asString() == "yami")
	{
		grid_choice = GRID_INFO_YAMI;
	}
	else if (queryMap["grid"].asString() == "nandi")
	{
		grid_choice = GRID_INFO_NANDI;
	}
	else if (queryMap["grid"].asString() == "mitra")
	{
		grid_choice = GRID_INFO_MITRA;
	}
	else if (queryMap["grid"].asString() == "radha")
	{
		grid_choice = GRID_INFO_RADHA;
	}
	else if (queryMap["grid"].asString() == "ravi")
	{
		grid_choice = GRID_INFO_RAVI;
	}
	else if (queryMap["grid"].asString() == "aruna")
	{
		grid_choice = GRID_INFO_ARUNA;
	}

	if(grid_choice != GRID_INFO_NONE)
	{
		LLViewerLogin::getInstance()->setGridChoice(grid_choice);
	}

	std::string startLocation = queryMap["location"].asString();

	if (startLocation == "specify")
	{
		LLURLSimString::setString(queryMap["region"].asString());
	}
	else if (startLocation == "home")
	{
		gSavedSettings.setBOOL("LoginLastLocation", FALSE);
		LLURLSimString::setString(LLStringUtil::null);
	}
	else if (startLocation == "last")
	{
		gSavedSettings.setBOOL("LoginLastLocation", TRUE);
		LLURLSimString::setString(LLStringUtil::null);
	}
}

bool LLLoginHandler::handle(const LLSD& tokens,
							const LLSD& query_map,
							LLWebBrowserCtrl* web)
{	
	parse(query_map);
	
	//if we haven't initialized stuff yet, this is 
	//coming in from the GURL handler, just parse
	if (STATE_FIRST == LLStartUp::getStartupState())
	{
		return true;
	}
	
	std::string password = query_map["password"].asString();

	if (!password.empty())
	{
		gSavedSettings.setBOOL("RememberPassword", TRUE);

		if (password.substr(0,3) != "$1$")
		{
			LLMD5 pass((unsigned char*)password.c_str());
			char md5pass[33];		/* Flawfinder: ignore */
			pass.hex_digest(md5pass);
			password = ll_safe_string(md5pass, 32);
			save_password_to_disk(password.c_str());
		}
	}
	else
	{
		save_password_to_disk(NULL);
		gSavedSettings.setBOOL("RememberPassword", FALSE);
	}
			

	if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)  //on splash page
	{
		if (!mFirstName.empty() || !mLastName.empty())
		{
			// Fill in the name, and maybe the password, preserving the
			// remember-password setting. JC
			std::string ignore;
			BOOL remember;
			LLPanelLogin::getFields(ignore, ignore, ignore, remember);
			LLPanelLogin::setFields(mFirstName, mLastName, password, remember);
		}

		if (mWebLoginKey.isNull())
		{
			LLPanelLogin::loadLoginPage();
		}
		else
		{
			LLStartUp::setStartupState( STATE_LOGIN_CLEANUP );
		}
	}
	return true;
}
