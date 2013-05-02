/** 
 * @file llwlhandlers.h
 * @brief Headers for classes in llwlhandlers.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLWLHANDLERS_H
#define LL_LLWLHANDLERS_H

#include "llviewerprecompiledheaders.h"
#include "llhttpclient.h"

class LLEnvironmentRequest
{
	LOG_CLASS(LLEnvironmentRequest);
public:
	/// @return true if request was successfully sent
	static bool initiate();

private:
	static void onRegionCapsReceived(const LLUUID& region_id);
	static bool doRequest();
};

class LLEnvironmentRequestResponder: public LLHTTPClient::Responder
{
	LOG_CLASS(LLEnvironmentRequestResponder);
public:
	virtual void result(const LLSD& content);
	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content);

private:
	friend class LLEnvironmentRequest;

	LLEnvironmentRequestResponder();
	static int sCount;
	int mID;
};

class LLEnvironmentApply
{
	LOG_CLASS(LLEnvironmentApply);
public:
	/// @return true if request was successfully sent
	static bool initiateRequest(const LLSD& content);

private:
	static clock_t sLastUpdate;
	static clock_t UPDATE_WAIT_SECONDS;
};

class LLEnvironmentApplyResponder: public LLHTTPClient::Responder
{
	LOG_CLASS(LLEnvironmentApplyResponder);
public:
	/*
	 * Expecting reply from sim in form of:
	 * {
	 *   regionID : uuid,
	 *   messageID: uuid,
	 *   success : true
	 * }
	 * or
	 * {
	 *   regionID : uuid,
	 *   success : false,
	 *   fail_reason : string
	 * }
	 */
	virtual void result(const LLSD& content);

	// non-200 errors only
	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content);

private:
	friend class LLEnvironmentApply;
	
	LLEnvironmentApplyResponder() {}
};

#endif // LL_LLWLHANDLERS_H
