/**
 * @file slplugin.cpp
 * @brief Loader shell for plugins, intended to be launched by the plugin host application, which directly loads a plugin dynamic library.
 *
 * @cond
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 *
 * @endcond
 */


#include "linden_common.h"

#include "llpluginprocesschild.h"
#include "llpluginmessage.h"
#include "llerrorcontrol.h"
#include "llapr.h"
#include "llstring.h"

#include <iostream>
#include <fstream>
using namespace std;


#if LL_DARWIN
	#include "slplugin-objc.h"
#endif

#if LL_DARWIN || LL_LINUX
	#include <signal.h>
#endif

/*
	On Mac OS, since we call WaitNextEvent, this process will show up in the dock unless we set the LSBackgroundOnly or LSUIElement flag in the Info.plist.

	Normally non-bundled binaries don't have an info.plist file, but it's possible to embed one in the binary by adding this to the linker flags:

	-sectcreate __TEXT __info_plist /path/to/slplugin_info.plist

	which means adding this to the gcc flags:

	-Wl,-sectcreate,__TEXT,__info_plist,/path/to/slplugin_info.plist
	
	Now that SLPlugin is a bundled app on the Mac, this is no longer necessary (it can just use a regular Info.plist file), but I'm leaving this comment in for posterity.
*/

#if LL_DARWIN || LL_LINUX
// Signal handlers to make crashes not show an OS dialog...
static void crash_handler(int sig)
{
	// Just exit cleanly.
	// TODO: add our own crash reporting
	_exit(1);
}
#endif

#if LL_WINDOWS
#include <windows.h>
////////////////////////////////////////////////////////////////////////////////
//	Our exception handler - will probably just exit and the host application
//	will miss the heartbeat and log the error in the usual fashion.
LONG WINAPI myWin32ExceptionHandler( struct _EXCEPTION_POINTERS* exception_infop )
{
	//std::cerr << "This plugin (" << __FILE__ << ") - ";
	//std::cerr << "intercepted an unhandled exception and will exit immediately." << std::endl;

	// TODO: replace exception handler before we exit?
	return EXCEPTION_EXECUTE_HANDLER;
}

// Taken from : http://blog.kalmbachnet.de/?postid=75
// The MSVC 2005 CRT forces the call of the default-debugger (normally Dr.Watson)
// even with the other exception handling code. This (terrifying) piece of code
// patches things so that doesn't happen.
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(
	LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter )
{
	return NULL;
}

