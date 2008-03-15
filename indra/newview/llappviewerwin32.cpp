/**
 * @file llappviewerwin32.cpp
 * @brief The LLAppViewerWin32 class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llappviewerwin32.h"

#include "llmemtype.h"

#include "llwindowwin32.cpp" // *FIX: for setting gIconResource.
#include "res/resource.h" // *FIX: for setting gIconResource.

#include <fcntl.h>		//_O_APPEND
#include <io.h>			//_open_osfhandle()
#include <errorrep.h>	// for AddERExcludedApplicationA()
#include <process.h>	// _spawnl()
#include <tchar.h>		// For TCHAR support

#include "llviewercontrol.h"
#include "lldxhardware.h"

#include "llweb.h"
#include "llsecondlifeurls.h"

#include "llwindebug.h"

#include "llviewernetwork.h"
#include "llmd5.h"

void fill_args(int& argc, char** argv, const S32 max_args, LPSTR cmd_line)
{
	char *token = NULL;
	if( cmd_line[0] == '\"' )
	{
		// Exe name is enclosed in quotes
		token = strtok( cmd_line, "\"" );
		argv[argc++] = token;
		token = strtok( NULL, " \t," );
	}
	else
	{
		// Exe name is not enclosed in quotes
		token = strtok( cmd_line, " \t," );
	}

	while( (token != NULL) && (argc < max_args) )
	{
		argv[argc++] = token;
		/* Get next token: */
		if (*(token + strlen(token) + 1) == '\"')		/* Flawfinder: ignore*/
		{
			token = strtok( NULL, "\"");
		}
		else
		{
			token = strtok( NULL, " \t," );
		}
	}
}

// *NOTE:Mani - this code is stolen from LLApp, where its never actually used.
LONG WINAPI viewer_windows_exception_handler(struct _EXCEPTION_POINTERS *exception_infop)
{
	// Translate the signals/exceptions into cross-platform stuff
	// Windows implementation
	llinfos << "Entering Windows Exception Handler..." << llendl;

	// Make sure the user sees something to indicate that the app crashed.
	LONG retval;

	if (LLApp::isError())
	{
		llwarns << "Got another fatal signal while in the error handler, die now!" << llendl;
		retval = EXCEPTION_EXECUTE_HANDLER;
		return retval;
	}

	// Generate a minidump if we can.
	// Before we wake the error thread...
	// Which will start the crash reporting.
	LLWinDebug::handleException(exception_infop);
	
	// Flag status to error, so thread_error starts its work
	LLApp::setError();

	// Block in the exception handler until the app has stopped
	// This is pretty sketchy, but appears to work just fine
	while (!LLApp::isStopped())
	{
		ms_sleep(10);
	}

	//
	// At this point, we always want to exit the app.  There's no graceful
	// recovery for an unhandled exception.
	// 
	// Just kill the process.
	retval = EXCEPTION_EXECUTE_HANDLER;	
	return retval;
}


#if DEBUGGING_SEH_FILTER
#	define WINMAIN DebuggingWinMain
#else
#	define WINMAIN WinMain
#endif

int APIENTRY WINMAIN(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	LLMemType mt1(LLMemType::MTYPE_STARTUP);
	
	// *FIX: global
	gIconResource = MAKEINTRESOURCE(IDI_LL_ICON);

	// In Win32, we need to generate argc and argv ourselves...
	// Note: GetCommandLine() returns a  potentially return a LPTSTR
	// which can resolve to a LPWSTR (unicode string).
	// (That's why it's different from lpCmdLine which is a LPSTR.)
	// We don't currently do unicode, so call the non-unicode version
	// directly.
	LPSTR cmd_line_including_exe_name = GetCommandLineA();

	const S32	MAX_ARGS = 100;
	int argc = 0;
	char* argv[MAX_ARGS];		/* Flawfinder: ignore */

	fill_args(argc, argv, MAX_ARGS, cmd_line_including_exe_name);

	LLAppViewerWin32* viewer_app_ptr = new LLAppViewerWin32();

	// *FIX:Mani This method is poorly named, since the exception
	// is now handled by LLApp. 
	bool ok = LLWinDebug::setupExceptionHandler(); 
	
	// Actually here's the exception setup.
	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	prev_filter = SetUnhandledExceptionFilter(viewer_windows_exception_handler);
	if (!prev_filter)
	{
		llwarns << "Our exception handler (" << (void *)LLWinDebug::handleException << ") replaced with NULL!" << llendl;
		ok = FALSE;
	}
	if (prev_filter != LLWinDebug::handleException)
	{
		llwarns << "Our exception handler (" << (void *)LLWinDebug::handleException << ") replaced with " << prev_filter << "!" << llendl;
		ok = FALSE;
	}

	viewer_app_ptr->setErrorHandler(LLAppViewer::handleViewerCrash);

	ok = viewer_app_ptr->tempStoreCommandOptions(argc, argv);
	if(!ok)
	{
		llwarns << "Unable to parse command line." << llendl;
		return -1;
	}

	ok = viewer_app_ptr->init();
	if(!ok)
	{
		llwarns << "Application init failed." << llendl;
		return -1;
	}

		// Run the application main loop
	if(!LLApp::isQuitting()) 
	{
		viewer_app_ptr->mainLoop();
	}

	if (!LLApp::isError())
	{
		//
		// We don't want to do cleanup here if the error handler got called -
		// the assumption is that the error handler is responsible for doing
		// app cleanup if there was a problem.
		//
		viewer_app_ptr->cleanup();
	}
	delete viewer_app_ptr;
	viewer_app_ptr = NULL;
	return 0;
}

