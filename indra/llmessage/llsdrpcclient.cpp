/** 
 * @file llsdrpcclient.cpp
 * @author Phoenix
 * @date 2005-11-05
 * @brief Implementation of the llsd client classes.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "linden_common.h"
#include "llsdrpcclient.h"

#include "llbufferstream.h"
#include "llfiltersd2xmlrpc.h"
#include "llpumpio.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llurlrequest.h"

/**
 * String constants
 */
static std::string LLSDRPC_RESPONSE_NAME("response");
static std::string LLSDRPC_FAULT_NAME("fault");

/** 
 * LLSDRPCResponse
 */
LLSDRPCResponse::LLSDRPCResponse() :
	mIsError(false),
	mIsFault(false)
{
}

// virtual
LLSDRPCResponse::~LLSDRPCResponse()
{
}

bool LLSDRPCResponse::extractResponse(const LLSD& sd)
{
	bool rv = true;
	if(sd.has(LLSDRPC_RESPONSE_NAME))
	{
		mReturnValue = sd[LLSDRPC_RESPONSE_NAME];
		mIsFault = false;
	}
	else if(sd.has(LLSDRPC_FAULT_NAME))
	{
		mReturnValue = sd[LLSDRPC_FAULT_NAME];
		mIsFault = true;
	}
	else
	{
		mReturnValue.clear();
		mIsError = true;
		rv = false;
	}
	return rv;
}

static LLFastTimer::DeclareTimer FTM_SDRPC_RESPONSE("SDRPC Response");

// virtual
LLIOPipe::EStatus LLSDRPCResponse::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLFastTimer t(FTM_SDRPC_RESPONSE);
	PUMP_DEBUG;
	if(mIsError)
	{
		error(pump);
	}
	else if(mIsFault)
	{
		fault(pump);
	}
	else
	{
		response(pump);
	}
	PUMP_DEBUG;
	return STATUS_DONE;
}

/** 
 * LLSDRPCClient
 */

LLSDRPCClient::LLSDRPCClient() :
	mState(STATE_NONE),
	mQueue(EPBQ_PROCESS)
{
}

// virtual
LLSDRPCClient::~LLSDRPCClient()
{
}

bool LLSDRPCClient::call(
	const std::string& uri,
	const std::string& method,
	const LLSD& parameter,
	LLSDRPCResponse* response,
	EPassBackQueue queue)
{
	//llinfos << "RPC: " << uri << "." << method << "(" << *parameter << ")"
	//		<< llendl;
	if(method.empty() || !response)
	{
		return false;
	}
	mState = STATE_READY;
	mURI.assign(uri);
	std::stringstream req;
	req << LLSDRPC_REQUEST_HEADER_1 << method
		<< LLSDRPC_REQUEST_HEADER_2;
	LLSDSerialize::toNotation(parameter, req);
	req << LLSDRPC_REQUEST_FOOTER;
	mRequest = req.str();
	mQueue = queue;
	mResponse = response;
	return true;
}

bool LLSDRPCClient::call(
	const std::string& uri,
	const std::string& method,
	const std::string& parameter,
	LLSDRPCResponse* response,
	EPassBackQueue queue)
{
	//llinfos << "RPC: " << uri << "." << method << "(" << parameter << ")"
	//		<< llendl;
	if(method.empty() || parameter.empty() || !response)
	{
		return false;
	}
	mState = STATE_READY;
	mURI.assign(uri);
	std::stringstream req;
	req << LLSDRPC_REQUEST_HEADER_1 << method
		<< LLSDRPC_REQUEST_HEADER_2 << parameter
		<< LLSDRPC_REQUEST_FOOTER;
	mRequest = req.str();
	mQueue = queue;
	mResponse = response;
	return true;
}

static LLFastTimer::DeclareTimer FTM_PROCESS_SDRPC_CLIENT("SDRPC Client");

// virtual
LLIOPipe::EStatus LLSDRPCClient::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLFastTimer t(FTM_PROCESS_SDRPC_CLIENT);
	PUMP_DEBUG;
	if((STATE_NONE == mState) || (!pump))
	{
		// You should have called the call() method already.
		return STATUS_PRECONDITION_NOT_MET;
	}
	EStatus rv = STATUS_DONE;
	switch(mState)
	{
	case STATE_READY:
	{
		PUMP_DEBUG;
//		lldebugs << "LLSDRPCClient::process_impl STATE_READY" << llendl;
		buffer->append(
			channels.out(),
			(U8*)mRequest.c_str(),
			mRequest.length());
		context[CONTEXT_DEST_URI_SD_LABEL] = mURI;
		mState = STATE_WAITING_FOR_RESPONSE;
		break;
	}
	case STATE_WAITING_FOR_RESPONSE:
	{
		PUMP_DEBUG;
		// The input channel has the sd response in it.
		//lldebugs << "LLSDRPCClient::process_impl STATE_WAITING_FOR_RESPONSE"
		//		 << llendl;
		LLBufferStream resp(channels, buffer.get());
		LLSD sd;
		LLSDSerialize::fromNotation(sd, resp, buffer->count(channels.in()));
		LLSDRPCResponse* response = (LLSDRPCResponse*)mResponse.get();
		if (!response)
		{
			mState = STATE_DONE;
			break;
		}
		response->extractResponse(sd);
		if(EPBQ_PROCESS == mQueue)
		{
			LLPumpIO::chain_t chain;
			chain.push_back(mResponse);
			pump->addChain(chain, DEFAULT_CHAIN_EXPIRY_SECS);
		}
		else
		{
			pump->respond(mResponse.get());
		}
		mState = STATE_DONE;
		break;
	}
	case STATE_DONE:
	default:
		PUMP_DEBUG;
		llinfos << "invalid state to process" << llendl;
		rv = STATUS_ERROR;
		break;
	}
	return rv;
}
