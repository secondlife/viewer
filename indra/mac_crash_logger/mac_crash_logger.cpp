/** 
 * @file mac_crash_logger.cpp
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

#include "linden_common.h"
#include "llcrashloggermac.h"
#include "indra_constants.h"
#include "llpidlock.h"

#include <iostream>
    
int main(int argc, char **argv)
{
	LLCrashLoggerMac app;
	app.parseCommandOptions(argc, argv);

    LLSD options = LLApp::instance()->getOptionData(
                        LLApp::PRIORITY_COMMAND_LINE);
    
    if (!(options.has("pid") && options.has("dumpdir")))
    {
        LL_WARNS() << "Insufficient parameters to crash report." << LL_ENDL;
    }
    
	if (! app.init())
	{
		LL_WARNS() << "Unable to initialize application." << LL_ENDL;
		return 1;
	}
    if (app.getCrashBehavior() != CRASH_BEHAVIOR_ALWAYS_SEND)
    {
//        return NSApplicationMain(argc, (const char **)argv);
    }
	app.mainLoop();
	app.cleanup();

	LL_INFOS() << "Crash reporter finished normally." << LL_ENDL;
    
	return 0;
}
