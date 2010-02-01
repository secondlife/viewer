/** 
 * @file llsecapi.cpp
 * @brief Security API for services such as certificate handling
 * secure local storage, etc.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
#include "llsecapi.h"
#include "llsechandler_basic.h"
#include <map>


std::map<std::string, LLPointer<LLSecAPIHandler> > gHandlerMap;
LLPointer<LLSecAPIHandler> gSecAPIHandler;

void initializeSecHandler()
{
	gHandlerMap[BASIC_SECHANDLER] = new LLSecAPIBasicHandler();
	
	// Currently, we only have the Basic handler, so we can point the main sechandler
	// pointer to the basic handler.  Later, we'll create a wrapper handler that
	// selects the appropriate sechandler as needed, for instance choosing the
	// mac keyring handler, with fallback to the basic sechandler
	gSecAPIHandler = gHandlerMap[BASIC_SECHANDLER];
}
// start using a given security api handler.  If the string is empty
// the default is used
LLPointer<LLSecAPIHandler> getSecHandler(const std::string& handler_type)
{
	if (gHandlerMap.find(handler_type) != gHandlerMap.end())
	{
		return gHandlerMap[handler_type];
	}
	else
	{
		return LLPointer<LLSecAPIHandler>(NULL);
	}
}
// register a handler
void registerSecHandler(const std::string& handler_type, 
						LLPointer<LLSecAPIHandler>& handler)
{
	gHandlerMap[handler_type] = handler;
}

std::ostream& operator <<(std::ostream& s, const LLCredential& cred)
{
	return s << (std::string)cred;
}


LLSD LLCredential::getLoginParams()
{
	LLSD result = LLSD::emptyMap();
	if (mIdentifier["type"].asString() == "agent")
	{
		// legacy credential
		result["passwd"] = "$1$" + mAuthenticator["secret"].asString();
		result["first"] = mIdentifier["first_name"];
		result["last"] = mIdentifier["last_name"];
	
	}
	else if (mIdentifier["type"].asString() == "account")
	{
		result["username"] = mIdentifier["username"];
		result["passwd"] = mAuthenticator["secret"];
                                    
	}
	return result;
}
