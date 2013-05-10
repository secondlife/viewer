/** 
 * @file llsdrpcserver.cpp
 * @author Phoenix
 * @date 2005-10-11
 * @brief Implementation of the LLSDRPCServer and related classes.
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
#include "llsdrpcserver.h"

#include "llbuffer.h"
#include "llbufferstream.h"
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
}

LLSDRPCServer::~LLSDRPCServer()
{
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

static LLFastTimer::DeclareTimer FTM_PROCESS_SDRPC_SERVER("SDRPC Server");

// virtual
LLIOPipe::EStatus LLSDRPCServer::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLFastTimer t(FTM_PROCESS_SDRPC_SERVER);
	PUMP_DEBUG;
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
