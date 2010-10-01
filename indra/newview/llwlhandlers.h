/** 
 * @file llwlhandlers.h
 * @brief Headers for classes in llwlhandlers.cpp
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

#ifndef LL_LLWLHANDLERS_H
#define LL_LLWLHANDLERS_H

#include "llviewerprecompiledheaders.h"
#include "llhttpclient.h"

class LLEnvManager;

class LLEnvironmentRequestResponder: public LLHTTPClient::Responder
{
public:
	virtual void result(const LLSD& content);
	virtual void error(U32 status, const std::string& reason);

private:
	friend class LLEnvManager;
	// returns true if request was sucessfully sent
	static bool initiateRequest();

	LLEnvironmentRequestResponder();
	static int sCount;
	int mID;
};

class LLEnvironmentApplyResponder: public LLHTTPClient::Responder
{
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

	virtual void error(U32 status, const std::string& reason); // non-200 errors only

private:
	friend class LLEnvManager;
	
	static clock_t sLastUpdate;
	static clock_t UPDATE_WAIT_SECONDS;

	// for format of packet expected, see llwlscrubbers.cpp in llmessage
	static bool initiateRequest(const LLSD& packet);

	LLEnvironmentApplyResponder() {}
};

#endif // LL_LLWLHANDLERS_H