BOOL PreventSetUnhandledExceptionFilter()
{
	// remove the scary stuff that also isn't supported on 64 bit Windows
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//	Hook our exception handler and replace the system one
void initExceptionHandler()
{
	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;

	// save old exception handler in case we need to restore it at the end
	prev_filter = SetUnhandledExceptionFilter( myWin32ExceptionHandler );
	PreventSetUnhandledExceptionFilter();
}

bool checkExceptionHandler()
{
	bool ok = true;
	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	prev_filter = SetUnhandledExceptionFilter(myWin32ExceptionHandler);

	PreventSetUnhandledExceptionFilter();

	if (prev_filter != myWin32ExceptionHandler)
	{
		LL_WARNS("AppInit") << "Our exception handler (" << (void *)myWin32ExceptionHandler << ") replaced with " << prev_filter << "!" << LL_ENDL;
		ok = false;
	}

	if (prev_filter == NULL)
	{
		ok = FALSE;
		if (NULL == myWin32ExceptionHandler)
		{
			LL_WARNS("AppInit") << "Exception handler uninitialized." << LL_ENDL;
		}
		else
		{
			LL_WARNS("AppInit") << "Our exception handler (" << (void *)myWin32ExceptionHandler << ") replaced with NULL!" << LL_ENDL;
		}
	}

	return ok;
}
#endif

// If this application on Windows platform is a console application, a console is always
// created which is bad. Making it a Windows "application" via CMake settings but not
// adding any code to explicitly create windows does the right thing.
#if LL_WINDOWS
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#else
int main(int argc, char **argv)
#endif
{

	ll_init_apr();

	// Set up llerror logging
	{
		LLError::initForApplication(".");
		LLError::setDefaultLevel(LLError::LEVEL_INFO);
//		LLError::setTagLevel("Plugin", LLError::LEVEL_DEBUG);
//		LLError::logToFile("slplugin.log");
	}

#if LL_WINDOWS
	if( strlen( lpCmdLine ) == 0 )
	{
		LL_ERRS("slplugin") << "usage: " << "SLPlugin" << " launcher_port" << LL_ENDL;
	};

	U32 port = 0;
	if(!LLStringUtil::convertToU32(lpCmdLine, port))
	{
		LL_ERRS("slplugin") << "port number must be numeric" << LL_ENDL;
	};

	// Insert our exception handler into the system so this plugin doesn't
	// display a crash message if something bad happens. The host app will
	// see the missing heartbeat and log appropriately.
	initExceptionHandler();
#elif LL_DARWIN || LL_LINUX
	if(argc < 2)
	{
		LL_ERRS("slplugin") << "usage: " << argv[0] << " launcher_port" << LL_ENDL;
	}

	U32 port = 0;
	if(!LLStringUtil::convertToU32(argv[1], port))
	{
		LL_ERRS("slplugin") << "port number must be numeric" << LL_ENDL;
	}

	// Catch signals that most kinds of crashes will generate, and exit cleanly so the system crash dialog isn't shown.
	signal(SIGILL, &crash_handler);		// illegal instruction
	signal(SIGFPE, &crash_handler);		// floating-point exception
	signal(SIGBUS, &crash_handler);		// bus error
	signal(SIGSEGV, &crash_handler);	// segmentation violation
	signal(SIGSYS, &crash_handler);		// non-existent system call invoked
#endif
# if LL_DARWIN
	signal(SIGEMT, &crash_handler);		// emulate instruction executed

    LLCocoaPlugin cocoa_interface;
	cocoa_interface.setupCocoa();
	cocoa_interface.createAutoReleasePool();
#endif //LL_DARWIN

	LLPluginProcessChild *plugin = new LLPluginProcessChild();

	plugin->init(port);

#if LL_DARWIN
    cocoa_interface.deleteAutoReleasePool();
#endif

	LLTimer timer;
	timer.start();

#if LL_WINDOWS
	checkExceptionHandler();
#endif

#if LL_DARWIN
    
	// If the plugin opens a new window (such as the Flash plugin's fullscreen player), we may need to bring this plugin process to the foreground.
	// Use this to track the current frontmost window and bring this process to the front if it changes.
 //   cocoa_interface.mEventTarget = GetEventDispatcherTarget();
#endif
	while(!plugin->isDone())
	{
#if LL_DARWIN
		cocoa_interface.createAutoReleasePool();
#endif
		timer.reset();
		plugin->idle();
#if LL_DARWIN
		{
			cocoa_interface.processEvents();
        }
#endif
		F64 elapsed = timer.getElapsedTimeF64();
		F64 remaining = plugin->getSleepTime() - elapsed;

		if(remaining <= 0.0f)
		{
			// We've already used our full allotment.
//			LL_INFOS("slplugin") << "elapsed = " << elapsed * 1000.0f << " ms, remaining = " << remaining * 1000.0f << " ms, not sleeping" << LL_ENDL;

			// Still need to service the network...
			plugin->pump();
		}
		else
		{

//			LL_INFOS("slplugin") << "elapsed = " << elapsed * 1000.0f << " ms, remaining = " << remaining * 1000.0f << " ms, sleeping for " << remaining * 1000.0f << " ms" << LL_ENDL;
//			timer.reset();

			// This also services the network as needed.
			plugin->sleep(remaining);

//			LL_INFOS("slplugin") << "slept for "<< timer.getElapsedTimeF64() * 1000.0f << " ms" <<  LL_ENDL;
		}
        
        
#if LL_WINDOWS
	// More agressive checking of interfering exception handlers.
	// Doesn't appear to be required so far - even for plugins
	// that do crash with a single call to the intercept
	// exception handler such as QuickTime.
	//checkExceptionHandler();
#endif

#if LL_DARWIN
		cocoa_interface.deleteAutoReleasePool();
#endif
	}
	delete plugin;

	ll_cleanup_apr();


	return 0;
}
