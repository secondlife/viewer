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
#include <openssl/evp.h>
#include <map>
#include "llhttpclient.h"



std::map<std::string, LLPointer<LLSecAPIHandler> > gHandlerMap;
LLPointer<LLSecAPIHandler> gSecAPIHandler;

void initializeSecHandler()
{
	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_ciphers();
	OpenSSL_add_all_digests();	
	gHandlerMap[BASIC_SECHANDLER] = new LLSecAPIBasicHandler();
	
	
	// Currently, we only have the Basic handler, so we can point the main sechandler
	// pointer to the basic handler.  Later, we'll create a wrapper handler that
	// selects the appropriate sechandler as needed, for instance choosing the
	// mac keyring handler, with fallback to the basic sechandler
	gSecAPIHandler = gHandlerMap[BASIC_SECHANDLER];

	// initialize all SecAPIHandlers
	LLProtectedDataException ex = LLProtectedDataException("");
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
			ex = e;
		}
	}
	if (ex.getMessage().length() > 0 )  // an exception was thrown.
	{
		throw ex;
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
		chain->validate(VALIDATION_POLICY_SSL, store, validation_params);
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
