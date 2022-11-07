/** 
 * @file llloginhandler.h
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
