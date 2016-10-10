/**
 * @file llappviewerwin32.cpp
 * @brief The LLAppViewerWin32 class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#ifdef INCLUDE_VLD
#include "vld.h"
#endif
#include "llwin32headers.h"

#include "llwindowwin32.h" // *FIX: for setting gIconResource.

#include "llappviewerwin32.h"

#include "llgl.h"
#include "res/resource.h" // *FIX: for setting gIconResource.

#include <fcntl.h>		//_O_APPEND
#include <io.h>			//_open_osfhandle()
#include <WERAPI.H>		// for WerAddExcludedApplication()
#include <process.h>	// _spawnl()
#include <tchar.h>		// For TCHAR support

#include "llviewercontrol.h"
#include "lldxhardware.h"

#include "nvapi/nvapi.h"
#include "nvapi/NvApiDriverSettings.h"

#include <stdlib.h>

#include "llweb.h"

#include "llviewernetwork.h"
#include "llmd5.h"
#include "llfindlocale.h"

#include "llcommandlineparser.h"
#include "lltrans.h"

#ifndef LL_RELEASE_FOR_DOWNLOAD
#include "llwindebug.h"
#endif

#include "stringize.h"

#include <exception>
namespace
{
    void (*gOldTerminateHandler)() = NULL;
}

static void exceptionTerminateHandler()
{
	// reinstall default terminate() handler in case we re-terminate.
	if (gOldTerminateHandler) std::set_terminate(gOldTerminateHandler);
	// treat this like a regular viewer crash, with nice stacktrace etc.
    long *null_ptr;
    null_ptr = 0;
    *null_ptr = 0xDEADBEEF; //Force an exception that will trigger breakpad.
	//LLAppViewer::handleViewerCrash();
	// we've probably been killed-off before now, but...
	gOldTerminateHandler(); // call old terminate() handler
}

LONG WINAPI catchallCrashHandler(EXCEPTION_POINTERS * /*ExceptionInfo*/)
{
	LL_WARNS() << "Hit last ditch-effort attempt to catch crash." << LL_ENDL;
	exceptionTerminateHandler();
	return 0;
}

// *FIX:Mani - This hack is to fix a linker issue with libndofdev.lib
// The lib was compiled under VS2005 - in VS2003 we need to remap assert
#ifdef LL_DEBUG
#ifdef LL_MSVC7 
extern "C" {
    void _wassert(const wchar_t * _Message, const wchar_t *_File, unsigned _Line)
    {
        LL_ERRS() << _Message << LL_ENDL;
    }
}
#endif
#endif

const std::string LLAppViewerWin32::sWindowClass = "Second Life";

/*
    This function is used to print to the command line a text message 
    describing the nvapi error and quits
*/
void nvapi_error(NvAPI_Status status)
{
    NvAPI_ShortString szDesc = {0};
	NvAPI_GetErrorMessage(status, szDesc);
	LL_WARNS() << szDesc << LL_ENDL;

	//should always trigger when asserts are enabled
	//llassert(status == NVAPI_OK);
}

// Create app mutex creates a unique global windows object. 
// If the object can be created it returns true, otherwise
// it returns false. The false result can be used to determine 
// if another instance of a second life app (this vers. or later)
// is running.
// *NOTE: Do not use this method to run a single instance of the app.
// This is intended to help debug problems with the cross-platform 
// locked file method used for that purpose.
bool create_app_mutex()
{
	bool result = true;
	LPCWSTR unique_mutex_name = L"SecondLifeAppMutex";
	HANDLE hMutex;
	hMutex = CreateMutex(NULL, TRUE, unique_mutex_name); 
	if(GetLastError() == ERROR_ALREADY_EXISTS) 
	{     
		result = false;
	}
	return result;
}

