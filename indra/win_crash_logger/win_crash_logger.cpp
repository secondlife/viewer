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

// win_crash_logger.cpp : Defines the entry point for the application.
//

// Must be first include, precompiled headers.
#include "linden_common.h"

#include "stdafx.h"

#include <stdlib.h>

#include "llcrashloggerwindows.h"



//
// Implementation
//

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	llinfos << "Starting crash reporter" << llendl;

	LLCrashLoggerWindows app;
	app.setHandle(hInstance);
	bool ok = app.init();
	if(!ok)
	{
		llwarns << "Unable to initialize application." << llendl;
		return -1;
	}

		// Run the application main loop
	if(!LLApp::isQuitting()) app.mainLoop();

	if (!app.isError())
	{
		//
		// We don't want to do cleanup here if the error handler got called -
		// the assumption is that the error handler is responsible for doing
		// app cleanup if there was a problem.
		//
		app.cleanup();
	}
	return 0;
}
