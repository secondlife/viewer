/** 
 * @file win_crash_logger.cpp
 * @brief Windows crash logger implementation
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// win_crash_logger.cpp : Defines the entry point for the application.
//

// Must be first include, precompiled headers.
#include "stdafx.h"

#include "linden_common.h"
#include "llcontrol.h"
#include "resource.h"

#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wininet.h>

#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK, CRASH_SETTING_NAME
#include "llerror.h"
#include "lltimer.h"
#include "lldir.h"

#include "llstring.h"
#include "lldxhardware.h"

LLControlGroup gCrashSettings;	// saved at end of session

// Constants
#define MAX_LOADSTRING 100
const char* const SETTINGS_FILE_HEADER = "version";
const S32 SETTINGS_FILE_VERSION = 101;

// Functions
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool handle_button_click(WORD button_id);
S32 load_crash_behavior_setting();
bool save_crash_behavior_setting(S32 crash_behavior);
void send_crash_report();
void write_debug(const char *str);
void write_debug(std::string& str);

// Global Variables:
HINSTANCE hInst= NULL;					// current instance
TCHAR szTitle[MAX_LOADSTRING];				/* Flawfinder: ignore */		// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];		/* Flawfinder: ignore */		// The title bar text

LLString gUserText;			// User's description of the problem
time_t gStartTime = 0;
HWND gHwndReport = NULL;	// Send/Don't Send dialog
HWND gHwndProgress = NULL;	// Progress window
HCURSOR gCursorArrow = NULL;
HCURSOR gCursorWait = NULL;
BOOL gFirstDialog = TRUE;	// Are we currently handling the Send/Don't Send dialog?
BOOL gCrashInPreviousExec = FALSE;
FILE *gDebugFile = NULL;
LLString gUserserver;
WCHAR gProductName[512];

//
// Implementation
//

// Include product name in the window caption.
void ProcessCaption(HWND hWnd)
{
	TCHAR templateText[1024];		/* Flawfinder: ignore */
	TCHAR finalText[2048];		/* Flawfinder: ignore */
	GetWindowText(hWnd, templateText, sizeof(templateText));
	swprintf(finalText, templateText, gProductName);		/* Flawfinder: ignore */
	SetWindowText(hWnd, finalText);
}