void ll_nvapi_init(NvDRSSessionHandle hSession)
{
	// (2) load all the system settings into the session
	NvAPI_Status status = NvAPI_DRS_LoadSettings(hSession);
	if (status != NVAPI_OK) 
	{
		nvapi_error(status);
		return;
	}

	NvAPI_UnicodeString profile_name;
	std::string app_name = LLTrans::getString("APP_NAME");
	llutf16string w_app_name = utf8str_to_utf16str(app_name);
	wsprintf(profile_name, L"%s", w_app_name.c_str());
	status = NvAPI_DRS_SetCurrentGlobalProfile(hSession, profile_name);
	if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}

	// (3) Obtain the current profile. 
	NvDRSProfileHandle hProfile = 0;
	status = NvAPI_DRS_GetCurrentGlobalProfile(hSession, &hProfile);
	if (status != NVAPI_OK) 
	{
		nvapi_error(status);
		return;
	}

	// load settings for querying 
	status = NvAPI_DRS_LoadSettings(hSession);
	if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}

	//get the preferred power management mode for Second Life
	NVDRS_SETTING drsSetting = {0};
	drsSetting.version = NVDRS_SETTING_VER;
	status = NvAPI_DRS_GetSetting(hSession, hProfile, PREFERRED_PSTATE_ID, &drsSetting);
	if (status == NVAPI_SETTING_NOT_FOUND)
	{ //only override if the user hasn't specifically set this setting
		// (4) Specify that we want the VSYNC disabled setting
		// first we fill the NVDRS_SETTING struct, then we call the function
		drsSetting.version = NVDRS_SETTING_VER;
		drsSetting.settingId = PREFERRED_PSTATE_ID;
		drsSetting.settingType = NVDRS_DWORD_TYPE;
		drsSetting.u32CurrentValue = PREFERRED_PSTATE_PREFER_MAX;
		status = NvAPI_DRS_SetSetting(hSession, hProfile, &drsSetting);
		if (status != NVAPI_OK) 
		{
			nvapi_error(status);
			return;
		}

        // (5) Now we apply (or save) our changes to the system
        status = NvAPI_DRS_SaveSettings(hSession);
        if (status != NVAPI_OK) 
        {
            nvapi_error(status);
            return;
        }
	}
	else if (status != NVAPI_OK)
	{
		nvapi_error(status);
		return;
	}
}

//#define DEBUGGING_SEH_FILTER 1
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
	const S32 MAX_HEAPS = 255;
	DWORD heap_enable_lfh_error[MAX_HEAPS];
	S32 num_heaps = 0;
	
	LLWindowWin32::setDPIAwareness();

#if WINDOWS_CRT_MEM_CHECKS && !INCLUDE_VLD
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); // dump memory leaks on exit
#elif 0
	// Experimental - enable the low fragmentation heap
	// This results in a 2-3x improvement in opening a new Inventory window (which uses a large numebr of allocations)
	// Note: This won't work when running from the debugger unless the _NO_DEBUG_HEAP environment variable is set to 1

	// Enable to get mem debugging within visual studio.
#if LL_DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#else
	_CrtSetDbgFlag(0); // default, just making explicit
	
	ULONG ulEnableLFH = 2;
	HANDLE* hHeaps = new HANDLE[MAX_HEAPS];
	num_heaps = GetProcessHeaps(MAX_HEAPS, hHeaps);
	for(S32 i = 0; i < num_heaps; i++)
	{
		bool success = HeapSetInformation(hHeaps[i], HeapCompatibilityInformation, &ulEnableLFH, sizeof(ulEnableLFH));
		if (success)
			heap_enable_lfh_error[i] = 0;
		else
			heap_enable_lfh_error[i] = GetLastError();
	}
#endif
#endif
	
	// *FIX: global
	gIconResource = MAKEINTRESOURCE(IDI_LL_ICON);

	LLAppViewerWin32* viewer_app_ptr = new LLAppViewerWin32(lpCmdLine);
	
	gOldTerminateHandler = std::set_terminate(exceptionTerminateHandler);

	viewer_app_ptr->setErrorHandler(LLAppViewer::handleViewerCrash);

#if LL_SEND_CRASH_REPORTS 
	// ::SetUnhandledExceptionFilter(catchallCrashHandler); 
