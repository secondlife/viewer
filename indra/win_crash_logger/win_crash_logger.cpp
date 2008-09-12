/** 
 * @file win_crash_logger.cpp
 * @brief Windows crash logger implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

	// In Win32, we need to generate argc and argv ourselves...
	// Note: GetCommandLine() returns a  potentially return a LPTSTR
	// which can resolve to a LPWSTR (unicode string).
	// (That's why it's different from lpCmdLine which is a LPSTR.)
	// We don't currently do unicode, so call the non-unicode version
	// directly.
	llinfos << "Processing command line" << llendl;
	LPSTR cmd_line_including_exe_name = GetCommandLineA();

	const S32	MAX_ARGS = 100;
	int argc = 0;
	char *argv[MAX_ARGS];		

	char *token = NULL;
	if( cmd_line_including_exe_name[0] == '\"' )
	{
		// Exe name is enclosed in quotes
		token = strtok( cmd_line_including_exe_name, "\"" );
		argv[argc++] = token;
		token = strtok( NULL, " \t," );
	}
	else
	{
		// Exe name is not enclosed in quotes
		token = strtok( cmd_line_including_exe_name, " \t," );
	}

	while( (token != NULL) && (argc < MAX_ARGS) )
	{
		argv[argc++] = token;
		/* Get next token: */
		if (*(token + strlen(token) + 1) == '\"')		
		{
			token = strtok( NULL, "\"");
		}
		else
		{
			token = strtok( NULL, " \t," );
		}
	}

	LLCrashLoggerWindows app;
	bool ok = app.parseCommandOptions(argc, argv);
	if(!ok)
	{
		llwarns << "Unable to parse command line." << llendl;
	}
	
	app.setHandle(hInstance);
	ok = app.init();
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