// Include product name in the diaog item text.
void ProcessDlgItemText(HWND hWnd, int nIDDlgItem)
{
	TCHAR templateText[1024];		/* Flawfinder: ignore */
	TCHAR finalText[2048];		/* Flawfinder: ignore */
	GetDlgItemText(hWnd, nIDDlgItem, templateText, sizeof(templateText));
	swprintf(finalText, templateText, gProductName);		/* Flawfinder: ignore */
	SetDlgItemText(hWnd, nIDDlgItem, finalText);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	llinfos << "Starting crash reporter" << llendl;
	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");
	
	// Default to the product name "Second Life" (this is overridden by the -name argument)
	swprintf(gProductName, L"Second Life");		/* Flawfinder: ignore */

	gCrashSettings.declareS32(CRASH_BEHAVIOR_SETTING, CRASH_BEHAVIOR_ASK, "Controls behavior when viewer crashes "
		"(0 = ask before sending crash report, 1 = always send crash report, 2 = never send crash report)");

	llinfos << "Loading crash behavior setting" << llendl;
	S32 crash_behavior = load_crash_behavior_setting();

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
	char *argv[MAX_ARGS];		/* Flawfinder: ignore */

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
		if (*(token + strlen(token) + 1) == '\"')		/* Flawfinder: ignore */
		{
			token = strtok( NULL, "\"");
		}
		else
		{
			token = strtok( NULL, " \t," );
		}
	}

	S32 i;
	for (i=0; i<argc; i++)
	{
		if(!strcmp(argv[i], "-previous"))
		{
			llinfos << "Previous execution did not remove SecondLife.exec_marker" << llendl;
			gCrashInPreviousExec = TRUE;
		}

		if(!strcmp(argv[i], "-dialog"))
		{
			llinfos << "Show the user dialog" << llendl;
			crash_behavior = CRASH_BEHAVIOR_ASK;
		}

		if(!strcmp(argv[i], "-user"))
		{
			if ((i + 1) < argc)
			{
				i++;
				gUserserver = argv[i];
				llinfos << "Got userserver " << gUserserver << llendl;
			}
		}

		if(!strcmp(argv[i], "-name"))
		{
			if ((i + 1) < argc)
			{
				i++;
				
				mbstowcs(gProductName, argv[i], sizeof(gProductName)/sizeof(gProductName[0]));
				gProductName[ sizeof(gProductName)/sizeof(gProductName[0]) - 1 ] = 0;
				llinfos << "Got product name " << argv[i] << llendl;
			}
		}
	}

	// If user doesn't want to send, bail out
	if (crash_behavior == CRASH_BEHAVIOR_NEVER_SEND)
	{
		llinfos << "Crash behavior is never_send, quitting" << llendl;
		return 0;
	}

	// Get the current time
	time(&gStartTime);

	llinfos << "Loading dialogs" << llendl;

	// Store instance handle in our global variable
	hInst = hInstance;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WIN_CRASH_LOGGER, szWindowClass, MAX_LOADSTRING);

	gCursorArrow = LoadCursor(NULL, IDC_ARROW);
	gCursorWait = LoadCursor(NULL, IDC_WAIT);

	// Register a window class that will be used by our dialogs
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = DLGWINDOWEXTRA;  // Required, since this is used for dialogs!
	wndclass.hInstance = hInst;
	wndclass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE( IDI_WIN_CRASH_LOGGER ) );
	wndclass.hCursor = gCursorArrow;
	wndclass.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szWindowClass;
	RegisterClass( &wndclass );

	// Note: parent hwnd is 0 (the desktop).  No dlg proc.  See Petzold (5th ed) HexCalc example, Chapter 11, p529
	// win_crash_logger.rc has been edited by hand.
	// Dialogs defined with CLASS "WIN_CRASH_LOGGER" (must be same as szWindowClass)

	gHwndProgress = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PROGRESS), 0, NULL);
	ProcessCaption(gHwndProgress);
	ShowWindow(gHwndProgress, SW_HIDE );

	if (crash_behavior == CRASH_BEHAVIOR_ALWAYS_SEND)
	{
		ShowWindow(gHwndProgress, SW_SHOW );
		send_crash_report();
		return 0;
	}

	if (crash_behavior == CRASH_BEHAVIOR_ASK)
	{
		gHwndReport = CreateDialog(hInst, MAKEINTRESOURCE(IDD_REPORT), 0, NULL);

		// Include the product name in the caption and various dialog items.
		ProcessCaption(gHwndReport);
		ProcessDlgItemText(gHwndReport, IDC_STATIC_WHATINFO);
		ProcessDlgItemText(gHwndReport, IDC_STATIC_MOTIVATION);

		// Update the header to include whether or not we crashed on the last run.
		WCHAR header[2048];
		if (gCrashInPreviousExec)
		{
			swprintf(header, L"%s appears to have crashed or frozen the last time it ran.", gProductName);		/* Flawfinder: ignore */
		}
		else
		{
			swprintf(header, L"%s appears to have crashed.", gProductName);		/* Flawfinder: ignore */
		}
		SetDlgItemText(gHwndReport, IDC_STATIC_HEADER, header);
		ShowWindow(gHwndReport, SW_SHOW );
		
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return msg.wParam;
	}
	else
	{
		llwarns << "Unknown crash behavior " << crash_behavior << llendl;
		return 1;
	}
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
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
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
	}
	bool success = save_crash_behavior_setting(crash_behavior);
	if (!success)
	{
		llwarns << "Failed to save crash settings" << llendl;
	}

	// We're done with this dialog.
	gFirstDialog = FALSE;

	// Send the crash report if requested
	if (button_id == IDOK)
	{
		// Don't let users type anything.  They believe the reports
		// get read by humans, and get angry when we don't respond. JC
		//WCHAR wbuffer[20000];
		//GetDlgItemText(gHwndReport, // handle to dialog box
		//				IDC_EDIT1,  // control identifier
		//				wbuffer, // pointer to buffer for text
		//				20000 // maximum size of string
		//				);
		//gUserText = wstring_to_utf8str(utf16str_to_wstring(wbuffer)).c_str();
		//llinfos << gUserText << llendl;

		// Activate and show the window.
		ShowWindow(gHwndProgress, SW_SHOW); 
		// Try doing this second to make the progress window go frontmost.
		ShowWindow(gHwndReport, SW_HIDE);

		send_crash_report();
	}

	// Quit the app
	PostQuitMessage(0);

	return true;
}


class LLFileEncoder
{
public:
	LLFileEncoder(const char *formname, const char *filename);
	~LLFileEncoder();

	BOOL isValid() const { return mIsValid; }
	LLString encodeURL(const S32 max_length = 0);
public:
	BOOL mIsValid;
	LLString mFilename;
	LLString mFormname;
	S32 mBufLength;
	U8 *mBuf;
};

LLString encode_string(const char *formname, const LLString &str);

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


