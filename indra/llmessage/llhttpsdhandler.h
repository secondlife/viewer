/**
* @file llhttpsdhandler.h
* @brief Public-facing declarations for the HttpHandler class
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#ifndef _LLHTTPSDHANDLER_H_
#define _LLHTTPSDHANDLER_H_
#include "httpcommon.h"
#include "httphandler.h"
#include "lluri.h"

/// Handler class LLCore's HTTP library. Splitting with separate success and 
/// failure routines and parsing the result body into LLSD on success.  It 
/// is intended to be subclassed for specific capability handling.
/// 
// *TODO: This class self deletes at the end of onCompleted method.  This is 
// less than ideal and should be revisited.
class LLHttpSDHandler : public LLCore::HttpHandler //,
//    public std::enable_shared_from_this<LLHttpSDHandler>
{
public:

    virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);
    
protected:
    LLHttpSDHandler();

    virtual void onSuccess(LLCore::HttpResponse * response, const LLSD &content) = 0;
    virtual void onFailure(LLCore::HttpResponse * response, LLCore::HttpStatus status) = 0;


};

#endif
