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
#include <openssl/evp.h>
#include <openssl/err.h>
#include <map>
#include "llhttpclient.h"



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
		catch (LLProtectedDataException e)
		{
			exception_msg = e.getMessage();
		}
	}
	if (!exception_msg.empty())  // an exception was thrown.
	{
		throw LLProtectedDataException(exception_msg.c_str());
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

	
// secapiSSLCertVerifyCallback
// basic callback called when a cert verification is requested.
// calls SECAPI to validate the context
// not initialized in the above initialization function, due to unit tests
// see llappviewer

int secapiSSLCertVerifyCallback(X509_STORE_CTX *ctx, void *param)
{
	LLURLRequest *req = (LLURLRequest *)param;
	LLPointer<LLCertificateStore> store = gSecAPIHandler->getCertificateStore("");
	LLPointer<LLCertificateChain> chain = gSecAPIHandler->getCertificateChain(ctx);
	LLSD validation_params = LLSD::emptyMap();
	LLURI uri(req->getURL());
	validation_params[CERT_HOSTNAME] = uri.hostName();
	try
	{
		// we rely on libcurl to validate the hostname, as libcurl does more extensive validation
		// leaving our hostname validation call mechanism for future additions with respect to
		// OS native (Mac keyring, windows CAPI) validation.
		store->validate(VALIDATION_POLICY_SSL & (~VALIDATION_POLICY_HOSTNAME), chain, validation_params);
	}
	catch (LLCertValidationTrustException& cert_exception)
	{
		LL_WARNS("AppInit") << "Cert not trusted: " << cert_exception.getMessage() << LL_ENDL;
		return 0;		
	}
	catch (LLCertException& cert_exception)
	{
		LL_WARNS("AppInit") << "cert error " << cert_exception.getMessage() << LL_ENDL;
		return 0;
	}
	catch (...)
	{
		LL_WARNS("AppInit") << "cert error " << LL_ENDL;
		return 0;
	}
	return 1;
}

LLSD LLCredential::getLoginParams()
{
	LLSD result = LLSD::emptyMap();
	try 
	{
		if (mIdentifier["type"].asString() == "agent")
		{
			// legacy credential
			result["passwd"] = "$1$" + mAuthenticator["secret"].asString();
			result["first"] = mIdentifier["first_name"];
			result["last"] = mIdentifier["last_name"];
		
		}
		else if (mIdentifier["type"].asString() == "account")
		{
			result["username"] = mIdentifier["account_name"];
			result["passwd"] = mAuthenticator["secret"];
										
		}
	}
	catch (...)
	{
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