void show_progress(const char* message)
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


void send_crash_report()
{
	update_messages();
	show_progress("Starting up...");
	update_messages();

	const S32 SL_MAX_SIZE = 100000;			// Maximum size of the Second Life log file.

	update_messages();

	// Lots of silly variable, replicated for each log file.
	std::string db_file_name; // debug.log
	std::string sl_file_name; // SecondLife.log
	std::string md_file_name; // minidump (SecondLife.dmp) file name
	std::string st_file_name; // stats.log file
	std::string si_file_name; // settings.ini file
	std::string ml_file_name; // message.log file

	LLFileEncoder *db_filep = NULL;
	LLFileEncoder *sl_filep = NULL;
	LLFileEncoder *st_filep = NULL;
	LLFileEncoder *md_filep = NULL;
	LLFileEncoder *si_filep = NULL;
	LLFileEncoder *ml_filep = NULL;

	// DX hardware probe blocks, so we can't cancel during it
	SetCursor(gCursorWait);

	// Need to do hardware detection before we grab the files, otherwise we don't send the debug log updates
	// to the server (including the list of hardware).
	update_messages();
	show_progress("Detecting hardware, please wait...");
	update_messages();
	gDXHardware.setWriteDebugFunc(write_debug);
	gDXHardware.getInfo(FALSE);
	update_messages();
	gDXHardware.dumpDevices();
	update_messages();
	fclose(gDebugFile);
	gDebugFile = NULL;

	// At this point we're responsive enough the user could click the close button
	SetCursor(gCursorArrow);

	///////////////////////////////////
	//
	// We do the parsing for the debug_info file first, as that will
	// give us the location of the SecondLife.log file.
	//

	// Figure out the filename of the debug log
	db_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"debug_info.log");
	db_filep = new LLFileEncoder("DB", db_file_name.c_str());

	// Get the filename of the SecondLife.log file
	// *NOTE: This buffer size is hard coded into scanf() below.
	char tmp_sl_name[256];		/* Flawfinder: ignore */
	tmp_sl_name[0] = '\0';

	update_messages();
	show_progress("Looking for files...");
	update_messages();

	// Look for it in the debug_info.log file
	if (db_filep->isValid())
	{
		sscanf(
			(const char*)db_filep->mBuf,
			"SL Log: %255[^\r\n]",
			tmp_sl_name);
	}
	else
	{
		delete db_filep;
		db_filep = NULL;
	}

	if (gCrashInPreviousExec)
	{
		// If we froze, the crash log this time around isn't useful.  Use the
		// old one.
		sl_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.old");
	}
	else if (tmp_sl_name[0])
	{
		// If debug_info.log gives us a valid log filename, use that.
		sl_file_name = tmp_sl_name;
		llinfos << "Using log file from debug log " << sl_file_name << llendl;
	}
	else
	{
		// Figure out the filename of the default second life log
		sl_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.log");
	}

	// Now we get the SecondLife.log file if it's there
	sl_filep = new LLFileEncoder("SL", sl_file_name.c_str());
	if (!sl_filep->isValid())
	{
		delete sl_filep;
		sl_filep = NULL;
	}

	update_messages();
	show_progress("Looking for stats file...");
	update_messages();

	st_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stats.log");
	st_filep = new LLFileEncoder("ST", st_file_name.c_str());
	if (!st_filep->isValid())
	{
		delete st_filep;
		st_filep = NULL;
	}

	si_file_name = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings.ini");
	si_filep = new LLFileEncoder("SI", si_file_name.c_str());
	if (!si_filep->isValid())
	{
		delete si_filep;
		si_filep = NULL;
	}

	// Now we get the minidump
	md_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.dmp");
	md_filep = new LLFileEncoder("MD", md_file_name.c_str());
	if (!md_filep->isValid())
	{		
		delete md_filep;
		md_filep = NULL;
	}

	// Now we get the message log
	ml_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"message.log");
	ml_filep = new LLFileEncoder("ML", ml_file_name.c_str());
	if (!ml_filep->isValid())
	{		
		delete ml_filep;
		ml_filep = NULL;
	}

	LLString post_data;
	LLString tmp_url_buf;

	// Append the userserver
	tmp_url_buf = encode_string("USER", gUserserver);
	post_data += tmp_url_buf;
	llinfos << "PostData:" << post_data << llendl;

	if (gCrashInPreviousExec)
	{
		post_data.append(1, '&');
		tmp_url_buf = encode_string("EF", "Y");
		post_data += tmp_url_buf;
	}

	update_messages();
	show_progress("Encoding data");
	update_messages();
	if (db_filep)
	{
		post_data.append(1, '&');
		tmp_url_buf = db_filep->encodeURL();
		post_data += tmp_url_buf;
		llinfos << "Sending DB log file" << llendl;
	}
	else
	{
		llinfos << "Not sending DB log file" << llendl;
	}
	show_progress("Encoding data.");
	update_messages();

	if (sl_filep)
	{
		post_data.append(1, '&');
		tmp_url_buf = sl_filep->encodeURL(SL_MAX_SIZE);
		post_data += tmp_url_buf;
		llinfos << "Sending SL log file" << llendl;
	}
	else
	{
		llinfos << "Not sending SL log file" << llendl;
	}
	show_progress("Encoding data..");
	update_messages();

	if (st_filep)
	{
		post_data.append(1, '&');
		tmp_url_buf = st_filep->encodeURL(SL_MAX_SIZE);
		post_data += tmp_url_buf;
		llinfos << "Sending stats log file" << llendl;
	}
	else
	{
		llinfos << "Not sending stats log file" << llendl;
	}
	show_progress("Encoding data...");
	update_messages();

	if (md_filep)
	{
		post_data.append(1, '&');
		tmp_url_buf = md_filep->encodeURL();
		post_data += tmp_url_buf;
		llinfos << "Sending minidump log file" << llendl;
	}
	else
	{
		llinfos << "Not sending minidump log file" << llendl;
	}
	show_progress("Encoding data....");
	update_messages();

	if (si_filep)
	{
		post_data.append(1, '&');
		tmp_url_buf = si_filep->encodeURL();
		post_data += tmp_url_buf;
		llinfos << "Sending settings log file" << llendl;
	}
	else
	{
		llinfos << "Not sending settings.ini file" << llendl;
	}
	show_progress("Encoding data....");
	update_messages();

	if (ml_filep)
	{
		post_data.append(1, '&');
		tmp_url_buf = ml_filep->encodeURL(SL_MAX_SIZE);
		post_data += tmp_url_buf;
		llinfos << "Sending message log file" << llendl;
	}
	else
	{
		llinfos << "Not sending message.log file" << llendl;
	}
	show_progress("Encoding data....");
	update_messages();

	if (gUserText.size())
	{
		post_data.append(1, '&');
		tmp_url_buf = encode_string("UN", gUserText);
		post_data += tmp_url_buf;
	}

	delete db_filep;
	db_filep = NULL;
	delete sl_filep;
	sl_filep = NULL;
	delete md_filep;
	md_filep = NULL;

	// Post data to web server
	const S32 BUFSIZE = 65536;
	HINTERNET hinet, hsession, hrequest;
	char data[BUFSIZE];		/* Flawfinder: ignore */
	unsigned long bytes_read;

	llinfos << "Connecting to crash report server" << llendl;
	update_messages();
	show_progress("Connecting to server...");
	update_messages();

	// Init wininet subsystem
	hinet = InternetOpen(L"LindenCrashReporter", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hinet == NULL)
	{
		llinfos << "Couldn't open connection" << llendl;
		sleep_and_pump_messages( 5 );
//		return FALSE;
	}

	hsession = InternetConnect(hinet,
								L"secondlife.com",
								INTERNET_DEFAULT_HTTP_PORT,
								NULL,
								NULL,
								INTERNET_SERVICE_HTTP,
								NULL,
								NULL);

	if (!hsession)
	{
		llinfos << "Couldn't talk to crash report server" << llendl;
	}

	hrequest = HttpOpenRequest(hsession, L"POST", L"/cgi-bin/viewer_crash_reporter2", NULL, L"", NULL, 0, 0);
	if (!hrequest)
	{
		llinfos << "Couldn't open crash report URL!" << llendl;
	}
	
	llinfos << "Transmitting data" << llendl;
	llinfos << "Bytes: " << (post_data.size()) << llendl;

	update_messages();
	show_progress("Transmitting data...");
	update_messages();

	BOOL ok = HttpSendRequest(hrequest, NULL, 0, (void *)(post_data.c_str()), post_data.size());
	if (!ok)
	{
		llinfos << "Error posting data!" << llendl;
		sleep_and_pump_messages( 5 );
	}

	llinfos << "Response from crash report server:" << llendl;
	do
	{
		if (InternetReadFile(hrequest, data, BUFSIZE, &bytes_read))
		{
			if (bytes_read == 0)
			{
				// If InternetFileRead returns TRUE AND bytes_read == 0
				// we've successfully downloaded the entire file
				break;
			}
			else
			{
				data[bytes_read] = 0;
				llinfos << data << llendl;
			}
		}
		else
		{
			llinfos << "Couldn't read file!" << llendl;
			sleep_and_pump_messages( 5 );
//			return FALSE;
		}
	} while(TRUE);

	InternetCloseHandle(hrequest);
	InternetCloseHandle(hsession);
	InternetCloseHandle(hinet);
	update_messages();
	show_progress("Done.");
	sleep_and_pump_messages( 3 );
