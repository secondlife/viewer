/** 
 * @file lllog.cpp
 * @author Don
 * @date 2007-11-27
 * @brief  Class to log messages to syslog for streambase to process.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "lllog.h"

#include "llapp.h"
#include "llsd.h"
#include "llsdserialize.h"


class LLLogImpl
{
public:
	LLLogImpl(LLApp* app) : mApp(app) {}
	~LLLogImpl() {}

	void log(const std::string &message, LLSD& info);
	bool useLegacyLogMessage(const std::string &message);

private:
	LLApp* mApp;
};


//@brief Function to log a message to syslog for streambase to collect.
void LLLogImpl::log(const std::string &message, LLSD& info)
{
	static S32 sequence = 0;
    LLSD log_config = mApp->getOption("log-messages");
	if (log_config.has(message))
	{
		LLSD message_config = log_config[message];
		if (message_config.has("use-syslog"))
		{
			if (! message_config["use-syslog"].asBoolean())
			{
				return;
			}
		}
	}
	llinfos << "LLLOGMESSAGE (" << (sequence++) << ") " << message 
		<< " " << LLSDNotationStreamer(info) << llendl;
}

//@brief Function to check if specified legacy log message should be sent.
bool LLLogImpl::useLegacyLogMessage(const std::string &message)
{
    LLSD log_config = mApp->getOption("log-messages");
	if (log_config.has(message))
	{
		LLSD message_config = log_config[message];
		if (message_config.has("use-legacy"))
		{
			return message_config["use-legacy"].asBoolean();
		}
	}
	return true;
}


LLLog::LLLog(LLApp* app)
{
	mImpl = new LLLogImpl(app);
}

LLLog::~LLLog()
{
	delete mImpl;
	mImpl = NULL;
}

void LLLog::log(const std::string &message, LLSD& info)
{
	if (mImpl) mImpl->log(message, info);
}

bool LLLog::useLegacyLogMessage(const std::string &message)
{
	if (mImpl)
	{
		return mImpl->useLegacyLogMessage(message);
	}
	return true;
}

