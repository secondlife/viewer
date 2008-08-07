/** 
 * @file lliopipe.cpp
 * @author Phoenix
 * @date 2004-11-19
 * @brief Implementation of the LLIOPipe class
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
