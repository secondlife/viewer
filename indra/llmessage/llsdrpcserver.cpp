/** 
 * @file llsdrpcserver.cpp
 * @author Phoenix
 * @date 2005-10-11
 * @brief Implementation of the LLSDRPCServer and related classes.
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
#include "llsdrpcserver.h"

#include "llbuffer.h"
#include "llbufferstream.h"
#include "llmemtype.h"
#include "llpumpio.h"
#include "llsdserialize.h"
#include "llstl.h"

static const char FAULT_PART_1[] = "{'fault':{'code':i";
static const char FAULT_PART_2[] = ", 'description':'";
static const char FAULT_PART_3[] = "'}}";

static const char RESPONSE_PART_1[] = "{'response':";
static const char RESPONSE_PART_2[] = "}";

static const S32 FAULT_GENERIC = 1000;
static const S32 FAULT_METHOD_NOT_FOUND = 1001;

static const std::string LLSDRPC_METHOD_SD_NAME("method");
static const std::string LLSDRPC_PARAMETER_SD_NAME("parameter");


/**
 * LLSDRPCServer
 */
LLSDRPCServer::LLSDRPCServer() :
	mState(LLSDRPCServer::STATE_NONE),
	mPump(NULL),
	mLock(0)
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_SERVER);
}

LLSDRPCServer::~LLSDRPCServer()
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_SERVER);
	std::for_each(
		mMethods.begin(),
		mMethods.end(),
		llcompose1(
			DeletePointerFunctor<LLSDRPCMethodCallBase>(),
			llselect2nd<method_map_t::value_type>()));
	std::for_each(
		mCallbackMethods.begin(),
		mCallbackMethods.end(),
		llcompose1(
			DeletePointerFunctor<LLSDRPCMethodCallBase>(),
			llselect2nd<method_map_t::value_type>()));
}


// virtual
ESDRPCSStatus LLSDRPCServer::deferredResponse(
        const LLChannelDescriptors& channels,
	LLBufferArray* data) {
    // subclass should provide a sane implementation
    return ESDRPCS_DONE;
}

void LLSDRPCServer::clearLock()
{
	if(mLock && mPump)
	{
		mPump->clearLock(mLock);
		mPump = NULL;
		mLock = 0;
	}
}

// virtual
LLIOPipe::EStatus LLSDRPCServer::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_SD_SERVER);
//	lldebugs << "LLSDRPCServer::process_impl" << llendl;
	// Once we have all the data, We need to read the sd on
	// the the in channel, and respond on  the out channel
	if(!eos) return STATUS_BREAK;
	if(!pump || !buffer) return STATUS_PRECONDITION_NOT_MET;

	std::string method_name;
	LLIOPipe::EStatus status = STATUS_DONE;

	switch(mState)
	{
	case STATE_DEFERRED:
		PUMP_DEBUG;
		if(ESDRPCS_DONE != deferredResponse(channels, buffer.get()))
		{
			buildFault(
				channels,
				buffer.get(),
				FAULT_GENERIC,
				"deferred response failed.");
		}
		mState = STATE_DONE;
		return STATUS_DONE;

	case STATE_DONE:
//		lldebugs << "STATE_DONE" << llendl;
		break;
	case STATE_CALLBACK:
//		lldebugs << "STATE_CALLBACK" << llendl;
		PUMP_DEBUG;
		method_name = mRequest[LLSDRPC_METHOD_SD_NAME].asString();
		if(!method_name.empty() && mRequest.has(LLSDRPC_PARAMETER_SD_NAME))
		{
			if(ESDRPCS_DONE != callbackMethod(
				   method_name,
				   mRequest[LLSDRPC_PARAMETER_SD_NAME],
				   channels,
				   buffer.get()))
			{
				buildFault(
					channels,
					buffer.get(),
					FAULT_GENERIC,
					"Callback method call failed.");
			}
		}
		else
		{
			// this should never happen, since we should not be in
			// this state unless we originally found a method and
			// params during the first call to process.
			buildFault(
				channels,
				buffer.get(),
				FAULT_GENERIC,
				"Invalid LLSDRPC sever state - callback without method.");
		}
		pump->clearLock(mLock);
		mLock = 0;
		mState = STATE_DONE;
		break;
	case STATE_NONE:
//		lldebugs << "STATE_NONE" << llendl;
	default:
	{
		// First time we got here - process the SD request, and call
		// the method.
		PUMP_DEBUG;
		LLBufferStream istr(channels, buffer.get());
		mRequest.clear();
		LLSDSerialize::fromNotation(
			mRequest,
			istr,
			buffer->count(channels.in()));

		// { 'method':'...', 'parameter': ... }
		method_name = mRequest[LLSDRPC_METHOD_SD_NAME].asString();
		if(!method_name.empty() && mRequest.has(LLSDRPC_PARAMETER_SD_NAME))
		{
			ESDRPCSStatus rv = callMethod(
				method_name,
				mRequest[LLSDRPC_PARAMETER_SD_NAME],
				channels,
				buffer.get());
			switch(rv)
			{
			case ESDRPCS_DEFERRED:
				mPump = pump;
				mLock = pump->setLock();
				mState = STATE_DEFERRED;
				status = STATUS_BREAK;
				break;

			case ESDRPCS_CALLBACK:
			{
				mState = STATE_CALLBACK;
				LLPumpIO::LLLinkInfo link;
				link.mPipe = LLIOPipe::ptr_t(this);
				link.mChannels = channels;
				LLPumpIO::links_t links;
				links.push_back(link);
				pump->respond(links, buffer, context);
				mLock = pump->setLock();
				status = STATUS_BREAK;
				break;
			}
			case ESDRPCS_DONE:
				mState = STATE_DONE;
				break;
			case ESDRPCS_ERROR:
			default:
				buildFault(
					channels,
					buffer.get(),
					FAULT_GENERIC,
					"Method call failed.");
				break;
			}
		}
		else
		{
			// send a fault
			buildFault(
				channels,
				buffer.get(),
				FAULT_GENERIC,
				"Unable to find method and parameter in request.");
		}
		break;
	}
	}

	PUMP_DEBUG;
	return status;
}

