/** 
 * @file updater.cpp
 * @brief Windows auto-updater
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

//
// Usage: updater -url <url>
//

// We use dangerous fopen, strtok, mbstowcs, sprintf
// which generates warnings on VC2005.
// *TODO: Switch to fopen_s, strtok_s, etc.
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <fstream>

#define BUFSIZE 8192

int  gTotalBytesRead = 0;
DWORD gTotalBytes = -1;
HWND gWindow = NULL;
WCHAR gProgress[256];
char* gUpdateURL = NULL;

#if _DEBUG
std::ofstream logfile;
#define DEBUG(expr) logfile << expr << std::endl
#else
#define DEBUG(expr) /**/
#endif

char* wchars_to_utf8chars(const WCHAR* in_chars)
{
	int tlen = 0;
	const WCHAR* twc = in_chars;
	while (*twc++ != 0)
	{
		tlen++;
	}
	char* outchars = new char[tlen];
	char* res = outchars;
	for (int i=0; i<tlen; i++)
	{
		int cur_char = (int)(*in_chars++);
		if (cur_char < 0x80)
		{
			*outchars++ = (char)cur_char;
		}
		else
		{
			*outchars++ = '?';
		}
	}
	*outchars = 0;
	return res;
}

class Fetcher
{
public:
    Fetcher(const std::wstring& uri)
    {
        // These actions are broken out as separate methods not because it
        // makes the code clearer, but to avoid triggering AntiVir and
        // McAfee-GW-Edition virus scanners (DEV-31680).
        mInet = openInet();
        mDownload = openUrl(uri);
    }

    ~Fetcher()
    {
        DEBUG("Calling InternetCloseHandle");
        InternetCloseHandle(mDownload);
        InternetCloseHandle(mInet);
    }

    unsigned long read(char* buffer, size_t bufflen) const;

    DWORD getTotalBytes() const
    {
        DWORD totalBytes;
        DWORD sizeof_total_bytes = sizeof(totalBytes);
        HttpQueryInfo(mDownload, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                      &totalBytes, &sizeof_total_bytes, NULL);
        return totalBytes;
    }

    struct InetError: public std::runtime_error
    {
        InetError(const std::string& what): std::runtime_error(what) {}
    };

private:
    // We test results from a number of different MS functions with different
    // return types -- but the common characteristic is that 0 (i.e. (! result))
    // means an error of some kind.
    template <typename RESULT>
    static RESULT check(const std::string& desc, RESULT result)
    {
        if (result)
        {
            // success, show caller
            return result;
        }
        DWORD err = GetLastError();
        std::ostringstream out;
        out << desc << " Failed: " << err;
        DEBUG(out.str());
        throw InetError(out.str());
    }

    HINTERNET openUrl(const std::wstring& uri) const;
    HINTERNET openInet() const;

    HINTERNET mInet, mDownload;
};

