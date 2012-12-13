/** 
 * @file llcrashloggermac.cpp
 * @brief Mac OSX crash logger implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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


#include "llcrashloggermac.h"

#include <iostream>

#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK, CRASH_SETTING_NAME
#include "llerror.h"
#include "llfile.h"
#include "lltimer.h"
#include "llstring.h"
#include "lldir.h"
#include "llsdserialize.h"

// Windows Message Handlers

BOOL gFirstDialog = TRUE;	
LLFILE *gDebugFile = NULL;

std::string gUserNotes = "";
bool gSendReport = false;
bool gRememberChoice = false;

LLCrashLoggerMac::LLCrashLoggerMac(void)
{
}

LLCrashLoggerMac::~LLCrashLoggerMac(void)
{
}

bool LLCrashLoggerMac::init(void)
{	
	bool ok = LLCrashLogger::init();
    return ok;
}

void LLCrashLoggerMac::gatherPlatformSpecificFiles()
{
}

bool LLCrashLoggerMac::mainLoop()
{
    if (mCrashBehavior == CRASH_BEHAVIOR_ALWAYS_SEND)
	{
		gSendReport = true;
	}
	
	if(gRememberChoice)
	{
		if(gSendReport) saveCrashBehaviorSetting(CRASH_BEHAVIOR_ALWAYS_SEND);
		else saveCrashBehaviorSetting(CRASH_BEHAVIOR_NEVER_SEND);
	}
	
	if(gSendReport)
	{
		setUserText(gUserNotes);
		sendCrashLogs();
	}	
		
	return true;
}

bool LLCrashLoggerMac::cleanup()
{
	commonCleanup();
	return true;
}
