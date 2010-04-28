/**
 * @file slplugin.cpp
 * @brief Loader shell for plugins, intended to be launched by the plugin host application, which directly loads a plugin dynamic library.
 *
 * @cond
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
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
 *
 * @endcond
 */


#include "linden_common.h"

#include "llpluginprocesschild.h"
#include "llpluginmessage.h"
#include "llerrorcontrol.h"
#include "llapr.h"
#include "llstring.h"

#if LL_DARWIN
	#include <Carbon/Carbon.h>
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
// WARNING: This won't work on 64-bit Windows systems so we turn it off it.
//          It should work for any flavor of 32-bit Windows we care about.
//          If it's off, sometimes you will see an OS message when a plugin crashes
#ifndef _WIN64
	HMODULE hKernel32 = LoadLibraryA( "kernel32.dll" );
	if ( NULL == hKernel32 )
		return FALSE;

	void *pOrgEntry = GetProcAddress( hKernel32, "SetUnhandledExceptionFilter" );
	if( NULL == pOrgEntry )
		return FALSE;

	unsigned char newJump[ 100 ];
	DWORD dwOrgEntryAddr = (DWORD)pOrgEntry;
	dwOrgEntryAddr += 5; // add 5 for 5 op-codes for jmp far
	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
	DWORD dwNewEntryAddr = (DWORD) pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;

	newJump[ 0 ] = 0xE9;  // JMP absolute
	memcpy( &newJump[ 1 ], &dwRelativeAddr, sizeof( pNewFunc ) );
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory( GetCurrentProcess(), pOrgEntry, newJump, sizeof( pNewFunc ) + 1, &bytesWritten );
	return bRet;
#else
	return FALSE;
#endif
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
# if LL_DARWIN
	signal(SIGEMT, &crash_handler);		// emulate instruction executed
# endif // LL_DARWIN
	signal(SIGFPE, &crash_handler);		// floating-point exception
	signal(SIGBUS, &crash_handler);		// bus error
	signal(SIGSEGV, &crash_handler);	// segmentation violation
	signal(SIGSYS, &crash_handler);		// non-existent system call invoked
#endif

	LLPluginProcessChild *plugin = new LLPluginProcessChild();

	plugin->init(port);

	LLTimer timer;
	timer.start();

#if LL_WINDOWS
	checkExceptionHandler();
#endif

#if LL_DARWIN
	// If the plugin opens a new window (such as the Flash plugin's fullscreen player), we may need to bring this plugin process to the foreground.
	// Use this to track the current frontmost window and bring this process to the front if it changes.
	WindowRef front_window = NULL;
	WindowGroupRef layer_group = NULL;
	int window_hack_state = 0;
	CreateWindowGroup(kWindowGroupAttrFixedLevel, &layer_group);
	if(layer_group)
	{
		// Start out with a window layer that's way out in front (fixes the problem with the menubar not getting hidden on first switch to fullscreen youtube)
		SetWindowGroupName(layer_group, CFSTR("SLPlugin Layer"));
		SetWindowGroupLevel(layer_group, kCGOverlayWindowLevel);		
	}
#endif

#if LL_DARWIN
	EventTargetRef event_target = GetEventDispatcherTarget();
#endif
	while(!plugin->isDone())
	{
		timer.reset();
		plugin->idle();
#if LL_DARWIN
		{
			// Some plugins (webkit at least) will want an event loop.  This qualifies.
			EventRef event;
			if(ReceiveNextEvent(0, 0, kEventDurationNoWait, true, &event) == noErr)
			{
				SendEventToEventTarget (event, event_target);
				ReleaseEvent(event);
			}
			
			// Check for a change in this process's frontmost window.
			if(FrontWindow() != front_window)
			{
				ProcessSerialNumber self = { 0, kCurrentProcess };
				ProcessSerialNumber parent = { 0, kNoProcess };
				ProcessSerialNumber front = { 0, kNoProcess };
				Boolean this_is_front_process = false;
				Boolean parent_is_front_process = false;
				{
					// Get this process's parent
					ProcessInfoRec info;
					info.processInfoLength = sizeof(ProcessInfoRec);
					info.processName = NULL;
					info.processAppSpec = NULL;
					if(GetProcessInformation( &self, &info ) == noErr)
					{
						parent = info.processLauncher;
					}
					
					// and figure out whether this process or its parent are currently frontmost
					if(GetFrontProcess(&front) == noErr)
					{
						(void) SameProcess(&self, &front, &this_is_front_process);
						(void) SameProcess(&parent, &front, &parent_is_front_process);
					}
				}
								
				if((FrontWindow() != NULL) && (front_window == NULL))
				{
					// Opening the first window
					
					if(window_hack_state == 0)
					{
						// Next time through the event loop, lower the window group layer
						window_hack_state = 1;
					}

					if(layer_group)
					{
						SetWindowGroup(FrontWindow(), layer_group);
					}
					
					if(parent_is_front_process)
					{
						// Bring this process's windows to the front.
						(void) SetFrontProcess( &self );
					}

					ActivateWindow(FrontWindow(), true);					
				}
				else if((FrontWindow() == NULL) && (front_window != NULL))
				{
					// Closing the last window
					
					if(this_is_front_process)
					{
						// Try to bring this process's parent to the front
						(void) SetFrontProcess(&parent);
					}
				}
				else if(window_hack_state == 1)
				{
					if(layer_group)
					{
						// Set the window group level back to something less extreme
						SetWindowGroupLevel(layer_group, kCGNormalWindowLevel);
					}
					window_hack_state = 2;
				}

				front_window = FrontWindow();

			}
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
	}

	delete plugin;

	ll_cleanup_apr();

	return 0;
}