HINTERNET Fetcher::openInet() const
{
    DEBUG("Calling InternetOpen");
    // Init wininet subsystem
    return check("InternetOpen",
                 InternetOpen(L"LindenUpdater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0));
}

HINTERNET Fetcher::openUrl(const std::wstring& uri) const
{
    DEBUG("Calling InternetOpenUrl: " << wchars_to_utf8chars(uri.c_str()));
    return check("InternetOpenUrl",
                 InternetOpenUrl(mInet, uri.c_str(), NULL, 0, INTERNET_FLAG_NEED_FILE, NULL));
}

unsigned long Fetcher::read(char* buffer, size_t bufflen) const
{
    unsigned long bytes_read = 0;
    DEBUG("Calling InternetReadFile");
    check("InternetReadFile",
          InternetReadFile(mDownload, buffer, bufflen, &bytes_read));
    return bytes_read;
}

int WINAPI get_url_into_file(const std::wstring& uri, const std::string& path, int *cancelled)
{
	int success = FALSE;
	*cancelled = FALSE;

    DEBUG("Opening '" << path << "'");

	FILE* fp = fopen(path.c_str(), "wb");		/* Flawfinder: ignore */

	if (!fp)
	{
        DEBUG("Failed to open '" << path << "'");
		return success;
	}

    // Note, ctor can throw, since it uses check() function.
    Fetcher fetcher(uri);
    gTotalBytes = fetcher.getTotalBytes();

/*==========================================================================*|
    // nobody uses total_bytes?!? What's this doing here?
	DWORD total_bytes = 0;
	success = check("InternetQueryDataAvailable",
                    InternetQueryDataAvailable(hdownload, &total_bytes, 0, 0));
|*==========================================================================*/

	success = FALSE;
	while(!success && !(*cancelled))
	{
        char data[BUFSIZE];		/* Flawfinder: ignore */
        unsigned long bytes_read = fetcher.read(data, sizeof(data));

		if (!bytes_read)
		{
            DEBUG("InternetReadFile Read " << bytes_read << " bytes.");
		}

        DEBUG("Reading Data, bytes_read = " << bytes_read);
		
		if (bytes_read == 0)
		{
			// If InternetFileRead returns TRUE AND bytes_read == 0
			// we've successfully downloaded the entire file
			wsprintf(gProgress, L"Download complete.");
			success = TRUE;
		}
		else
		{
			// write what we've got, then continue
			fwrite(data, sizeof(char), bytes_read, fp);

			gTotalBytesRead += int(bytes_read);

			if (gTotalBytes != -1)
				wsprintf(gProgress, L"Downloaded: %d%%", 100 * gTotalBytesRead / gTotalBytes);
			else
				wsprintf(gProgress, L"Downloaded: %dK", gTotalBytesRead / 1024);

		}

        DEBUG("Calling InvalidateRect");
		
		// Mark the window as needing redraw (of the whole thing)
		InvalidateRect(gWindow, NULL, TRUE);

		// Do the redraw
        DEBUG("Calling UpdateWindow");
		UpdateWindow(gWindow);

        DEBUG("Calling PeekMessage");
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				// bail out, user cancelled
				*cancelled = TRUE;
			}
		}
	}

	fclose(fp);
	return success;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	HDC hdc;			// Drawing context
	PAINTSTRUCT ps;

	switch(message)
	{
	case WM_PAINT:
		{
			hdc = BeginPaint(hwnd, &ps);

			RECT rect;
			GetClientRect(hwnd, &rect);
			DrawText(hdc, gProgress, -1, &rect, 
				DT_SINGLELINE | DT_CENTER | DT_VCENTER);

			EndPaint(hwnd, &ps);
			return 0;
		}
	case WM_CLOSE:
	case WM_DESTROY:
		// Get out of full screen
		// full_screen_mode(false);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

#define win_class_name L"FullScreen"

int parse_args(int argc, char **argv)
{
	int j;

	for (j = 1; j < argc; j++) 
	{
		if ((!strcmp(argv[j], "-url")) && (++j < argc)) 
		{
			gUpdateURL = argv[j];
		}
	}

	// If nothing was set, let the caller know.
	if (!gUpdateURL)
	{
		return 1;
	}
	return 0;
}
	
int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// Parse the command line.
	LPSTR cmd_line_including_exe_name = GetCommandLineA();

	const int MAX_ARGS = 100;
	int argc = 0;
	char* argv[MAX_ARGS];		/* Flawfinder: ignore */

#if _DEBUG
	logfile.open("updater.log", std::ios_base::out);
    DEBUG("Parsing command arguments");
#endif
	
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

	gUpdateURL = NULL;

	/////////////////////////////////////////
	//
	// Process command line arguments
	//

    DEBUG("Processing command arguments");
	
	//
	// Parse the command line arguments
	//
	int parse_args_result = parse_args(argc, argv);
	
	WNDCLASSEX wndclassex = { 0 };
	//DEVMODE dev_mode = { 0 };

	const int WINDOW_WIDTH = 250;
	const int WINDOW_HEIGHT = 100;

	wsprintf(gProgress, L"Connecting...");

	/* Init the WNDCLASSEX */
	wndclassex.cbSize = sizeof(WNDCLASSEX);
	wndclassex.style = CS_HREDRAW | CS_VREDRAW;
	wndclassex.hInstance = hInstance;
	wndclassex.lpfnWndProc = WinProc;
	wndclassex.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclassex.lpszClassName = win_class_name;
	
	RegisterClassEx(&wndclassex);
	
	// Get the size of the screen
	//EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode);
	
	gWindow = CreateWindowEx(NULL, win_class_name, 
		L"Second Life Updater",
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		WINDOW_WIDTH, 
		WINDOW_HEIGHT,
		NULL, NULL, hInstance, NULL);

	ShowWindow(gWindow, nShowCmd);
	UpdateWindow(gWindow);

	if (parse_args_result)
	{
		MessageBox(gWindow, 
				L"Usage: updater -url <url> [-name <window_title>] [-program <program_name>] [-silent]",
				L"Usage", MB_OK);
		return parse_args_result;
	}

	// Did we get a userserver to work with?
	if (!gUpdateURL)
	{
		MessageBox(gWindow, L"Please specify the download url from the command line",
			L"Error", MB_OK);
		return 1;
	}

	// Can't feed GetTempPath into GetTempFile directly
	char temp_path[MAX_PATH];		/* Flawfinder: ignore */
	if (0 == GetTempPathA(sizeof(temp_path), temp_path))
	{
		MessageBox(gWindow, L"Problem with GetTempPath()",
			L"Error", MB_OK);
		return 1;
	}
    std::string update_exec_path(temp_path);
	update_exec_path.append("Second_Life_Updater.exe");

	WCHAR update_uri[4096];
    mbstowcs(update_uri, gUpdateURL, sizeof(update_uri));

	int success = 0;
	int cancelled = 0;

	// Actually do the download
    try
    {
        DEBUG("Calling get_url_into_file");
        success = get_url_into_file(update_uri, update_exec_path, &cancelled);
    }
    catch (const Fetcher::InetError& e)
    {
        (void)e;
        success = FALSE;
        DEBUG("Caught: " << e.what());
    }

	// WinInet can't tell us if we got a 404 or not.  Therefor, we check
	// for the size of the downloaded file, and assume that our installer
	// will always be greater than 1MB.
	if (gTotalBytesRead < (1024 * 1024) && ! cancelled)
	{
		MessageBox(gWindow,
			L"The Second Life auto-update has failed.\n"
			L"The problem may be caused by other software installed \n"
			L"on your computer, such as a firewall.\n"
			L"Please visit http://secondlife.com/download/ \n"
			L"to download the latest version of Second Life.\n",
			NULL, MB_OK);
		return 1;
	}

	if (cancelled)
	{
		// silently exit
		return 0;
	}

	if (!success)
	{
		MessageBox(gWindow, 
			L"Second Life download failed.\n"
			L"Please try again later.", 
			NULL, MB_OK);
		return 1;
	}

	// TODO: Make updates silent (with /S to NSIS)
	//char params[256];		/* Flawfinder: ignore */
	//sprintf(params, "/S");	/* Flawfinder: ignore */
	//MessageBox(gWindow, 
	//	L"Updating Second Life.\n\nSecond Life will automatically start once the update is complete.  This may take a minute...",
	//	L"Download Complete",
	//	MB_OK);
		
/*==========================================================================*|
    // DEV-31680: ShellExecuteA() causes McAfee-GW-Edition and AntiVir
    // scanners to flag this executable as a probable virus vector.
    // Less than or equal to 32 means failure
	if (32 >= (int) ShellExecuteA(gWindow, "open", update_exec_path.c_str(), NULL, 
		"C:\\", SW_SHOWDEFAULT))
|*==========================================================================*/
    // from http://msdn.microsoft.com/en-us/library/ms682512(VS.85).aspx
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (! CreateProcessA(update_exec_path.c_str(), // executable file
                  NULL,                            // command line
                  NULL,             // process cannot be inherited
                  NULL,             // thread cannot be inherited
                  FALSE,            // do not inherit existing handles
                  0,                // process creation flags
                  NULL,             // inherit parent's environment
                  NULL,             // inherit parent's current dir
                  &si,              // STARTUPINFO
                  &pi))             // PROCESS_INFORMATION
	{
		MessageBox(gWindow, L"Update failed.  Please try again later.", NULL, MB_OK);
		return 1;
	}

	// Give installer some time to open a window
	Sleep(1000);

	return 0;
}