//	return TRUE;
}

LLFileEncoder::LLFileEncoder(const char *form_name, const char *filename)
{
	mFormname = form_name;
	mFilename = filename;
	mIsValid = FALSE;
	mBuf = NULL;

	int res;
	
	llstat stat_data;
	res = LLFile::stat(mFilename.c_str(), &stat_data);
	if (res)
	{
		llwarns << "File " << mFilename << " is missing!" << llendl;
		return;
	}

	FILE *fp = NULL;
	S32 buf_size = 0;
	S32 count = 0;
	while (count < 5)
	{
		buf_size = stat_data.st_size;
		fp = LLFile::fopen(mFilename.c_str(), "rb");		/* Flawfinder: ignore */
		if (!fp)
		{
			llwarns << "Can't open file " << mFilename << ", wait for a second" << llendl;
			// Couldn't open the file, wait a bit and try again
			count++;
			ms_sleep(1000);
		}
		else
		{
			break;
		}
	}
	if (!fp)
	{
		return;
	}
	U8 *buf = new U8[buf_size + 1];
	fread(buf, 1, buf_size, fp);
	fclose(fp);

	mBuf = buf;
	mBufLength = buf_size;

	mIsValid = TRUE;
}

LLFileEncoder::~LLFileEncoder()
{
	if (mBuf)
	{
		delete mBuf;
		mBuf = NULL;
	}
}

