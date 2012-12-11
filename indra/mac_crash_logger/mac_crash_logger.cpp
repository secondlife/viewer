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

#include <iostream>
#include <fstream>

    
int main(int argc, char **argv)
{
    std::ofstream wlog;
    wlog.open("/Users/samantha/crashlogging.txt");
    wlog << "SPATTERS starting crash reporter.!\n";
	LLCrashLoggerMac app;
    wlog << "SPATTERS created app instance.!\n";
    for (int x=0;x<argc;++x) wlog << "SPATTERS arg " << x << " is '" << argv[x] << "'\n";
	app.parseCommandOptions(argc, argv);
    wlog << "SPATTERS parsed commands.!\n";

	if (! app.init())
	{
        wlog << "SPATTERS failed to init.!\n";

		llwarns << "Unable to initialize application." << llendl;
		return 1;
	}
    if (app.getCrashBehavior() != CRASH_BEHAVIOR_ALWAYS_SEND)
    {
        wlog << "SPATTERS wanted to dialog.!\n";

        
//        return NSApplicationMain(argc, (const char **)argv);
    }
    wlog << "SPATTERS starting mainloop.!\n";

	app.mainLoop();
    wlog << "SPATTERS finished main.!\n";

	app.cleanup();
    wlog << "SPATTERS finished cleanup.!\n";

	llinfos << "Crash reporter finished normally." << llendl;
    wlog.close();
    
	return 0;
}
