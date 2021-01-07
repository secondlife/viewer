/** 
 * @file llsecapi.cpp
 * @brief Security API for services such as certificate handling
 * secure local storage, etc.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llsecapi.h"
#include "llsechandler_basic.h"
#include "llexception.h"
#include "stringize.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <map>



std::map<std::string, LLPointer<LLSecAPIHandler> > gHandlerMap;
LLPointer<LLSecAPIHandler> gSecAPIHandler;

void initializeSecHandler()
{
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();

	gHandlerMap[BASIC_SECHANDLER] = new LLSecAPIBasicHandler();
	
	
	// Currently, we only have the Basic handler, so we can point the main sechandler
	// pointer to the basic handler.  Later, we'll create a wrapper handler that
	// selects the appropriate sechandler as needed, for instance choosing the
	// mac keyring handler, with fallback to the basic sechandler
	gSecAPIHandler = gHandlerMap[BASIC_SECHANDLER];

	// initialize all SecAPIHandlers
	std::string exception_msg;
	std::map<std::string, LLPointer<LLSecAPIHandler> >::const_iterator itr;
	for(itr = gHandlerMap.begin(); itr != gHandlerMap.end(); ++itr)
	{
		LLPointer<LLSecAPIHandler> handler = (*itr).second;
		try 
		{
			handler->init();
		}
		catch (LLProtectedDataException& e)
		{
			exception_msg = e.what();
		}
	}
	if (!exception_msg.empty())  // an exception was thrown.
	{
		LLTHROW(LLProtectedDataException(exception_msg));
	}

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
	std::string username;
	try 
	{
		if (mIdentifier["type"].asString() == "agent")
		{
			// legacy credential
			result["passwd"] = "$1$" + mAuthenticator["secret"].asString();
			result["first"] = mIdentifier["first_name"];
			result["last"] = mIdentifier["last_name"];
			username = result["first"].asString() + " " + result["last"].asString();
		}
		else if (mIdentifier["type"].asString() == "account")
		{
			result["username"] = mIdentifier["account_name"];
			result["passwd"] = mAuthenticator["secret"].asString();
			username = result["username"].asString();
		}
	}
	catch (...)
	{
		// nat 2016-08-18: not clear what exceptions the above COULD throw?!
		LOG_UNHANDLED_EXCEPTION(STRINGIZE("for '" << username << "'"));
		// we could have corrupt data, so simply return a null login param if so
		LL_WARNS("AppInit") << "Invalid credential" << LL_ENDL;
	}
	return result;
}

void LLCredential::identifierType(std::string &idType)
{
	if(mIdentifier.has("type"))
	{
		idType = mIdentifier["type"].asString();
	}
	else {
		idType = std::string();
		
	}
}

void LLCredential::authenticatorType(std::string &idType)
{
	if(mAuthenticator.has("type"))
	{
		idType = mAuthenticator["type"].asString();
	}
	else {
		idType = std::string();
		
	}
}

LLCertException::LLCertException(const LLSD& cert_data, const std::string& msg)
  : LLException(msg),
    mCertData(cert_data)
{
    LL_WARNS("SECAPI") << "Certificate Error: " << msg << LL_ENDL;
}