LLString LLFileEncoder::encodeURL(const S32 max_length)
{
	LLString result = mFormname;
	result.append(1, '=');

	S32 i = 0;

	if (max_length)
	{
		if (mBufLength > max_length)
		{
			i = mBufLength - max_length;
		}
	}

	S32 url_buf_size = 3*mBufLength + 1;
	char *url_buf = new char[url_buf_size];

	S32 cur_pos = 0;
	for (; i < mBufLength; i++)
	{
		S32 byte_val = mBuf[i];
		sprintf(url_buf + cur_pos, "%%%02x", byte_val);
		cur_pos += 3;
	}
	url_buf[i*3] = 0;

	result.append(url_buf);
	delete[] url_buf;
	return result;
}

LLString encode_string(const char *formname, const LLString &str)
{
	LLString result = formname;
	result.append(1, '=');
	// Not using LLString because of bad performance issues
	S32 buf_size = str.size();
	S32 url_buf_size = 3*str.size() + 1;
	char *url_buf = new char[url_buf_size];

	S32 cur_pos = 0;
	S32 i;
	for (i = 0; i < buf_size; i++)
	{
		sprintf(url_buf + cur_pos, "%%%02x", str[i]);
		cur_pos += 3;
	}
	url_buf[i*3] = 0;

	result.append(url_buf);
	delete[] url_buf;
	return result;
}

void write_debug(const char *str)
{
	if (!gDebugFile)
	{
		std::string debug_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"debug_info.log");
		llinfos << "Opening debug file " << debug_filename << llendl;
		gDebugFile = LLFile::fopen(debug_filename.c_str(), "a+");		/* Flawfinder: ignore */
        if (!gDebugFile)
        {
            fprintf(stderr, "Couldn't open %s: debug log to stderr instead.\n", debug_filename.c_str());
            gDebugFile = stderr;
        }
	}
	fprintf(gDebugFile, str);		/* Flawfinder: ignore */
	fflush(gDebugFile);
}

void write_debug(std::string& str)
{
	write_debug(str.c_str());
}

S32 load_crash_behavior_setting()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);

	gCrashSettings.loadFromFile(filename);
		
	S32 value = gCrashSettings.getS32(CRASH_BEHAVIOR_SETTING);
	
	if (value < CRASH_BEHAVIOR_ASK || CRASH_BEHAVIOR_NEVER_SEND < value) return CRASH_BEHAVIOR_ASK;

	return value;
}

bool save_crash_behavior_setting(S32 crash_behavior)
{
	if (crash_behavior < CRASH_BEHAVIOR_ASK) return false;
	if (crash_behavior > CRASH_BEHAVIOR_NEVER_SEND) return false;

	gCrashSettings.setS32(CRASH_BEHAVIOR_SETTING, crash_behavior);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);

	gCrashSettings.saveToFile(filename, FALSE);

	return true;
}
