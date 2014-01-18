/** 
 * @file lliopipe.cpp
 * @author Phoenix
 * @date 2004-11-19
 * @brief Implementation of the LLIOPipe class
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "lliopipe.h"

#include "llpumpio.h"

static const std::string STATUS_SUCCESS_NAMES[LLIOPipe::STATUS_SUCCESS_COUNT] =
{
	std::string("STATUS_OK"),
	std::string("STATUS_STOP"),
	std::string("STATUS_DONE"),
	std::string("STATUS_BREAK"),
	std::string("STATUS_NEED_PROCESS"),
};

static const std::string STATUS_ERROR_NAMES[LLIOPipe::STATUS_ERROR_COUNT] =
{
	std::string("STATUS_ERROR"),
	std::string("STATUS_NOT_IMPLEMENTED"),
	std::string("STATUS_PRECONDITION_NOT_MET"),
	std::string("STATUS_NO_CONNECTION"),
	std::string("STATUS_LOST_CONNECTION"),
	std::string("STATUS_EXPIRED"),
};

#ifdef LL_DEBUG_PUMPS
// Debugging schmutz for deadlock
const char	*gPumpFile = "";
S32			gPumpLine = 0;

void pump_debug(const char *file, S32 line)
{
	gPumpFile = file;
	gPumpLine = line;
}
#endif /* LL_DEBUG_PUMPS */

/**
 * LLIOPipe
 */
LLIOPipe::LLIOPipe() :
	mReferenceCount(0)
{
}

LLIOPipe::~LLIOPipe()
{
	//lldebugs << "destroying LLIOPipe" << llendl;
}

//virtual 
bool LLIOPipe::isValid() 
{
	return true ;
}

// static
std::string LLIOPipe::lookupStatusString(EStatus status)
{
	if((status >= 0) && (status < STATUS_SUCCESS_COUNT))
	{
		return STATUS_SUCCESS_NAMES[status];
	}
	else
	{
		S32 error_code = ((S32)status * -1) - 1;
		if(error_code < STATUS_ERROR_COUNT)
		{
			return STATUS_ERROR_NAMES[error_code];
		}
	}
	std::string rv;
	return rv;
}

LLIOPipe::EStatus LLIOPipe::process(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	return process_impl(channels, buffer, eos, context, pump);
}

// virtual
LLIOPipe::EStatus LLIOPipe::handleError(
	LLIOPipe::EStatus status,
	LLPumpIO* pump)
{
	// by default, the error is not handled.
	return status;
}
