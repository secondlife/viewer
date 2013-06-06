/**
 * @file lllog.h
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

#ifndef LL_LLLOG_H
#define LL_LLLOG_H

#include <string>

class LLLogImpl;
class LLApp;
class LLSD;

class LL_COMMON_API LLLog
{
public:
	LLLog(LLApp* app);
	virtual ~LLLog();

	virtual void log(const std::string &message, LLSD& info);
	virtual bool useLegacyLogMessage(const std::string &message);

private:
	LLLogImpl* mImpl;
};

#endif /* LL_LLLOG_H */

