/** 
 * @file llloginhandler.h
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
#ifndef LLLOGINHANDLER_H
#define LLLOGINHANDLER_H

#include "llcommandhandler.h"
#include "llsecapi.h"

class LLLoginHandler : public LLCommandHandler
{
 public:
	// allow from external browsers
	LLLoginHandler() : LLCommandHandler("login", UNTRUSTED_ALLOW) { }
	/*virtual*/ bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web);

	// Fill in our internal fields from a SLURL like
	// secondlife:///app/login?first=Bob&last=Dobbs
	bool parseDirectLogin(std::string url);

	// Web-based login unsupported
	//LLUUID getWebLoginKey() const { return mWebLoginKey; }

	LLPointer<LLCredential> loadSavedUserLoginInfo();  
	LLPointer<LLCredential> initializeLoginInfo();

private:
	void parse(const LLSD& queryMap);

};

extern LLLoginHandler gLoginHandler;

#endif