// virtual
ESDRPCSStatus LLSDRPCServer::callMethod(
	const std::string& method,
	const LLSD& params,
	const LLChannelDescriptors& channels,
	LLBufferArray* response)
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_SERVER);
	// Try to find the method in the method table.
	ESDRPCSStatus rv = ESDRPCS_DONE;
	method_map_t::iterator it = mMethods.find(method);
	if(it != mMethods.end())
	{
		rv = (*it).second->call(params, channels, response);
	}
	else
	{
		it = mCallbackMethods.find(method);
		if(it == mCallbackMethods.end())
		{
			// method not found.
			std::ostringstream message;
			message << "rpc server unable to find method: " << method;
			buildFault(
				channels,
				response,
				FAULT_METHOD_NOT_FOUND,
				message.str());
		}
		else
		{
			// we found it in the callback methods - tell the process
			// to coordinate calling on the pump callback.
			return ESDRPCS_CALLBACK;
		}
	}
	return rv;
}

// virtual
ESDRPCSStatus LLSDRPCServer::callbackMethod(
	const std::string& method,
	const LLSD& params,
	const LLChannelDescriptors& channels,
	LLBufferArray* response)
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_SERVER);
	// Try to find the method in the callback method table.
	ESDRPCSStatus rv = ESDRPCS_DONE;
	method_map_t::iterator it = mCallbackMethods.find(method);
	if(it != mCallbackMethods.end())
	{
		rv = (*it).second->call(params, channels, response);
	}
	else
	{
		std::ostringstream message;
		message << "pcserver unable to find callback method: " << method;
		buildFault(
			channels,
			response,
			FAULT_METHOD_NOT_FOUND,
			message.str());
	}
	return rv;
}

// static
void LLSDRPCServer::buildFault(
	const LLChannelDescriptors& channels,
	LLBufferArray* data,
	S32 code,
	const std::string& msg)
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_SERVER);
	LLBufferStream ostr(channels, data);
	ostr << FAULT_PART_1 << code << FAULT_PART_2 << msg << FAULT_PART_3;
	llinfos << "LLSDRPCServer::buildFault: " << code << ", " << msg << llendl;
}

// static
void LLSDRPCServer::buildResponse(
	const LLChannelDescriptors& channels,
	LLBufferArray* data,
	const LLSD& response)
{
	LLMemType m1(LLMemType::MTYPE_IO_SD_SERVER);
	LLBufferStream ostr(channels, data);
	ostr << RESPONSE_PART_1;
	LLSDSerialize::toNotation(response, ostr);
	ostr << RESPONSE_PART_2;
#if LL_DEBUG
	std::ostringstream debug_ostr;
	debug_ostr << "LLSDRPCServer::buildResponse: ";
	LLSDSerialize::toNotation(response, debug_ostr);
	llinfos << debug_ostr.str() << llendl;
#endif
}