#endif

	// Set a debug info flag to indicate if multiple instances are running.
	bool found_other_instance = !create_app_mutex();
	gDebugInfo["FoundOtherInstanceAtStartup"] = LLSD::Boolean(found_other_instance);

	bool ok = viewer_app_ptr->init();
	if(!ok)
	{
		LL_WARNS() << "Application init failed." << LL_ENDL;
		return -1;
	}
	
	NvAPI_Status status;
    
	// Initialize NVAPI
	status = NvAPI_Initialize();
	NvDRSSessionHandle hSession = 0;

    if (status == NVAPI_OK) 
	{
		// Create the session handle to access driver settings
		status = NvAPI_DRS_CreateSession(&hSession);
		if (status != NVAPI_OK) 
		{
			nvapi_error(status);
		}
		else
		{
			//override driver setting as needed
			ll_nvapi_init(hSession);
		}
	}

	// Have to wait until after logging is initialized to display LFH info
	if (num_heaps > 0)
	{
		LL_INFOS() << "Attempted to enable LFH for " << num_heaps << " heaps." << LL_ENDL;
		for(S32 i = 0; i < num_heaps; i++)
		{
			if (heap_enable_lfh_error[i])
			{
				LL_INFOS() << "  Failed to enable LFH for heap: " << i << " Error: " << heap_enable_lfh_error[i] << LL_ENDL;
			}
		}
	}
	
	// Run the application main loop
	while (! viewer_app_ptr->frame()) 
	{}

	if (!LLApp::isError())
	{
		//
		// We don't want to do cleanup here if the error handler got called -
		// the assumption is that the error handler is responsible for doing
		// app cleanup if there was a problem.
		//
#if WINDOWS_CRT_MEM_CHECKS
		LL_INFOS() << "CRT Checking memory:" << LL_ENDL;
		if (!_CrtCheckMemory())
		{
			LL_WARNS() << "_CrtCheckMemory() failed at prior to cleanup!" << LL_ENDL;
		}
		else
		{
			LL_INFOS() << " No corruption detected." << LL_ENDL;
		}
#endif

		gGLActive = TRUE;

		viewer_app_ptr->cleanup();

#if WINDOWS_CRT_MEM_CHECKS
		LL_INFOS() << "CRT Checking memory:" << LL_ENDL;
		if (!_CrtCheckMemory())
		{
			LL_WARNS() << "_CrtCheckMemory() failed after cleanup!" << LL_ENDL;
		}
		else
		{
			LL_INFOS() << " No corruption detected." << LL_ENDL;
		}
#endif

	}
	delete viewer_app_ptr;
	viewer_app_ptr = NULL;

	//start updater
	if(LLAppViewer::sUpdaterInfo)
	{
		_spawnl(_P_NOWAIT, LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str(), LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str(), LLAppViewer::sUpdaterInfo->mParams.str().c_str(), NULL);

		delete LLAppViewer::sUpdaterInfo ;
		LLAppViewer::sUpdaterInfo = NULL ;
	}



	// (NVAPI) (6) We clean up. This is analogous to doing a free()
	if (hSession)
	{
		NvAPI_DRS_DestroySession(hSession);
		hSession = 0;
	}
	
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
	std::string executable_name = gDirUtilp->getExecutableFilename();

	if( S_OK == WerAddExcludedApplication( utf8str_to_utf16str(executable_name).c_str(), FALSE ) )
	{
		LL_INFOS() << "WerAddExcludedApplication() succeeded for " << executable_name << LL_ENDL;
	}
	else
	{
		LL_INFOS() << "WerAddExcludedApplication() failed for " << executable_name << LL_ENDL;
	}
}

const S32 MAX_CONSOLE_LINES = 500;

static bool create_console()
{
	int h_con_handle;
	long l_std_handle;

	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	const bool isConsoleAllocated = AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	l_std_handle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	h_con_handle = _open_osfhandle(l_std_handle, _O_TEXT);
	if (h_con_handle == -1)
	{
		LL_WARNS() << "create_console() failed to open stdout handle" << LL_ENDL;
	}
	else
	{
		fp = _fdopen( h_con_handle, "w" );
		*stdout = *fp;
		setvbuf( stdout, NULL, _IONBF, 0 );
	}

	// redirect unbuffered STDIN to the console
	l_std_handle = (long)GetStdHandle(STD_INPUT_HANDLE);
	h_con_handle = _open_osfhandle(l_std_handle, _O_TEXT);
	if (h_con_handle == -1)
	{
		LL_WARNS() << "create_console() failed to open stdin handle" << LL_ENDL;
	}
	else
	{
		fp = _fdopen( h_con_handle, "r" );
		*stdin = *fp;
		setvbuf( stdin, NULL, _IONBF, 0 );
	}

	// redirect unbuffered STDERR to the console
	l_std_handle = (long)GetStdHandle(STD_ERROR_HANDLE);
	h_con_handle = _open_osfhandle(l_std_handle, _O_TEXT);
	if (h_con_handle == -1)
	{
		LL_WARNS() << "create_console() failed to open stderr handle" << LL_ENDL;
	}
	else
	{
		fp = _fdopen( h_con_handle, "w" );
		*stderr = *fp;
		setvbuf( stderr, NULL, _IONBF, 0 );
	}

    return isConsoleAllocated;
}

