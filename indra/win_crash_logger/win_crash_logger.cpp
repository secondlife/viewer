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

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	LL_INFOS() << "Starting crash reporter with args" << &lpCmdLine << LL_ENDL;
	LLCrashLoggerWindows app;
	app.setHandle(hInstance);
	app.parseCommandOptions(__argc, __argv);

	LLSD options = LLApp::instance()->getOptionData(
                   LLApp::PRIORITY_COMMAND_LINE);
    if (!(options.has("pid") && options.has("dumpdir")))
    {
        LL_WARNS() << "Insufficient parameters to crash report." << LL_ENDL; 
    }
	if (! app.init())
	{
		LL_WARNS() << "Unable to initialize application." << LL_ENDL;
		return -1;
	}

	app.processingLoop();
	app.frame();
	app.cleanup();
	LL_INFOS() << "Crash reporter finished normally." << LL_ENDL;
	return 0;
}
