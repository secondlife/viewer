/** 
 * @file llsdrpcclient.cpp
 * @author Phoenix
 * @date 2005-11-05
 * @brief Implementation of the llsd client classes.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#include "llsdrpcclient.h"

#include "llbufferstream.h"
#include "llfiltersd2xmlrpc.h"
#include "llmemtype.h"
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
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
}

// virtual
LLSDRPCResponse::~LLSDRPCResponse()
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
}

bool LLSDRPCResponse::extractResponse(const LLSD& sd)
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
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

// virtual
LLIOPipe::EStatus LLSDRPCResponse::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
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
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
}

// virtual
LLSDRPCClient::~LLSDRPCClient()
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
}

bool LLSDRPCClient::call(
	const std::string& uri,
	const std::string& method,
	const LLSD& parameter,
	LLSDRPCResponse* response,
	EPassBackQueue queue)
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
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
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
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

// virtual
LLIOPipe::EStatus LLSDRPCClient::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_SD_CLIENT);
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
