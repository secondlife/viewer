/** 
* @file llcrashloggerwindows.cpp
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
#include "resource.h"
#include "llcrashloggerwindows.h"

#include <sstream>

#include "boost/tokenizer.hpp"

#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK, CRASH_SETTING_NAME
#include "llerror.h"
#include "llfile.h"
#include "lltimer.h"
#include "llstring.h"
#include "lldxhardware.h"
#include "lldir.h"
#include "llsdserialize.h"

#define MAX_LOADSTRING 100
#define MAX_STRING 2048
const char* const SETTINGS_FILE_HEADER = "version";
const S32 SETTINGS_FILE_VERSION = 101;

// Windows Message Handlers

// Global Variables:
HINSTANCE hInst= NULL;					// current instance
TCHAR szTitle[MAX_LOADSTRING];				/* Flawfinder: ignore */		// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];		/* Flawfinder: ignore */		// The title bar text

std::string gProductName;
HWND gHwndReport = NULL;	// Send/Don't Send dialog
HWND gHwndProgress = NULL;	// Progress window
HCURSOR gCursorArrow = NULL;
HCURSOR gCursorWait = NULL;
BOOL gFirstDialog = TRUE;	// Are we currently handling the Send/Don't Send dialog?
std::stringstream gDXInfo;
bool gSendLogs = false;


//Conversion from char* to wchar*
//Replacement for ATL macros, doesn't allocate memory
//For more info see: http://www.codeguru.com/forum/showthread.php?t=337247
void ConvertLPCSTRToLPWSTR (const char* pCstring, WCHAR* outStr)
{
    if (pCstring != NULL)
    {
        int nInputStrLen = strlen (pCstring);
        // Double NULL Termination
        int nOutputStrLen = MultiByteToWideChar(CP_ACP, 0, pCstring, nInputStrLen, NULL, 0) + 2;
        if (outStr)
        {
            memset (outStr, 0x00, sizeof (WCHAR)*nOutputStrLen);
            MultiByteToWideChar (CP_ACP, 0, pCstring, nInputStrLen, outStr, nInputStrLen);
        }
    }
}

void write_debug(const char *str)
{
	gDXInfo << str;		/* Flawfinder: ignore */
}

void write_debug(std::string& str)
{
	write_debug(str.c_str());
}

void show_progress(const std::string& message)
{
	std::wstring msg = wstring_to_utf16str(utf8str_to_wstring(message));
	if (gHwndProgress)
	{
		SendDlgItemMessage(gHwndProgress,       // handle to destination window 
							IDC_LOG,
							WM_SETTEXT,			// message to send
							FALSE,				// undo option
							(LPARAM)msg.c_str());
	}
}

void update_messages()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			exit(0);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void sleep_and_pump_messages( U32 seconds )
{
	const U32 CYCLES_PER_SECOND = 10;
	U32 cycles = seconds * CYCLES_PER_SECOND;
	while( cycles-- )
	{
		update_messages();
		ms_sleep(1000 / CYCLES_PER_SECOND);
	}
}

// Include product name in the window caption.
void LLCrashLoggerWindows::ProcessCaption(HWND hWnd)
{
	TCHAR templateText[MAX_STRING];		/* Flawfinder: ignore */
	TCHAR header[MAX_STRING];
	std::string final;
	GetWindowText(hWnd, templateText, sizeof(templateText));
	final = llformat(ll_convert_wide_to_string(templateText, CP_ACP).c_str(), gProductName.c_str());
	ConvertLPCSTRToLPWSTR(final.c_str(), header);
	SetWindowText(hWnd, header);
}


// Include product name in the diaog item text.
void LLCrashLoggerWindows::ProcessDlgItemText(HWND hWnd, int nIDDlgItem)
{
	TCHAR templateText[MAX_STRING];		/* Flawfinder: ignore */
	TCHAR header[MAX_STRING];
	std::string final;
	GetDlgItemText(hWnd, nIDDlgItem, templateText, sizeof(templateText));
	final = llformat(ll_convert_wide_to_string(templateText, CP_ACP).c_str(), gProductName.c_str());
	ConvertLPCSTRToLPWSTR(final.c_str(), header);
	SetDlgItemText(hWnd, nIDDlgItem, header);
}

bool handle_button_click(WORD button_id)
{
	// Is this something other than Send or Don't Send?
	if (button_id != IDOK
		&& button_id != IDCANCEL)
	{
		return false;
	}

	// See if "do this next time" is checked and save state
	S32 crash_behavior = CRASH_BEHAVIOR_ASK;
	LRESULT result = SendDlgItemMessage(gHwndReport, IDC_CHECK_AUTO, BM_GETCHECK, 0, 0);
	if (result == BST_CHECKED)
	{
		if (button_id == IDOK)
		{
			crash_behavior = CRASH_BEHAVIOR_ALWAYS_SEND;
		}
		else if (button_id == IDCANCEL)
		{
			crash_behavior = CRASH_BEHAVIOR_NEVER_SEND;
		}
		((LLCrashLoggerWindows*)LLCrashLogger::instance())->saveCrashBehaviorSetting(crash_behavior);
	}
	
	// We're done with this dialog.
	gFirstDialog = FALSE;

	// Send the crash report if requested
	if (button_id == IDOK)
	{
		gSendLogs = TRUE;
		WCHAR wbuffer[20000];
		GetDlgItemText(gHwndReport, // handle to dialog box
						IDC_EDIT1,  // control identifier
						wbuffer, // pointer to buffer for text
						20000 // maximum size of string
						);
		std::string user_text(ll_convert_wide_to_string(wbuffer, CP_ACP));
		// Activate and show the window.
		ShowWindow(gHwndProgress, SW_SHOW); 
		// Try doing this second to make the progress window go frontmost.
		ShowWindow(gHwndReport, SW_HIDE);
		((LLCrashLoggerWindows*)LLCrashLogger::instance())->setUserText(user_text);
		((LLCrashLoggerWindows*)LLCrashLogger::instance())->sendCrashLogs();
	}
	// Quit the app
	LLApp::setQuitting();
	return true;
}


LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_CREATE:
		return 0;

	case WM_COMMAND:
		if( gFirstDialog )
		{
			WORD button_id = LOWORD(wParam);
			bool handled = handle_button_click(button_id);
			if (handled)
			{
				return 0;
			}
		}
		break;

	case WM_DESTROY:
		// Closing the window cancels
		LLApp::setQuitting();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}


LLCrashLoggerWindows::LLCrashLoggerWindows(void)
{
}

LLCrashLoggerWindows::~LLCrashLoggerWindows(void)
{
}

bool LLCrashLoggerWindows::init(void)
{	
	bool ok = LLCrashLogger::init();
	if(!ok) return false;

	/*
	mbstowcs( gProductName, mProductName.c_str(), LL_ARRAY_SIZE(gProductName) );
	gProductName[ LL_ARRY_SIZE(gProductName) - 1 ] = 0;
	swprintf(gProductName, L"Second Life");
	*/

	llinfos << "Loading dialogs" << llendl;

	// Initialize global strings
	LoadString(mhInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(mhInst, IDC_WIN_CRASH_LOGGER, szWindowClass, MAX_LOADSTRING);

	gCursorArrow = LoadCursor(NULL, IDC_ARROW);
	gCursorWait = LoadCursor(NULL, IDC_WAIT);

	// Register a window class that will be used by our dialogs
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = DLGWINDOWEXTRA;  // Required, since this is used for dialogs!
	wndclass.hInstance = mhInst;
	wndclass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE( IDI_WIN_CRASH_LOGGER ) );
	wndclass.hCursor = gCursorArrow;
	wndclass.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szWindowClass;
	RegisterClass( &wndclass );
	
	return true;
}

void LLCrashLoggerWindows::gatherPlatformSpecificFiles()
{
	updateApplication("Gathering hardware information. App may appear frozen.");
	// DX hardware probe blocks, so we can't cancel during it
	//Generate our dx_info.log file 
	SetCursor(gCursorWait);
	// At this point we're responsive enough the user could click the close button
	SetCursor(gCursorArrow);
	mDebugLog["DisplayDeviceInfo"] = gDXHardware.getDisplayInfo();
}

bool LLCrashLoggerWindows::mainLoop()
{	

	// Note: parent hwnd is 0 (the desktop).  No dlg proc.  See Petzold (5th ed) HexCalc example, Chapter 11, p529
	// win_crash_logger.rc has been edited by hand.
	// Dialogs defined with CLASS "WIN_CRASH_LOGGER" (must be same as szWindowClass)
	gProductName = mProductName;

	gHwndProgress = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PROGRESS), 0, NULL);
	ProcessCaption(gHwndProgress);
	ShowWindow(gHwndProgress, SW_HIDE );

	if (mCrashBehavior == CRASH_BEHAVIOR_ALWAYS_SEND)
	{
		ShowWindow(gHwndProgress, SW_SHOW );
		sendCrashLogs();
	}
	else if (mCrashBehavior == CRASH_BEHAVIOR_ASK)
	{
		gHwndReport = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PREVREPORTBOX), 0, NULL);
		// Ignore result
		(void) SendDlgItemMessage(gHwndReport, IDC_CHECK_AUTO, BM_SETCHECK, 0, 0);
		// Include the product name in the caption and various dialog items.
		ProcessCaption(gHwndReport);
		ProcessDlgItemText(gHwndReport, IDC_STATIC_MSG);

		// Update the header to include whether or not we crashed on the last run.
		std::string headerStr;
		TCHAR header[MAX_STRING];
		if (mCrashInPreviousExec)
		{
			headerStr = llformat("%s appears to have crashed or frozen the last time it ran.", mProductName.c_str());
		}
		else
		{
			headerStr = llformat("%s appears to have crashed.", mProductName.c_str());
		}
		ConvertLPCSTRToLPWSTR(headerStr.c_str(), header);
		SetDlgItemText(gHwndReport, IDC_STATIC_HEADER, header);		
		ShowWindow(gHwndReport, SW_SHOW );
		
		MSG msg;
		memset(&msg, 0, sizeof(msg));
		while (!LLApp::isQuitting() && GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return msg.wParam;
	}
	else
	{
		llwarns << "Unknown crash behavior " << mCrashBehavior << llendl;
		return 1;
	}
	return 0;
}

void LLCrashLoggerWindows::updateApplication(const std::string& message)
{
	LLCrashLogger::updateApplication();
	if(!message.empty()) show_progress(message);
	update_messages();
}

bool LLCrashLoggerWindows::cleanup()
{
	if(gSendLogs)
	{
		if(mSentCrashLogs) show_progress("Done");
		else show_progress("Could not connect to servers, logs not sent");
		sleep_and_pump_messages(3);
	}
	PostQuitMessage(0);
	return true;
}