LLAppViewerWin32::LLAppViewerWin32(const char* cmd_line) :
	mCmdLine(cmd_line),
	mIsConsoleAllocated(false)
{
}

LLAppViewerWin32::~LLAppViewerWin32()
{
}

bool LLAppViewerWin32::init()
{
	// Platform specific initialization.
	
	// Turn off Windows Error Reporting
	// (Don't send our data to Microsoft--at least until we are Logo approved and have a way
	// of getting the data back from them.)
	//
	// LL_INFOS() << "Turning off Windows error reporting." << LL_ENDL;
	disableWinErrorReporting();

#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLWinDebug::instance().init();
#endif

#if LL_WINDOWS
#if LL_SEND_CRASH_REPORTS


	LLAppViewer* pApp = LLAppViewer::instance();
	pApp->initCrashReporting();

#endif
#endif

	bool success = LLAppViewer::init();

    return success;
}

bool LLAppViewerWin32::cleanup()
{
	bool result = LLAppViewer::cleanup();

	gDXHardware.cleanup();

#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLWinDebug::instance().cleanup();
#endif

	if (mIsConsoleAllocated)
	{
		FreeConsole();
		mIsConsoleAllocated = false;
	}

	return result;
}

void LLAppViewerWin32::initLoggingAndGetLastDuration()
{
	LLAppViewer::initLoggingAndGetLastDuration();
}

void LLAppViewerWin32::initConsole()
{
	// pop up debug console
	mIsConsoleAllocated = create_console();
	return LLAppViewer::initConsole();
}

void write_debug_dx(const char* str)
{
	std::string value = gDebugInfo["DXInfo"].asString();
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
	if (TRUE == gSavedSettings.getBOOL("ProbeHardwareOnStartup") && FALSE == gSavedSettings.getBOOL("NoHardwareProbe"))
	{
		// per DEV-11631 - disable hardware probing for everything
		// but vram.
		BOOL vram_only = TRUE;

		LLSplashScreen::update(LLTrans::getString("StartupDetectingHardware"));

		LL_DEBUGS("AppInit") << "Attempting to poll DirectX for hardware info" << LL_ENDL;
		gDXHardware.setWriteDebugFunc(write_debug_dx);
		BOOL probe_ok = gDXHardware.getInfo(vram_only);

		if (!probe_ok
			&& gWarningSettings.getBOOL("AboutDirectX9"))
		{
			LL_WARNS("AppInit") << "DirectX probe failed, alerting user." << LL_ENDL;

			// Warn them that runnin without DirectX 9 will
			// not allow us to tell them about driver issues
			std::ostringstream msg;
			msg << LLTrans::getString ("MBNoDirectX");
			S32 button = OSMessageBox(
				msg.str(),
				LLTrans::getString("MBWarning"),
				OSMB_YESNO);
			if (OSBTN_NO== button)
			{
				LL_INFOS("AppInit") << "User quitting after failed DirectX 9 detection" << LL_ENDL;
				LLWeb::loadURLExternal("http://secondlife.com/support/", false);
				return false;
			}
			gWarningSettings.setBOOL("AboutDirectX9", FALSE);
		}
		LL_DEBUGS("AppInit") << "Done polling DirectX for hardware info" << LL_ENDL;

		// Only probe once after installation
		gSavedSettings.setBOOL("ProbeHardwareOnStartup", FALSE);

		// Disable so debugger can work
		std::string splash_msg;
		LLStringUtil::format_map_t args;
		args["[APP_NAME]"] = LLAppViewer::instance()->getSecondLifeTitle();
		splash_msg = LLTrans::getString("StartupLoading", args);

		LLSplashScreen::update(splash_msg);
	}

	if (!restoreErrorTrap())
	{
		LL_WARNS("AppInit") << " Someone took over my exception handler (post hardware probe)!" << LL_ENDL;
	}

	if (gGLManager.mVRAM == 0)
	{
		gGLManager.mVRAM = gDXHardware.getVRAM();
	}

	LL_INFOS("AppInit") << "Detected VRAM: " << gGLManager.mVRAM << LL_ENDL;

	return true;
}