#if DEBUGGING_SEH_FILTER
// The compiler doesn't like it when you use __try/__except blocks
// in a method that uses object destructors. Go figure.
// This winmain just calls the real winmain inside __try.
// The __except calls our exception filter function. For debugging purposes.
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    __try
    {
        WINMAIN(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    __except( viewer_windows_exception_handler( GetExceptionInformation() ) )
    {
        _tprintf( _T("Exception handled.\n") );
    }
}
#endif

void LLAppViewerWin32::disableWinErrorReporting()
{
	const char win_xp_string[] = "Microsoft Windows XP";
	BOOL is_win_xp = ( getOSInfo().getOSString().substr(0, strlen(win_xp_string) ) == win_xp_string );		/* Flawfinder: ignore*/
	if( is_win_xp )
	{
		// Note: we need to use run-time dynamic linking, because load-time dynamic linking will fail
		// on systems that don't have the library installed (all non-Windows XP systems)
		HINSTANCE fault_rep_dll_handle = LoadLibrary(L"faultrep.dll");		/* Flawfinder: ignore */
		if( fault_rep_dll_handle )
		{
			pfn_ADDEREXCLUDEDAPPLICATIONA pAddERExcludedApplicationA  = (pfn_ADDEREXCLUDEDAPPLICATIONA) GetProcAddress(fault_rep_dll_handle, "AddERExcludedApplicationA");
			if( pAddERExcludedApplicationA )
			{

				// Strip the path off the name
				const char* executable_name = gDirUtilp->getExecutableFilename().c_str();

				if( 0 == pAddERExcludedApplicationA( executable_name ) )
				{
					U32 error_code = GetLastError();
					llinfos << "AddERExcludedApplication() failed with error code " << error_code << llendl;
				}
				else
				{
					llinfos << "AddERExcludedApplication() success for " << executable_name << llendl;
				}
			}
			FreeLibrary( fault_rep_dll_handle );
		}
	}
}

const S32 MAX_CONSOLE_LINES = 500;

void create_console()
{
	int h_con_handle;
	long l_std_handle;

	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	l_std_handle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	h_con_handle = _open_osfhandle(l_std_handle, _O_TEXT);
	fp = _fdopen( h_con_handle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	l_std_handle = (long)GetStdHandle(STD_INPUT_HANDLE);
	h_con_handle = _open_osfhandle(l_std_handle, _O_TEXT);
	fp = _fdopen( h_con_handle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	l_std_handle = (long)GetStdHandle(STD_ERROR_HANDLE);
	h_con_handle = _open_osfhandle(l_std_handle, _O_TEXT);
	fp = _fdopen( h_con_handle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );
}

LLAppViewerWin32::LLAppViewerWin32()
{
}

LLAppViewerWin32::~LLAppViewerWin32()
{
}

bool LLAppViewerWin32::init()
{
	// Platform specific initialization.
	
	// Turn off Windows XP Error Reporting
	// (Don't send our data to Microsoft--at least until we are Logo approved and have a way
	// of getting the data back from them.)
	//
	llinfos << "Turning off Windows error reporting." << llendl;
	disableWinErrorReporting();

	return LLAppViewer::init();
}

bool LLAppViewerWin32::cleanup()
{
	bool result = LLAppViewer::cleanup();

	gDXHardware.cleanup();

	return result;
}

bool LLAppViewerWin32::initWindow()
{
	// pop up debug console if necessary
	if (gUseConsole && gSavedSettings.getBOOL("ShowConsoleWindow"))
	{
		create_console();
	}

	return LLAppViewer::initWindow();
}

void write_debug_dx(const char* str)
{
	LLString value = gDebugInfo["DXInfo"].asString();
	value += str;
	gDebugInfo["DXInfo"] = value;
}

void write_debug_dx(const std::string& str)
{
	write_debug_dx(str.c_str());
}

bool LLAppViewerWin32::initHardwareTest()
{
	//
	// Do driver verification and initialization based on DirectX
	// hardware polling and driver versions
	//
	if (gProbeHardware)
	{
		BOOL vram_only = !gSavedSettings.getBOOL("ProbeHardwareOnStartup");

		LLSplashScreen::update("Detecting hardware...");

		llinfos << "Attempting to poll DirectX for hardware info" << llendl;
		gDXHardware.setWriteDebugFunc(write_debug_dx);
		BOOL probe_ok = gDXHardware.getInfo(vram_only);

		if (!probe_ok
			&& gSavedSettings.getWarning("AboutDirectX9"))
		{
			llinfos << "DirectX probe failed, alerting user." << llendl;

			// Warn them that runnin without DirectX 9 will
			// not allow us to tell them about driver issues
			std::ostringstream msg;
			msg << 
				LLAppViewer::instance()->getSecondLifeTitle() << " is unable to detect DirectX 9.0b or greater.\n"
				"\n" <<
				LLAppViewer::instance()->getSecondLifeTitle() << " uses DirectX to detect hardware and/or\n"
				"outdated drivers that can cause stability problems,\n"
				"poor performance and crashes.  While you can run\n" <<
				LLAppViewer::instance()->getSecondLifeTitle() << " without it, we highly recommend running\n"
				"with DirectX 9.0b\n"
				"\n"
				"Do you wish to continue?\n";
			S32 button = OSMessageBox(
				msg.str().c_str(),
				"Warning",
				OSMB_YESNO);
			if (OSBTN_NO== button)
			{
				llinfos << "User quitting after failed DirectX 9 detection" << llendl;
				LLWeb::loadURLExternal(DIRECTX_9_URL);
				return false;
			}
			gSavedSettings.setWarning("AboutDirectX9", FALSE);
		}
		llinfos << "Done polling DirectX for hardware info" << llendl;

		// Only probe once after installation
		gSavedSettings.setBOOL("ProbeHardwareOnStartup", FALSE);

		// Disable so debugger can work
		std::ostringstream splash_msg;
		splash_msg << "Loading " << LLAppViewer::instance()->getSecondLifeTitle() << "...";

		LLSplashScreen::update(splash_msg.str().c_str());
	}

	if (!LLWinDebug::setupExceptionHandler())
	{
		llwarns << " Someone took over my exception handler (post hardware probe)!" << llendl;
	}

	gGLManager.mVRAM = gDXHardware.getVRAM();
	llinfos << "Detected VRAM: " << gGLManager.mVRAM << llendl;

	return true;
}

void LLAppViewerWin32::handleCrashReporting()
{
	// Windows only behaivor. Spawn win crash reporter.
	std::string exe_path = gDirUtilp->getAppRODataDir();
	exe_path += gDirUtilp->getDirDelimiter();
	exe_path += "win_crash_logger.exe";

	std::string arg_string = "-user ";
	arg_string += gGridName;

	switch(getCrashBehavior())
	{
	case CRASH_BEHAVIOR_ASK:
	default:
		arg_string += " -dialog ";
		_spawnl(_P_NOWAIT, exe_path.c_str(), exe_path.c_str(), arg_string.c_str(), NULL);
		break;

	case CRASH_BEHAVIOR_ALWAYS_SEND:
		_spawnl(_P_NOWAIT, exe_path.c_str(), exe_path.c_str(), arg_string.c_str(), NULL);
		break;

	case CRASH_BEHAVIOR_NEVER_SEND:
		break;
	}
}

std::string LLAppViewerWin32::generateSerialNumber()
{
	char serial_md5[MD5HEX_STR_SIZE];		// Flawfinder: ignore
	serial_md5[0] = 0;

	DWORD serial = 0;
	DWORD flags = 0;
	BOOL success = GetVolumeInformation(
			L"C:\\",
			NULL,		// volume name buffer
			0,			// volume name buffer size
			&serial,	// volume serial
			NULL,		// max component length
			&flags,		// file system flags
			NULL,		// file system name buffer
			0);			// file system name buffer size
	if (success)
	{
		LLMD5 md5;
		md5.update( (unsigned char*)&serial, sizeof(DWORD));
		md5.finalize();
		md5.hex_digest(serial_md5);
	}
	else
	{
		llwarns << "GetVolumeInformation failed" << llendl;
	}
	return serial_md5;
}