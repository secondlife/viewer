/** 
 * @file win_crash_logger.cpp
 * @brief Windows crash logger implementation
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
#include "stdafx.h"
#include <stdlib.h>
#include "llcrashloggerwindows.h"
#include <iostream>

//SPATTERS killme
#include <iostream>
#include <fstream>

std::ofstream gCLOG; //SPATTERS for testing only remove ASAP.

void flog (std::string msg)
{
    //Windows can't even write a goddamned file without fucking it up.
    static bool first_time=true;

    std::cout << "Init Crash Logger" << std::endl;
    static std::string logname="C:\\Users\\Aura\\AppData\\Roaming\\SecondLife\\logs\\WinCrashLog.log";
    if (first_time)
	{
        gCLOG.open(logname, std::fstream::out); //first time open for overwrite.
        first_time = false;
        gCLOG << "BEGINNING Windows Crash Report Log\n";
	}
	else
        gCLOG.open(logname,  std::fstream::out | std::fstream::app);

    gCLOG << msg << "\n";
    gCLOG.close();
}
//END SPATTERS KILLME

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	llinfos << "Starting crash reporter." << llendl;
	LLCrashLoggerWindows app;
	app.setHandle(hInstance);
	app.parseCommandOptions(__argc, __argv);

	LLSD options = LLApp::instance()->getOptionData(
                   LLApp::PRIORITY_COMMAND_LINE);
    if (!(options.has("pid") && options.has("dumpdir")))
    {
        llwarns << "Insufficient parameters to crash report." << llendl; 
    }
	if (! app.init())
	{
		llwarns << "Unable to initialize application." << llendl;
		return 1;
	}
	flog("Hi there.");
	app.processingLoop();
	app.mainLoop();
	app.cleanup();
	llinfos << "Crash reporter finished normally." << llendl;
	return 0;
}