bool LLAppViewerWin32::initParseCommandLine(LLCommandLineParser& clp)
{
	if (!clp.parseCommandLineString(mCmdLine))
	{
		return false;
	}

	// Find the system language.
	FL_Locale *locale = NULL;
	FL_Success success = FL_FindLocale(&locale, FL_MESSAGES);
	if (success != 0)
	{
		if (success >= 2 && locale->lang) // confident!
		{
			LL_INFOS("AppInit") << "Language: " << ll_safe_string(locale->lang) << LL_ENDL;
			LL_INFOS("AppInit") << "Location: " << ll_safe_string(locale->country) << LL_ENDL;
			LL_INFOS("AppInit") << "Variant: " << ll_safe_string(locale->variant) << LL_ENDL;
			LLControlVariable* c = gSavedSettings.getControl("SystemLanguage");
			if(c)
			{
				c->setValue(std::string(locale->lang), false);
			}
		}
	}
	FL_FreeLocale(&locale);

	return true;
}

bool LLAppViewerWin32::restoreErrorTrap()
{	
	return true;
	//return LLWinDebug::checkExceptionHandler();
}

void LLAppViewerWin32::initCrashReporting(bool reportFreeze)
{
	if (isSecondInstance()) return; //BUG-5707 do not start another crash reporter for second instance.

	const char* logger_name = "win_crash_logger.exe";
	std::string exe_path = gDirUtilp->getExecutableDir();
	exe_path += gDirUtilp->getDirDelimiter();
	exe_path += logger_name;

    std::string logdir = gDirUtilp->getExpandedFilename(LL_PATH_DUMP, "");
    std::string appname = gDirUtilp->getExecutableFilename();

	S32 slen = logdir.length() -1;
	S32 end = slen;
	while (logdir.at(end) == '/' || logdir.at(end) == '\\') end--;
	
	if (slen !=end)
	{
		logdir = logdir.substr(0,end+1);
	}
	//std::string arg_str = "\"" + exe_path + "\" -dumpdir \"" + logdir + "\" -procname \"" + appname + "\" -pid " + stringize(LLApp::getPid());
	//_spawnl(_P_NOWAIT, exe_path.c_str(), arg_str.c_str(), NULL);
	std::string arg_str =  "\"" + exe_path + "\" -dumpdir \"" + logdir + "\" -procname \"" + appname + "\" -pid " + stringize(LLApp::getPid()); 

	STARTUPINFO startInfo={sizeof(startInfo)};
	PROCESS_INFORMATION processInfo;

	std::wstring exe_wstr;
	exe_wstr=wstringize(exe_path);

	std::wstring arg_wstr;
	arg_wstr=wstringize(arg_str);

	LL_INFOS("CrashReport") << "Creating crash reporter process " << exe_path << " with params: " << arg_str << LL_ENDL;
    if(CreateProcess(exe_wstr.c_str(),     
                     &arg_wstr[0],                 // Application arguments
                     0,
                     0,
                     FALSE,
                     CREATE_DEFAULT_ERROR_MODE,
                     0,
                     0,                              // Working directory
                     &startInfo,
                     &processInfo) == FALSE)
      // Could not start application -> call 'GetLastError()'
	{
        LL_WARNS("CrashReport Launch") << "CreateProcess failed " << GetLastError() << LL_ENDL;
        return;
    }
}

//virtual
bool LLAppViewerWin32::sendURLToOtherInstance(const std::string& url)
{
	wchar_t window_class[256]; /* Flawfinder: ignore */   // Assume max length < 255 chars.
	mbstowcs(window_class, sWindowClass.c_str(), 255);
	window_class[255] = 0;
	// Use the class instead of the window name.
	HWND other_window = FindWindow(window_class, NULL);

	if (other_window != NULL)
	{
		LL_DEBUGS() << "Found other window with the name '" << getWindowTitle() << "'" << LL_ENDL;
		COPYDATASTRUCT cds;
		const S32 SLURL_MESSAGE_TYPE = 0;
		cds.dwData = SLURL_MESSAGE_TYPE;
		cds.cbData = url.length() + 1;
		cds.lpData = (void*)url.c_str();

		LRESULT msg_result = SendMessage(other_window, WM_COPYDATA, NULL, (LPARAM)&cds);
		LL_DEBUGS() << "SendMessage(WM_COPYDATA) to other window '" 
				 << getWindowTitle() << "' returned " << msg_result << LL_ENDL;
		return true;
	}
	return false;
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
		LL_WARNS() << "GetVolumeInformation failed" << LL_ENDL;
	}
	return serial_md5;
}
