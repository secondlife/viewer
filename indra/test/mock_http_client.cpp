/** 
 * @file mock_http_client.cpp
 * @brief Framework for testing HTTP requests
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "linden_common.h"
#include "llsdhttpserver.h"
#include "lliohttpserver.h"

namespace tut
{
	class SuccessNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
	   	{ 
	   		LLSD result;
			result["state"] = "complete";
	   		result["test"] = "test";
	   		r->result(result);
	   	}
		void post(ResponsePtr r, const LLSD& context, const LLSD& input) const
		{
			LLSD result;
			result["state"] = "complete";
			result["test"] = "test";
			r->result(result);
		}
	};

	class ErrorNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
			{ r->status(599, "Intentional error"); }
		void post(ResponsePtr r, const LLSD& context, const LLSD& input) const
			{ r->status(input["status"], input["reason"]); }
	};

	class TimeOutNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
		{
            /* do nothing, the request will eventually time out */ 
		}
	};

	LLSD storage;
	
	class LLSDStorageNode : public LLHTTPNode
	{
	public:
		LLSD get() const{ return storage; }
		LLSD put(const LLSD& value) const{ storage = value; return LLSD(); }
	};

	LLHTTPRegistration<LLSDStorageNode> gStorageNode("/test/storage");
	LLHTTPRegistration<SuccessNode>		gSuccessNode("/test/success");
	LLHTTPRegistration<ErrorNode>		gErrorNode("/test/error");
	LLHTTPRegistration<TimeOutNode>		gTimeOutNode("/test/timeout");
}
